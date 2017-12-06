#include <stdint.h>

#ifndef _QUOTE_FORMAT_DEFINE_H
#define _QUOTE_FORMAT_DEFINE_H

typedef struct
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
} Stock_Internal_Book;

typedef struct
{
	int  feed_type; /* type of the data source */
	char symbol[64];
	int16_t exchange;
	int  int_time;  /* 090059000, HourMintueSecondMilliSeconds*/
	float pre_close_px;
	float pre_settle_px;
	double pre_open_interest;
	double open_interest;
	float open_px;
	float high_px;
	float low_px;
	float avg_px;
	float last_px;
	float bp_array[5];
	float ap_array[5];
	int  bv_array[5];
	int  av_array[5];
	uint64_t total_vol;
	double total_notional;  /* Trade Notional, Turnover */
	float upper_limit_px;
	float lower_limit_px;
	float close_px;	/* Today's close price */
	float settle_px;
	/* DCE */
	int implied_bid_size[5];  /* Implied Bid/Ask Size */
	int implied_ask_size[5];
	/* Statistics Info, DCE have these from another feed */
	int total_buy_ordsize;		/* Total Open Buy Order Size */
	int total_sell_ordsize;     /* Total Open Sell Order Size */
	float weighted_buy_px;   /* Weighted Buy Order Price */
	float weighted_sell_px;  /* Weighted Sell Order Price */
} Futures_Internal_Book;

typedef struct
{
	char symbol[64];
	int  int_time;
	double open;
	double close;
	double high;
	double low;
	uint64_t volume;   // Total shares/vol
	double turnover;   // Total Traded Notional
	double upper_limit;
	double lower_limit;
	double open_interest;
	int bar_index;
} Internal_Bar;

typedef struct order_info {
     int order_count;  //需要下单的个数
     int msg_size;     //下单信息的字节数
     long seq_no;      //redis记录的本次操作的序号，在策略执行完下单后的响应中需要带上
     char buf[0];      //存放下单信息，有order_count个单，需要逐个解析
} manual_order_info_t;   

/**
 * 其中buf部分存放的是数个下单信息，结构体如下：
 */
struct manual_order {
    int     order_type;
    long    serial_no;
    char    symbol[32];
    int     direction;
    int     volume;
    double  price;
    int     price_type;
                
    int     c_p;
    int     expiry;
    int     strike;
    double  sigma;
    double  mat_next;
    double  mat_by_trade;
    double  unit_price;
    double  delta_coff;
    double  theta;
    int     date;
    int     tdx_date;

    char    exchg[32];
    int     open_close;
    int     speculator;    
};

#endif
