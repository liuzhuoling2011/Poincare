/*
* smart_execution.h
*
* Execution algorithm
*
* Copyright(C) by MY Capital Inc. 2007-2999
*/
#pragma once
#ifndef __SMART_EXECUTION_H__
#define __SMART_EXECUTION_H__

#include <unordered_map>
#include <string>
#include <iostream>
#include <iomanip>
#include "core/base_define.h"
#include "core/sdp_handler.h"
#include "utils/log.h"
#include "utils/list.h"
#include "utils/utils.h"
#include "utils/MyArray.h"
#include "utils/MyHash.h"
#include "vwap_hash.h"

#define MORNING_MARKET_OPEN_TIME     93000000
#define MORNING_MARKET_CLOSE_TIME    113000000
#define AFTERNOON_MARKET_OPEN_TIME   130000000
#define AFTERNOON_MARKET_CLOSE_TIME  150000000
/************************************************************************/
/* Smart Execution class                                                */
/************************************************************************/
class SmartExecution
{
public:
	SmartExecution(SDPHandler* sdp_handler, int type, int length, void * cfg);
	~SmartExecution();
	
	int on_book(int type, int length, void * book);
	int on_response(int type, int length, void * resp);
	int on_timer(int type, int length, void * info) { return 0; }

private:
	int send_execution_orders(Contract *instr, EXCHANGE exch, int ttl_odrsize, DIRECTION side, double price,
		int a_start_time, int a_end_time, EXEC_ALGORITHM a_algo_type);
	int open_execution_file(std::string a_symbol, uint64_t a_sig_id, int ttl_odrsize, DIRECTION side, int a_start_time, int a_end_time, EXEC_ALGORITHM a_algo_type);
	void create_twap_profile(Contract *instr, int ttl_odrsize, int a_start_time, int a_end_time);
	void create_vwap_profile(Contract *instr, int ttl_odrsize, int a_start_time, int a_end_time);
	int cal_target_trade_size(parent_order* vwap_ord, MyArray<int>* l_VWAP_target, int int_time);
	int read_VWAP_ADV(char *a_ev_file_path);
	/*Time related*/
	int add_trading_time(int int_time, int seconds);
	int get_trading_time_diff(int time1, int time2);
	/*Send orders related*/
	int process_vwap_signal(parent_order* a_vwap_order, Contract *l_instr, DIRECTION side, int size, double price, int a_target_size_t, int a_target_size_tplusT);
	/*Position related*/
	int get_traded_pos_by_side(Contract *a_instr, DIRECTION side);

	void finish_vwap_orders(int a_int_time);
	void paser_line(char* line);
	bool read_Vwap_file(const char* file);

	/************************************************************************/
	/* Called from strategy                                                 */
	/************************************************************************/
	int send_orders_vwap(Contract *instr, EXCHANGE exch, int ttl_odrsize, DIRECTION side, double price,
		int a_start_time, int a_end_time);
	int send_orders_twap(Contract *instr, EXCHANGE exch, int ttl_odrsize, DIRECTION side, double price,
		int a_start_time, int a_end_time);

	/************************************************************************/
	/* Called from manager                                                  */
	/************************************************************************/
	void process_VWAP(Stock_Internal_Book *a_book);
	int read_VWAP_config(char *a_ev_file_path);
	void create_twap_config();
	/* handle events */
	void handle_order_update(st_response_t *a_ord_update);
	void handle_day_finish();

public:
	char m_file_path[PATH_LEN];
	SDPHandler *m_sdp_handler = NULL;

private:
	int m_strat_id = 0;
	int m_sim_date = 0;

	int m_VWAP_order_count = 0;
	uint64_t m_cur_vwap_parent_id = 0;
	/************************************************************************/
	/* VWAP related data */
	/************************************************************************/
	MyArray<double> * m_twap_vol_profile = nullptr;
	MyHash<MyArray<double> *> m_vol_profile;
	MyHash<MyArray<int> *> m_VWAP_target;
	MyHash<MyArray<int> *> m_VWAP_traded;
	MyArray<FILE *> m_indi_output;
	MyArray<FILE *> m_exec_output;
	MyHash<smart_execution_config *> m_vwap_config;
	std::unordered_map<uint64_t, child_order*> m_child_orders_map;
	VwapParentOrderHash *vwap_parent_orders;

	bool m_is_vwap_called = false;
	st_config_t *m_config;
};

#endif