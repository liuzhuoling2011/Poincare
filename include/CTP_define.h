#pragma once

#define PATH_LEN 256
#define SYMBOL_LEN 64#define TRADING_DAY_LEN 9#define BROKER_ID_LEN 8#define ORDER_REF_LEN 13

enum TraderDirection
{
	TRADER_BUY = '0',
	TRADER_SELL = '1'
};

struct TAccount
{
	char  FRONT[SYMBOL_LEN];
	char  BROKER_ID[BROKER_ID_LEN];
	char  USER_ID[SYMBOL_LEN];
	char  PASSWORD[SYMBOL_LEN];
};
struct TraderConfig
{
	char  QUOTE_FRONT[SYMBOL_LEN];
	char  QBROKER_ID[BROKER_ID_LEN];
	char  QUSER_ID[SYMBOL_LEN];
	char  QPASSWORD[SYMBOL_LEN];
	char  TRADER_FRONT[SYMBOL_LEN];	
	char  TBROKER_ID[BROKER_ID_LEN];
	char  TUSER_ID[SYMBOL_LEN];
	char  TPASSWORD[SYMBOL_LEN];

	char  REDIS_IP[SYMBOL_LEN];
	int   REDIS_PORT;
	char  REDIS_CONTRACT[SYMBOL_LEN];
	char  REDIS_QUOTE[SYMBOL_LEN];
	char  REDIS_MULTI_QUOTE[SYMBOL_LEN];
	char  REDIS_QUOTE_CACHE[SYMBOL_LEN];
	char  REDIS_MULTI_QUOTE_CACHE[SYMBOL_LEN];

	char *INSTRUMENTS[SYMBOL_LEN];
	int   INSTRUMENT_COUNT;

	bool  ONLY_RECEIVE_SUBSCRIBE_INSTRUMENTS_POSITION;
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
	
	TAccount ACCOUNT;
	char  ASSIST_LOG[PATH_LEN];
};

struct TraderInfo
{
    int FrontID;	//ǰ�ñ��	int SessionID;	//�Ự���
	int MaxOrderRef;	//��������
	char TradingDay[TRADING_DAY_LEN]; //��ǰ������	char LoginTime[TRADING_DAY_LEN]; //��ǰ��½ʱ��
	long long START_TIME_STAMP;
	int SELF_CODE;};