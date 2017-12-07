/*
* contract.h
*
* Provide utils functions
*
* Copyright(C) by MY Capital Inc. 2007-2999
*/
#pragma once
#ifndef __CONTRACT_H__
#define __CONTRACT_H__

#include <vector>
#include "list.h"
#include "platform/strategy_base_define.h"

class ContractHash
{
public:
	ContractHash();
	ContractHash(ContractHash *src);
	~ContractHash();

	//Get an empty node from table
	Contract *create();
	//Reset the data in the table
	void clear();

	//Node will hash to the table
	void insert(Contract *cnt);
	//Remove node from the list
	void erase(int index);

	//query node by symbol or index
	//Get existing or new node
	Contract *find(const char *symbol, bool is_create = false);
	Contract *find_product(const char *product, bool forward = false);
	void find_product(const char *product, std::vector<Contract *> &contracts);
	void find_group_id(int group_id, std::vector<Contract *> &contracts);
	Contract *find(int num);

	bool is_empty();

	//number for current use
	inline int size(){
		return m_use_count;
	}

	inline bool exists_at(int index){
		return (p_use_index[index] == 1);
	}
	
private:
	void init_hash_table();
	uint64_t hash_value(const char *str_key);

	void init_position(Position &post);
	void init_contract(Contract *cont);

	int          m_use_count;
	char        *p_use_index;

	hash_bucket  *p_bucket;
	Contract     *p_cont_head;
};


void add_contract_group_id(Contract *dest, int group_id);
void duplicate(Contract *src, Contract *dest);

//positions related

double avg_px(Position &pos);

void add_feed_type(Contract *cont, int type);

int position(Contract *cont);

int long_position(Contract *cont);

int short_position(Contract *cont);

double long_notional(Contract *cont);

double short_notional(Contract *cont);

double avg_buy_price(Contract *cont);

double avg_sell_price(Contract *cont);

double get_transaction_fee(Contract *cont, int size, double price, bool flag_close_yes = false);

double get_realized_PNL(Contract *cont);

double get_unrealized_PNL(Contract *cont);

double  get_contract_PNL(Contract *cont);

double  get_contract_Cash(Contract *cont);


#endif
