#ifndef C_QUOTE_SUNGARD_H
#define C_QUOTE_SUNGARD_H

/*
 * sungard's stock data
 */

#include <stdint.h>

struct sgd_option_quote
{
    int64_t time;                     // 行情时间 HHMMSSmmm
    int32_t market;
    char scr_code[32];
    int64_t seq_id;                   // 快照序号
    int64_t high_price;
    int64_t low_price;
    int64_t last_price;
    int64_t bgn_total_vol;
    int64_t bgn_total_amt;
    int64_t auction_price;            // 动态参考价格
    int64_t auction_qty;              // 虚拟匹配数量
    int64_t sell_price[5];
    int64_t bid_price[5];
    int64_t sell_volume[5];
    int64_t bid_volume[5];

    int64_t long_position;            // 当前合约未平仓数

    int64_t error_mark;               // 错误字段域
    int64_t publish_tm1;              // 一级发布时间
    int64_t publish_tm2;              // 二级发布时间
    int64_t pub_num;                  // 行情信息编号
    char trad_phase[4];            // 产品实施阶段及标志
    char md_report_id[12];         // 行情信息编号

    // added on 20150709 (static fields from code table)
    char contract_code[32];  // 510050C1512M02650
    int64_t high_limit_price;             // 涨停价 * 10000
    int64_t low_limit_price;              // 跌停价 * 10000
    int64_t pre_close_price;              // 昨收盘价 * 10000
    int64_t open_price;                   // 开盘价 * 10000
};

// 实时订单队列
struct sgd_order_queue
{
    int32_t time;
    int32_t market;
    char scr_code[32];
    char insr_txn_tp_code[4];      // 指令交易类型 'B','S'
    int32_t ord_price;                // 订单价格
    int32_t ord_qty;                  // 订单数量
    int32_t ord_nbr;                  // 明细个数
    int32_t ord_detail_vol[200];      // 订单明细数量 （参考宏汇接口，定义成200个整数数组）
};

// 实时逐笔委托
struct sgd_per_entrust
{
    int32_t market;
    char scr_code[32];
    int32_t entrt_time;               // 委托时间
    int32_t entrt_price;              // 委托价格
    int64_t entrt_id;                 // 委托编号
    int64_t entrt_vol;                // 委托数量

    char insr_txn_tp_code[4];      // 指令交易类型 'B','S'
    char entrt_tp[4];              // 委托类别
};

// 实时逐笔成交
struct sgd_per_bargain
{
    int32_t time;                     // 成交时间
    int32_t market;
    char scr_code[32];
    int64_t bgn_id;                   // 成交编号
    int32_t bgn_price;                // 成交价格
    int64_t bgn_qty;                  // 成交数量
    int64_t bgn_amt;                  // 成交金额

    char bgn_flg[4];               // 成交类别
    char nsr_txn_tp_code[4];       // 指令交易类型
};

#endif