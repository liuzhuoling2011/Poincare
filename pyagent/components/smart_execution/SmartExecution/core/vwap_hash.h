/*
* smart_execution.h
*
* Execution algorithm
*
* Copyright(C) by MY Capital Inc. 2007-2999
*/
#pragma once
#ifndef __VWAP_HASH_H__
#define __VWAP_HASH_H__

#include "stdlib.h"
#include "utils/list.h"
#include "base_define.h"

struct hash_vwap_bucket{
	struct list_head head;
};

/************************************************************************/
/* Enums                                                                */
/************************************************************************/
enum EXEC_ALGORITHM{
	DMA = 1,
	VWAP = 2,
	TWAP = 3
};

enum EXEC_ORDER_TYPE{
	PASSIVE = 1,
	AGGRESSIVE = 2
};

typedef struct {
	unsigned int type;
	unsigned int size;
	unsigned long clock;
	unsigned long ms;

	char for_future_use[32];
} msg_head_t;

/************************************************************************/
/* send info structs													*/
/************************************************************************/
typedef struct parent_order_info{
	uint64_t parent_order_id;
	char symbol[64];
	int parent_order_size;
	double limit_price;
	int algorithm;
	int start_time;
	int end_time;
	int side;
} parent_order_info_t;

typedef struct child_order_info {
	uint64_t child_order_id;
	uint64_t parent_order_id;
	int child_order_size;
	double execution_price;
	int order_type;
	double indicator;
} child_order_info_t;

typedef struct strategy_parent_order_info {
	msg_head_t head;
	parent_order_info_t data;
} strategy_parent_order_info_t;

typedef struct strategy_child_order_info {
	msg_head_t head;
	child_order_info_t data;
} strategy_child_order_info_t;

/************************************************************************/
/* VWAP structures                                                                     */
/************************************************************************/
struct smart_execution_config{
	/*vwap config*/
	int  time_base;
	double  aggressiveness;
	double  aggressive_indicator;
	double	passive_indicator;
	bool VWAP_flag;
};

struct parent_order
{
	uint64_t parent_order_id;
	Contract *contr;
	char symbol[SYMBOL_LEN];

	/*send_orders_vwap*/
	int VWAP_start_time;
	int VWAP_end_time;
	int nxt_tranche_start_time;
	DIRECTION VWAP_side;
	double VWAP_limit_price;
	int VWAP_parent_order_size;
	bool VWAP_flag;
	int tranche_time;
	bool VWAP_flag_synch_cancel;

	/*process_vwap*/
	int VWAP_child_order_size;
	bool VWAP_flag_in_the_market = false;
	double VWAP_indicator;
	int VWAP_reserved_qty;
	int VWAP_start_target_size;
	int VWAP_last_order_additional_shares;   //Vtraded upper bound
	int VWAP_traded_size_tminusone;

	/*output*/
	char VWAP_order_type[SYMBOL_LEN];
	int VWAP_int_time;                       // Time the child order is sent
	int VWAP_target_volume;
	unsigned long long FirstOrderVOL;
	double FirstOrderAMT;
	bool VWAP_flag_first_order = false;

	/*TO CHANGE*/
	int stream_index;

	/* Vwap hash*/
	struct list_head hs_link; // hash link
	struct list_head pd_link; // pending link
	struct list_head fr_link; // free unused link
	struct list_head search_link; // used to place in the search list
};

struct child_order{
	Contract *contr;
	/*VWAP related*/
	uint64_t parent_order_id;
	uint64_t child_order_id;
	int VWAP_parent_order_size;
	int VWAP_child_order_size;
	char VWAP_order_type[SYMBOL_LEN];
	int VWAP_target_volume;
	double VWAP_indicator;
	int VWAP_int_time;
};


class VwapParentOrderHash{
public:
	VwapParentOrderHash();
	~VwapParentOrderHash();

	bool empty(){ return m_use_count == 0 ? true : false; }
	int size(){ return m_use_count; }
	void clear();
	int get_hash_value(int index);
	bool erase(int index);
	bool erase(parent_order *ord);

	parent_order* update_order(int index, Contract* contr,
		double price, int size, DIRECTION side,
		uint64_t sig_id, int a_start_time, int a_end_time);

	void set_parameter(parent_order *ord, Contract* contr,
		double a_price, int a_size, DIRECTION a_side,
		uint64_t a_signal_id, int a_start_time, int a_end_time);

	parent_order* get_first_free_order();

	void push_back(parent_order* item, list_head* a_list);

	void set_order_default(parent_order *ord);

	parent_order* query_order(int index);

	parent_order* query_order(int index, uint64_t ord_id);

	int get_buy_sell_list_size(DIRECTION a_side); //return the count of order in the list

	list_head * get_order_by_side(DIRECTION a_side);

	void get_active_order_list(list_head **a_buy_list, list_head **a_sell_list);

	int get_free_order_size();

	int get_search_order_size();

	void search_vwap_order_by_instr(const char* a_symbol, list_head *a_list);

	void search_order_by_side_instr(DIRECTION a_side, const char* a_symbol, list_head *a_list);


private:
	int m_use_count;

	parent_order *p_order_head;

	hash_vwap_bucket  *p_bucket;

	struct list_head *p_free_list;
	struct list_head *p_buy_list;
	struct list_head *p_sell_list;
	struct list_head *p_search_list;
};
#endif


