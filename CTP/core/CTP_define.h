#pragma once

#define SYMBOL_LEN 64#define TRADING_DAY_LEN 9#define BROKER_ID_LEN 8#define ORDER_REF_LEN 13
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