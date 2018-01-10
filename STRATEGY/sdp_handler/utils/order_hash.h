/*
* order.h
*
* Order Class
*
* Copyright(C) by MY Capital Inc. 2007-2999
*/
#pragma once
#ifndef __ORDER_H__
#define __ORDER_H__

#include <string>
#include "list.h"
#include "core/base_define.h"

struct hash_bucket {
	list_t head;
};

DIRECTION switch_side(DIRECTION side);

bool is_active(Order *a_order);

bool is_cancelable(Order *a_order);

bool is_finished(Order *a_order);

int leaves_qty(Order *ord);

int get_list_size(list_t *a_list);

int reverse_index(uint64_t ord_id);

class OrderHash{
public:
	OrderHash();
	~OrderHash();

	bool empty(){ return m_use_count == 0 ? true : false; }
	bool full();
	int size(){ return m_use_count; }
	void clear();
	int get_hash_value(int index);
	bool erase(Order *ord);

	Order* update_order(int index, Contract *a_contr,
		double price, int size, DIRECTION side,
		uint64_t sig_id, OPEN_CLOSE openclose);

	void set_parameter(Order *ord, Contract *a_contr,
		double a_price, int a_size, DIRECTION a_side,
		uint64_t a_signal_id, OPEN_CLOSE a_openclose = ORDER_OPEN);

	Order* get_first_free_order();

	void push_back(Order* item, list_t* a_list);

	void set_order_default(Order *ord);

	void update_order_list(Order* ord);
	 
	Order* query_order(uint64_t ord_id);

	Order* query_order(int index, uint64_t ord_id);

	list_t* get_order_by_side(DIRECTION a_side);

	int active_order_size(DIRECTION a_side);

	int get_free_order_size();

	int get_search_order_size();

	int get_pending_cancel_order_size_by_instr(Contract *a_instr);

	int get_buy_sell_list_size(DIRECTION a_side); //return the count of order in the list

	void get_active_order_list(list_t **a_buy_list, list_t **a_sell_list);

	void search_order_by_instr(const char* a_symbol, list_t *a_list);

	void search_order_by_side_instr(DIRECTION a_side, const char* a_symbol, list_t *a_list);

	void add_pending_size(Order *ord, int quantity);

	void minus_pending_size(Order *ord, int quantity);

	int open_order_size(Contract* a_contr, DIRECTION a_side);

	int get_pending_buy_size(Contract* contr) { return contr->pending_buy_close_size + contr->pending_buy_open_size; }

	int get_pending_sell_size(Contract* contr) { return contr->pending_sell_close_size + contr->pending_sell_open_size; }

	char* get_all_active_order_info();

private:
	int m_use_count;
	Order *p_order_head;
	hash_bucket *p_bucket;

	list_t *p_buy_list;
	list_t *p_sell_list;
	list_t *p_free_list;
	list_t *p_search_list;
};



#endif
