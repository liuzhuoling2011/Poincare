#pragma once

#define PATH_LEN 256
#define SYMBOL_LEN 64#define TRADING_DAY_LEN 9#define BROKER_ID_LEN 8

enum TraderDirection
{
	TRADER_BUY = '0',
	TRADER_SELL = '1'
};

struct TraderConfig
{
	char FRONT[SYMBOL_LEN];
	char BROKER_ID[BROKER_ID_LEN];
	char USER_ID[SYMBOL_LEN];
	char PASSWORD[SYMBOL_LEN];

	bool CONTRACT_INFO_FLAG;
	bool ALL_CONTRACT_INFO_FLAG;	//查询所有合约信息
	
	bool POSITION_FLAG;	  //查询持仓信息
	bool ORDER_INFO_FLAG; //查询订单信息
	bool TRADE_INFO_FLAG; //查询成交记录
	char ASSIST_LOG[PATH_LEN];
};

struct TraderInfo
{
    int FrontID;	//前置编号	int SessionID;	//会话编号
	int MaxOrderRef;	//报单引用
	char TradingDay[TRADING_DAY_LEN]; //当前交易日	char LoginTime[TRADING_DAY_LEN]; //当前登陆时间
};