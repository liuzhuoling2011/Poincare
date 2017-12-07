/*
* The implementation of the general data structure
* Copyright(C) by MY Capital Inc. 2007-2999
*/
#pragma once
#ifndef __QUOTE_CONVERT_H__
#define __QUOTE_CONVERT_H__

#include "platform/strategy_base_define.h"
#include "utils/utils.h"

#define  QUOTE_LVL_CNT  5

enum Direction{
	DIR_BUY = 0,
	DIR_SELL = 1
};

enum Owner{
    OWNER_MY  = 0,
    OWNER_MKT = 1
};

struct quote_t {
	MI_TYPE  q_type;
	void    *quote;
};

struct pv_pair {
	double price;
	int    vol;
};

struct mat_quote {
	// buy & sell REQUEST_TYPE_QUOTE
	struct pv_pair bs_pv[2][QUOTE_LVL_CNT];

	int    int_time;
	double last_price;
	int    last_vol;

	double high_price;
	double low_price;
	double amount;
	int    total_volume;

	/* for czce only */
	int tot_b_vol;
	int tot_s_vol;
};

struct quote_info{
	int        mult_way;
	int        mi_types[QUOTE_LVL_CNT];
	double     high_limit;
	double     low_limit;

	mat_quote  last_quote;
	mat_quote  curr_quote;
	mat_quote  opposite;
};

//The price comparison function
typedef int(*better_price_func)(double, double);

inline int
better_buy(double left, double right){
	return (left > right);
}

inline int
better_sell(double left, double right){
	return (left < right);
}

void process_stock_book(void *origin, quote_info* info);

void process_future_book(void *origin, quote_info* info);

void process_internal_bar(void *origin, int type, quote_info* info);



/************ Convert Data to Internal_Book, Internal_Bar ************/
void
normalize_bar(MI_TYPE a_quote_type, void *a_quote);
void
normalize_book(MI_TYPE a_quote_type, void *a_quote, Stock_Internal_Book *s_book, Futures_Internal_Book *f_book);

int
convert_ib_depth_to_futures_book(ib_deep_quote *a_quote,
	Futures_Internal_Book *a_book_out);
int
convert_my_level1_to_futures_book(my_level1_quote *a_quote,
	Futures_Internal_Book *a_quote_out);
int
convert_stock_index_to_stock_book(my_stock_index_quote *a_quote,
	Stock_Internal_Book *a_quote_out);
int
convert_stock_depth_to_stock_book(my_stock_lv2_quote *a_quote,
	Stock_Internal_Book *a_book_out);
int
convert_stock_option_to_stock_book(sgd_option_quote *a_quote,
	Stock_Internal_Book *a_book);
int
convert_DCE_depth_to_futures_book(dce_my_best_deep_quote *a_quote,
	Futures_Internal_Book *a_book_out);
int
convert_CZCE_depth_to_futures_book(czce_my_deep_quote *p_stcZZQuoteIn,
	Futures_Internal_Book *a_book_out);
int
convert_SHFE_depth_to_futures_book(shfe_my_quote *p_stSHQuoteIn,
	Futures_Internal_Book *a_book_out);
int
convert_CFFEX_depth_to_futures_book(cffex_deep_quote *a_quote,
	Futures_Internal_Book *a_book_out);
int
convert_sge_depth_to_futures_book(sge_deep_quote *a_quote,
	Futures_Internal_Book *a_quote_out);
int
convert_esunny_frn_to_futures_book(my_esunny_frn_quote *a_quote,
	Futures_Internal_Book *a_quote_out);
int
convert_DCE_level1_to_futures_book(dce_my_ctp_deep_quote *a_quote,
	Futures_Internal_Book *a_book_out);
int
convert_DCE_OrderStat_to_futures_book(dce_my_ord_stat_quote * a_quote,
	Futures_Internal_Book *a_book_out);

#endif
