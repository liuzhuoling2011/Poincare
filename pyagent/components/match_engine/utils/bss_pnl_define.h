#ifndef __BSS_PNL_DEFINE_H__
#define __BSS_PNL_DEFINE_H__

#include "task_define.h"

#define BSS_MSG_SIZE 128

#pragma pack (8)

struct bss_order_info
{
	int       place_cnt;  //下单次数
	int       trade_cnt;  //成交次数
	double    total_amt;  //成交额
	int       total_vol;  //成交量
};

struct bss_position
{ 
	int       long_volume; //剩余仓位数
	int       short_volume;
	double    long_price;
	double    short_price;
	double    today_cost;
};

struct bss_pnl_info
{
	int       strat_id;    //策略id
	int       param_index;
	char      strat_name[MIN_BUFF_SIZE];
	int       max_vol;     //最大手数
	int       cancel_cnt;  //撤单数
	double    principal;   //交易本金
	double    rounds;      //总回合数

	double    fee_vol;    //手续费
	double    gross_pnl;  //毛利润
	double    net_pnl;    //净收益

	double    parameter[3]; //当前参数值
	double    max_live_pnl; //最大收益
	double    max_tick_dd;  //最大回撤

	bss_order_info  order[3];  //买-0, 卖-1， 总-2
	bss_position    remain_pos;  //剩余仓位
};

struct bss_day_node{
	int            trade_date;           //交易日期
	int            day_night;            //日夜盘信息
	int            pnl_cnt;              //统计信息的个数
	double         total_pnl;            //日内总收益
	double         earnings;             //日内收益率
	bss_pnl_info **pnl_nodes;            //统计信息
};


struct  bss_task_ack 
{
	int    task_type;     //任务类型
	int    status;        //返回状态码
	char   message[BSS_MSG_SIZE];  //错误信息

	char   exchange;
	char   product[MAX_NAME_SIZE];
	char   symbol[MIN_BUFF_SIZE];
	int            day_cnt;   //处理的天数
	bss_day_node  *day_nodes;  //每天的数据结点
};

#pragma pack ()

#endif

