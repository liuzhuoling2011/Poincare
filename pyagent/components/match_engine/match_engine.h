/*
* The synchronous match engine for simulation
* Copyright(C) by MY Capital Inc. 2007-2999
* Author:Luan.Feng & Chen.xiaowen
* Version:V1.0.2  modify:20161121
*/

#pragma once
#ifndef __MATCH_ENGINE_H__
#define __MATCH_ENGINE_H__

#include <stdio.h>
#include <assert.h>
#include <cmath>
#include <fstream>
#include <unordered_map>
#include "quote_convert.h"
#include "order_types_def.h"
#include "tools.h"
#include "utils.h"
#include "list.h"

#define  MAX_TRD_PV_CNT   (16)
#define  MAX_PLACE_SIZE   (256)
#define  SWITCH_DIR(dir)  (dir==DIR_BUY ? 1 : 0)
#define  BAD_RESULT(dir)  (dir==DIR_BUY ? 1 : -1)
#define  GET_ST_ID(serial) (serial % 10000)

#ifdef   _WIN32
#define  g_likely(x)     x
#define  g_unlikely(x)   x
#else
#define  g_likely(x)     __builtin_expect(!!(x), 1)
#define  g_unlikely(x)   __builtin_expect(!!(x), 0)
#endif

//-----------Global structure & data definition----------
struct price_node{
	int        used;
	double     price;    //当前的价格
	long       volume;
	long       mkt_vol;  //当前价位上市场的量
	long       curr_vol; //当前tick上的下单量
	long       last_vol; //上一个tick的下单量
	long       opp_match_tick;  //对价上撮合的tick
	int        direct;   //买卖方向
	list_head  list;
	list_head  tick_list;
};

struct tick_node{
	int         used;
	long        tick;  //当前tick
	long        volume;
	int         owner; //0：OWNER_MY； 1：OWNER_MKT
	price_node* parent; 
	list_head   list;
	list_head   ord_list; //挂接订单列表
	list_head   st_list;  //挂接策略列表
};

struct strat_node {
	int               used;
	int               st_id;    //策略id
	int               vol;
	struct tick_node* parent;   //挂接上级父结点
	struct list_head  ord_list; //挂接订单列表
	struct list_head  list;
};

struct order_node {
	order_request      pl_ord;    //下单结构体信息
	int                data_flag;
	long               volume;       //订单剩余量
	double             trade_price;  //成交价
	long               trade_vol;    //成交量
	long               entrust_no;   //委托号
	OrderStatus        status;       //订单状态
	tick_node*         parent;
	strat_node*        the_st;
	list_head          list;
};

enum MARKET_TYPE{ KDB_QUOTE = 1, NUMPY_QUOTE = 2 };
enum Special{ Filter = 0, Update = 1, Process = 2 };
typedef std::unordered_map<uint64_t, order_node *>   PlaceOrderMap;
double get_midprice(double p1, double p2);


class  MatchEngine{

public:
	MatchEngine(char *symbol);
	virtual ~MatchEngine();

	//对外的通用接口
	virtual void  clear();
	virtual void  round_finish();
	virtual void  set_config(match_engine_config* cfg);

	virtual void  place_order(order_request* pl_ord, signal_response *ord_rtn);
	virtual void  cancel_order(order_request* cl_ord, signal_response *ord_rtn);
	virtual void  feed_stock_quote(int type, void* quote, int market, bool is_bar);
	virtual void  feed_future_quote(int type, void* quote, int market, bool is_bar);
	virtual void  execute_match(void** trd_rtn_ar, int* rtn_cnt);

	virtual void  update_order_book();
	virtual void  update_market_book(price_node* pn, int direct, double price);
	virtual void  do_match_on_price(price_node *pn, int direct);
	virtual void  find_trade_volume(double price, int direct);


public:
	//订单及列表的管理操作
	bool  is_empty_order();
	void  convert_stock_quote(void *quote, int type);
	void  convert_future_quote(void *quote, int type);
	void  malloc_node_data();
	mat_quote  *get_current_quote(bool is_first);
	order_node *get_order_node(uint64_t serial_no);
	order_node* remove_order_from_pool(order_request* cl_ord);
	list_head*  get_price_handle(int direct);
	price_node* add_price_node(int direct, double price);
	tick_node*  find_tick_node(price_node* pn, long tick, int owner);
	tick_node*  add_tick_node(price_node* pn, long tick, int owner);
	order_node* add_order_node(tick_node * tn, order_node* node);

	void  update_tick_node(tick_node* tn, int trad_vol);
	void  update_order_node(order_node* on, int trad_vol);
	void  set_order_result(order_node* order, signal_response *resp);

	//撮合过程的执行函数
	void  do_match_on_direct(int direct);
	void  do_match_on_overstep(price_node *pn, int direct);

	void  do_match_can_trade(price_node *pn);
	int   do_match_on_tick(price_node *pn, int exe_vol, bool is_all, int &self_trade);
	int   do_match_on_opposite(price_node *pn, int exe_vol, double exe_price);
	int   do_match_on_order(tick_node *tn, int exe_vol, double exe_price=0);

	void  calc_trade_volume();
	int   find_trade_volume(double price);

	//计算查询等辅助函数
	bool  is_skip_order(order_node* node);
	int   is_overstep(int direct, double price);
	int   is_market_shift(int direct, double price, bool is_curr);
	void  statis_order(tick_node *tn, int &count, int &total);
	int   calc_overstep_data(int direct, int volume, double &exe_price);
	int   is_can_trade(double market, double self, int direct);
	int   find_market_volume(int direct, double price, bool is_curr);
	int   find_opposite_volume(int direct, double price, double &exe_price);
	void  add_mult_quote_way(int m_type);
	bool  update_first_quote(int type, void *quote);
	int   round_stock_vol(int exe_vol);

	// 调试
	void show_price_list(bool flag=true);
	void show_tick_list(price_node* pn);
	void show_order_list(tick_node *tn);


public:
	long         m_entrust_num;
	long         m_tick_counter;

	//行情缓存控制部分
	bool         m_is_stock;   // True: stock; False: future
	bool         m_is_bar;
	int          m_market_type;
	bool         m_last_empty;
	int          m_last_type;
	char         m_last_quote[2048];
	void        *m_last_numpy;

	int          m_is_filter;
	int          m_mi_type;
	quote_info   m_quote;
	match_engine_config m_cfg;
	std::ofstream  m_write_log;

	//价格拆分的数据
	MyArray     *m_split_data;
	int          m_can_trade_num;
	pv_pair     *m_can_trade_list[MAX_TRD_PV_CNT];

	//存储订单列表的结点空间
	bool         m_is_malloc_node;
	MyArray     *m_trade_rtn;
	MyArray     *m_place_orders;
	MyArray     *m_price_data;
	MyArray     *m_ticks_data;

	int          m_place_num;
	order_node  *m_place_array[MAX_PLACE_SIZE];
	list_head    m_buy_prices;
	list_head    m_sell_prices;
	PlaceOrderMap   *m_place_hash;
	bool         m_debug_flag = false;
};

#endif
