#ifndef _BASE_DEFINE_H_
#define _BASE_DEFINE_H_

#include <stdint.h>
#include "utils/list.h"
#include "strategy_interface.h"

struct hash_bucket {
	list_t head;
};

struct Position {
	int     qty;
	double  notional;   /* A cost of current position, including fees in points */
};

enum POSITION_FLAG {
	LONG_OPEN = 0,
	LONG_CLOSE = 1,
	SHORT_OPEN = 2,
	SHORT_CLOSE = 3
};

struct Contract {
	char      account[ACCOUNT_LEN];    // Related Account, used to calculate cash.
	char      symbol[SYMBOL_LEN];
	int       expiration_date;        /* 20161020 */
	EXCHANGE  exch;

	int       init_today_long_qty;   /* Remaining Today's long position, including yesterday's pos */
	int       init_today_short_qty;	 /* Remaining Today's short position, including yesterday's pos */
	int       pre_long_qty_remain;   /* Remaining pre long position */
	int       pre_short_qty_remain;  /* Remaining pre short position */
	Position  positions[4];          // POSITION_FLAG, 1-4 LONG_OPEN = 1,LONG_CLOSE = 2,SHORT_OPEN = 3,SHORT_CLOSE = 4

	/* Risk Related */
	int       max_pos;
	int       max_accum_open_vol;     /* 交易所规定，最大双边累计开仓上限，例如上期 rb 7700, 目前图灵会传入，但SDP 不检查. Read from rss config in simulation */
	int       single_side_max_pos;    /* 最大单边仓位上限，用于锁仓功能 */
	int       order_cum_open_vol;

	bool      fee_by_lot;	          // True - Fee by Lot, False - Fee by Notional
	double    exchange_fee;
	double    yes_exchange_fee;
	double    broker_fee;
	double    stamp_tax;
	double    acc_transfer_fee;      // Account Transfer Fee - only for SSE
	double    tick_size;
	double    multiplier;

	int       bv1;
	int       av1;
	double    bp1;
	double    ap1;
	int       pending_buy_open_size;
	int       pending_sell_open_size;
	int       pending_buy_close_size;
	int       pending_sell_close_size;
	int       order_no_entrusted_num;
	int       pre_data_time;
	int       m_order_no_entrusted_num;
};

struct Order {
	uint64_t signal_id;
	EXCHANGE exch;
	Contract *contr;
	double price;
	int volume;
	DIRECTION side;
	OPEN_CLOSE openclose;
	INVESTOR_TYPE invest_type;
	ORDER_TYPE ord_type;
	TIME_IN_FORCE time_in_force;
	char symbol[SYMBOL_LEN];

	ORDER_STATUS status;

	double last_px;
	int last_qty;
	int cum_qty;
	double cum_amount;

	long orig_cl_ord_id;
	long orig_ord_id;

	bool pending_cancel;

	list_t hs_link; // hash link
	list_t pd_link; // pending link
	list_t fr_link; // free unused link
	list_t search_link; // used to place in the search list
};

struct Stock_Internal_Book
{
	char wind_code[32];         //600001.SH
	char ticker[32];             //原始Code
	int action_day;             //业务发生日(自然日)
	int trading_day;            //交易日
	int exch_time;				//时间(HHMMSSmmm)
	int status;				    //股票状态
	float pre_close_px;				//前收盘价 
	float open_px;					//开盘价 
	float high_px;					//最高价 
	float low_px;					//最低价 
	float last_px;				//最新价 
	float ap_array[10];			//申卖价 
	uint32_t av_array[10];			//申卖量
	float bp_array[10];			//申买价 
	uint32_t bv_array[10];			//申买量
	uint32_t num_of_trades;			//成交笔数
	int64_t total_vol;				//成交总量
	int64_t total_notional;		    //成交总额准确值,Turnover
	int64_t total_bid_vol;			//委托买入总量
	int64_t total_ask_vol;			//委托卖出总量
	float weighted_avg_bp;	//加权平均委买价格 
	float weighted_avg_ap;  //加权平均委卖价格
	float IOPV;					//IOPV净值估值  （基金）
	float yield_to_maturity;		//到期收益率	（债券）
	float upper_limit_px;			//涨停价 
	float lower_limit_px;			//跌停价 
	char prefix[4];			//证券信息前缀
	int PE1;					//市盈率1	未使用（当前值为0）
	int PE2;					//市盈率2	未使用（当前值为0）
	int change;					//升跌2（对比上一笔）	未使用（当前值为0）
};

#endif
