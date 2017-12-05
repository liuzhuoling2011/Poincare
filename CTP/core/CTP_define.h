#pragma once

#define PATH_LEN 256
#define SYMBOL_LEN 64#define TRADING_DAY_LEN 9#define BROKER_ID_LEN 8#define ORDER_REF_LEN 13

const static char OPEN_CLOSE_STR[][16] = { "OPEN", "CLOSE", "CLOSE_TOD", "CLOSE_YES" };
const static char BUY_SELL_STR[][8] = { "BUY", "SELL" };
const static char DAY_NIGHT_STR[][8] = { "DAY", "NIGHT" };

enum TraderDirection
{
	TRADER_BUY = '0',
	TRADER_SELL = '1'
};
struct TraderConfig
{
	char  QUOTE_FRONT[SYMBOL_LEN]; // 行情前置
	char  QBROKER_ID[BROKER_ID_LEN];
	char  QUSER_ID[SYMBOL_LEN];
	char  QPASSWORD[SYMBOL_LEN];
	char  TRADER_FRONT[SYMBOL_LEN];	//交易前置
	char  TBROKER_ID[BROKER_ID_LEN];
	char  TUSER_ID[SYMBOL_LEN];
	char  TPASSWORD[SYMBOL_LEN];
	char *INSTRUMENTS[SYMBOL_LEN];
	int   INSTRUMENT_COUNT;

	int   STRAT_ID;
	char  STRAT_NAME[SYMBOL_LEN];
	char  STRAT_PATH[PATH_LEN];
	char  STRAT_EV[PATH_LEN];
	char  STRAT_OUTPUT[PATH_LEN];

	int   TIME_INTERVAL;
};

struct TraderInfo
{
    int FrontID;	//前置编号	int SessionID;	//会话编号
	int MaxOrderRef;	//报单引用
	char TradingDay[TRADING_DAY_LEN]; //当前交易日	char LoginTime[TRADING_DAY_LEN]; //当前登陆时间};