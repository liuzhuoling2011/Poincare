//OriginMatch engine implementation
#include "extend_match.h"
#include <assert.h>

#define  EXCH_CZCE            'C'
#define  MAX_STRAT_SIZE       (1024 * 4)

static better_price_func  compare_price[2] = { better_buy, better_sell };
static int get_decimal_count(double val)
{
	int  count = 0;
	long tmp_val = (long)val;
	while (compare(tmp_val, val) != 0) {
		val *= 10;
		tmp_val = (long)val;
		count++;
	}
	return count;
}

//------------OriginMatch implementation-------------
OriginMatch::OriginMatch(char *symbol): MatchEngine(symbol)
{
	m_factor = 1;
	m_trd_pv_cnt = 0;
	memset(pv_ar_data, 0, sizeof(pv_ar_data));
};

OriginMatch::~OriginMatch()
{
	if (m_is_malloc_node){
		delete m_strat_data;
	}
}

void OriginMatch::clear()
{
	MatchEngine::clear();
	m_trd_pv_cnt = 0;
	memset(pv_ar_data, 0, sizeof(pv_ar_data));
	if (m_is_malloc_node){
		m_strat_data->reset();
	}
}

void OriginMatch::set_config(match_engine_config* cfg)
{
	MatchEngine::set_config(cfg);
	int dec_vol = get_decimal_count(cfg->pi.unit);
	m_factor = (int)std::pow(10, dec_vol);
}

void  
OriginMatch::place_order(order_request* pl_ord, signal_response *ord_rtn)
{
	ord_rtn->error_no = 0;
	if (pl_ord->order_size <= 0) {
		ord_rtn->status = ME_ORDER_REJECTED;
		goto place_end;
	}

	if (m_is_malloc_node == false){
		m_strat_data = new MyArray(sizeof(strat_node), MAX_STRAT_SIZE);
		malloc_node_data();
	}

	m_entrust_num++;
	if(m_is_stock) {
		process_stock_book(m_last_quote, &m_quote);
	} else {
		process_future_book(m_last_quote, &m_quote);
	}
	if (ogn_update_order_book(pl_ord)){
		ord_rtn->status = ME_ORDER_ENTRUSTED;
	}else{
		assert(0);
	}

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

void  
OriginMatch::cancel_order(order_request* cl_ord, signal_response *ord_rtn)
{
	order_node *on = NULL;
	strat_node *sn = NULL;
	tick_node  *tn = NULL;
	price_node *pn = NULL;

	ord_rtn->error_no = 0;
	auto need = m_place_hash->find(cl_ord->org_order_id);
	if (need == m_place_hash->end()){
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

	on = (order_node *)need->second;
	if (on->status == ME_ORDER_SUCCESS
		|| on->status == ME_ORDER_CANCEL){
		ord_rtn->status = ME_ORDER_CANCEL_REJECTED;
		goto canl_end;
	}

	sn = on->the_st;
	list_del(&on->list);
	//tn->volume -= on->volume;
	tn = sn->parent;
	sn->vol -= on->volume;
	if (list_empty(&sn->ord_list)) {
		list_del(&sn->list);
		sn->used = 0;
	}

	pn = tn->parent;
	tn->volume -= on->volume;
	if (list_empty(&tn->st_list)) {
		ogn_del_tick_node( pn, tn);
	}

	pn->volume -= on->volume;
	if (list_empty(&pn->tick_list)) {
		list_del(&pn->list);
		pn->used = 0;
	}

	on->status = ME_ORDER_CANCEL;
	ord_rtn->status = on->status;
	m_place_hash->erase(need);

canl_end:
	ord_rtn->serial_no = on->pl_ord.order_id;
	strlcpy(ord_rtn->symbol, on->pl_ord.symbol, MAX_NAME_SIZE);
	ord_rtn->direction = on->pl_ord.direction;
	ord_rtn->open_close = on->pl_ord.open_close;
	ord_rtn->exe_price = on->pl_ord.limit_price;
	ord_rtn->exe_volume = on->volume;
	ord_rtn->strat_id = on->pl_ord.strat_id;
	ord_rtn->entrust_no = m_entrust_num;
}

void
OriginMatch::feed_stock_quote(int type, void* quote, int market, bool is_first)
{
	m_mi_type = type;
	m_is_filter = Filter;
	m_tick_counter++;
	m_last_type = type;

	memcpy(m_last_quote, quote, sizeof(Stock_Internal_Book));
	if (is_empty_order()) {
		return;
	}

	memcpy(&m_quote.last_quote, &m_quote.curr_quote, sizeof(mat_quote));
	process_stock_book(quote, &m_quote);
	memcpy(&m_quote.opposite, &m_quote.curr_quote, sizeof(mat_quote));
	m_is_filter = Process;
}

void
OriginMatch::feed_future_quote(int type, void* quote, int market, bool is_first)
{
	m_mi_type = type;
	m_is_filter = Filter;
	m_tick_counter++;
	m_last_type = type;

	memcpy(m_last_quote, quote, sizeof(Futures_Internal_Book));
	if (is_empty_order()) {
		return;
	}

	memcpy(&m_quote.last_quote, &m_quote.curr_quote, sizeof(mat_quote));
	process_future_book(quote, &m_quote);
	memcpy(&m_quote.opposite, &m_quote.curr_quote, sizeof(mat_quote));
	m_is_filter = Process;
}

void
OriginMatch::execute_match(void** trd_rtn_ar, int* rtn_cnt)
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

	calc_trade_pv(pv_ar_data, &m_trd_pv_cnt);
	memcpy(pv_array_buy, pv_ar_data, sizeof(pv_ar_data));
	memcpy(pv_array_sell, pv_ar_data, sizeof(pv_ar_data));

	if (is_empty_order()) goto proc_end;
	ogn_match_on_dir(DIR_BUY);
	ogn_match_on_dir(DIR_SELL);

proc_end:
	*rtn_cnt = m_trade_rtn->size();
	*trd_rtn_ar = m_trade_rtn->head();
}


price_node *
OriginMatch::ogn_find_price_node(double price, int direct)
{
	struct list_head *head, *pos;
	struct price_node *pn;
	better_price_func cmp_func;
	head = get_price_handle(direct);
	cmp_func = compare_price[direct];

	list_for_each(pos, head) {
		pn = list_entry(pos, struct price_node, list);
		if (compare(pn->price, price) == 0) {
			return pn;
		}
		else if (cmp_func(price, pn->price)) {
			return NULL;
		}
	}
	return NULL;
}


price_node*
OriginMatch::ogn_add_price_node(double price, int direct)
{
	struct list_head *head, *pos, *insert_pos;
	struct price_node *pn;
	better_price_func cmp_func;
	head = get_price_handle(direct);
	cmp_func = compare_price[direct];
	
	insert_pos = head;
	list_for_each(pos, head) {
		pn = list_entry(pos, struct price_node, list);
		if (cmp_func(price, pn->price)) {
			insert_pos = pos;
			break;
		}else {
			insert_pos = pos->next;
		}
	}

	pn = (price_node *)m_price_data->next();
	if (NULL == pn) {
		PRINT_ERROR("Can't get available price_node");
		return NULL;
	}
	memset(pn, 0, sizeof(*pn));
	INIT_LIST_HEAD(&pn->list);
	INIT_LIST_HEAD(&pn->tick_list);
	pn->used = 1;
	pn->price = price;
	pn->direct = direct;
	list_add_before(&pn->list, insert_pos);
	return pn;
}

tick_node *
OriginMatch::ogn_find_tick_node(price_node *pn, long tick, int owner)
{
	struct list_head *head, *pos;
	struct tick_node *tn;
	head = &pn->tick_list;
	list_for_each(pos, head) {
		tn = list_entry(pos, struct tick_node, list);
		if (tick < tn->tick) {
			return NULL;
		}
		if (tn->tick == tick
			&& tn->owner == owner) {
			return tn;
		}
	}
	return NULL;
}

tick_node  *
OriginMatch::ogn_add_tick_node(price_node *pn, long tick, int owner)
{
	struct list_head *head;
	struct tick_node *tn;
	head = &pn->tick_list;
	tn = (tick_node *)m_ticks_data->next();
	if (NULL == tn) {
		PRINT_ERROR("Can't get available tick_node");
		return NULL;
	}
	memset(tn, 0, sizeof(*tn));
	INIT_LIST_HEAD(&tn->list);
	INIT_LIST_HEAD(&tn->st_list);
	tn->used = 1;
	tn->tick = tick;
	tn->parent = pn;
	tn->owner = owner;
	list_add_before(&tn->list, head);
	return tn;
}


strat_node *
OriginMatch::ogn_find_strat_node(tick_node *tn, int st_id)
{
	struct list_head *head, *pos;
	struct strat_node *sn;
	head = &tn->st_list;
	list_for_each(pos, head) {
		sn = list_entry(pos, struct strat_node, list);
		if (st_id < sn->st_id) {
			return NULL;
		}if (st_id == sn->st_id) {
			return sn;
		}
	}
	return NULL;
}


strat_node *
OriginMatch::ogn_add_strat_node(tick_node *tn, int st_id)
{
	struct list_head *head, *pos, *insert_pos;
	struct strat_node *sn, *tmp;
	sn = (strat_node *)m_strat_data->next();
	if (NULL == sn) {
		PRINT_WARN("Can't get available strat_node");
		return NULL;
	}

	memset(sn, 0, sizeof(*sn));
	INIT_LIST_HEAD(&sn->ord_list);
	INIT_LIST_HEAD(&sn->list);
	sn->used = 1;
	sn->st_id = st_id;
	sn->parent = tn;
	head = &tn->st_list;
	insert_pos = head;
	list_for_each(pos, head) {
		tmp = list_entry(pos, struct strat_node, list);
		if (st_id < tmp->st_id) {
			insert_pos = pos;
			break;
		}else if (st_id == tmp->st_id) {
			PRINT_ERROR("add duplicated strat_node:%d", st_id);
		}
	}
	list_add_before(&sn->list, insert_pos);
	return sn;
}

order_node *
OriginMatch::ogn_add_order_node(strat_node *sn, order_request *ord)
{
	list_head *head = &sn->ord_list;
	order_node *on = (order_node *)m_place_orders->next();
	if (NULL == on) {
		PRINT_WARN("Can't get available order_node");
		return NULL;
	}
	memset(on, 0, sizeof(*on));
	INIT_LIST_HEAD(&on->list);
	memcpy(&on->pl_ord, ord, sizeof(*ord));
	on->the_st = sn;
	on->status = ME_ORDER_ENTRUSTED;
	on->volume = ord->order_size;
	on->entrust_no = m_entrust_num;
	list_add_before(&on->list, head);
	return on;
}

void 
OriginMatch::ogn_del_tick_node(price_node *pn, tick_node *tn)
{
	struct list_head *pos, *p, *head;
	struct tick_node *tmp;
	list_del(&tn->list);
	tn->used = 0;

	head = &pn->tick_list;
	list_for_each_prev_safe(pos, p, head) {
		tmp = list_entry(pos, struct tick_node, list);
		if (OWNER_MKT == tmp->owner) {
			pn->mkt_vol -= tmp->volume;
			list_del(pos);
			tmp->used = 0;
		}else {
			break;
		}
	}
}

bool  
OriginMatch::ogn_update_order_book(order_request *pl_ord)
{
	price_node* pn = ogn_find_price_node(pl_ord->limit_price, pl_ord->direction);
	if (NULL == pn) {
		pn = ogn_add_price_node(pl_ord->limit_price, pl_ord->direction);
	}
	if (pn == NULL){
		return false;
	}

	int mkt_vol = get_volume_prior(pl_ord);
	if (mkt_vol > pn->mkt_vol) {
		tick_node *mkt_tn = ogn_find_tick_node(pn, m_tick_counter, OWNER_MKT);
		if (mkt_tn == NULL){
			mkt_tn = ogn_add_tick_node(pn, m_tick_counter, OWNER_MKT);
		}
		if (mkt_tn == NULL){
			return false;
		}
		mkt_tn->volume = (mkt_vol - pn->mkt_vol);
		pn->mkt_vol = mkt_vol;
	}

	tick_node * tn = ogn_find_tick_node(pn, m_tick_counter, OWNER_MY);
	if (tn == NULL) {
		tn = ogn_add_tick_node(pn, m_tick_counter, OWNER_MY);
	}
	if (tn == NULL){
		return false;
	}

	strat_node* sn = ogn_find_strat_node(tn, pl_ord->strat_id);
	if (NULL == sn) {
		sn = ogn_add_strat_node(tn, pl_ord->strat_id);
	}
	if (sn == NULL){
		return false;
	}

	order_node* on = ogn_add_order_node(sn, pl_ord);
	if (on == NULL){
		return false;
	}

	on->status = ME_ORDER_ENTRUSTED;
	on->entrust_no = m_entrust_num;
	sn = on->the_st;
	sn->vol += pl_ord->order_size;
	tn = sn->parent;
	tn->volume += pl_ord->order_size;
	pn = tn->parent;
	pn->volume += pl_ord->order_size;
	m_place_hash->insert(std::make_pair(pl_ord->order_id, on));
	return true;
}

void 
OriginMatch::ogn_match_on_dir(int direct)
{
	struct list_head *pos, *p, *head;
	struct price_node *pn = NULL;
	struct pv_pair    *pv_ar;
	int before_vol, after_vol;
	int trade_vol;

	head = get_price_handle(direct);
	if (DIR_BUY == direct) {
		pv_ar = pv_array_buy;
	}else {
		pv_ar = pv_array_sell;
	}

	list_for_each_safe(pos, p, head) {
		pn = list_entry(pos, struct price_node, list);
		before_vol = pn->volume + pn->mkt_vol;
		after_vol = 0;

		trade_vol = calc_tradeable_vol(pv_ar, m_trd_pv_cnt, pn->price, direct);	
		ogn_match_on_price(pn, trade_vol, 1);
		if (list_empty(&pn->tick_list)) {
			list_del(pos);
			pn->used = 0;
			continue;
		}

		after_vol = pn->volume + pn->mkt_vol;
		int total_vol = before_vol - after_vol;
		update_tradeable_vol(pv_ar, m_trd_pv_cnt, pn->price, direct, total_vol);

		int opp_vol = calc_opp_trade_vol(pn->price, direct);
		if (opp_vol >= (pn->mkt_vol + pn->volume)) {
			before_vol = pn->volume + pn->mkt_vol;
			ogn_match_on_price(pn, opp_vol, 0);
			after_vol = pn->volume + pn->mkt_vol;
			update_opposite_vol(direct, (after_vol - before_vol));
		}

		update_mkt_tick(pn, direct);
		if (list_empty(&pn->tick_list)) {
			list_del(pos);
			pn->used = 0;
		}
	}
}

void 
OriginMatch::ogn_match_on_price(price_node *pn, int exe_vol, int flag)
{
	struct list_head *pos, *p, *head;
	struct tick_node *tn = NULL;
	int remain_vol, trade_vol;

	if (pn->volume <= 0) return;
	remain_vol = exe_vol;
	head = &pn->tick_list;

	list_for_each_safe(pos, p, head) {
		tn = list_entry(pos, struct tick_node, list);
		trade_vol = 0;
		if (OWNER_MY == tn->owner) {
			int weighted_vol = remain_vol;
			if (1 == flag) {
				if ((m_tick_counter - tn->tick) > 1) {
					weighted_vol = weighted_vol;
				}else {
					weighted_vol = (int)(weighted_vol * m_cfg.mode_cfg.trade_ratio);
				}
			}
			trade_vol = MIN(weighted_vol, tn->volume);
			ogn_match_on_tick(pn, tn, trade_vol);
			tn->volume -= trade_vol;
			pn->volume -= trade_vol;
		}else {
			trade_vol = MIN(tn->volume, remain_vol);
			tn->volume -= trade_vol;
			pn->mkt_vol -= trade_vol;
		}
		remain_vol -= trade_vol;
		if (0 == tn->volume) {
			list_del(pos);
			tn->used = 0;
		}
		if (0 == remain_vol) break;
	}
}

int  
OriginMatch::ogn_match_on_tick(price_node *pn, tick_node *tn, int exe_vol)
{
	struct list_head *pos, *p, *head;
	struct strat_node *sn = NULL;
	int remain_vol = MIN(exe_vol, tn->volume);
	int deno = tn->volume;
	int total_vol = 0;

	head = &tn->st_list;
	list_for_each_safe(pos, p, head) {
		sn = list_entry(pos, struct strat_node, list);
		float f_trd_vol = (float)sn->vol / deno * remain_vol;
		int i_trd_vol = (int)f_trd_vol;
		if (compare(f_trd_vol, i_trd_vol) != 0) {
			i_trd_vol += 1;
		}

		i_trd_vol = MIN(i_trd_vol, sn->vol);
		i_trd_vol = MIN(i_trd_vol, remain_vol);
		remain_vol -= i_trd_vol;
		deno -= sn->vol;
		int cur_trd_vol = ogn_match_on_strat(tn, sn, i_trd_vol);
		total_vol += cur_trd_vol;
		sn->vol -= cur_trd_vol;
		if (sn->vol <= 0) {
			list_del(&sn->list);
			sn->used = 0;
		}
		if (remain_vol <= 0 || deno <= 0) {
			break;
		}
	}
	return total_vol;
}

int
OriginMatch::ogn_match_on_strat(tick_node *tn, strat_node *sn, int exe_vol)
{
	signal_response *resp = NULL;
	struct list_head *pos, *p, *head;
	struct order_node *on;
	int total_vol = 0;
	int remain_vol = exe_vol;

	head = &sn->ord_list;
	list_for_each_safe(pos, p, head) {
		int cur_trd_vol;
		if (exe_vol <= 0) break;
		resp = (signal_response *)m_trade_rtn->next();
		if (resp == NULL) break;

		on = list_entry(pos, struct order_node, list);
		cur_trd_vol = MIN(on->volume, remain_vol);
		if (cur_trd_vol < on->volume) {
			on->status = ME_ORDER_PARTED;
		}else {
			on->status = ME_ORDER_SUCCESS;
		}

		remain_vol -= cur_trd_vol;
		total_vol += cur_trd_vol;
		on->volume -= cur_trd_vol;
		on->trade_vol = cur_trd_vol;
		on->trade_price = on->pl_ord.limit_price;

		set_order_result(on, resp);
		if (on->volume <= 0) {
			list_del(&on->list);
			//auto need = m_place_hash->find(on->pl_ord.serial_no);
			//if (need != m_place_hash->end()){
				//m_place_hash->erase(need);
			//}
		}
	}
	return total_vol;
}

void  
OriginMatch::update_mkt_tick(price_node *pn, int direct)
{
	int new_vol_mkt;
	int travel_vol = 0;
	struct tick_node *tn;
	struct list_head *pos, *p, *head;

	int vol_mkt = pn->mkt_vol;
	if (0 == vol_mkt) return;
	new_vol_mkt = get_vol_on_price(pn->price, vol_mkt, direct);
	if (new_vol_mkt >= vol_mkt) return;
	head = &pn->tick_list;
	list_for_each_safe(pos, p, head) {
		tn = list_entry(pos, struct tick_node, list);
		if (OWNER_MY == tn->owner) continue;
		if (travel_vol >= new_vol_mkt) {
			list_del(pos);
			tn->used = 0;
			continue;
		}
		if ((travel_vol + tn->volume) <= new_vol_mkt) {
			travel_vol += tn->volume;
		}
		else if ((travel_vol + tn->volume) > new_vol_mkt) {
			tn->volume = new_vol_mkt - travel_vol;
			travel_vol = new_vol_mkt;
		}
	}
	pn->mkt_vol = new_vol_mkt;
}

void  
OriginMatch::update_tradeable_vol(pv_pair *pv_ar, int cnt, double my_price, int direct, int vol)
{
	for (int i = 0; i < cnt; i++) {
		pv_pair *pv = pv_ar + i;
		if (if_equal_trade(pv->price, my_price, direct)) {
			int dec_vol = MIN(pv->vol, vol);
			pv->vol -= dec_vol;
			vol -= dec_vol;
			if (0 == vol) {
				break;
			}
		}
	}
}

void 
OriginMatch::update_opposite_vol(int direct, int vol)
{
	struct pv_pair *pv_ar = m_quote.opposite.bs_pv[direct];
	for (int i = 0; i < QUOTE_LVL_CNT; i++) {
		int dec_vol = MIN(vol, pv_ar[i].vol);
		pv_ar[i].vol -= dec_vol;
		vol -= dec_vol;
		if (vol <= 0) {
			break;
		}
	}
}

int
OriginMatch::calc_opp_trade_vol(double my_price, int direct)
{
	int opp_vol = 0;
	if (EXCH_CZCE == m_cfg.pi.xchg_code) {
		if (if_czce_is_changed() == false) {
			return 0;
		}
		if (m_quote.curr_quote.int_time == m_quote.last_quote.int_time) {
			return 0;
		}
	}

	pv_pair *pv_ar = m_quote.opposite.bs_pv[SWITCH_DIR(direct)];
	for (int i = 0; i < QUOTE_LVL_CNT; i++) {
		if (compare(pv_ar[i].price, 0.0) == 0) {
			break;
		}
		if (if_equal_trade(pv_ar[i].price, my_price, direct)) {
			opp_vol += pv_ar[i].vol;
		}else {
			break;
		}
	}
	return opp_vol;

}

void  OriginMatch::calc_trade_pv(pv_pair *pv_ar, int *pv_ar_cnt)
{
	mat_quote data = {0};
	data.amount = m_quote.curr_quote.amount - m_quote.last_quote.amount;
	data.total_volume = m_quote.curr_quote.total_volume - m_quote.last_quote.total_volume;
	data.last_price = m_quote.curr_quote.last_price;
	data.last_vol = m_quote.curr_quote.last_vol;
	if (EXCH_CZCE == m_cfg.pi.xchg_code) {
		*pv_ar_cnt = 1;
		pv_ar[0].price = data.last_price;
		pv_ar[0].vol = data.total_volume;
		return;
	}
	if (m_quote.curr_quote.high_price > m_quote.last_quote.high_price) {
		data.high_price = m_quote.curr_quote.high_price;
	}
	if (m_quote.curr_quote.low_price < m_quote.last_quote.low_price) {
		data.low_price = m_quote.curr_quote.low_price;
	}
	double total_amount = data.amount;
	int total_vol = data.total_volume;
	if (g_unlikely((total_amount < 0)
		|| compare(total_amount, 0.0) == 0
		|| (total_vol <= 0))) {
		*pv_ar_cnt = 0;
		return;
	}

	int index_count = 0;
	double p1 = 0.0, p2 = 0.0;
	divide_price(total_amount, total_vol, p1, p2);
	total_amount /= m_cfg.pi.multiple;
	int v1 = int((total_amount - p2 * total_vol) / (p1 - p2) + 0.5);
	int v2 = total_vol - v1;
	if (v1 != 0) {
		pv_ar[index_count].price = p1;
		pv_ar[index_count].vol = v1;
		index_count++;
	}
	if (v2 != 0) {
		pv_ar[index_count].price = p2;
		pv_ar[index_count].vol = v2;
		index_count++;
	}
	*pv_ar_cnt = index_count;
}

bool 
OriginMatch::if_czce_is_changed()
{
	mat_quote *q_curr = &m_quote.curr_quote;
	mat_quote *q_last = &m_quote.last_quote;
	for (int i = 1; i < QUOTE_LVL_CNT - 1; i++) {
		if (q_last->bs_pv[0][i].vol != q_curr->bs_pv[0][i].vol
			|| compare(q_last->bs_pv[0][i].price, q_curr->bs_pv[0][i].price) != 0) {
			return true;
		}
	}

	if (q_last->tot_b_vol != q_curr->tot_b_vol
	 || q_last->tot_s_vol != q_curr->tot_s_vol) {
		return true;
	}
	return false;
}

int   
OriginMatch::if_equal_trade(double mkt_price, double my_price, int direct)
{
	return (compare(mkt_price, my_price) == 0
		|| is_can_trade(mkt_price, my_price, direct));
}

int   
OriginMatch::calc_tradeable_vol(pv_pair *pv_ar, int cnt, double price, int direct)
{
	int trade_vol = 0;
	for (int i = 0; i < cnt; i++) {
		pv_pair *pv = pv_ar + i;
		if (if_equal_trade(pv->price, price, direct)) {
			trade_vol += pv->vol;
		}
	}
	return trade_vol;
}

void  
OriginMatch::divide_price(double total_amount, int total_vol, double &p1, double &p2)
{
	int64_t l_avg, l_amount;
	int64_t high_price, low_price;
	int64_t i_unit = (int64_t)(m_cfg.pi.unit * m_factor);

	double f_amount = total_amount * m_factor / m_cfg.pi.multiple;
	l_amount = (int64_t)f_amount;
	l_avg = l_amount / total_vol;
	high_price = (l_avg / i_unit + 1) * i_unit;
	low_price = high_price - i_unit;
	p1 = (double)high_price / m_factor;
	p2 = (double)low_price / m_factor;
}

int  
OriginMatch::get_vol_on_price(double price, int volume, int direct)
{
	struct pv_pair *pv_ar = m_quote.curr_quote.bs_pv[direct];
	if (is_can_trade(pv_ar[0].price, price, direct)) {
		return 0;
	}
	else if (is_can_trade(price, pv_ar[QUOTE_LVL_CNT - 1].price, direct)) {
		return volume;
	}
	for (int i = 0; i < QUOTE_LVL_CNT; i++) {
		if (compare(price, pv_ar[i].price) == 0) {
			return pv_ar[i].vol;
		}
	}
	if (is_can_trade(pv_ar[QUOTE_LVL_CNT - 1].price, price, direct)) {
		return 0;
	}
	return 0;
}

int 
OriginMatch::get_volume_prior(order_request *pl_ord)
{
	struct mat_quote *cur_q = &(m_quote.curr_quote);
	struct pv_pair *pv_ar = cur_q->bs_pv[pl_ord->direction];
	for (int i = 0; i < QUOTE_LVL_CNT; i++) {
		if (compare(pl_ord->limit_price, pv_ar[i].price) == 0) {
			return pv_ar[i].vol;
		}
	}
	return 0;
}

