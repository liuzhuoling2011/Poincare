#pragma once

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
	char  QUOTE_FRONT[SYMBOL_LEN]; // ����ǰ��
	char  QBROKER_ID[BROKER_ID_LEN];
	char  QUSER_ID[SYMBOL_LEN];
	char  QPASSWORD[SYMBOL_LEN];
	char  TRADER_FRONT[SYMBOL_LEN];	//����ǰ��
	char  TBROKER_ID[BROKER_ID_LEN];
	char  TUSER_ID[SYMBOL_LEN];
	char  TPASSWORD[SYMBOL_LEN];
	char *INSTRUMENTS[SYMBOL_LEN];
	int   INSTRUMENT_COUNT;
};

struct TraderInfo
{
    int FrontID;	//ǰ�ñ��	int SessionID;	//�Ự���
	int MaxOrderRef;	//��������
	char TradingDay[TRADING_DAY_LEN]; //��ǰ������	char LoginTime[TRADING_DAY_LEN]; //��ǰ��½ʱ��};