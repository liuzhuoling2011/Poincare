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
	bool ALL_CONTRACT_INFO_FLAG;	//��ѯ���к�Լ��Ϣ
	
	bool POSITION_FLAG;	  //��ѯ�ֲ���Ϣ
	bool ORDER_INFO_FLAG; //��ѯ������Ϣ
	bool TRADE_INFO_FLAG; //��ѯ�ɽ���¼
	char ASSIST_LOG[PATH_LEN];
};

struct TraderInfo
{
    int FrontID;	//ǰ�ñ��	int SessionID;	//�Ự���
	int MaxOrderRef;	//��������
	char TradingDay[TRADING_DAY_LEN]; //��ǰ������	char LoginTime[TRADING_DAY_LEN]; //��ǰ��½ʱ��
};