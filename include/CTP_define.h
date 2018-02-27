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
	TAccount ACCOUNT;

	char  REDIS_IP[SYMBOL_LEN];
	int   REDIS_PORT;
	char  REDIS_CONTRACT[SYMBOL_LEN];
	char  REDIS_QUOTE[SYMBOL_LEN];
	char  REDIS_MULTI_QUOTE[SYMBOL_LEN];
	char  REDIS_QUOTE_CACHE[SYMBOL_LEN];
	char  REDIS_MULTI_QUOTE_CACHE[SYMBOL_LEN];

	char *INSTRUMENTS[SYMBOL_LEN];
	int   INSTRUMENT_COUNT;

	char  ASSIST_LOG[PATH_LEN];
};

struct TraderInfo
{
    int FrontID;	//ǰ�ñ��	int SessionID;	//�Ự���
	int MaxOrderRef;	//��������
	char TradingDay[TRADING_DAY_LEN]; //��ǰ������	char LoginTime[TRADING_DAY_LEN]; //��ǰ��½ʱ��
	long long START_TIME_STAMP;
	int SELF_CODE;};