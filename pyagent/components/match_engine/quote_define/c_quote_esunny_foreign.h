/*
 *      c_quote_tap.h
 *
 *      the datatype of TapApi
 *
 *      Copyright(c) 2007-2015, by MY Capital Inc.
 */
#if !defined(_C_QUOTE_ESUNNY_FOREIGN_H_)
#define _C_QUOTE_ESUNNY_FOREIGN_H_

#pragma pack(push, 8)

// 易盛外盘行情 即时数据结构
struct my_esunny_frn_quote
{
	char	Market[40];		//市场中文名
	char	Code[66];		//合约代码

	float	YClose;			//昨收盘
	float	YSettle;		//昨结算
	float	Open;			//开盘价
	float	High;			//最高价
	float	Low;			//最低价
	float	New;			//最新价
	float	NetChg;			//涨跌
	float	Markup;			//涨跌幅
	float	Swing;			//振幅
	float	Settle;			//结算价
	float	Volume;			//成交量
	float	Amount;			//持仓量

	float	Ask[10];			//申卖价
	float	AskVol[10];		//申卖量
	float	Bid[10];			//申买价
	float	BidVol[10];		//申买量

	float	AvgPrice;		//平均价

	float   LimitUp;		//涨停板
	float   LimitDown;		//跌停板
	float   HistoryHigh;	//合约最高
	float   HistoryLow;		//合约最低

	float	YOPI;			//昨持仓
	float   ZXSD;			//昨虚实度
	float   JXSD;			//今虚实度
	float   CJJE;			//成交金额

	//新增外盘
	float TClose;			//收盘价
	float Lastvol;			//最新成交量
	float status;			//合约交易状态 -1:未知 0:开市 1:无红利 2:竞价 3:挂起 4:闭市 5:开市前 6:闭市前 7:快市
	float updatetime;			//更新时间,形如：235959表示23：59：59

	float		BestBPrice;					//最优买价
	float		BestBVol;					//最优买量
	float		BestSPrice;					//最优卖价
	float		BestSVol;					//最优卖量
};

#pragma pack(pop)

#endif 
