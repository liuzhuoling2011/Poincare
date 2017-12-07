#include "vwap_hash.h"
#include "utils/utils.h"
#include "utils/log.h"

#define HASH_ORDER_SIZE	  (2048)
#define MAX_VWAP_ORDER_SIZE	  (2048)

VwapParentOrderHash::VwapParentOrderHash(){
	p_bucket = NULL;
	p_order_head = NULL;
	char *ht = NULL;

	int size = MAX_VWAP_ORDER_SIZE * sizeof(parent_order) +
		HASH_ORDER_SIZE * sizeof(hash_vwap_bucket);

	void * all_mem = calloc(1, size);
	if (all_mem == NULL) {
		PRINT_ERROR("init_hash_table malloc memory fail.");
		return;
	}

	ht = (char *)all_mem;
	p_order_head = (parent_order *)(all_mem);
	p_bucket = (hash_vwap_bucket *)(ht + MAX_VWAP_ORDER_SIZE * sizeof(parent_order));

	p_buy_list = (list_head *)malloc(sizeof(list_head));
	p_sell_list = (list_head *)malloc(sizeof(list_head));
	p_free_list = (list_head *)malloc(sizeof(list_head));
	p_search_list = (list_head *)malloc(sizeof(list_head));

	clear();
}

VwapParentOrderHash::~VwapParentOrderHash(){
	free(p_buy_list);
	free(p_sell_list);
	free(p_free_list);
	free(p_search_list);
	free(p_order_head);
}

void
VwapParentOrderHash::clear(){
	m_use_count = 0;

	for (int i = 0; i < HASH_ORDER_SIZE; i++){
		INIT_LIST_HEAD(&p_bucket[i].head);
	}

	INIT_LIST_HEAD(p_buy_list);
	INIT_LIST_HEAD(p_sell_list);
	INIT_LIST_HEAD(p_free_list);
	INIT_LIST_HEAD(p_search_list);

	parent_order temp;
	set_order_default(&temp);
	for (int i = 0; i < MAX_VWAP_ORDER_SIZE; i++){
		memcpy(p_order_head + i, &temp, sizeof(parent_order));
		INIT_LIST_HEAD(&p_order_head[i].hs_link);
		INIT_LIST_HEAD(&p_order_head[i].pd_link);
		list_add_after(&p_order_head[i].fr_link, p_free_list);
		INIT_LIST_HEAD(&p_order_head[i].search_link);
	}
}


int
VwapParentOrderHash::get_hash_value(int index){
	return index % HASH_ORDER_SIZE;
}

parent_order*
VwapParentOrderHash::get_first_free_order()
{
	if (p_free_list->next != p_free_list){
		parent_order* l_ord = list_entry(p_free_list->next, parent_order, fr_link);
		list_del(&l_ord->fr_link);
		INIT_LIST_HEAD(&l_ord->fr_link);
		return l_ord;
	}
	return NULL;
}

parent_order*
VwapParentOrderHash::update_order(int index, Contract* contr,
double price, int size, DIRECTION side,
uint64_t sig_id, int start_time, int end_time)
{
	int hash_value = get_hash_value(index);
	if (hash_value < 0 || hash_value > HASH_ORDER_SIZE - 1) {
		PRINT_ERROR("Hash value %d is illegal", hash_value);
		return NULL;
	}

	parent_order *item = get_first_free_order();
	if (item == NULL){
		PRINT_ERROR("VwapHash Can't get free order.");
		return NULL;
	}

	set_parameter(item, contr, price, size, side, sig_id, start_time, end_time);
	list_add_after(&item->hs_link, &p_bucket[hash_value].head);

	if (item->VWAP_side == ORDER_BUY){
		list_add_after(&item->pd_link, p_buy_list);
	}
	else if (item->VWAP_side == ORDER_SELL){
		list_add_after(&item->pd_link, p_sell_list);
	}

	m_use_count++;
	return item;
}

void
VwapParentOrderHash::set_parameter(parent_order *ord, Contract* a_contr,
double a_price, int a_size, DIRECTION a_side,
uint64_t a_signal_id, int a_start_time, int a_end_time)
{
	set_order_default(ord);
	ord->parent_order_id = a_signal_id;
	ord->contr = a_contr;
	my_strncpy(ord->symbol, a_contr->symbol, SYMBOL_LEN);
	ord->VWAP_start_time = a_start_time;
	ord->VWAP_end_time = a_end_time;
	ord->nxt_tranche_start_time = a_start_time;
	ord->VWAP_side = a_side;
	ord->VWAP_limit_price = a_price;
	ord->VWAP_parent_order_size = a_size;

}

void
VwapParentOrderHash::push_back(parent_order* item, list_head *a_list)
{
	list_add_after(&item->search_link, a_list);
}

bool VwapParentOrderHash::erase(int index)
{
	parent_order *ord = query_order(index);
	if (ord == NULL)
		return false;

	return erase(ord);
}

bool VwapParentOrderHash::erase(parent_order *ord)
{
	if (ord->hs_link.next == &ord->hs_link)
		return false;

	list_del(&ord->hs_link);
	list_del(&ord->pd_link);
	list_add_after(&ord->fr_link, p_free_list);

	m_use_count--;
	return true;
}

void VwapParentOrderHash::set_order_default(parent_order *ord)
{
	ord->parent_order_id = 0;
	ord->contr = NULL;
	memset(ord->symbol, 0, SYMBOL_LEN);

	ord->VWAP_start_time = 0;
	ord->VWAP_end_time = 0;
	ord->nxt_tranche_start_time = 0;
	ord->VWAP_side = UNDEFINED_SIDE;
	ord->VWAP_flag = false;
	ord->VWAP_limit_price = 0;
	ord->VWAP_parent_order_size = 0;
	
	ord->tranche_time = 0;

	ord->VWAP_child_order_size = 0;
	ord->VWAP_flag_in_the_market = false;
	ord->VWAP_indicator = 0;
	ord->VWAP_reserved_qty = 0;
	ord->VWAP_start_target_size = 0;
	ord->VWAP_last_order_additional_shares = 0;   //Vtraded upper bound
	ord->VWAP_traded_size_tminusone = 0;

	//char VWAP_order_type[SYMBOL_LEN];
	memset(ord->VWAP_order_type, 0, SYMBOL_LEN);
	ord->VWAP_int_time = 0;
	ord->VWAP_target_volume = 0;
	ord->FirstOrderVOL = 0;
	ord->FirstOrderAMT = 0;
	ord->VWAP_flag_first_order = false;

	ord->stream_index = 0;
}

parent_order *
VwapParentOrderHash::query_order(int index, uint64_t ord_id)
{
	struct list_head *pos = nullptr, *n = nullptr;
	parent_order *l_ord;

	int hash_value = get_hash_value(index);
	if (hash_value < 0 || hash_value > HASH_ORDER_SIZE - 1)
		return NULL;

	list_for_each_safe(pos, n, &p_bucket[hash_value].head){
		l_ord = list_entry(pos, parent_order, hs_link);
		if (l_ord->parent_order_id == ord_id) {
			return l_ord;
		}
	}

	return NULL;
}

parent_order *
VwapParentOrderHash::query_order(int index)
{
	return query_order(index, (long)index);
}

//return the count of order in the list
int
VwapParentOrderHash::get_buy_sell_list_size(DIRECTION a_side)
{
	struct list_head *pos, *n, *a_list = nullptr;
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

list_head *
VwapParentOrderHash::get_order_by_side(DIRECTION a_side)
{
	if (a_side == ORDER_BUY)
		return p_buy_list;
	if (a_side == ORDER_SELL)
		return p_sell_list;
	return NULL;
}

void
VwapParentOrderHash::get_active_order_list(list_head **a_buy_list, list_head **a_sell_list)
{
	*a_buy_list = p_buy_list;
	*a_sell_list = p_sell_list;
}

int
VwapParentOrderHash::get_free_order_size()
{
	struct list_head *pos, *n;
	int l_size = 0;

	list_for_each_safe(pos, n, p_free_list){
		l_size++;
	}
	return l_size;
}

int
VwapParentOrderHash::get_search_order_size()
{
	struct list_head *pos, *n;
	int l_size = 0;

	list_for_each_safe(pos, n, p_search_list){
		l_size++;
	}
	return l_size;
}

void
VwapParentOrderHash::search_vwap_order_by_instr(const char* a_symbol, list_head *a_list)
{
	struct list_head *pos, *n;
	parent_order *l_ord;
	INIT_LIST_HEAD(a_list);

	list_for_each_safe(pos, n, p_buy_list){
		l_ord = list_entry(pos, parent_order, pd_link);
		if (my_strcmp(l_ord->symbol, a_symbol) == 0)
			push_back(l_ord, a_list);
	}

	list_for_each_safe(pos, n, p_sell_list){
		l_ord = list_entry(pos, parent_order, pd_link);
		if (my_strcmp(l_ord->symbol, a_symbol) == 0)
			push_back(l_ord, a_list);
	}
}

void
VwapParentOrderHash::search_order_by_side_instr(DIRECTION a_side, const char* a_symbol, list_head *a_list)
{
	struct list_head *pos, *n;
	parent_order *l_ord;
	INIT_LIST_HEAD(a_list);

	if (a_side == ORDER_BUY){
		list_for_each_safe(pos, n, p_buy_list){
			l_ord = list_entry(pos, parent_order, pd_link);
			if (my_strcmp(l_ord->symbol, a_symbol) == 0)
				push_back(l_ord, a_list);
		}
	}
	else if (a_side == ORDER_SELL){
		list_for_each_safe(pos, n, p_sell_list){
			l_ord = list_entry(pos, parent_order, pd_link);
			if (my_strcmp(l_ord->symbol, a_symbol) == 0)
				push_back(l_ord, a_list);
		}
	}
}
