#pragma once

#define PATH_LEN 256
#define SYMBOL_LEN 64

//const static char OPEN_CLOSE_STR[][16] = { "OPEN", "CLOSE", "CLOSE_TOD", "CLOSE_YES" };
//const static char BUY_SELL_STR[][8] = { "BUY", "SELL" };
//const static char DAY_NIGHT_STR[][8] = { "DAY", "NIGHT" };

enum TraderDirection
{
	TRADER_BUY = '0',
	TRADER_SELL = '1'
};
struct TraderConfig
{
	char  QUOTE_FRONT[SYMBOL_LEN]; // ����ǰ��
	char  QBROKER_ID[BROKER_ID_LEN];
	char  QUSER_ID[SYMBOL_LEN];
	char  QPASSWORD[SYMBOL_LEN];
	char  TRADER_FRONT[SYMBOL_LEN];	//����ǰ��
	char  TBROKER_ID[BROKER_ID_LEN];
	char  TUSER_ID[SYMBOL_LEN];
	char  TPASSWORD[SYMBOL_LEN];

	char  REDIS_IP[SYMBOL_LEN];
	int   REDIS_PORT;
	char  REDIS_CONTRACT[SYMBOL_LEN];
	char  REDIS_QUOTE[SYMBOL_LEN];
	char  REDIS_MULTI_QUOTE[SYMBOL_LEN];
	char  REDIS_QUOTE_CACHE[SYMBOL_LEN];

	char *INSTRUMENTS[SYMBOL_LEN];
	int   INSTRUMENT_COUNT;
	bool  ONLY_RECEIVE_SUBSCRIBE_INSTRUMENTS_QUOTE;
	int   QUOTE_TYPE;

	int   STRAT_ID;
	char  STRAT_NAME[SYMBOL_LEN];
	char  STRAT_PATH[PATH_LEN];
	char  STRAT_EV[PATH_LEN];
	char  STRAT_OUTPUT[PATH_LEN];

	int   TIME_INTERVAL;
	char  TRADER_LOG[PATH_LEN];
	char  STRAT_LOG[PATH_LEN];
};

struct TraderInfo
{
    int FrontID;	//ǰ�ñ��
	int MaxOrderRef;	//��������
	char TradingDay[TRADING_DAY_LEN]; //��ǰ������