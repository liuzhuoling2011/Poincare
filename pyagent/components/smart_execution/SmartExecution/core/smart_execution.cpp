#include <stdio.h>
#include "smart_execution.h"
#include "utils/order_hash.h"

#define HASH_ORDER_SIZE      2048
#define MAX_VWAP_ORDER_SIZE  2048
#define MAX_VOL_RECORD_SIZE  400
#define STOCK_TICK_FREQUENCY 3
#define TOKEN_BUFFER_SIZE    4096
#define MAX_VWAP_TRADED_VOL_SIZE 8196
#define DEBUG_MODE -1

const char* smart_execution_version = "Version: 1.2";
using namespace std;

extern bool debug_mode;

SmartExecution::SmartExecution(SDPHandler* sdp_handler, int type, int length, void * cfg){
	if (length == DEBUG_MODE)
		debug_mode = true;

	m_sdp_handler = sdp_handler;
	m_config = (st_config_t*)cfg;
	m_strat_id = (int)m_config->vst_id;
	m_sim_date = m_config->trading_date;
	vwap_parent_orders = new VwapParentOrderHash();
	
}

SmartExecution::~SmartExecution(){
	handle_day_finish();
	if (vwap_parent_orders != NULL){
		delete vwap_parent_orders;
		vwap_parent_orders = NULL;
	}
}

int SmartExecution::on_book(int type, int length, void * book)
{
	int ret = 0;
	switch (type) {
		case DEFAULT_STOCK_QUOTE: {
			Stock_Internal_Book *s_book = (Stock_Internal_Book *)((st_data_t*)book)->info;
			process_VWAP(s_book);
			break;
		}
		case S_PLACE_ORDER_DMA: {
			special_order_t *order = (special_order_t *)((st_data_t*)book)->info;

			PRINT_INFO("Send Order Dma: %c %s %s %d %f %d %d %d %d",
				order->exch, order->symbol, order->direction == ORDER_BUY? "Buy": "Sell", order->volume, order->price, 
				order->start_time, order->end_time, order->status, order->sync_cancel);

			Contract *instr = m_sdp_handler->find_contract(order->symbol);
			ret = m_sdp_handler->process_strat_signal(instr, (DIRECTION)order->direction, order->volume, order->price, order->sync_cancel);
			break;
		}
		case S_PLACE_ORDER_TWAP: {
			if(m_is_vwap_called == false) {
				read_VWAP_config(m_config->output_file_path);
				create_twap_config();
				m_is_vwap_called = true;
			}
			special_order_t *order = (special_order_t *)((st_data_t*)book)->info;
			
			PRINT_INFO("Send Order Twap: %c %s %s %d %f %d %d %d %d",
				order->exch, order->symbol, order->direction == ORDER_BUY ? "Buy" : "Sell", order->volume, order->price,
				order->start_time, order->end_time, order->status, order->sync_cancel);
			
			Contract *instr = m_sdp_handler->find_contract(order->symbol);
			ret = send_orders_twap(instr, (EXCHANGE)order->exch, order->volume, (DIRECTION)order->direction, order->price, order->start_time, order->end_time);
			order->status = ret;
			break;
		}
		case S_PLACE_ORDER_VWAP: {
			if (m_is_vwap_called == false) {
				read_VWAP_config(m_config->output_file_path);
				create_twap_config();
				m_is_vwap_called = true;
			}
			special_order_t *order = (special_order_t *)((st_data_t*)book)->info;
			
			PRINT_INFO("Send Order Vwap: %c %s %s %d %f %d %d %d %d",
				order->exch, order->symbol, order->direction == ORDER_BUY ? "Buy" : "Sell", order->volume, order->price,
				order->start_time, order->end_time, order->status, order->sync_cancel);
			
			Contract *instr = m_sdp_handler->find_contract(order->symbol);
			ret = send_orders_vwap(instr, (EXCHANGE)order->exch, order->volume, (DIRECTION)order->direction, order->price, order->start_time, order->end_time);
			order->status = ret; 
			break;
		}
		default: {
			m_sdp_handler->m_send_info_func(type, length, book);
		}
	}
	return ret;
}

int SmartExecution::on_response(int type, int length, void * resp)
{
	if(m_is_vwap_called == true) {
		st_response_t* l_resp = (st_response_t*)((st_data_t*)resp)->info;
		Order* l_order = m_sdp_handler->m_orders->query_order(l_resp->order_id);
		if (l_order != NULL && l_order->algorithm == ALGORITHM_TVWAP) {
			//PRINT_ERROR("Orderid: %lld, algorithm: %d", l_order->signal_id, l_order->algorithm);
			handle_order_update(l_resp);
		}
	}
	m_sdp_handler->m_send_resp_func(type, length, resp);
	return 0;
}

int SmartExecution::send_orders_vwap(Contract *instr, EXCHANGE exch, int ttl_odrsize, DIRECTION side, double price,
	int a_start_time, int a_end_time)
{
	return send_execution_orders(instr,exch, ttl_odrsize, side,price, a_start_time,  a_end_time, VWAP);
}

int SmartExecution::send_orders_twap(Contract *instr, EXCHANGE exch, int ttl_odrsize, DIRECTION side, double price,
	int a_start_time, int a_end_time)
{
	return send_execution_orders(instr, exch, ttl_odrsize, side, price, a_start_time, a_end_time, TWAP);

}

int SmartExecution::open_execution_file(std::string a_symbol, uint64_t a_sig_id, int ttl_odrsize, DIRECTION side, int a_start_time, int a_end_time, EXEC_ALGORITHM a_algo_type)
{
	// Open file
	std::string l_str_dir;
	std::string l_sim_real;
	if (side == ORDER_BUY){
		l_str_dir = "buy";
	}
	else if (side == ORDER_SELL){
		l_str_dir = "sell";
	}
	std::string l_str_date = std::to_string(m_sim_date);
	
	std::string l_str_algo;
	if (a_algo_type == VWAP){
		l_str_algo = "_vwap";
	}
	else if (a_algo_type == TWAP){
		l_str_algo = "_twap";
	}

	std::string l_str_odrsz = std::to_string(ttl_odrsize);
	std::string l_str_starttime = std::to_string(a_start_time / 1000);
	std::string l_str_endtime = std::to_string(a_end_time / 1000);
	std::string l_str_id = std::to_string(a_sig_id);

	std::string indi_output_dir = std::string(m_file_path) + "/" + l_str_date + "_" + a_symbol + "_" + l_str_odrsz + "_" + l_str_dir + "_" + l_str_starttime + "_" + l_str_endtime + "_" + l_str_id + l_str_algo + "_indicator.csv";
	FILE * indi_output = fopen(indi_output_dir.c_str(), "w+");
	if (indi_output == NULL){
		PRINT_WARN("open indi file failed: %s", indi_output_dir.c_str());
		return -1;
	}
	else{
		fprintf(indi_output, "Time,BP1,SP1,BV1,SV1,FirstAmt,FirstVolume,Amt,Volume,Vtarget,Vtraded,Ind,OutstandingQty\n");
		fflush(indi_output);
	}

	std::string exec_output_dir = std::string(m_file_path) + "/" + l_str_date + "_" + a_symbol + "_" + l_str_odrsz + "_" + l_str_dir + "_" + l_str_starttime + "_" + l_str_endtime + "_" + l_str_id + l_str_algo + "_execution.csv";
	FILE * exec_output = fopen(exec_output_dir.c_str(), "w+");
	if (exec_output == NULL){
		PRINT_WARN("open exec file failed: %s", exec_output_dir.c_str());
		return -1;
	}
	else{
		fprintf(exec_output, "Time,ParentOrderSize,ChildOrderSize,ExecSize,ExecPrice,Order_type,Vtraded,Vtarget,Indicator\n");
		fflush(exec_output);
	}
	
	m_indi_output.push_back(indi_output);
	m_exec_output.push_back(exec_output);
	return 0;
}

void
SmartExecution::create_vwap_profile(Contract *instr, int ttl_odrsize, int a_start_time, int a_end_time){
	
	// Normailze, calculate VWAP Target based on Vol Profile
	MyArray<double> *l_vol_profile = nullptr;
	MyArray<int> *l_VWAP_target = nullptr;
	if (m_vol_profile.exist(instr->symbol)){
		l_vol_profile = m_vol_profile.at(instr->symbol);
	}
	else{
		PRINT_ERROR("Cannot find volume profile for %s", instr->symbol);
		return;
	}

	if (m_VWAP_target.exist(instr->symbol)) {
		l_VWAP_target = m_VWAP_target.at(instr->symbol);
	} else {
		l_VWAP_target = new MyArray<int>(MAX_VOL_RECORD_SIZE);
		m_VWAP_target.insert(instr->symbol, l_VWAP_target);
	}

	int l_cum_target = 0;
	for (int l_time = a_start_time; l_time <= a_end_time; l_time = add_trading_time(l_time, 60)){ // Add 1 min each time.
		//What if 93030000? Find a way to get vol at time t.
		double l_strat_vol = *l_vol_profile->at(get_trading_time_diff(MORNING_MARKET_OPEN_TIME, a_start_time) / 60);
		double l_end_vol = *l_vol_profile->at(get_trading_time_diff(MORNING_MARKET_OPEN_TIME, a_end_time) / 60);
		double l_vol = *l_vol_profile->at(get_trading_time_diff(MORNING_MARKET_OPEN_TIME, l_time) / 60);
		l_cum_target = int((l_vol - l_strat_vol) / (l_end_vol - l_strat_vol) * ttl_odrsize);
		if (l_VWAP_target != NULL)
			l_VWAP_target->push_back(l_cum_target);
	}
	// Initialize Vtraded
	MyArray<int> *l_VWAP_traded = nullptr;
	if (m_VWAP_traded.exist(instr->symbol) == false) {
		l_VWAP_traded = new MyArray<int>(MAX_VWAP_TRADED_VOL_SIZE);
		m_VWAP_traded.insert(instr->symbol, l_VWAP_traded);
	}
}

void
SmartExecution::create_twap_profile(Contract *instr, int ttl_odrsize, int a_start_time, int a_end_time){

	// Normailze, calculate TWAP Target based on Vol Profile
	MyArray<int> *l_VWAP_target = nullptr;
	
	if (m_VWAP_target.exist(instr->symbol)) {
		l_VWAP_target = m_VWAP_target.at(instr->symbol);
	} else {
		l_VWAP_target = new MyArray<int>(MAX_VOL_RECORD_SIZE);
		m_VWAP_target.insert(instr->symbol, l_VWAP_target);
	}

	int l_cum_target = 0;
	for (int l_time = a_start_time; l_time <= a_end_time; l_time = add_trading_time(l_time, 60)){ // Add 1 min each time.
		//What if 93030000? Find a way to get vol at time t.
		double l_strat_vol = *(double *)m_twap_vol_profile->at(get_trading_time_diff(MORNING_MARKET_OPEN_TIME, a_start_time) / 60);
		double l_end_vol = *(double *)m_twap_vol_profile->at(get_trading_time_diff(MORNING_MARKET_OPEN_TIME, a_end_time) / 60);
		double l_vol = *(double *)m_twap_vol_profile->at(get_trading_time_diff(MORNING_MARKET_OPEN_TIME, l_time) / 60);
		l_cum_target = int((l_vol - l_strat_vol) / (l_end_vol - l_strat_vol) * ttl_odrsize);
		if (l_VWAP_target != NULL)
			l_VWAP_target->push_back(l_cum_target);
	}
	// Initialize Vtraded
	MyArray<int> *l_VWAP_traded = nullptr;
	if (m_VWAP_traded.exist(instr->symbol) == false) {
		l_VWAP_traded = new MyArray<int>(MAX_VWAP_TRADED_VOL_SIZE);
		m_VWAP_traded.insert(instr->symbol, l_VWAP_traded);
	}
}

int
SmartExecution::send_execution_orders(Contract *instr, EXCHANGE exch, int ttl_odrsize, DIRECTION side, double price,
int a_start_time, int a_end_time, EXEC_ALGORITHM a_algo_type)
{
	// Do input Checks. 
	if (exch != SSE && exch != SZSE){
		PRINT_WARN("Smart Execution algorithm is only used for stock right now.");
		return -1;
	}
	if (a_start_time > a_end_time){
		PRINT_WARN("Smart Execution algorithm fails because of start is behind the end time %d %d", a_start_time, a_end_time);
		return -1;
	}
	if (ttl_odrsize <= 0){
		PRINT_WARN("%s Smart Execution algorithm fails because ttl_odrsize is not greater than zero. %d", instr->symbol, ttl_odrsize);
		return -1;
	}
	if (side == ORDER_SELL && ttl_odrsize > instr->pre_long_qty_remain){
		PRINT_WARN("Smart Execution algorithm fails because ttl_odrsize is greater than the long position it can sell. %d %d", ttl_odrsize, instr->pre_long_qty_remain);
		return -1;
	}
	if (a_algo_type != TWAP && a_algo_type != VWAP){
		PRINT_WARN("Smart Execution algorithm fails because algorithm type is invalid! %d", a_algo_type);
		return -1;
	}
	if (instr->pre_data_time == 0){
		PRINT_WARN("Smart Execution algorithm fails. Try to call again after first tick on book.");
		return -1;
	}
	if (a_start_time < MAX(MORNING_MARKET_OPEN_TIME, instr->pre_data_time)){
		PRINT_WARN("Start_time is modifiled to %d since the time user specified %d is ahead of market open or current time %d.",
			MAX(MORNING_MARKET_OPEN_TIME, instr->pre_data_time), a_start_time, instr->pre_data_time);
		a_start_time = MAX(MORNING_MARKET_OPEN_TIME, instr->pre_data_time) / 100000 * 100000;  // remove seconds for used in normalizing Vtarget
	}
	if (a_end_time > AFTERNOON_MARKET_CLOSE_TIME){
		PRINT_WARN("End_time is modifiled to 150000000 since the time user specified %d is behind market close time.", a_end_time);
		a_end_time = AFTERNOON_MARKET_CLOSE_TIME;
	}

	std::string l_symbol(instr->symbol);
	if (m_vwap_config.exist(instr->symbol) == false) {
		return -1;
	}

	// Create a vwap object
	m_VWAP_order_count++;
	uint64_t l_sig_id = 0;
	time_t now = time(NULL);
	struct tm *l_now_tm = localtime(&now);
	int l_time = l_now_tm->tm_hour  * 100 + l_now_tm->tm_min;
	l_sig_id = (m_VWAP_order_count * 10000 + l_time) * 10000000000 + m_strat_id;
	
	smart_execution_config *l_vwap_cfg = m_vwap_config.at(instr->symbol);
// 	l_vwap_cfg->aggressiveness = 0.2;
// 	l_vwap_cfg->passive_indicator = 0.2;
// 	l_vwap_cfg->aggressive_indicator = 0.8;
// 	l_vwap_cfg->time_base = 480;

	if (open_execution_file(l_symbol, l_sig_id, ttl_odrsize, side, a_start_time, a_end_time, a_algo_type) == -1){
		PRINT_WARN("Open execution files failed.");
		return -1;
	}

	parent_order *vwap_ord = vwap_parent_orders->update_order(m_VWAP_order_count, instr, price, ttl_odrsize, side, l_sig_id, a_start_time, a_end_time);
	//Store variable 
	vwap_ord->stream_index = int(m_indi_output.size() - 1);
	l_vwap_cfg->VWAP_flag = true;

	// Calc some variables.
	vwap_ord->tranche_time = int(l_vwap_cfg->time_base * MAX(0.5, 1 - l_vwap_cfg->aggressiveness));
	//PRINT_DEBUG("Tbase: " << time_base << " aggressiveness: " << aggressiveness << " tranche time: "
	//	<< vwap_ord->tranche_time << " passive indicator: " << passive_indicator << " aggressive indicator: " << aggressive_indicator);
	//tranche_time = 240;
	vwap_ord->VWAP_flag_synch_cancel = false;

	if (a_algo_type == VWAP){
		create_vwap_profile(instr, ttl_odrsize, a_start_time, a_end_time);
	}
	else{
		create_twap_profile(instr, ttl_odrsize, a_start_time, a_end_time);
	}

	strategy_parent_order_info_t l_msg = { 0 };
	l_msg.head.type = S_STRATEGY_PARENT_ORDER_INFO;
	l_msg.head.size = sizeof(strategy_parent_order_info_t);
	l_msg.data.parent_order_id = l_sig_id;
	my_strncpy(l_msg.data.symbol, instr->symbol, SYMBOL_LEN);
	l_msg.data.parent_order_size = ttl_odrsize;
	l_msg.data.limit_price = price;
	l_msg.data.algorithm = a_algo_type;
	l_msg.data.start_time = a_start_time;
	l_msg.data.end_time = a_end_time;
	l_msg.data.side = side;
	m_sdp_handler->m_send_info_func(S_STRATEGY_PARENT_ORDER_INFO, sizeof(l_msg), (void*)&l_msg);
	
	PRINT_INFO("%s successfully sent parent orders", vwap_ord->contr->symbol);
	return 0;
}


void
SmartExecution::process_VWAP(Stock_Internal_Book *a_book)
{
	if (m_vwap_config.size() == 0 || m_is_vwap_called == false) return;

	Contract *l_instr = NULL;
	if (m_sdp_handler->m_contracts->exist(a_book->ticker))
		l_instr = &m_sdp_handler->m_contracts->at(a_book->ticker);
	else
		return;

	if (m_vwap_config.exist(l_instr->symbol) == false) return;

	smart_execution_config *l_vwap_cfg = m_vwap_config.at(l_instr->symbol);

	list_head* pos, *n;
	list_head l_vwap_orders;
	vwap_parent_orders->search_vwap_order_by_instr(l_instr->symbol, &l_vwap_orders);

	list_for_each_safe(pos, n, &l_vwap_orders){
		parent_order *vwap_ord = list_entry(pos, parent_order, search_link);
		m_cur_vwap_parent_id = vwap_ord->parent_order_id;
		if (l_vwap_cfg->VWAP_flag && a_book->exch_time >= vwap_ord->VWAP_start_time && a_book->exch_time < vwap_ord->VWAP_end_time
			&& (a_book->exch_time <= MORNING_MARKET_CLOSE_TIME || a_book->exch_time >= AFTERNOON_MARKET_OPEN_TIME)){

			MyArray<int> *l_VWAP_traded = nullptr;
			MyArray<int> *l_VWAP_target = nullptr;
			if (m_VWAP_traded.exist(l_instr->symbol)) {
				l_VWAP_traded = m_VWAP_traded.at(l_instr->symbol);
			} else {
				PRINT_WARN("Cannot find Vtraded in m_VWAP_traded");
				return;
			}

			if (m_VWAP_target.exist(l_instr->symbol)) {
				l_VWAP_target = m_VWAP_target.at(l_instr->symbol);
			} else {
				PRINT_WARN("Cannot find Vtarget in m_VWAP_target");
				return;
			}
			
			if (get_trading_time_diff(a_book->exch_time, vwap_ord->nxt_tranche_start_time) <= 0){  // A new tranche.
				vwap_ord->nxt_tranche_start_time = add_trading_time(vwap_ord->nxt_tranche_start_time, vwap_ord->tranche_time);
				vwap_ord->VWAP_flag_in_the_market = false;
				if (get_trading_time_diff(vwap_ord->nxt_tranche_start_time, vwap_ord->VWAP_end_time) <= vwap_ord->tranche_time){
					vwap_ord->VWAP_flag_synch_cancel = true;
				}
			}

			// Calculate indicator.
			int l_target_size_t = cal_target_trade_size(vwap_ord, l_VWAP_target, a_book->exch_time);
			int l_target_size_tplusT = cal_target_trade_size(vwap_ord, l_VWAP_target, add_trading_time(a_book->exch_time, vwap_ord->tranche_time));

			int l_traded_size_tminusT = 0;
			int l_traded_size_t = 0;

			// Reserved qty is regarded as traded. l_traded_size_t should not be greater than l_target_size_tplusT even when additional shares are added during handling price limit.
			// VWAP_target_size_tplusT_fixed is the target of origin schedule for last child orders. 
			// If we use MIN(m_sdp_handler->get_pos_by_side(l_instr, l_instr->VWAP_side) + l_instr->VWAP_reserved_qty,VWAP_target_size_tplusT_fixed), Vtraded will uprush when new orders are placed
			// but no shares are filled.
			// 
			l_traded_size_t = MAX(get_traded_pos_by_side(l_instr, vwap_ord->VWAP_side) + vwap_ord->VWAP_reserved_qty - vwap_ord->VWAP_last_order_additional_shares,
				vwap_ord->VWAP_traded_size_tminusone);
			vwap_ord->VWAP_traded_size_tminusone = l_traded_size_t; // Not using l_traded_index - 1 because of risk of missing quotes.
			int l_traded_index = get_trading_time_diff(vwap_ord->VWAP_start_time, a_book->exch_time) / STOCK_TICK_FREQUENCY;
			*(int *)l_VWAP_traded->at(l_traded_index) = l_traded_size_t;   //Store l_traded_size_t
			if (l_traded_index - vwap_ord->tranche_time / STOCK_TICK_FREQUENCY >= 0){
				l_traded_size_tminusT = *(int *)l_VWAP_traded->at(l_traded_index - vwap_ord->tranche_time / STOCK_TICK_FREQUENCY);
				//Handle lack of traded_size caused by quote missing.
				if (l_traded_size_tminusT == 0 && l_traded_index > 0){
					int l_index = 1;
					while (l_index <= l_traded_index - vwap_ord->tranche_time / STOCK_TICK_FREQUENCY){
						if (*(int *)l_VWAP_traded->at(l_traded_index - vwap_ord->tranche_time / STOCK_TICK_FREQUENCY - l_index) != 0){
							l_traded_size_tminusT = *(int *)l_VWAP_traded->at(l_traded_index - vwap_ord->tranche_time / STOCK_TICK_FREQUENCY - l_index);
							break;
						}
						l_index++;
					}
					*(int *)l_VWAP_traded->at(l_traded_index - vwap_ord->tranche_time / STOCK_TICK_FREQUENCY) = l_traded_size_tminusT;
				}
				vwap_ord->VWAP_indicator = (double)(l_target_size_t - l_traded_size_t) / (l_target_size_tplusT - l_traded_size_tminusT);
			}
			else{
				vwap_ord->VWAP_indicator = (double)(l_target_size_t - l_traded_size_t) / l_target_size_tplusT;
			}

			//st_log("%d %.4f %d %d\n", a_book->int_time, l_VWAP_indicator, l_traded_size_t, l_traded_size_tminusT);
			//st_log("%d %.4f %d %d %d %d\n", a_book->int_time, l_VWAP_indicator, l_traded_size_t, l_traded_size_tminusT, l_target_size_t, l_target_size_tplusT);


			/*Check trade condition*/
			bool l_flag_cannot_trade = false;
			bool l_flag_reach_limit = false;
			bool l_flag_last_30_sec = false;
			bool l_flag_reach_price = false; // For passive orders only
			if ((float_compare(a_book->ap_array[0], a_book->lower_limit_px) == 0 && vwap_ord->VWAP_side == ORDER_SELL)
				|| (float_compare(a_book->bp_array[0], a_book->upper_limit_px) == 0 && vwap_ord->VWAP_side == ORDER_BUY)){
				l_flag_reach_limit = true;
			}
			if ((float_compare(a_book->ap_array[0], vwap_ord->VWAP_limit_price) < 0 && vwap_ord->VWAP_side == ORDER_SELL)
				|| (float_compare(a_book->bp_array[0], vwap_ord->VWAP_limit_price) > 0 && vwap_ord->VWAP_side == ORDER_BUY)){
				l_flag_reach_price = true;
			}
			if (l_flag_reach_limit || l_flag_reach_price){
				l_flag_cannot_trade = true;
				//l_instr->VWAP_start_target_size = l_target_size_t; // TO be decided
			}
			double price = 0.0;
			/*Handle finish status*/
			if (get_trading_time_diff(a_book->exch_time, vwap_ord->VWAP_end_time) < 5){
				m_sdp_handler->cancel_all_orders();
				goto aggressive_pass_price_limit;
			}
			else if (get_trading_time_diff(a_book->exch_time, vwap_ord->VWAP_end_time) <= 15){
				finish_vwap_orders(a_book->exch_time);
				goto aggressive_pass_price_limit;
			}
			else if (get_trading_time_diff(a_book->exch_time, vwap_ord->VWAP_end_time) <= 30){
				l_flag_last_30_sec = true;
			}
			/*Prepare trade*/
			//TODO: Add a random size to l_target_size_tplusT  
			
			if (vwap_ord->VWAP_indicator > l_vwap_cfg->aggressive_indicator || l_flag_last_30_sec){
				// Reaching limit and reaching price are considered seperately in aggressive mode
				if (l_flag_reach_limit == false){  //WantToTradeMustTradeAggressive
					// In normal situation without price limits, VWAP_reserved_qty is equal to 0 and hence l_desired_position = l_target_size_tplusT
					int l_based_shares = l_target_size_tplusT - vwap_ord->VWAP_reserved_qty;
					vwap_ord->VWAP_last_order_additional_shares = int((double)(l_target_size_tplusT - vwap_ord->VWAP_start_target_size) / (vwap_ord->VWAP_parent_order_size - vwap_ord->VWAP_start_target_size)
						* vwap_ord->VWAP_reserved_qty);
					int l_desired_position = l_based_shares + vwap_ord->VWAP_last_order_additional_shares;
					// X_vol is the size of child order to be placed. Compare X_vol with the volume in the market. 
					int X_vol = l_desired_position - l_traded_size_t;
					
					if (vwap_ord->VWAP_side == ORDER_BUY){
						if (X_vol > 2 * (a_book->av_array[0] + a_book->av_array[1])){
							X_vol = a_book->av_array[0] + a_book->av_array[1];
						}
						else if ((X_vol < 2 * (a_book->av_array[0] + a_book->av_array[1])) && (X_vol > 2 * a_book->av_array[0])){
							X_vol = MIN(X_vol, (a_book->av_array[0] + a_book->av_array[1]));
						}
						if (X_vol <= a_book->av_array[0]){
							price = a_book->ap_array[0];
						}
						else{
							price = a_book->ap_array[0] + l_instr->tick_size;
						}
						// Check trade condition: price assigned by users
						if (double_compare(price, vwap_ord->VWAP_limit_price) > 0){ //WantToTradeCannotTrade
							m_sdp_handler->cancel_all_orders(l_instr);  // TODO: decide whether we should keep those orders or not.
							//l_instr->VWAP_reserved_qty += (l_target_size_tplusT - l_traded_size_t);
							vwap_ord->VWAP_reserved_qty = l_target_size_tplusT - get_traded_pos_by_side(l_instr, vwap_ord->VWAP_side);
							vwap_ord->VWAP_start_target_size = l_target_size_t;
							goto aggressive_pass_price_limit;
						}
					}
					else if (vwap_ord->VWAP_side == ORDER_SELL){
						if (X_vol > 2 * (a_book->bv_array[0] + a_book->bv_array[1])){
							X_vol = a_book->bv_array[0] + a_book->bv_array[1];
						}
						else if ((X_vol < 2 * (a_book->bv_array[0] + a_book->bv_array[1])) && (X_vol > 2 * a_book->bv_array[0])){
							X_vol = MIN(X_vol, (a_book->bv_array[0] + a_book->bv_array[1]));
						}
						if (X_vol <= a_book->bv_array[0]){
							price = a_book->bp_array[0];
						}
						else{
							price = a_book->bp_array[0] - l_instr->tick_size;
						}
						// Check trade condition: price assigned by users
						if (double_compare(price, vwap_ord->VWAP_limit_price) < 0){ //WantToTradeCannotTrade
							m_sdp_handler->cancel_all_orders(l_instr);  // TODO: decide whether we should keep those orders or not.
							//l_instr->VWAP_reserved_qty += (l_target_size_tplusT - l_traded_size_t);
							vwap_ord->VWAP_reserved_qty = l_target_size_tplusT - get_traded_pos_by_side(l_instr, vwap_ord->VWAP_side); // No need to minus when withdrawl and filled?
							vwap_ord->VWAP_start_target_size = l_target_size_t;
							goto aggressive_pass_price_limit;
						}
					}
					l_desired_position = l_traded_size_t + X_vol;

					/*Information for execution report*/
					vwap_ord->VWAP_target_volume = l_target_size_t;
					//m_child_order_size = X_vol;  //
					std::string l_str("Aggressive");
					my_strncpy(vwap_ord->VWAP_order_type, l_str.c_str(), SYMBOL_LEN);
					vwap_ord->VWAP_int_time = a_book->exch_time;

					/*Send orders*/
					if (process_vwap_signal(vwap_ord, l_instr, vwap_ord->VWAP_side, l_desired_position, price, l_target_size_t, l_target_size_tplusT) == 0){
						//send_single_order(m_VWAP_instr, m_VWAP_instr->exch, price, ORDER_BUY,m_VWAP_side, a_session, SIG_ACTION_OPEN);
						if (vwap_ord->VWAP_flag_first_order == false){
							vwap_ord->FirstOrderVOL = a_book->total_vol;
							vwap_ord->FirstOrderAMT = a_book->total_notional;
							vwap_ord->VWAP_flag_first_order = true;
						}
					}
				}
				else{  //WantToTradeCannotTrade
					m_sdp_handler->cancel_all_orders(l_instr);  // Cancel the limit orders placed on upper/lower limit during passive mode. TODO: decide whether we should keep those orders or not.
					//l_instr->VWAP_reserved_qty += (l_target_size_tplusT - l_traded_size_t); // Not correct if there is more than one period of WantToTradeCannotTrade (not consecutive)
					vwap_ord->VWAP_reserved_qty = l_target_size_tplusT - get_traded_pos_by_side(l_instr, vwap_ord->VWAP_side);
					vwap_ord->VWAP_start_target_size = l_target_size_t;
				}
			}
			else if (vwap_ord->VWAP_indicator > l_vwap_cfg->passive_indicator && vwap_ord->VWAP_flag_in_the_market == false){
				// Only reaching price are considered in passive mode because there is no limit for passive orders.
				if (l_flag_reach_price == false){ //WantToTradeMustTradePassive
					if (vwap_ord->VWAP_side == ORDER_BUY){
						price = a_book->bp_array[0];
					}
					else if (vwap_ord->VWAP_side == ORDER_SELL){
						price = a_book->ap_array[0];
					}
					// In normal situation without price limits, VWAP_reserved_qty is equal to 0 and hence l_desired_position = l_target_size_tplusT
					int l_based_shares = l_target_size_tplusT - vwap_ord->VWAP_reserved_qty;
					vwap_ord->VWAP_last_order_additional_shares = int((double)(l_target_size_tplusT - vwap_ord->VWAP_start_target_size) / (vwap_ord->VWAP_parent_order_size - vwap_ord->VWAP_start_target_size)
						* vwap_ord->VWAP_reserved_qty);
					int l_desired_position = l_based_shares + vwap_ord->VWAP_last_order_additional_shares;

					/*Information for execution report*/
					vwap_ord->VWAP_target_volume = l_target_size_t;
					//m_child_order_size = X_vol;  //
					std::string l_str("Passive");
					my_strncpy(vwap_ord->VWAP_order_type, l_str.c_str(), SYMBOL_LEN);
					vwap_ord->VWAP_int_time = a_book->exch_time;

					//process_strat_signal(l_instr, l_instr->VWAP_side, l_target_size_tplusT, price, a_session);
					if (process_vwap_signal(vwap_ord, l_instr, vwap_ord->VWAP_side, l_desired_position, price, l_target_size_t, l_target_size_tplusT) == 0){
						if (vwap_ord->VWAP_flag_first_order == false){
							vwap_ord->FirstOrderVOL = a_book->total_vol;
							vwap_ord->FirstOrderAMT = a_book->total_notional;
							vwap_ord->VWAP_flag_first_order = true;
						}
					}

				}
				else{ //WantToTradeCannotTrade
					// TODO: decide if need to add into reserved_qty
				}
			}
			else if (vwap_ord->VWAP_flag_in_the_market == false){
				//m_sdp_handler->cancel_all_orders(a_session);
			}

		aggressive_pass_price_limit:
			int OutstandingQty = 0;
			if (vwap_ord->VWAP_side == ORDER_BUY)
				OutstandingQty = l_instr->pending_buy_open_size;
			else{
				OutstandingQty = l_instr->pending_sell_open_size + l_instr->pending_sell_close_size; //TODO: only sell close size is enough without async cancel order.
			}
			
			if (m_indi_output[vwap_ord->stream_index] != NULL){
				fprintf(m_indi_output[vwap_ord->stream_index], "%d,%.2f,%.2f,%d,%d,%.2f,%llu,%llu,%llu,%d,%d,%.6f,%d\n", a_book->exch_time, a_book->bp_array[0], a_book->ap_array[0], a_book->bv_array[0], a_book->av_array[0],
					vwap_ord->FirstOrderAMT, vwap_ord->FirstOrderVOL, a_book->total_notional, a_book->total_vol, l_target_size_t, l_traded_size_t, vwap_ord->VWAP_indicator, OutstandingQty);
				fflush(m_indi_output[vwap_ord->stream_index]);
			}
		}
	}
}

int
SmartExecution::cal_target_trade_size(parent_order* vwap_ord, MyArray<int>* l_VWAP_target, int int_time){
	if (int_time >= vwap_ord->VWAP_end_time){
		return (*(int *)l_VWAP_target->at(l_VWAP_target->size() - 1));
	}
	int l_target_interpolate = get_trading_time_diff(vwap_ord->VWAP_start_time, int_time) % 60;  //seconds
	int l_target_index = get_trading_time_diff(vwap_ord->VWAP_start_time, int_time) / 60;
	int l_target_size = *(int *)l_VWAP_target->at(l_target_index) + l_target_interpolate *
		(*(int *)l_VWAP_target->at(l_target_index + 1) - *(int *)l_VWAP_target->at(l_target_index)) / 60;

	return l_target_size;
}

void 
SmartExecution::paser_line(char* line) {
	smart_execution_config *l_vwap_config = new smart_execution_config();

	const char delims[] = ",";
	int count = 0;
	char symbol[SYMBOL_LEN];
	char *pNext = NULL;
	char* token = strtok_s(line, delims, &pNext);
	if (token != NULL && my_strcmp(token, "Symbol") == 0) {
		goto error_path;
	}
	while (token != NULL) {
		switch (count) {
			case 0: { strlcpy(symbol, token, SYMBOL_LEN); break; }
			case 1: { 
				if (atoi(token) >= 93000) {
					if (my_strcmp(token, "130000") == 0) {   // Ignore 130000 because it is the same with 113000
						goto error_path;
					}
					if (m_sdp_handler->m_contracts->exist(symbol) == false)
						goto error_path;
				}
				break; 
			}
			case 2: {
				double l_vol_tmp = atof(token);
				if (m_vol_profile.exist(symbol)) {
					m_vol_profile.at(symbol)->push_back(l_vol_tmp);
				} else {
					MyArray<double> *vol_profile = new MyArray<double>(MAX_VOL_RECORD_SIZE);
					vol_profile->push_back(l_vol_tmp);
					m_vol_profile.insert(symbol, vol_profile);
				}
			}
			case 3: { l_vwap_config->time_base = atoi(token); break; }
			case 4: { l_vwap_config->aggressiveness = atof(token); break; }
			case 5: { l_vwap_config->aggressive_indicator = atof(token); break; }
			case 6: { l_vwap_config->passive_indicator = atof(token); break; }
			default: {}
		}
		token = strtok_s(NULL, delims, &pNext);
		count++;
	}
	m_vwap_config.insert(symbol, l_vwap_config);

	return;

error_path:
	delete l_vwap_config;
}

bool 
SmartExecution::read_Vwap_file(const char* file) {
	FILE *file_p = fopen(file, "r");
	if (file_p == NULL) {
		return false;
	}

	char line[TOKEN_BUFFER_SIZE] = { 0 };
	while (fgets(line, TOKEN_BUFFER_SIZE, file_p)) {
		paser_line(line);
	}
	fclose(file_p);
	return true;
}

int
SmartExecution::read_VWAP_config(char *file_path)
{
	int ret = -1;
	
	PRINT_INFO("Current Smart Execution %s.", smart_execution_version);

	my_strncpy(m_file_path, file_path, sizeof(file_path));
	create_dir(m_file_path);

	char vol_profile_file[PATH_LEN] = "";
	snprintf(vol_profile_file, PATH_LEN, "./st_ev/volume_profile.csv");

	if(read_Vwap_file(vol_profile_file)) {
		PRINT_INFO("Read Vwap Config Successfully!");
	} else {
		PRINT_ERROR("Read Vwap Config Failed!");
	}

	/*Volume profile check*/
	ret = 0;
	for	(auto iter = m_vol_profile.begin(); iter != m_vol_profile.end(); iter++) {
		if (iter->second->size() != 241) {		// Valid vol_profile should have 241 vols (From 0930 to 1500).
			ret = -1;
		}
	}
	
	return ret;
}

void
SmartExecution::create_twap_config(){
	m_twap_vol_profile = new MyArray<double>(MAX_VOL_RECORD_SIZE);
	for (int i = 0; i < 241; i++){
		double l_tmp = double(i) / 240;
		m_twap_vol_profile->push_back(l_tmp);
	}
}

void
SmartExecution::handle_order_update(st_response_t *a_ord_update){
	child_order *l_child_order = NULL;
	auto l_iter = m_child_orders_map.find(a_ord_update->order_id);
	if (l_iter != m_child_orders_map.end()){
		l_child_order = l_iter->second;
	}
	if (l_child_order == NULL) {
		PRINT_ERROR("Error: unable to find order %s", a_ord_update->symbol);
		return;
	}
	int index = static_cast<int> (l_child_order->parent_order_id / 100000000000000);
	parent_order *vwap_order = vwap_parent_orders->query_order(index, l_child_order->parent_order_id);
	if (vwap_order == NULL || m_exec_output[vwap_order->stream_index] == NULL){
		return;
	}
	if (l_child_order->contr->pre_data_time >= vwap_order->VWAP_end_time){
		return;
	}

	switch (a_ord_update->status){
	case SIG_STATUS_PARTED:
		fprintf(m_exec_output[vwap_order->stream_index], "%d,%d,%d,%d,%.2f,%s,%d,%d,%.6f\n", l_child_order->contr->pre_data_time, l_child_order->VWAP_parent_order_size, 
			l_child_order->VWAP_child_order_size , a_ord_update->exe_volume, a_ord_update->exe_price, l_child_order->VWAP_order_type, get_traded_pos_by_side(vwap_order->contr, vwap_order->VWAP_side),
			l_child_order->VWAP_target_volume, l_child_order->VWAP_indicator);
		fflush(m_exec_output[vwap_order->stream_index]);

		break;
	case SIG_STATUS_SUCCEED:
		fprintf(m_exec_output[vwap_order->stream_index], "%d,%d,%d,%d,%.2f,%s,%d,%d,%.6f\n", l_child_order->contr->pre_data_time, l_child_order->VWAP_parent_order_size,
			l_child_order->VWAP_child_order_size, a_ord_update->exe_volume, a_ord_update->exe_price, l_child_order->VWAP_order_type, get_traded_pos_by_side(vwap_order->contr, vwap_order->VWAP_side),
			l_child_order->VWAP_target_volume, l_child_order->VWAP_indicator);
		fflush(m_exec_output[vwap_order->stream_index]);
		
		delete l_child_order;
		m_child_orders_map.erase(a_ord_update->order_id);
		break;
	case SIG_STATUS_CANCELED:
		fprintf(m_exec_output[vwap_order->stream_index], "%d,%d,%d,%d,%.2f,%s,%d,%d,%.6f\n", l_child_order->contr->pre_data_time, l_child_order->VWAP_parent_order_size,
			l_child_order->VWAP_child_order_size, 0, a_ord_update->exe_price, l_child_order->VWAP_order_type, get_traded_pos_by_side(vwap_order->contr, vwap_order->VWAP_side),
			l_child_order->VWAP_target_volume, l_child_order->VWAP_indicator);
		fflush(m_exec_output[vwap_order->stream_index]);

		delete l_child_order;
		m_child_orders_map.erase(a_ord_update->order_id);
		break;
	case SIG_STATUS_CANCEL_REJECTED:
		break;
	case SIG_STATUS_INTRREJECTED:
	case SIG_STATUS_REJECTED:{
		std::string l_str("Passive");
		if (strcmp_case(vwap_order->VWAP_order_type, (char*)l_str.c_str()) == 0){
			vwap_order->VWAP_flag_in_the_market = false;
		}
		break;
	}
	case SIG_STATUS_INIT:
	case SIG_STATUS_ENTRUSTED:
		break;
	}
}


int
SmartExecution::read_VWAP_ADV(char *a_ev_file_path){
	int ret = -1;
// 	std::string l_str_date = std::to_string(m_sim_date);
// 	SplitVec l_split;
// 	split_string(a_ev_file_path, ",", l_split);
// 	std::cout << "Reading ADV. " << std::endl;
// 	for (unsigned int i = 0; i < l_split.size(); i++) {
// 		FileParser l_ev_parser(l_split[i], ",");
// 		auto const l_ev = l_ev_parser.read_ev();
// 		if (l_ev.size() >= 1 && l_ev[0].size() >= 0 && l_ev[0][0] == "Date") {
// 			for (auto const &l_line : l_ev) {
// 				if (l_line.size() == 0) continue;
// 				if (strcmp_case(l_line[0].c_str(), l_str_date.c_str()) != 0) continue;
// 				std::string l_tmp_symbol(l_line[1]);
// 				if (l_line[1].size() < 6){  // Convert "1" to "000001"
// 					l_tmp_symbol.insert(0, 6 - l_line[1].size(), '0');
// 				}
// 				Contract *l_instr = m_sdp_handler->contracts->find(l_tmp_symbol.c_str());
// 
// 				if (l_line.size() > 6) {
// 					if (l_instr != NULL){
// 						l_instr->VWAP_adv = (uint64_t)atoi(l_line[2].c_str());
// 						l_instr->VWAP_adv_percent = atof(l_line[3].c_str());
// 						l_instr->VWAP_side = (DIRECTION)atoi(l_line[4].c_str() - 1);
// 						l_instr->VWAP_start_time = atoi(l_line[5].c_str());
// 						l_instr->VWAP_end_time = atoi(l_line[6].c_str());
// 					}
// 				}
// 			}
// 		}
// 	}
	return ret;
}

void SmartExecution::handle_day_finish()
{
	MyArray<double> *value_double = NULL;
	for (auto iter = m_vol_profile.begin(); iter != m_vol_profile.end(); iter++) {
		value_double = iter->second;
		if (value_double != NULL) {
			delete value_double;
			value_double = NULL;
		}
	}
	
	MyArray<int> *value_int = NULL;
	for (auto iter = m_VWAP_target.begin(); iter != m_VWAP_target.end(); iter++) {
		value_int = iter->second;
		if (value_int != NULL) {
			delete value_int;
			value_int = NULL;
		}
	}
	for (auto iter = m_VWAP_traded.begin(); iter != m_VWAP_traded.end(); iter++) {
		value_int = iter->second;
		if (value_int != NULL) {
			delete value_int;
			value_int = NULL;
		}
	}
	
	smart_execution_config *l_config = NULL;
	for (auto iter = m_vwap_config.begin(); iter != m_vwap_config.end(); iter++) {
		l_config = iter->second;
		if (l_config != NULL) {
			delete l_config;
			l_config = NULL;
		}
	}

	for (auto &iter : m_child_orders_map){
		if (iter.second != NULL){
			delete iter.second;
			iter.second = NULL;
		}
	}
	if (m_twap_vol_profile != NULL){
		delete m_twap_vol_profile;
		m_twap_vol_profile = NULL;
	}
	for (int i = 0; i < m_indi_output.size(); i++){
		fclose(m_indi_output[i]);
		m_indi_output[i] = NULL;
	}
	for (int i = 0; i < m_exec_output.size(); i++){
		fclose(m_exec_output[i]);
		m_exec_output[i] = NULL;
	}
	m_VWAP_order_count = 0;
	m_indi_output.clear();
	m_exec_output.clear();
	m_vol_profile.clear();
	m_VWAP_target.clear();
	m_VWAP_traded.clear();
	m_vwap_config.clear();
	m_child_orders_map.clear();
	vwap_parent_orders->clear();
}

int
SmartExecution::add_trading_time(int int_time, int seconds){
	int time = add_time(int_time, seconds);
	if (time > MORNING_MARKET_CLOSE_TIME && int_time <= MORNING_MARKET_CLOSE_TIME){
		time = add_time(time, get_time_diff(MORNING_MARKET_CLOSE_TIME, AFTERNOON_MARKET_OPEN_TIME));
	}
	return time;
}

int
SmartExecution::get_trading_time_diff(int time1, int time2){
	int seconds = 0;
	if (time1 < MORNING_MARKET_CLOSE_TIME && time2 > MORNING_MARKET_CLOSE_TIME){
		if (time2 <= AFTERNOON_MARKET_OPEN_TIME){
			seconds = get_time_diff(time1, MORNING_MARKET_CLOSE_TIME);
		}
		else{
			seconds = get_time_diff(time1, MORNING_MARKET_CLOSE_TIME) + get_time_diff(AFTERNOON_MARKET_OPEN_TIME, time2);
		}
	}
	else if (time1 < AFTERNOON_MARKET_OPEN_TIME && time2 > AFTERNOON_MARKET_OPEN_TIME){
		if (time1 >= MORNING_MARKET_CLOSE_TIME){
			seconds = get_time_diff(AFTERNOON_MARKET_OPEN_TIME, time2);
		}
		else{
			seconds = get_time_diff(time1, MORNING_MARKET_CLOSE_TIME) + get_time_diff(AFTERNOON_MARKET_OPEN_TIME, time2);
		}
	}
	else if (time1 >= MORNING_MARKET_CLOSE_TIME && time1 <= AFTERNOON_MARKET_OPEN_TIME && time2 >= MORNING_MARKET_CLOSE_TIME && time2 <= AFTERNOON_MARKET_OPEN_TIME){
		seconds = 0;
	}
	else{
		seconds = get_time_diff(time1, time2);
	}
	return seconds;
}

int 
SmartExecution::process_vwap_signal(parent_order* a_vwap_order, Contract *l_instr, DIRECTION a_side, int desired_size, double a_price, 
int a_target_size_t, int a_target_size_tplusT){
	// Check instrument is not NULL
	if (l_instr == NULL) {
		//PRINT_LOG('E', "Strategy::process_strat_signal - l_instr is NULL");
		return -1;
	}
	if (l_instr->exch != SZSE && l_instr->exch != SSE) {
		return -1;
	}
	if (l_instr->order_no_entrusted_num != 0) {
		PRINT_WARN("No entrusted response received.");
		return -1;
	}

	PRINT_INFO("%d %s Enter process_vwap_signal, desire %s %d@%f", m_strat_id, l_instr->symbol,
		(a_side == ORDER_BUY ? "Buy" : "Sell"), desired_size, a_price);

	if (desired_size > a_vwap_order->VWAP_parent_order_size){
		PRINT_WARN("Desire size is larger than parent order size. %d %d\n", desired_size, a_vwap_order->VWAP_parent_order_size);
		return -1;
	}

	int pending_open_size = 0;
	int pending_close_size = 0;
	int pending_close_yes_size = 0;
	m_sdp_handler->cancel_orders_with_dif_px(l_instr, a_side, a_price, pending_open_size, pending_close_size, pending_close_yes_size);
	m_sdp_handler->cancel_orders_by_side(l_instr, switch_side(a_side));

	int ret_send_order = 0;
	int order_size = 0;
	if (a_side == ORDER_BUY){
		// Considering we might have both yd and td, use init_today_long_qty instead of l_instr->pre_long_pos.qty
		order_size = desired_size - (position(l_instr) - l_instr->init_today_long_qty) - pending_open_size; 
		// Odd lot
		if (order_size % 100 != 0){
			order_size -= (order_size % 100);
		}
		account_t& l_account = m_sdp_handler->m_accounts.at(l_instr->account);
		if (double_compare(l_account.cash_available, a_price * order_size * 1.0025) < 0) {
			int l_size = int(l_account.cash_available / a_price / 100) * 100;
			PRINT_WARN("%s current cash: %f. Can only buy %d rather than %d @ %f.",
				l_instr->symbol, l_account.cash_available, l_size, order_size, a_price);
			a_vwap_order->VWAP_reserved_qty = a_target_size_tplusT - get_traded_pos_by_side(l_instr, a_side) - pending_open_size - l_size;
			a_vwap_order->VWAP_start_target_size = a_target_size_t;
			order_size = l_size;
		}
		if (order_size == 0){
			return -1;
		}
		if (a_vwap_order->VWAP_flag_synch_cancel == false)
			ret_send_order = m_sdp_handler->send_single_order(ALGORITHM_TVWAP, l_instr, l_instr->exch, a_price, order_size, a_side, ORDER_OPEN);
		else
			ret_send_order = m_sdp_handler->send_single_order(ALGORITHM_TVWAP, l_instr, l_instr->exch, a_price, order_size, a_side, ORDER_OPEN, true);
	}
	else if (a_side == ORDER_SELL){
		// (l_instr->pre_long_pos.qty - (position(l_instr) - (l_instr->init_today_long_qty - l_instr->pre_long_pos.qty)))
		order_size = desired_size - (l_instr->init_today_long_qty - position(l_instr)) - pending_close_size;
		// Odd lot
		if (order_size % 100 != 0){
			order_size -= (order_size % 100);
		}
		if (l_instr->pre_long_qty_remain < order_size) {
			PRINT_WARN("%s Remaining pre long position: %d. Cannot sell %d shares.",
				l_instr->symbol, l_instr->pre_long_qty_remain, order_size);
			return -1;
		}
		// Only selling close yesterday's position is allowed.
		if (a_vwap_order->VWAP_flag_synch_cancel == false)
			ret_send_order = m_sdp_handler->send_single_order(ALGORITHM_TVWAP, l_instr, l_instr->exch, a_price, order_size, a_side, ORDER_CLOSE, false, true);
		else
			ret_send_order = m_sdp_handler->send_single_order(ALGORITHM_TVWAP, l_instr, l_instr->exch, a_price, order_size, a_side, ORDER_CLOSE, true, true);
	}
	if (ret_send_order == 0){
		// To confirm: No order splited is send_single_order for stocks.
		child_order *l_child_order = new child_order();
		l_child_order->contr = l_instr;
		l_child_order->parent_order_id = a_vwap_order->parent_order_id;
		l_child_order->child_order_id = m_sdp_handler->m_cur_ord_id_arr[0];
		l_child_order->VWAP_child_order_size = order_size;
		l_child_order->VWAP_parent_order_size = a_vwap_order->VWAP_parent_order_size;
		l_child_order->VWAP_int_time = a_vwap_order->VWAP_int_time;
		my_strncpy(l_child_order->VWAP_order_type, a_vwap_order->VWAP_order_type, SYMBOL_LEN);
		l_child_order->VWAP_target_volume = a_vwap_order->VWAP_target_volume;
		l_child_order->VWAP_indicator = a_vwap_order->VWAP_indicator;
		m_child_orders_map.insert(std::pair<uint64_t, child_order *>(l_child_order->child_order_id, l_child_order));

		a_vwap_order->VWAP_flag_in_the_market = true;
		
		int l_order_type = (strcmp_case(l_child_order->VWAP_order_type, "Aggressive") == 0) ? AGGRESSIVE : PASSIVE;

		strategy_child_order_info l_msg = { 0 };
		l_msg.head.type = S_STRATEGY_CHILD_ORDER_INFO;
		l_msg.head.size = sizeof(strategy_child_order_info);
		l_msg.data.child_order_id = l_child_order->child_order_id;
		l_msg.data.parent_order_id = l_child_order->parent_order_id;
		l_msg.data.child_order_size = l_child_order->VWAP_child_order_size;
		l_msg.data.execution_price = a_price;
		l_msg.data.order_type = l_order_type;
		l_msg.data.indicator = l_child_order->VWAP_indicator;
		m_sdp_handler->m_send_info_func(S_STRATEGY_CHILD_ORDER_INFO, sizeof(l_msg), (void*)&l_msg);

		return ret_send_order;
	}
	else{
		return -1;
	}
}
int
SmartExecution::get_traded_pos_by_side(Contract *a_instr, DIRECTION side){
	if (side == ORDER_BUY){
		return position(a_instr) - a_instr->init_today_long_qty;
	}
	else if (side == ORDER_SELL){
		return a_instr->init_today_long_qty - position(a_instr);
	}
	else{
		PRINT_WARN("Unknown SIG_DIRECTION");
		return -1;
	}
}

/* To target at parent order size especially for those symbols with no quote at last 30 seconds*/
void 
SmartExecution::finish_vwap_orders(int a_int_time){
	struct list_head *pos, *n;
	parent_order *vwap_ord;
	list_head * buy_list = vwap_parent_orders->get_order_by_side(ORDER_BUY);
	

	list_for_each_safe(pos, n, buy_list){
		vwap_ord = list_entry(pos, parent_order, pd_link);
		/* Trading Time Check*/
		if (get_trading_time_diff(a_int_time, vwap_ord->VWAP_end_time) > 15 || vwap_ord->VWAP_end_time < a_int_time){
			continue;
		}
		if (get_trading_time_diff(a_int_time, vwap_ord->VWAP_end_time) < 5){
			m_sdp_handler->cancel_all_orders(vwap_ord->contr);
			continue;
		}

		MyArray<int> *l_VWAP_target = NULL;
		if (m_VWAP_target.exist(vwap_ord->symbol)) {
			l_VWAP_target = m_VWAP_target.at(vwap_ord->symbol);
		} else {
			PRINT_WARN("Cannot find Vtarget in finish_vwap_orders");
			continue;
		}
		
		int a_target_size_t = cal_target_trade_size(vwap_ord, l_VWAP_target, a_int_time);
		int l_cur_pos = get_traded_pos_by_side(vwap_ord->contr, vwap_ord->VWAP_side);
		if (vwap_ord->VWAP_parent_order_size - l_cur_pos >= 100){
			PRINT_INFO("Finish vwap orders for %s on %d at %d", vwap_ord->contr->symbol, m_sim_date, a_int_time);
			int X_vol = vwap_ord->VWAP_parent_order_size - l_cur_pos;
			if (X_vol > vwap_ord->contr->av1){
				X_vol = vwap_ord->contr->av1;
			}
			int l_desired_position = l_cur_pos + X_vol;
			/* Handle limit_price and upper limit*/
			if (double_compare(vwap_ord->contr->ap1, vwap_ord->VWAP_limit_price) <= 0 && double_compare(vwap_ord->contr->ap1, 0) > 0){
				/*Information for execution report*/
				vwap_ord->VWAP_target_volume = a_target_size_t;
				std::string l_str("Aggressive");
				my_strncpy(vwap_ord->VWAP_order_type, l_str.c_str(), SYMBOL_LEN);
				vwap_ord->VWAP_int_time = a_int_time;

				/*Send orders*/
				process_vwap_signal(vwap_ord, vwap_ord->contr, vwap_ord->VWAP_side, l_desired_position, vwap_ord->contr->ap1, a_target_size_t, vwap_ord->VWAP_parent_order_size);
			}
			
		}
	}

	list_head * sell_list = vwap_parent_orders->get_order_by_side(ORDER_SELL);
	list_for_each_safe(pos, n, sell_list){
		vwap_ord = list_entry(pos, parent_order, pd_link);
		/* Trading Time Check*/
		if (get_trading_time_diff(a_int_time, vwap_ord->VWAP_end_time) > 15 || vwap_ord->VWAP_end_time < a_int_time){
			continue;
		}
		if (get_trading_time_diff(a_int_time, vwap_ord->VWAP_end_time) < 5){
			m_sdp_handler->cancel_all_orders(vwap_ord->contr);
			continue;
		}

		MyArray<int> *l_VWAP_target = NULL;
		if (m_VWAP_target.exist(vwap_ord->symbol)){
			l_VWAP_target = m_VWAP_target.at(vwap_ord->symbol);
		} else{
			PRINT_INFO("Cannot find Vtarget in finish_vwap_orders");
			continue;
		}
		int a_target_size_t = cal_target_trade_size(vwap_ord, l_VWAP_target, a_int_time);
		int l_cur_pos = get_traded_pos_by_side(vwap_ord->contr, vwap_ord->VWAP_side);
		if (vwap_ord->VWAP_parent_order_size - l_cur_pos >= 100){
			PRINT_INFO("Finish vwap orders for %s on %d at %d\n", vwap_ord->contr->symbol, m_sim_date, a_int_time);
			int X_vol = vwap_ord->VWAP_parent_order_size - l_cur_pos;
			if (X_vol > vwap_ord->contr->bv1){
				X_vol = vwap_ord->contr->bv1;
			}
			int l_desired_position = l_cur_pos + X_vol;
			/* Handle limit_price and upper limit*/
			if (double_compare(vwap_ord->contr->bp1, vwap_ord->VWAP_limit_price) >= 0 && double_compare(vwap_ord->contr->bp1, 0) > 0){
				/*Information for execution report*/
				vwap_ord->VWAP_target_volume = a_target_size_t;
				std::string l_str("Aggressive");
				my_strncpy(vwap_ord->VWAP_order_type, l_str.c_str(), SYMBOL_LEN);
				vwap_ord->VWAP_int_time = a_int_time;

				/*Send orders*/
				process_vwap_signal(vwap_ord, vwap_ord->contr, vwap_ord->VWAP_side, l_desired_position, vwap_ord->contr->bp1, a_target_size_t, vwap_ord->VWAP_parent_order_size);
			}

		}
	}
	
}
