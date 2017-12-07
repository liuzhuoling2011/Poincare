/*
* The match engine external caller
* Copyright(C) by MY Capital Inc. 2007-2999
*/

#pragma once
#ifndef __MY_TRADE_H__
#define __MY_TRADE_H__

#include  "order_types_def.h"

struct match_node;
struct hash_bucket;
class  MatchEngine;
class  MyArray;

class EngineHash
{
public:
	EngineHash();
	~EngineHash();
	void clear();

	match_node *add(char* symbol, product_info *product, mode_config *mode, bool is_bar);
	match_node *find(char* symbol);
	match_node *find(int index);

	inline int size(){
		return m_max_count;
	}

private:
	match_node *hash_find(char* symbol);
	match_node *traverse_find(char* symbol);
	uint64_t hash_value(char *symbol);

private:
	int               m_use_count;
	int               m_max_count;
	hash_bucket      *m_bucket;
	match_node       *m_match_head;
};


class TradeHandler{
public:
	TradeHandler(int match_type);
	~TradeHandler();
	void clear();
	bool is_empty_order();
	void place_order(order_request* pl_ord, signal_response *ord_rtn);
	void cancel_order(order_request* cl_ord, signal_response *ord_rtn);
	void feed_quote(void* quote, int mi_type, signal_response** trd_rtn_ar, 
		            int* rtn_cnt, int market, bool is_bar = false);

	int  load_product(product_info* pi, int count);
	int  load_config(mode_config * m_cfg, int count);

private:
	product_info* default_product(char* symbol, char *prd, Exchanges exch);
	product_info* find_product(char* symbol, Exchanges exch);
	match_node*   find_engine(char* symbol, Exchanges exch, bool is_bar = false);
	mode_config * find_mode_config(product_info* prd);

//public:
//	ContractHash     *m_contracts;

private:
    int               m_match_type;

	mode_config       m_default_cfg;
	MyArray          *m_mode_cfg;

	int               m_pi_cnt;
	product_info     *m_pi_data;
	product_info      m_default_pi;

	EngineHash       *m_engine;
};

#endif
