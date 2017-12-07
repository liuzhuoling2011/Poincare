//match_engine implementation
#include "match_engine.h"

#define  MAX_PRICE_SIZE   (1024 * 10) 
#define  MAX_TICK_SIZE    (1024 * 40) 
#define  MAX_ORDER_SIZE   (1024 * 20)
#define  ARRAY_SIZE(a)    (sizeof(a)/sizeof(a[0]))

static better_price_func  compare_price[2] = {better_buy, better_sell};
double get_midprice(double p1, double p2)
{
	return (compare(p1, 0) > 0 && compare(p2, 0) > 0) ? (p1 + p2) / 2 : p1 + p2;
}

//-----------MatchEngine external interface----------

MatchEngine::MatchEngine(char *symbol)
{
	m_is_malloc_node = false;
	m_place_hash = new PlaceOrderMap();
	clear();

	if (m_debug_flag) {
		char log_name[TOKEN_BUFFER_SIZE] = "";
		snprintf(log_name, TOKEN_BUFFER_SIZE, "logs/match_engine_%s.log", symbol);
		m_write_log.open(log_name, std::ios_base::trunc);
		m_write_log << "Current Match Engine Version: V1.1.0\n";
	}
}

MatchEngine::~MatchEngine()
{
	delete  m_place_hash;
	if (m_is_malloc_node){
		delete  m_place_orders;
		delete  m_price_data;
		delete  m_ticks_data;
		delete  m_split_data;
		delete  m_trade_rtn;
	}
	if (m_debug_flag) {
		m_write_log.close();
	}
}

// ������������ݿռ�
void 
MatchEngine::clear()
{
	m_entrust_num = 0;
	m_tick_counter = 0;
	m_place_num = 0;
	m_last_empty = false;
	m_last_numpy = NULL;
	INIT_LIST_HEAD(&m_buy_prices);
	INIT_LIST_HEAD(&m_sell_prices);
	memset(&m_quote, 0, sizeof(quote_info));
	m_place_hash->clear();
	if (m_is_malloc_node){
		m_price_data->reset();
		m_ticks_data->reset();
		m_place_orders->reset();
	}
}

void
MatchEngine::set_config(match_engine_config* cfg)
{
	memcpy(&m_cfg, cfg, sizeof(match_engine_config));
}

// ���δ����ɺ�Ĵ�����
void  MatchEngine::round_finish()
{
	if (m_debug_flag) {
		m_write_log << "Param exch=" << m_cfg.mode_cfg.exch << " product=" << m_cfg.mode_cfg.product
			<< " trade_ratio=" << m_cfg.mode_cfg.trade_ratio << std::endl;
	}
}

// ��鵱ǰ�����б��Ƿ�Ϊ�գ� true-�գ�
bool 
MatchEngine::is_empty_order()
{
	return list_empty(&m_buy_prices) && list_empty(&m_sell_prices);
}

void MatchEngine::convert_stock_quote(void *quote, int type)
{
	if (m_is_bar) {
		process_internal_bar(quote, type, &m_quote);
	} else {
		process_stock_book(quote, &m_quote);
	}
}

void MatchEngine::convert_future_quote(void *quote, int type)
{
	if (m_is_bar) {
		process_internal_bar(quote, type, &m_quote);
	} else {
		process_future_book(quote, &m_quote);
	}
}

void MatchEngine::malloc_node_data()
{
	m_is_malloc_node = true;
	m_place_orders = new MyArray(sizeof(order_node), MAX_ORDER_SIZE);
	m_price_data = new MyArray(sizeof(price_node), MAX_PRICE_SIZE);
	m_ticks_data = new MyArray(sizeof(tick_node), MAX_TICK_SIZE);
	m_split_data = new MyArray(sizeof(pv_pair), MAX_TRD_PV_CNT);
	m_trade_rtn = new MyArray(sizeof(signal_response), MAX_PLACE_SIZE);
}

// ��ȡ��ǰ����������
mat_quote *MatchEngine::get_current_quote(bool is_first)
{
	if (is_first){
		void *last_data = (m_market_type == KDB_QUOTE ? m_last_quote : m_last_numpy);
		if (last_data != NULL){
			if (m_is_stock) {
				convert_stock_quote(last_data, m_last_type);
			} else {
				convert_future_quote(last_data, m_last_type);
			}
		}
	}
	return &(m_quote.curr_quote);
}

// ��ѯ��ʷ������Ϣ
order_node *MatchEngine::get_order_node(uint64_t serial_no)
{
	auto need = m_place_hash->find(serial_no);
	if (need != m_place_hash->end()){
		return need->second;
	}else{
		return NULL;
	}
}

// �µ��Ľӿں���
void
MatchEngine::place_order(order_request* pl_ord, signal_response *ord_rtn)
{
	order_node* node = NULL;
	if (pl_ord->order_size <= 0) {
		ord_rtn->error_no = -1;
		strcpy(ord_rtn->error_info, "Send a negative order size!");
		ord_rtn->status = ME_ORDER_REJECTED;
		goto place_end;
	}

	if (m_place_num >= MAX_PLACE_SIZE){
		ord_rtn->error_no = -2;
		strcpy(ord_rtn->error_info, "Send more than 256 orders in a tick, reach our max orders allowed in a tick.");
		ord_rtn->status = ME_ORDER_REJECTED;
		goto place_end;
	}

	if (m_is_malloc_node == false){
		malloc_node_data();
	}

	node = (order_node *)m_place_orders->next();
	if (node == NULL){
		PRINT_ERROR("Can't get available order_node");
		ord_rtn->error_no = -3;
		strcpy(ord_rtn->error_info, "total place order number too large!");
		ord_rtn->status = ME_ORDER_REJECTED;
		goto place_end;
	}

	ord_rtn->error_no = 0;
	memcpy(&node->pl_ord, pl_ord, sizeof(order_request));
	node->entrust_no = (++m_entrust_num);
	ord_rtn->status = ME_ORDER_ENTRUSTED;
	m_place_array[m_place_num++] = node;
	node->data_flag = m_is_filter;

place_end:
	ord_rtn->serial_no = pl_ord->order_id;
	strlcpy(ord_rtn->symbol, pl_ord->symbol, MAX_NAME_SIZE);
	ord_rtn->direction = pl_ord->direction;
	ord_rtn->open_close = pl_ord->open_close;
	ord_rtn->exe_price = pl_ord->limit_price;
	ord_rtn->exe_volume = pl_ord->order_size;
	ord_rtn->strat_id = pl_ord->strat_id;
	ord_rtn->entrust_no = m_entrust_num;
}

// �����Ľӿں���
void
MatchEngine::cancel_order(order_request* cl_ord, signal_response *ord_rtn)
{
	auto need = m_place_hash->end();
	ord_rtn->error_no = 0;
	order_node* node = remove_order_from_pool(cl_ord);
	if (node != NULL){
		node->status = ME_ORDER_CANCEL;
		ord_rtn->status = node->status;
		goto canl_end;
	}

	//�ڶ��������б��в���
	need = m_place_hash->find(cl_ord->org_order_id);
	if (need == m_place_hash->end()){
		ord_rtn->error_no = -4;
		strcpy(ord_rtn->error_info, "cancel order serial_no error!");
		ord_rtn->serial_no = cl_ord->org_order_id;
		ord_rtn->strat_id = cl_ord->strat_id;
		ord_rtn->entrust_no = m_entrust_num;
		ord_rtn->status = ME_ORDER_CANCEL_REJECTED;
		strlcpy(ord_rtn->symbol, cl_ord->symbol, MAX_NAME_SIZE);
		ord_rtn->direction = cl_ord->direction;
		ord_rtn->open_close = cl_ord->open_close;
		ord_rtn->exe_price = 0;
		ord_rtn->exe_volume = 0;
		return;
	}

	node = need->second;
	if (node->status == ME_ORDER_SUCCESS
	 || node->status == ME_ORDER_CANCEL){
		ord_rtn->error_no = -5;
		strcpy(ord_rtn->error_info, "cancel order have finished!");
		ord_rtn->status = ME_ORDER_CANCEL_REJECTED;
		goto canl_end;
	}

	node->status = ME_ORDER_CANCEL;
	m_place_hash->erase(need);
	update_order_node(node, node->volume);
	ord_rtn->status = node->status;

canl_end:
	ord_rtn->serial_no = node->pl_ord.order_id;
	strlcpy(ord_rtn->symbol, node->pl_ord.symbol, MAX_NAME_SIZE);
	ord_rtn->direction = node->pl_ord.direction;
	ord_rtn->open_close = node->pl_ord.open_close;
	ord_rtn->exe_price = node->pl_ord.limit_price;
	ord_rtn->exe_volume = node->volume;
	ord_rtn->strat_id = node->pl_ord.strat_id;
	ord_rtn->entrust_no = m_entrust_num;
}

// �Ӷ������Ƴ���Ӧ����
order_node* 
MatchEngine::remove_order_from_pool(order_request* cl_ord)
{
	for (int i = 0; i < m_place_num; i++){
		order_node* node = m_place_array[i];
		if (node->pl_ord.order_id == cl_ord->org_order_id){
			for (int start = i; start < m_place_num - 1; start++){
				m_place_array[start] = m_place_array[start+1];
			}
			m_place_num--;
			return node;
		}
	}
	return NULL;
}

// ������Ľӿ�
void
MatchEngine::feed_stock_quote(int type, void* quote, int market, bool is_bar)
{
	m_mi_type = type;
	m_is_bar = is_bar;
	m_is_stock = true;
	m_market_type = market;
	m_is_filter = Filter;
	if (g_unlikely(type >= MI_SIZE)){
		return;
	}

	m_tick_counter++;
	if (m_quote.mult_way > 1){
		if (type == MI_MY_LEVEL1 || type == MI_DCE_LV1 
		 || type == MI_DCE_BEST_DEEP){
			goto Convert;
		}else{
			return;
		}
	}else{
		//�ж��Ƿ��·����Ĵ���
		add_mult_quote_way(type);
	}

Convert:
	if (!update_first_quote(type, quote)) {
		return;
	}

	memcpy(&m_quote.last_quote, &m_quote.curr_quote, sizeof(mat_quote));
	convert_stock_quote(quote, type);
	memcpy(&m_quote.opposite, &m_quote.curr_quote, sizeof(mat_quote));
	m_is_filter = Process;
}

void
MatchEngine::feed_future_quote(int type, void* quote, int market, bool is_bar)
{
	m_mi_type = type;
	m_is_bar = is_bar;
	m_is_stock = false;
	m_market_type = market;
	m_is_filter = Filter;
	if (g_unlikely(type >= MI_SIZE)) {
		return;
	}

	m_tick_counter++;
	if (m_quote.mult_way > 1) {
		if (type == MI_MY_LEVEL1 || type == MI_DCE_LV1
			|| type == MI_DCE_BEST_DEEP) {
			goto Convert;
		}  else {
			return;
		}
	}
	else {
		//�ж��Ƿ��·����Ĵ���
		add_mult_quote_way(type);
	}

Convert:
	if (!update_first_quote(type, quote)) {
		return;
	}

	memcpy(&m_quote.last_quote, &m_quote.curr_quote, sizeof(mat_quote));
	convert_future_quote(quote, type);
	memcpy(&m_quote.opposite, &m_quote.curr_quote, sizeof(mat_quote));
	m_is_filter = Process;
}

// ִ�д�ϵ���ں���
void 
MatchEngine::execute_match(void** trd_rtn_ar, int* rtn_cnt)
{
	if (m_is_malloc_node == false){
		*rtn_cnt = 0;
		*trd_rtn_ar = NULL;
		return;
	}

	if (g_unlikely(m_trade_rtn->size() > 0)){
		m_trade_rtn->reset();
	}
	if (m_is_filter == Filter){
		goto proc_end;
	}

	m_split_data->clear();
	calc_trade_volume();
	update_order_book();

	if (m_debug_flag) {
		show_price_list();
	}
	if (m_is_filter == Process){
		do_match_on_direct(DIR_BUY);
		do_match_on_direct(DIR_SELL);
	}

	if (m_debug_flag) {
		show_price_list(false);
	}

proc_end:
	*rtn_cnt = m_trade_rtn->size();
	*trd_rtn_ar = m_trade_rtn->head();
}

//-----------MatchEngine internal interface----------
// ���¶��������б�
void 
MatchEngine::update_order_book()
{
	price_node* pn = NULL;
	tick_node * tn = NULL;
	order_request* pl_ord = NULL;

	for (int i = 0; i < m_place_num; i++){
		order_node* node = m_place_array[i];
		pl_ord = &node->pl_ord;
		pn = add_price_node(pl_ord->direction, pl_ord->limit_price);
		update_market_book(pn, pl_ord->direction, pl_ord->limit_price);

		tn = find_tick_node(pn, m_tick_counter, OWNER_MY);
		if (tn == NULL){
			tn = add_tick_node(pn, m_tick_counter, OWNER_MY);
		}
		add_order_node(tn, node);
		node->volume = pl_ord->order_size;
		tn->volume += pl_ord->order_size;
		pn->volume += pl_ord->order_size;
	}
	m_place_num = 0;
}

// �����г��Ķ���
void  
MatchEngine::update_market_book(price_node* pn, int direct, double price)
{
	tick_node * tn = find_tick_node(pn, m_tick_counter, OWNER_MKT);
	if (tn == NULL){
		pn->last_vol = find_market_volume(direct, price, false);
		if (pn->last_vol < 0) pn->last_vol = 0;
		pn->curr_vol = find_market_volume(direct, price, true);
		if (pn->curr_vol < 0) pn->curr_vol = pn->last_vol;
		int	split_vol = find_trade_volume(price);
		int	rto_vol = int(m_cfg.mode_cfg.trade_ratio*(pn->curr_vol - pn->last_vol + split_vol));
		int mkt_vol = pn->last_vol + rto_vol - pn->mkt_vol;
		if (g_unlikely(mkt_vol > 0)){
			tn = add_tick_node(pn, m_tick_counter, OWNER_MKT);
			tn->volume += mkt_vol;
			pn->mkt_vol += mkt_vol;
		}
	}
}

// ���price���
price_node* 
MatchEngine::add_price_node(int direct, double price)
{
	price_node* node = NULL;
	list_head* pos, *insert_pos; 
	list_head* head = get_price_handle(direct);
	int cmp_vol, bad_ret = BAD_RESULT(direct);

	insert_pos = head;
	list_for_each(pos, head){
		node = list_entry(pos, price_node, list);
		cmp_vol = compare(price, node->price);
		if (cmp_vol == 0)  return node;
		if (cmp_vol == bad_ret) {
			insert_pos = pos;
			goto not_find;
		}else{
			insert_pos = pos->next;
		}
	}

not_find:
	node = (price_node *)m_price_data->next();
	if (node == NULL){
		PRINT_ERROR("Can't get available price_node");
		assert(0);
	}else{
		node->used = 1;
		node->price = price;
		node->direct = direct;
		node->volume = 0;
		node->mkt_vol = 0;
		node->last_vol = 0;
		node->curr_vol = 0;
		INIT_LIST_HEAD(&node->list);
		INIT_LIST_HEAD(&node->tick_list);
		list_add_before(&node->list, insert_pos);
	}
	return node;
}

// ��ѯ�����tick���
tick_node*  
MatchEngine::find_tick_node(price_node* pn, long tick, int owner)
{
	tick_node *node = NULL;
	list_head* head, *pos;
	head = &pn->tick_list;

	list_for_each(pos, head){
		node = list_entry(pos, tick_node, list);
		if (tick < node->tick){
			return NULL;
		}
		if (node->tick == tick
		 && node->owner == owner){
			return node;
		}
	}
	return NULL;
}

// ����µ�tick���
tick_node*  
MatchEngine::add_tick_node(price_node* pn, long tick, int owner)
{
	list_head* head = &pn->tick_list;
	tick_node *node = (tick_node *)m_ticks_data->next();
	if (node == NULL){
		PRINT_ERROR("Error: Can't get available tick_node");
		assert(0);
	}else{
		node->used = 1;
		node->tick = tick;
		node->owner = owner;
		node->parent = pn;
		node->volume = 0;
		INIT_LIST_HEAD(&node->ord_list);
		list_add_before(&node->list, head);
	}
	return node;
}

// ����µ�order���
order_node*
MatchEngine::add_order_node(tick_node * tn, order_node* node)
{
	list_head* head = &tn->ord_list;
	node->parent = tn;
	node->status = ME_ORDER_ENTRUSTED;
	list_add_before(&node->list, head);
	m_place_hash->insert(std::make_pair(node->pl_ord.order_id, node));
	return node;
}

// ��ȡ��/���б�ľ��
list_head *
MatchEngine::get_price_handle(int direct){
	if (direct == DIR_BUY){
		return &m_buy_prices;
	}else{
		return &m_sell_prices;
	}
}

// ����tick�������
void  
MatchEngine::update_tick_node(tick_node* tn, int trad_vol)
{
	price_node* pn = tn->parent;
	list_head* pos, *p, *head;
	tn->volume -= trad_vol;	
	if (tn->owner == OWNER_MKT){
		pn->mkt_vol -= trad_vol;
	}else{
		pn->volume -= trad_vol;
	}
	if (tn->volume <= 0){
		list_del(&tn->list);
		tn->used = 0;
	}
	if (pn->volume <= 0){
		list_del(&pn->list);
		pn->used = 0;
		head = &pn->tick_list;
		list_for_each_safe(pos, p, head){
			tick_node *node = list_entry(pos, tick_node, list);
			list_del(&node->list);
			node->used = 0;
		}
	}
}

// ����order�������
void
MatchEngine::update_order_node(order_node* on, int trad_vol)
{
	on->volume -= trad_vol;
	if (on->volume <= 0){
		list_del(&on->list);
	}
	tick_node* tn = on->parent;
	update_tick_node(tn, trad_vol);
}

void 
MatchEngine::set_order_result(order_node* order, signal_response *rtn)
{
	rtn->serial_no = order->pl_ord.order_id;
	my_strncpy(rtn->symbol, order->pl_ord.symbol, MAX_SYMBOL_SIZE);
	rtn->direction = order->pl_ord.direction;
	rtn->open_close = order->pl_ord.open_close;
	rtn->exe_price = order->trade_price;
	rtn->exe_volume = order->trade_vol;
	rtn->status = order->status;
	rtn->error_no = 0;
	rtn->strat_id = order->pl_ord.strat_id;
	rtn->entrust_no = m_entrust_num;
}

// �ڲ�ͬ�ķ����Ͻ��д��
void  
MatchEngine::do_match_on_direct(int direct)
{
	price_node* node = NULL;
	list_head* pos, *p, *head;

	head = get_price_handle(direct);
	list_for_each_safe(pos, p, head){
		node = list_entry(pos, price_node, list);
		int over = is_overstep(node->direct, node->price);
		if (g_unlikely(over)){
			do_match_on_overstep(node, direct);
		}else{
			do_match_on_price(node, direct);
		}
	}
}

// �ڲ�ͬ�ļ�λ�Ͻ��д��
void  
MatchEngine::do_match_on_price(price_node *pn, int direct)
{
	int self_trade = 0;
	pn->last_vol = pn->curr_vol;
	pn->curr_vol = find_market_volume(direct, pn->price, true);
	if (pn->curr_vol < 0) pn->curr_vol = pn->last_vol;
	int	split_vol = find_trade_volume(pn->price);
	int canl_vol = pn->last_vol - pn->curr_vol - split_vol; //cancel volume
	do_match_on_tick(pn, canl_vol, false, self_trade);

	double avg_price = 0.0;
	int opp_vol = find_opposite_volume(direct, pn->price, avg_price);
	do_match_on_opposite(pn, opp_vol, avg_price);
	do_match_can_trade(pn);
}

void  
MatchEngine::do_match_on_overstep(price_node *pn, int direct)
{
	tick_node *node = NULL;
	list_head* pos, *p, *head;
	head = &pn->tick_list;
	double exe_price = 0.0;

	list_for_each_safe(pos, p, head){
		node = list_entry(pos, tick_node, list);
		int trade_vol = calc_overstep_data(direct, node->volume, exe_price);
		if (trade_vol > 0){
			trade_vol -= do_match_on_order(node, trade_vol, exe_price);
			update_tick_node(node, trade_vol);
		}
	}
}

void 
MatchEngine::do_match_can_trade(price_node *pn)
{
	int self_trade = 0;
	find_trade_volume(pn->price, pn->direct);
	for (int i = 0; i < m_can_trade_num; i++){
		pv_pair *trade_pair = m_can_trade_list[i];
		trade_pair->vol = do_match_on_tick(pn, trade_pair->vol, true, self_trade);
	}
}

// �ڲ�ͬ��tick�Ͻ��д��
int
MatchEngine::do_match_on_tick(price_node *pn, int exe_vol, bool is_all, int &self_trade)
{
	self_trade = 0;
	if (g_unlikely(exe_vol <= 0)) return 0;
	tick_node *node = NULL;
	list_head* pos, *p, *head;
	int trade_vol, remain_vol = exe_vol;

	head = &pn->tick_list;
	list_for_each_safe(pos, p, head){
		node = list_entry(pos, tick_node, list);
		if (is_all || node->owner == OWNER_MKT){
			trade_vol = MIN(node->volume, remain_vol);
			if (node->owner == OWNER_MY){
				int real_vol = do_match_on_order(node, trade_vol);
				trade_vol -= real_vol;
				self_trade += trade_vol;
			}

			remain_vol -= trade_vol;
			update_tick_node(node, trade_vol);
			if (remain_vol <= 0) break;
		}
	}
	return remain_vol;
}

// �ڶ�λ�Ͻ��д��
int  
MatchEngine::do_match_on_opposite(price_node *pn, int exe_vol, double exe_price)
{
	if (g_unlikely(exe_vol <= 0)) return 0;
	tick_node *node = NULL;
	list_head* pos, *p, *head;
	int remain_vol = exe_vol;

	head = &pn->tick_list;
	list_for_each_safe(pos, p, head){
		node = list_entry(pos, tick_node, list);
		if (node->volume > remain_vol) break;
		if (node->owner == OWNER_MY){
			do_match_on_order(node, node->volume, exe_price);
		}
		remain_vol -= node->volume;
		update_tick_node(node, node->volume);
		if (remain_vol <= 0) break;
	}
	return remain_vol;
}

#if 0
int  
MatchEngine::do_match_on_order(tick_node *tn, int exe_vol, double exe_price)
{
	order_node * node = NULL;
	list_head* pos, *p, *head;
	int trade_vol = 0;
	int remain_vol = exe_vol;
	head = &tn->ord_list;

	list_for_each_safe(pos, p, head){
		node = list_entry(pos, order_node, list);
		if (is_skip_order(node)) continue;
		trade_vol = MIN(node->volume, remain_vol);
		if (g_unlikely(trade_vol <= 0)) continue;

		node->volume -= trade_vol;
		remain_vol -= trade_vol;
		node->trade_vol = trade_vol;
		if (g_unlikely(exe_price > 0)){
			node->trade_price = exe_price;
		}else{
			node->trade_price = node->pl_ord.limit_price;
		}
		if (node->volume <= 0){
			node->status = STAT_FULL;
			list_del(&node->list);
		}else{
			node->status = STAT_PARTIAL;
		}

		set_order_result(node);
		if (remain_vol <= 0) break;
	}
	return remain_vol;
}
#endif

void 
MatchEngine::statis_order(tick_node *tn, int &count, int &total)
{
	order_node * node = NULL;
	list_head* head, *pos;
	count = total = 0;
	head = &tn->ord_list;

	list_for_each(pos, head){
		node = list_entry(pos, order_node, list);
		if (is_skip_order(node)) continue;
		total += node->pl_ord.order_size;
		count++;
	}
}

// �����������д��
int
MatchEngine::do_match_on_order(tick_node *tn, int exe_vol, double exe_price)
{
	signal_response *resp = NULL;
	order_node * node = NULL;
	list_head* pos, *p, *head;
	int trade_vol = 0, proc_index = 0;
	int remain_vol = exe_vol;
	int order_count, total_pos;

	statis_order(tn, order_count, total_pos);
	if (total_pos <= 0) return remain_vol;

	head = &tn->ord_list;
	list_for_each_safe(pos, p, head){
		node = list_entry(pos, order_node, list);
		if (is_skip_order(node)) continue;

		if (proc_index >= order_count - 1){ //last order
			trade_vol = MIN(node->volume, remain_vol);
		}else{
			int avg_vol = (int)ceil(remain_vol * node->pl_ord.order_size / (double)total_pos);
			trade_vol = MIN(node->volume, avg_vol);
		}

		trade_vol = round_stock_vol(trade_vol);
		if (g_unlikely(trade_vol <= 0)) continue;
		resp = (signal_response *)m_trade_rtn->next();
		if (resp == NULL) break;

		node->volume -= trade_vol;
		remain_vol -= trade_vol;
		node->trade_vol = trade_vol;
		total_pos -= node->pl_ord.order_size;
		if (g_unlikely(exe_price > 0)){
			node->trade_price = exe_price;
		}else{
			node->trade_price = node->pl_ord.limit_price;
		}
		if (node->volume <= 0){
			node->status = ME_ORDER_SUCCESS;
			list_del(&node->list);
		}else{
			node->status = ME_ORDER_PARTED;
		}

		proc_index++;
		set_order_result(node, resp);
		if (remain_vol <= 0) break;
	}
	return remain_vol;
}

// ���۲�ֵĺ���
void
MatchEngine::calc_trade_volume()
{
	pv_pair *high = (pv_pair *)m_split_data->next();
	pv_pair *low = (pv_pair *)m_split_data->next();

	mat_quote *q_curr = &m_quote.curr_quote;
	mat_quote *q_last = &m_quote.last_quote;
	pv_pair* b_last = q_last->bs_pv[0];
	pv_pair* s_last = q_last->bs_pv[1];

	double tickSize = m_cfg.pi.unit;
	int trade_unit = m_cfg.pi.multiple;
	double diff_amount = (q_curr->amount - q_last->amount) / trade_unit;
	int diff_vol = q_curr->total_volume - q_last->total_volume;

	if (m_cfg.pi.xchg_code == CZCE || (diff_amount == 0 && diff_vol > 0)){
		double midp1 = get_midprice(q_curr->bs_pv[0][0].price, q_curr->bs_pv[1][0].price);
		double midp0 = get_midprice(b_last[0].price, s_last[0].price);
		//////////////////normal////////////////////
		//low->price = int((midp0 + q_curr->last_price) / 2.0 / tickSize)*tickSize;
		//high->price = low->price + tickSize;
		//if (fabs(midp1 - midp0) < tickSize / 2){
		//	high->vol = diff_vol / 2;
		//}
		//else if (midp1 - midp0 > tickSize / 2){
		//	high->vol = diff_vol;
		//}
		//else{
		//	high->vol = 0;
		//}
		//low->vol = diff_vol - high->vol;

		//////////Here By Zhang.jialiang modify////////
		if (fabs(midp1 - midp0) < PRECISION){
			low->price = int(midp1 / tickSize)*tickSize;
			if (low->price < m_quote.low_limit - PRECISION){
				high->price = int(midp1 / tickSize + 0.5)*tickSize;
				low->vol = 0;
				high->vol = diff_vol;
			}else{
				high->price = low->price + tickSize;
				if (fabs(midp1 - low->price) < PRECISION || high->price > m_quote.high_limit + PRECISION){
					low->vol = diff_vol;
					high->vol = 0;
				}else{
					high->vol = diff_vol / 2;
					low->vol = diff_vol - high->vol;
				}
			}
		}
		else if (midp1 - midp0 >= PRECISION){
			low->price = int(q_curr->bs_pv[0][0].price / tickSize)*tickSize;
			if (fabs(low->price) < 2 * tickSize || low->price < m_quote.low_limit - PRECISION){
				high->price = int(q_curr->bs_pv[1][0].price / tickSize + 0.5)*tickSize;
				low->vol = 0;
				high->vol = diff_vol;
			}else{
				high->price = low->price + tickSize;
				if (fabs(q_curr->bs_pv[0][0].price - low->price) < PRECISION || high->price > m_quote.high_limit + PRECISION){
					low->vol = diff_vol;
					high->vol = 0;
				}else{
					high->vol = diff_vol / 2;
					low->vol = diff_vol - high->vol;
				}
			}
		}
		else{
			low->price = int(q_curr->bs_pv[1][0].price / tickSize)*tickSize;
			if (low->price < m_quote.low_limit - PRECISION){
				high->price = int(q_curr->bs_pv[1][0].price / tickSize + 0.5)*tickSize;
				low->vol = 0;
				high->vol = diff_vol;
			}else{
				high->price = low->price + tickSize;
				if (fabs(q_curr->bs_pv[1][0].price - low->price) < PRECISION || high->price > m_quote.high_limit + PRECISION){
					low->vol = diff_vol;
					high->vol = 0;
				}else{
					high->vol = diff_vol / 2;
					low->vol = diff_vol - high->vol;
				}
			}
		}
	}else if (diff_amount > 0 && diff_vol > 0){
		double p0 = int(diff_amount / diff_vol / tickSize) * tickSize;
		high->vol = (int)((diff_amount - diff_vol * p0 + PRECISION) / tickSize);
		low->vol = diff_vol - high->vol;
		low->price = p0;
		high->price = p0 + tickSize;
	}

	if ((high->price > 0 && high->price < q_curr->last_price - 10 * tickSize)
		|| (low->price > 0 && low->price > q_curr->last_price + 10 * tickSize)){
		printf("Warning: Split price is wrong!\n");
	}
}

// ���Ҽ�λƥ�����
int
MatchEngine::find_trade_volume(double price)
{
	for (unsigned int i = 0; i < m_split_data->size(); i++){
		pv_pair* pair = (pv_pair *)m_split_data->at(i);
		if (compare(price, pair->price) == 0){
			return pair->vol;
		}
	}
	return 0;
}

void 
MatchEngine::find_trade_volume(double price, int direct)
{
	pv_pair* pair = NULL;
	m_can_trade_num = 0;
	//Buy: price >= split_price  add pair
	//Sell: price <= split_price  add pair
	if (direct == DIR_BUY){
		for (unsigned int i = 0; i < m_split_data->size(); i++){
			pair = (pv_pair *)m_split_data->at(i);
			if (is_can_trade(pair->price, price, direct)
				|| compare(price, pair->price) == 0){
				m_can_trade_list[m_can_trade_num++] = pair;
			}
		}
	}else{
		for (int i = (int)m_split_data->size() - 1; i >= 0; i--){
			pair = (pv_pair *)m_split_data->at(i);
			if (is_can_trade(pair->price, price, direct)
				|| compare(price, pair->price) == 0){
				m_can_trade_list[m_can_trade_num++] = pair;
			}
		}
	}
}

// ���ҵ�ǰ����һ����tick��ƥ�����
int  
MatchEngine::find_market_volume(int direct, double price, bool is_curr)
{
	pv_pair* pv_ar = NULL;
	if (is_curr){
		pv_ar = m_quote.curr_quote.bs_pv[direct];
	}else{
		pv_ar = m_quote.last_quote.bs_pv[direct];
	}

	int cmp_vol, bad_ret = BAD_RESULT(direct);
	for (int i = 0; i < QUOTE_LVL_CNT; i++) {
		if (pv_ar[i].price <= 0)  goto proc_end;
		cmp_vol = compare(price, pv_ar[i].price);
		if (cmp_vol == bad_ret)  goto proc_end;
		if (cmp_vol == 0) return  pv_ar[i].vol;
	}

proc_end:
	return is_market_shift(direct, price, is_curr);
}

int 
MatchEngine::find_opposite_volume(int direct, double price, double &exe_price)
{
	pv_pair* pv_ar = m_quote.opposite.bs_pv[SWITCH_DIR(direct)];
	int cmp_vol, bad_ret = BAD_RESULT(direct);

	int total_vol = 0;
	double total_amount = 0.0;
	for (int i = 0; i < QUOTE_LVL_CNT; i++) {
		if (pv_ar[i].price <= 0)  return 0;
		cmp_vol = compare(price, pv_ar[i].price);
		if (cmp_vol == bad_ret){
			total_amount += pv_ar[i].price * pv_ar[i].vol;
			total_vol += pv_ar[i].vol;
			continue;
		}else if (cmp_vol == 0) {
			exe_price = pv_ar[i].price;
			return  pv_ar[i].vol;
		}else{
			exe_price = total_vol > 0 ? total_amount / total_vol : 0;
			return (i > 0 ? pv_ar[i - 1].vol : 0);
		}
	}
	return 0;
}

int   
MatchEngine::is_overstep(int direct, double price)
{
	pv_pair* pv_ar = m_quote.curr_quote.bs_pv[SWITCH_DIR(direct)];
	if (pv_ar[QUOTE_LVL_CNT - 1].price > 0){
		return (compare(price, pv_ar[QUOTE_LVL_CNT - 1].price) == BAD_RESULT(direct));
	}else{
		return 0;
	}
}

int 
MatchEngine::is_market_shift(int direct, double price, bool is_curr)
{
	int use_level = QUOTE_LVL_CNT - 1;
	if (m_mi_type == MI_MY_LEVEL1 || m_mi_type == MI_DCE_LV1){
		use_level = 0;
	}

	double mkt_price = 0.0;
	if (is_curr){
		mkt_price = m_quote.curr_quote.bs_pv[direct][use_level].price;
	}else{
		mkt_price = m_quote.last_quote.bs_pv[direct][use_level].price;
	}
	 
	if (compare(mkt_price, price) == BAD_RESULT(direct)){
		return -1;
	}else{
		return 0;
	}
}

// ��·����ʱ���˺���
bool  
MatchEngine::is_skip_order(order_node* node)
{
	if (m_mi_type == MI_SHFE_MY_LVL3 
	&& m_tick_counter == node->parent->tick
	&& node->data_flag == Filter){
		return true;
	}
	return false;
}

int  
MatchEngine::round_stock_vol(int exe_vol)
{
	if (m_cfg.pi.xchg_code == SSE || m_cfg.pi.xchg_code == SZSE){
		return exe_vol / 100 * 100;
	}else{
		return exe_vol;
	}
}

int  
MatchEngine::is_can_trade(double market, double self, int direct)
{
	return compare_price[direct](self, market);
}

int 
MatchEngine::calc_overstep_data(int direct, int volume, double &exe_price)
{
	double total_amount = 0.0;
	int total_vol = 0, real_vol = 0, trade_vol[QUOTE_LVL_CNT];
	pv_pair* pv_ar = m_quote.opposite.bs_pv[SWITCH_DIR(direct)];
	for (int i = 0; i < QUOTE_LVL_CNT; i++){
		trade_vol[i] = int(pv_ar[i].vol * m_cfg.mode_cfg.trade_ratio);
		total_vol += trade_vol[i];
		total_amount += pv_ar[i].price * trade_vol[i];
	}
	exe_price = total_amount / total_vol;
	if (total_vol > volume){
		for (int i = 0; i < QUOTE_LVL_CNT-1; i++){
			trade_vol[i] = int(volume * trade_vol[i] / total_vol);
			real_vol += trade_vol[i];
		}
		int remain_vol = pv_ar[4].vol > (volume-real_vol) ? (volume-real_vol) : pv_ar[4].vol;
		trade_vol[4] = remain_vol;
		total_vol = volume;
	}

	for (int i = 0; i < QUOTE_LVL_CNT; i++){
		pv_ar[i].vol -= trade_vol[i];
	}
	return total_vol;
}

void  
MatchEngine::add_mult_quote_way(int m_type)
{
	for (int i = 0; i < m_quote.mult_way; i++){
		if (m_quote.mi_types[i] == m_type) return;
	}
	m_quote.mi_types[m_quote.mult_way++] = m_type;
}

bool  
MatchEngine::update_first_quote(int type, void *quote)
{
	bool result = true;
	if (m_place_num == 0 && is_empty_order()){
		m_last_empty = true;
		result = false;
		goto proc_end;
	}else{
		if (m_last_empty){ //update last REQUEST_TYPE_QUOTE 
			void *last_data = (m_market_type == KDB_QUOTE ? m_last_quote : m_last_numpy);
			if (m_is_stock) {
				convert_stock_quote(last_data, m_last_type);
			} else {
				convert_future_quote(last_data, m_last_type);
			}
		}
		m_last_empty = false;
		result = true;
		goto proc_end;
	}

proc_end:
	m_last_type = type;
	if (m_market_type == KDB_QUOTE){
		if (m_is_stock) {
			memcpy(m_last_quote, quote, sizeof(Stock_Internal_Book));
		} else {
			memcpy(m_last_quote, quote, sizeof(Futures_Internal_Book));
		}
	}else{
		m_last_numpy = quote;
	}
	return result;
}

void MatchEngine::show_price_list(bool flag)
{
	price_node* node = NULL;
	list_head* pos, *p, *head;
	if (m_tick_counter > 0){
		m_write_log << "tick=" << m_tick_counter << " split:";
		for (unsigned int i = 0; i < m_split_data->size(); i++){
			pv_pair *tmp = (pv_pair *)m_split_data->at(i);
			m_write_log << " price=" << tmp->price << " vol=" << tmp->vol;
		}
		m_write_log << std::endl;
		if (flag){
			pv_pair* pv_buy = m_quote.curr_quote.bs_pv[0];
			pv_pair* pv_sel = m_quote.curr_quote.bs_pv[1];
			m_write_log << "Time " << m_quote.curr_quote.int_time << " buy " << pv_buy[0].price << "#" << pv_buy[0].vol << "  "
				<< pv_buy[1].price << "#" << pv_buy[1].vol << "  " << pv_buy[2].price << "#" << pv_buy[2].vol
				<< " | sell " << pv_sel[0].price << "#" << pv_sel[0].vol << "  "
				<< pv_sel[1].price << "#" << pv_sel[1].vol << "  " << pv_sel[2].price << "#" << pv_sel[2].vol << std::endl;
		}
		head = get_price_handle(DIR_SELL);
		list_for_each_safe(pos, p, head){
			node = list_entry(pos, price_node, list);
			if (node->volume > 0){
				m_write_log << "Sell List:";
				show_tick_list(node);
				m_write_log << std::endl;
			}
		}
		head = get_price_handle(DIR_BUY);
		list_for_each_safe(pos, p, head){
			node = list_entry(pos, price_node, list);
			if (node->volume > 0){
				m_write_log << "Buy List:";
				show_tick_list(node);
				m_write_log << std::endl;
			}
		}
	}
}

void MatchEngine::show_tick_list(price_node* pn)
{
	list_head* head, *pos;
	head = &pn->tick_list;
	m_write_log << "price=" << pn->price << " volume=" << pn->volume << " mkt_vol=" << pn->mkt_vol;
	list_for_each(pos, head){
		tick_node *node = list_entry(pos, tick_node, list);
		m_write_log << " tick=" << node->tick << " vol=" << node->volume << " owner=" << node->owner;
		show_order_list(node);
	}
}

void MatchEngine::show_order_list(tick_node *tn)
{
	list_head* head, *pos;
	head = &tn->ord_list;
	list_for_each(pos, head){
		order_node * node = list_entry(pos, order_node, list);
		m_write_log << " order=" << node->volume << " status=" << node->status;
	}
}


