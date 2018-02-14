/*
* order.cpp
* collection of management order
* Copyright(C) by MY Capital Inc. 2007-2999
*/

#include "order_hash.h"
#include "utils.h"
#include "log.h"

#define HASH_ORDER_SIZE	  (4096)
#define MAX_ORDER_SIZE	  (4096)


//挂单成交手数的剩余量。
int
leaves_qty(Order *ord)
{
    //如果 申报的量大于已经成交的量，返回差值。
	if (ord->volume > ord->cum_qty)
		return ord->volume - ord->cum_qty;
	else
		return 0;
}

//返回参数相反的方向。BUY 就返回 SELL
DIRECTION switch_side(DIRECTION side)
{
	if (side == ORDER_BUY)
		return ORDER_SELL;
	else
		return ORDER_BUY;
}

//是否是活跃的，下面这么几种状态是活跃的。
bool
is_active(Order *a_order)
{
	if (a_order->status == SIG_STATUS_ENTRUSTED //挂单成功
		|| a_order->status == SIG_STATUS_PARTED //部分成交
		|| a_order->status == SIG_STATUS_CANCEL_REJECTED) //撤单被拒
		return true;
	else
		return false;
}

//是否可以被撤单，也就是active的，且之前没有被撤单。
bool
is_cancelable(Order *a_order)
{
    //pending_cancel 当你下完一次cancel之后的状态
	return is_active(a_order) && !a_order->pending_cancel;
}

//从Order List 移除的一些状态， 
bool
is_finished(Order *a_order)
{
	if (a_order->status == SIG_STATUS_SUCCEED //成交
		|| a_order->status == SIG_STATUS_CANCELED //撤单成功
		|| a_order->status == SIG_STATUS_REJECTED //挂单失败
		|| a_order->status == SIG_STATUS_INTRREJECTED) //。。。
		return true;
	else
		return false;
}

//双向链表list_t的长度
int
get_list_size(list_t *a_list)
{
	list_t *pos, *n;
	int l_size = 0;
     //pos 第一个， n next 下一个， n 变成pos
	list_for_each_safe(pos, n, a_list){
		l_size++;
	}
	return l_size;
}


OrderHash::OrderHash(){
	p_bucket = NULL;
	p_order_head = NULL;
	char *ht = NULL;

	int size = MAX_ORDER_SIZE * sizeof(Order) +
		HASH_ORDER_SIZE * sizeof(hash_bucket);

	void * all_mem = calloc(1, size);
	if (all_mem == NULL) {
		LOG_LN("init_hash_table malloc memory fail.");
		return;
	}

	ht = (char *)all_mem;
	p_order_head = (Order *)(all_mem);
	p_bucket = (hash_bucket *)(ht + MAX_ORDER_SIZE * sizeof(Order));

	p_buy_list = (list_t *)malloc(sizeof(list_t));
	p_sell_list = (list_t *)malloc(sizeof(list_t));
	p_free_list = (list_t *)malloc(sizeof(list_t));
	p_search_list = (list_t *)malloc(sizeof(list_t));

	clear();
}

OrderHash::~OrderHash(){
	free(p_buy_list);
	free(p_sell_list);
	free(p_free_list);
	free(p_search_list);
	free(p_order_head);
}

bool OrderHash::full()
{
	return m_use_count >= HASH_ORDER_SIZE ? true : false;
}

void
OrderHash::clear(){
	m_use_count = 0;

	for (int i = 0; i < HASH_ORDER_SIZE; i++){
		INIT_LIST_HEAD(&p_bucket[i].head);
	}
    //INIT_LIST_HEAD 初始化双向链表，是linux内核中宏定义的方法。
	INIT_LIST_HEAD(p_buy_list);
	INIT_LIST_HEAD(p_sell_list);
	INIT_LIST_HEAD(p_free_list);
	INIT_LIST_HEAD(p_search_list);

	Order temp;
	set_order_default(&temp);
	for (int i = 0; i < MAX_ORDER_SIZE; i++){
		memcpy(p_order_head + i, &temp, sizeof(Order));
		INIT_LIST_HEAD(&p_order_head[i].hs_link);
		INIT_LIST_HEAD(&p_order_head[i].pd_link);
		list_add_after(&p_order_head[i].fr_link, p_free_list);
		INIT_LIST_HEAD(&p_order_head[i].search_link);
	}
}


int
OrderHash::get_hash_value(int index){
	return index % HASH_ORDER_SIZE;
}


//从 已经初始化的free中 取来用。内存池
Order*
OrderHash::get_first_free_order()
{
	if (p_free_list->next != p_free_list){
    //取某一个。通过结构体中放入连接。地址来找到结构体的起始地址。
		Order* l_ord = list_entry(p_free_list->next, Order, fr_link);
		list_del(&l_ord->fr_link);
		INIT_LIST_HEAD(&l_ord->fr_link);
		return l_ord;
	}
	return NULL;
}

Order*
OrderHash::update_order(int index, Contract *a_contr,
	double price, int size, DIRECTION side,
	uint64_t sig_id, OPEN_CLOSE openclose)
{
	int hash_value = get_hash_value(index);
	if (hash_value < 0 || hash_value > HASH_ORDER_SIZE - 1) {
		LOG_LN("Hash value %d is illegal.", hash_value);
		return NULL;
	}

	Order *item = get_first_free_order();
	if (item == NULL){
		LOG_LN("OrderHash Can't get free order.");
		return NULL;
	}

	set_parameter(item, a_contr, price, size, side, sig_id, openclose);
	list_add_after(&item->hs_link, &p_bucket[hash_value].head);

	if (item->side == ORDER_BUY){
		list_add_after(&item->pd_link, p_buy_list);
	}
	else if (item->side == ORDER_SELL){
		list_add_after(&item->pd_link, p_sell_list);
	}

	m_use_count++;
	add_pending_size(item, size);
	return item;
}

void
OrderHash::set_parameter(Order *ord, Contract *a_contr,
double a_price, int a_size, DIRECTION a_side,
uint64_t a_signal_id, OPEN_CLOSE a_openclose)
{
	ord->signal_id = a_signal_id;
	ord->exch = a_contr->exch;
	ord->contr = a_contr;
	ord->price = a_price;
	ord->volume = a_size;
	ord->side = a_side;
	ord->openclose = a_openclose;
	my_strncpy(ord->symbol, a_contr->symbol, SYMBOL_LEN);
	ord->status = SIG_STATUS_INIT;
}

void
OrderHash::push_back(Order* item, list_t *a_list)
{
	list_add_after(&item->search_link, a_list);
}

bool OrderHash::erase(Order *ord)
{
	set_order_default(ord);
	if (ord->hs_link.next == &ord->hs_link)
		return false;

	list_del(&ord->hs_link);
	list_del(&ord->pd_link);
	list_add_after(&ord->fr_link, p_free_list);

	m_use_count--;
	return true;
}


//完善Order的参数
void OrderHash::set_order_default(Order *ord)
{
	ord->signal_id = 0;
	ord->exch = UNDEFINED_EXCH;
	ord->status = SIG_STATUS_INIT;
	ord->price = 0;
	ord->volume = 0;
	ord->side = ORDER_BUY; //direction
	ord->last_px = 0;
	ord->last_qty = 0;
	ord->cum_qty = 0;
	ord->cum_amount = 0;
	ord->orig_cl_ord_id = 0;
	ord->orig_ord_id = 0;
	ord->openclose = ORDER_OPEN;
	ord->pending_cancel = false;
	memset(ord->symbol, 0, SYMBOL_LEN);
}

// 将成交的，撤单的等移除。
void OrderHash::update_order_list(Order* ord)
{
	switch (ord->status){
	case SIG_STATUS_ENTRUSTED:{
		break;
	}
	case SIG_STATUS_SUCCEED:{
		erase(ord);
		break;
	}
	case SIG_STATUS_PARTED:{
		break;
	}
	case SIG_STATUS_CANCELED:{
		erase(ord);
		break;
	}
	case SIG_STATUS_INTRREJECTED:
	case SIG_STATUS_REJECTED:{
		if (ord->pending_cancel == true){ //Cancel reject
 			ord->pending_cancel = false;
		} else {
			erase(ord);
		}
		break;
	}
	case SIG_STATUS_CANCEL_REJECTED:
		break;
	}
}

Order *
OrderHash::query_order(int index, uint64_t ord_id)
{
	list_t *pos, *n;
	Order *l_ord;

	int hash_value = get_hash_value(index);
	if (hash_value < 0 || hash_value > HASH_ORDER_SIZE - 1)
		return NULL;

	list_for_each_safe(pos, n, &p_bucket[hash_value].head){
		l_ord = list_entry(pos, Order, hs_link);
		if (l_ord->signal_id == ord_id) {
			return l_ord;
		}
	}

	return NULL;
} 

int reverse_index(uint64_t ord_id) {
	return ord_id / 10000000000;
}

Order *
OrderHash::query_order(uint64_t ord_id)
{
	int index = reverse_index(ord_id);
	return query_order(index, ord_id);
}

//返回给定方向的挂单的数量。
int
OrderHash::get_buy_sell_list_size(DIRECTION a_side)
{
	list_t *pos, *n, *a_list = NULL;
	int l_size = 0;

	if (a_side == ORDER_BUY){
		a_list = p_buy_list;
	}
	else if (a_side == ORDER_SELL){
		a_list = p_sell_list;
	}

	list_for_each_safe(pos, n, a_list){
		l_size++;
	}
	return l_size;
}




// 返回 买 或者 卖 列表的头节点。
list_t *
OrderHash::get_order_by_side(DIRECTION a_side)
{
	if (a_side == ORDER_BUY)
		return p_buy_list;
	if (a_side == ORDER_SELL)
		return p_sell_list;
	return NULL;
}


//返回给定方向的订单还挂着多少手
int 
OrderHash::active_order_size(DIRECTION a_side)
{
	list_t *tempHead = get_order_by_side(a_side);
	list_t *pos, *n;
	Order *l_ord;
	int l_active_size = 0;

	list_for_each_safe(pos, n, tempHead){
		l_ord = list_entry(pos, Order, pd_link);
		l_active_size += leaves_qty(l_ord);
	}
	return l_active_size;
}


// 返回活跃的买和卖 list
void
OrderHash::get_active_order_list(list_t **a_buy_list, list_t **a_sell_list)
{
	*a_buy_list = p_buy_list;
	*a_sell_list = p_sell_list;
}

//取得内存池中还有多少free块。
int
OrderHash::get_free_order_size()
{
	list_t *pos, *n;
	int l_size = 0;

	list_for_each_safe(pos, n, p_free_list){
		l_size++;
	}
	return l_size;
}

int
OrderHash::get_search_order_size()
{
	list_t *pos, *n;
	int l_size = 0;

	list_for_each_safe(pos, n, p_search_list){
		l_size++;
	}
	return l_size;
}

const static char OPEN_CLOSE_STR[][16] = { "OPEN", "CLOSE", "CLOSE_TOD", "CLOSE_YES" };
const static char BUY_SELL_STR[][8] = { "BUY", "SELL" };
static std::string g_all_order_info;


// 打印出所有活跃订单的基本信息
char*
OrderHash::get_all_active_order_info()
{
	list_t *pos, *n;
	Order *l_ord;
	char temp[1024];
	g_all_order_info = "Current active orders:\n";

	list_for_each_prev_safe(pos, n, p_buy_list) {
		l_ord = list_entry(pos, Order, pd_link);
		sprintf(temp, "---> %d %lld %s %s %d@%f leaves_qty: %d %s\n", l_ord->insert_time, l_ord->signal_id, BUY_SELL_STR[l_ord->side], OPEN_CLOSE_STR[l_ord->openclose],
			l_ord->volume, l_ord->price, leaves_qty(l_ord), l_ord->pending_cancel == true ? "canceling..." : "");
		g_all_order_info += temp;
	}

	list_for_each_prev_safe(pos, n, p_sell_list) {
		l_ord = list_entry(pos, Order, pd_link);
		sprintf(temp, "---> %d %lld %s %s %d@%f leaves_qty: %d %s\n", l_ord->insert_time, l_ord->signal_id, BUY_SELL_STR[l_ord->side], OPEN_CLOSE_STR[l_ord->openclose],
			l_ord->volume, l_ord->price, leaves_qty(l_ord), l_ord->pending_cancel == true ? "canceling..." : "");
		g_all_order_info += temp;
	}
	return (char*)g_all_order_info.c_str();
}


//取得某一个合约等待撤单的数量。
int
OrderHash::get_pending_cancel_order_size_by_instr(Contract *a_instr)
{
	list_t *pos, *n;
	Order *l_ord;
	int l_size = 0;

	list_for_each_safe(pos, n, p_buy_list){
		l_ord = list_entry(pos, Order, pd_link);
		if (l_ord->contr == a_instr && l_ord->pending_cancel == true)
			l_size++;
	}

	list_for_each_safe(pos, n, p_sell_list){
		l_ord = list_entry(pos, Order, pd_link);
		if (l_ord->contr == a_instr && l_ord->pending_cancel == true)
			l_size++;
	}
	return l_size;
}

// 返回给定symbol的所有的挂单。返回结构在a_list
void 
OrderHash::search_order_by_instr(const char* a_symbol, list_t *a_list)
{
	list_t *pos, *n;
	Order *l_ord;
	INIT_LIST_HEAD(a_list);

	list_for_each_safe(pos, n, p_buy_list){
		l_ord = list_entry(pos, Order, pd_link);
		if (my_strcmp(l_ord->symbol, a_symbol) == 0)
			push_back(l_ord, a_list);
	}

	list_for_each_safe(pos, n, p_sell_list){
		l_ord = list_entry(pos, Order, pd_link);
		if (my_strcmp(l_ord->symbol, a_symbol) == 0)
			push_back(l_ord, a_list);
	}
}


//返回所有的这一方向，这一个symbol的所有的order.返回是a_list
void
OrderHash::search_order_by_side_instr(DIRECTION a_side, const char* a_symbol, list_t *a_list)
{
	list_t *pos, *n;
	Order *l_ord;
	INIT_LIST_HEAD(a_list);

	if (a_side == ORDER_BUY){
		list_for_each_safe(pos, n, p_buy_list){
			l_ord = list_entry(pos, Order, pd_link);
			if (my_strcmp(l_ord->symbol, a_symbol) == 0)
				push_back(l_ord, a_list);
		}
	}
	else if (a_side == ORDER_SELL){
		list_for_each_safe(pos, n, p_sell_list){
			l_ord = list_entry(pos, Order, pd_link);
			if (my_strcmp(l_ord->symbol, a_symbol) == 0)
				push_back(l_ord, a_list);
		}
	}
}

//根据Order的买卖方向，增加 Order中挂单的手数
void
OrderHash::add_pending_size(Order *ord, int quantity)
{
	if (ord->side == ORDER_BUY) {
		if (ord->openclose == ORDER_OPEN)
			ord->contr->pending_buy_open_size += quantity;
		else if (ord->openclose == ORDER_CLOSE || ord->openclose == ORDER_CLOSE_YES)
			ord->contr->pending_buy_close_size += quantity;
	}
	else if (ord->side == ORDER_SELL) {
		if (ord->openclose == ORDER_OPEN)
			ord->contr->pending_sell_open_size += quantity;
		else if (ord->openclose == ORDER_CLOSE || ord->openclose == ORDER_CLOSE_YES)
			ord->contr->pending_sell_close_size += quantity;
	}
}

//根据Order的买卖方向，减少 Order中挂单的手数
void
OrderHash::minus_pending_size(Order *ord, int quantity)
{
	if (ord->side == ORDER_BUY) {
		if (ord->openclose == ORDER_OPEN)
			ord->contr->pending_buy_open_size -= quantity;
		else if (ord->openclose == ORDER_CLOSE || ord->openclose == ORDER_CLOSE_YES)
			ord->contr->pending_buy_close_size -= quantity;
	}
	else if (ord->side == ORDER_SELL) {
		if (ord->openclose == ORDER_OPEN)
			ord->contr->pending_sell_open_size -= quantity;
		else if (ord->openclose == ORDER_CLOSE || ord->openclose == ORDER_CLOSE_YES)
			ord->contr->pending_sell_close_size -= quantity;
	}
}
//发出买指令等待被买的数量（包括开平）或者 发出卖指令等待被卖的数量（包括开平），
int OrderHash::open_order_size(Contract * a_contr, DIRECTION a_side)
{
	if (a_side == ORDER_BUY) {
		return get_pending_buy_size(a_contr);
	}
	else if (a_side == ORDER_SELL) {
		return get_pending_sell_size(a_contr);
	}
	return -1;
}
