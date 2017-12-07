//extend match engine implementation
#include "extend_match.h"


//------------IdealMatch implementation-------------
//IdealMatch不接受撤单处理
void IdealMatch::cancel_order(order_request* cl_ord, signal_response *ord_rtn)
{
	ord_rtn->error_no = 0;
	ord_rtn->strat_id = cl_ord->strat_id;
	ord_rtn->serial_no = cl_ord->org_order_id;
	ord_rtn->status = ME_ORDER_CANCEL_REJECTED;
	strlcpy(ord_rtn->symbol, cl_ord->symbol, MAX_NAME_SIZE);
	ord_rtn->direction = cl_ord->direction;
	ord_rtn->open_close = cl_ord->open_close;
	ord_rtn->exe_price = 0;
	ord_rtn->exe_volume = 0;
}

void IdealMatch::execute_ideal_match()
{
	signal_response *resp = NULL;
	order_node* node = NULL;
	for (int i = 0; i < m_place_num; i++){
		node = m_place_array[i];
		if (node == NULL) break;
		node->trade_price = node->pl_ord.limit_price;
		node->trade_vol = node->pl_ord.order_size;
		node->status = ME_ORDER_SUCCESS;
		resp = (signal_response *)m_trade_rtn->next();
		if (resp == NULL) break;
		set_order_result(node, resp);
	}
	m_place_num = 0;
}

void  
IdealMatch::execute_match(void** trd_rtn_ar, int* rtn_cnt)
{
	if (m_is_malloc_node == false){
		*rtn_cnt = 0;
		*trd_rtn_ar = NULL;
		return;
	}

	if (g_unlikely(m_trade_rtn->size() > 0)){
		m_trade_rtn->reset();
	}
		
	execute_ideal_match();
	*rtn_cnt = m_trade_rtn->size();
	*trd_rtn_ar = m_trade_rtn->head();
}


//------------LinerMatch implementation-------------
void  LinearMatch::clear()
{
	MatchEngine::clear();
	m_param_index = 0;
	memset(&m_param, 0, sizeof(line_mode_param));
}

void  LinearMatch::round_finish()
{
	printf("Date:%d, Contract: %s, totalImpactCost: %f\n", m_cfg.task.task_date, m_cfg.contract, m_param.totalImpactCost);
	if(m_debug_flag) {
		m_write_log << m_cfg.task.task_date << ", " << m_cfg.contract << ", "
			<< m_param.totalImpactVol << ", " << m_param.totalImpactCost << std::endl;
	}
}

void
LinearMatch::execute_match(void** trd_rtn_ar, int* rtn_cnt)
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
	if (m_tick_counter > 1){
		line_mode_calc_trade_volume();
	}
	
	update_order_book();
	if (m_is_filter == Process){
		do_match_on_direct(DIR_BUY);
		do_match_on_direct(DIR_SELL);
	}

proc_end:
	*rtn_cnt = m_trade_rtn->size();
	*trd_rtn_ar = m_trade_rtn->head();
}

void
LinearMatch::do_match_on_price(price_node *pn, int direct)
{
	int self_trade = 0;
	pn->last_vol = pn->curr_vol;
	pn->curr_vol = find_market_volume(direct, pn->price, true);
	if (pn->curr_vol < 0) pn->curr_vol = pn->last_vol;
	int	split_vol = find_trade_volume(pn->price);
	int canl_vol = pn->last_vol - pn->curr_vol - split_vol;
	do_match_on_tick(pn, canl_vol, false, self_trade);

	double avg_price = 0.0;
	int opp_vol = find_opposite_volume(direct, pn->price, avg_price);
	do_match_can_trade(pn);
	line_mode_do_opposite(pn, opp_vol);
}

void
LinearMatch::line_mode_calc_trade_volume()
{
	pv_pair *high = (pv_pair *)m_split_data->next();
	pv_pair *low = (pv_pair *)m_split_data->next();
	mat_quote *q_curr = &m_quote.curr_quote;
	mat_quote *q_last = &m_quote.last_quote;
	pv_pair* b_last = q_last->bs_pv[0];
	pv_pair* s_last = q_last->bs_pv[1];

	double tickSize = m_cfg.pi.unit;
	double diff_amount = (q_curr->amount - q_last->amount) / m_cfg.pi.multiple;
	int diff_vol = q_curr->total_volume - q_last->total_volume;
	double midp1 = get_midprice(q_curr->bs_pv[0][0].price, q_curr->bs_pv[1][0].price);
	double midp0 = get_midprice(b_last[0].price, s_last[0].price);
	int index = m_param_index % int(m_cfg.mode_cfg.params[2]);

	if (diff_amount > 0 && diff_vol > 0){
		double vwap = diff_amount / diff_vol;
		if (vwap > midp0){
			m_param.volBuyLst[index] = diff_vol;
			m_param.buyMQPchgLst[index] = midp1 - midp0;
			m_param_index++;
		}else if (vwap < midp0){
			m_param.volSellLst[index] = diff_vol;
			m_param.sellMQPchgLst[index] = midp0 - midp1;
			m_param_index++;
		}
		double p0 = int(vwap / tickSize) * tickSize;
		high->vol = (int)((diff_amount - diff_vol * p0) / tickSize);
		low->vol = diff_vol - high->vol;
		low->price = p0;
		high->price = p0 + tickSize;
	}
	else if (diff_amount == 0 && diff_vol > 0){
		low->price = int((midp0 + q_curr->last_price) / 2.0 / tickSize)*tickSize;
		high->price = low->price + tickSize;
		if (fabs(midp1 - midp0) < tickSize / 2){
			high->vol = diff_vol / 2;
		}else if (midp1 - midp0 > tickSize / 2){
			high->vol = diff_vol;
			m_param.volBuyLst[index] = diff_vol;
			m_param.buyMQPchgLst[index] = midp1 - midp0;
			m_param_index++;
		}else{
			high->vol = 0;
			m_param.volSellLst[index] = diff_vol;
			m_param.sellMQPchgLst[index] = midp0 - midp1;
			m_param_index++;
		}
		low->vol = diff_vol - high->vol;
	}
}

double
LinearMatch::get_impact_cost(int cost_vol, int direct)
{
	double  amount = 0.0, volume = 0.0;
	for (int i = 0; i < MAX_PARAM_SIZE; i++){
		if (direct == DIR_BUY && m_param.volBuyLst[i] > 1){
			amount += m_param.buyMQPchgLst[i];
			volume += log(m_param.volBuyLst[i]);
		}else if (direct == DIR_SELL && m_param.volSellLst[i] > 1){
			amount += m_param.sellMQPchgLst[i];
			volume += log(m_param.volSellLst[i]);
		}
	}

	if (volume > 0 && cost_vol > 1){
		return MAX(amount * log(cost_vol) / volume * m_cfg.mode_cfg.params[1], 0);
	}else{
		return  0.0;
	}
}

void
LinearMatch::line_mode_do_opposite(price_node *pn, int exe_vol)
{
	double avg_price = 0.0;
	if (g_unlikely(exe_vol <= 0)) return;
	int pre_opp_vol = find_opposite_volume(pn->direct, pn->price, avg_price);
	int diff_tick = m_tick_counter - pn->opp_match_tick;
	int can_trade_vol = (int)(exe_vol * (1 - exp(-diff_tick / m_cfg.mode_cfg.params[0])) + MAX(exe_vol - pre_opp_vol, 0));

	if (can_trade_vol > 0){
		int cost_vol = 0;
		pn->opp_match_tick = m_tick_counter;
		do_match_on_tick(pn, can_trade_vol, true, cost_vol);
		m_param.totalImpactVol += cost_vol;
		double result = get_impact_cost(cost_vol, pn->direct);
		m_param.totalImpactCost += result * cost_vol;
	}
}


//------------GaussianMatch implementation-------------
static double normal_cdf(double x) // Phi(-INFI, x) aka N(x)
{
	return std::erfc(-x / std::sqrt(2)) / 2;
}

void
GaussianMatch::update_market_book(price_node* pn, int direct, double price)
{
	tick_node * tn = find_tick_node(pn, m_tick_counter, OWNER_MKT);
	if (tn == NULL){
		pn->last_vol = find_market_volume(direct, price, false);
		if (pn->last_vol < 0) pn->last_vol = 0;
		pn->curr_vol = find_market_volume(direct, price, true);
		if (pn->curr_vol < 0) pn->curr_vol = pn->last_vol;
		int split_vol = find_trade_volume_Gaussian(price, direct);
		int rto_vol = int(ceil(m_cfg.mode_cfg.trade_ratio*(pn->curr_vol - pn->last_vol + split_vol)));   // More conservative	
		int mkt_vol = pn->last_vol + rto_vol - pn->mkt_vol;
		if (g_unlikely(mkt_vol > 0)){
			tn = add_tick_node(pn, m_tick_counter, OWNER_MKT);
			tn->volume += mkt_vol;
			pn->mkt_vol += mkt_vol;
		}
	}
}

void
GaussianMatch::do_match_on_price(price_node *pn, int direct)
{
	int self_trade = 0;
	pn->last_vol = pn->curr_vol;
	pn->curr_vol = find_market_volume(direct, pn->price, true);
	if (pn->curr_vol < 0) pn->curr_vol = pn->last_vol;
	int	split_vol = find_trade_volume_Gaussian(pn->price, direct);
	int canl_vol = pn->last_vol - pn->curr_vol - split_vol;
	do_match_on_tick(pn, canl_vol, false, self_trade);

	double avg_price = 0.0;
	int opp_vol = find_opposite_volume(direct, pn->price, avg_price);
	do_match_on_opposite(pn, opp_vol, avg_price);
	do_match_can_trade(pn);
}

void
GaussianMatch::find_trade_volume(double price, int direct)
{
	pv_pair* pair = NULL;
	m_can_trade_num = 0;
	if (direct == DIR_BUY){
		for (unsigned int i = 0; i < m_split_data->size(); i++){
			pair = (pv_pair *)m_split_data->at(i);
			int cum_exe_vol = find_trade_volume_Gaussian(price, direct);
			if (cum_exe_vol > 0){
				pair->vol = cum_exe_vol;
				m_can_trade_list[m_can_trade_num++] = pair;
			}
			break;
		}
	}else{
		for (int i = (int)m_split_data->size() - 1; i >= 0; i--){
			pair = (pv_pair *)m_split_data->at(i);
			int cum_exe_vol = find_trade_volume_Gaussian(price, direct);
			if (cum_exe_vol > 0){
				pair->vol = cum_exe_vol;
				m_can_trade_list[m_can_trade_num++] = pair;
			}
			break;
		}
	}
}

//正态模型-特有的计算成交量的函数
int
GaussianMatch::find_trade_volume_Gaussian(double price, int direct)
{
	double l_std = 0.0;
	double l_d_cum_exe_vol = 0.0;
	if (compare(m_quote.curr_quote.low_price, m_quote.last_quote.low_price) < 0){
		m_price_from = m_quote.curr_quote.low_price;
	}else{
		double l_last_price = m_quote.curr_quote.last_price;
		m_price_from = MAX(m_quote.curr_quote.low_price, MIN(MIN(m_quote.last_quote.bs_pv[0][0].price,
			m_quote.curr_quote.bs_pv[0][0].price), l_last_price));
		if (m_split_data->size() > 1){
			pv_pair* pair = (pv_pair *)m_split_data->at(1);
			if (compare(pair->price, m_price_from) < 0){
				m_price_from = MAX(m_quote.curr_quote.low_price, MIN(MIN(MIN(m_quote.last_quote.bs_pv[0][1].price,
					m_quote.curr_quote.bs_pv[0][1].price), m_price_from), pair->price));
			}
		}
	}
	if (compare(m_quote.curr_quote.high_price, m_quote.last_quote.high_price) > 0){
		m_price_to = m_quote.curr_quote.high_price;
	}else{
		double l_last_price = m_quote.curr_quote.last_price;
		m_price_to = MIN(m_quote.last_quote.high_price, MAX(MAX(m_quote.last_quote.bs_pv[1][0].price,
			m_quote.curr_quote.bs_pv[1][0].price), l_last_price));
		if (m_split_data->size() > 1){
			pv_pair* pair = (pv_pair *)m_split_data->at(0);
			if (compare(pair->price, m_price_to) > 0){
				m_price_to = MIN(m_quote.curr_quote.high_price, MAX(MAX(MAX(m_quote.last_quote.bs_pv[1][1].price,
					m_quote.curr_quote.bs_pv[1][1].price), m_price_to), pair->price));
			}
		}
	}
	l_std = 0.14 * (m_price_to - m_price_from) * (m_price_to - m_price_from);
	//if (fabs(l_std) < PRECISION) return 0;
	pv_pair* low = (pv_pair *)m_split_data->at(1);
	pv_pair* high = (pv_pair *)m_split_data->at(0);
	if (direct == DIR_BUY){
		if (compare(price, m_price_from) < 0) return 0;
		l_d_cum_exe_vol = low->vol * normal_cdf((price - low->price + 0.5 * m_cfg.pi.unit) / l_std) +
			high->vol * normal_cdf((price - high->price + 0.5 * m_cfg.pi.unit) / l_std);
	}else{
		if (compare(price, m_price_to) > 0) return 0;
		l_d_cum_exe_vol = low->vol * (1 - normal_cdf((price - low->price - 0.5 * m_cfg.pi.unit) / l_std)) +
			high->vol * (1 - normal_cdf((price - high->price - 0.5 * m_cfg.pi.unit) / l_std));
	}
	return (int)l_d_cum_exe_vol;
}

//------------BarCloseMatch implementation-------------
void BarCloseMatch::feed_stock_quote(int type, void* quote, int market, bool is_bar)
{
	m_mi_type = type;
	m_tick_counter++;
	m_is_bar = is_bar;
	m_market_type = market;
	if (market == KDB_QUOTE) {
		memcpy(m_last_quote, quote, sizeof(Internal_Bar));
	}
	else {
		m_last_numpy = quote;
	}
	process_internal_bar(quote, type, &m_quote);
}

void BarCloseMatch::feed_future_quote(int type, void* quote, int market, bool is_bar)
{
	m_mi_type = type;
	m_tick_counter++;
	m_is_bar = is_bar;
	m_market_type = market;
	if (market == KDB_QUOTE) {
		memcpy(m_last_quote, quote, sizeof(Internal_Bar));
	}
	else {
		m_last_numpy = quote;
	}
	process_internal_bar(quote, type, &m_quote);
}

void
BarCloseMatch::execute_match(void** trd_rtn_ar, int* rtn_cnt)
{
	if (m_is_malloc_node == false){
		*rtn_cnt = 0;
		*trd_rtn_ar = NULL;
		return;
	}

	if (g_unlikely(m_trade_rtn->size() > 0)){
		m_trade_rtn->reset();
	}

	bar_update_order_book();
	if (m_debug_flag) {
		show_price_list();
	}

	bar_match_on_price(DIR_BUY);
	bar_match_on_price(DIR_SELL);

	*rtn_cnt = m_trade_rtn->size();
	*trd_rtn_ar = m_trade_rtn->head();
}

void BarCloseMatch::bar_update_order_book()
{
	for (int i = 0; i < m_place_num; i++){
		order_node* node = m_place_array[i];
		order_request* pl_ord = &node->pl_ord;

		price_node* pn = add_price_node(pl_ord->direction, pl_ord->limit_price);
		tick_node* tn = find_tick_node(pn, m_tick_counter, OWNER_MY);
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

void BarCloseMatch::bar_match_on_price(int direct)
{
	price_node* node = NULL;
	list_head* pos, *p, *head;

	head = get_price_handle(direct);
	list_for_each_safe(pos, p, head){
		node = list_entry(pos, price_node, list);
		bar_match_on_tick(node);
	}
}

void
BarCloseMatch::bar_match_on_tick(price_node *pn)
{
	tick_node *node = NULL;
	list_head* pos, *p, *head;
	int bad_vol = BAD_RESULT(pn->direct);

	head = &pn->tick_list;
	//按总量的一定比例进行成交
	int exe_vol = int(m_quote.curr_quote.total_volume * m_cfg.mode_cfg.trade_ratio);
	int remain_vol = exe_vol;
	list_for_each_safe(pos, p, head){
		node = list_entry(pos, tick_node, list);
		remain_vol = bar_match_on_order(node, bad_vol, remain_vol);
	}
	if (exe_vol - remain_vol > 0){
		update_tick_node(node, exe_vol - remain_vol);
	}
}

int
BarCloseMatch::bar_match_on_order(tick_node *tn, int bad_vol, int exe_vol)
{
	signal_response *resp = NULL;
	order_node * node = NULL;
	list_head* pos, *p, *head;
	int remain_vol = exe_vol;

	head = &tn->ord_list;
	list_for_each_safe(pos, p, head){
		node = list_entry(pos, order_node, list);
		if (compare(m_quote.curr_quote.last_price,
			node->pl_ord.limit_price) == bad_vol){
			break;
		}
		resp = (signal_response *)m_trade_rtn->next();
		if (resp == NULL) break;

		int trade_vol = MIN(node->volume, remain_vol);
		node->volume -= trade_vol;
		remain_vol -= trade_vol;
		node->trade_vol = trade_vol;

		node->trade_price = node->pl_ord.limit_price;
		if (node->volume <= 0){
			node->status = ME_ORDER_SUCCESS;
			list_del(&node->list);
		}else{
			node->status = ME_ORDER_PARTED;
		}

		set_order_result(node, resp);
		if (remain_vol <= 0) break;
	}
	return remain_vol;
}

