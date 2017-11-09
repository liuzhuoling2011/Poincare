#pragma once

struct TraderConfig
{
	char  QUOTE_FRONT[64]; // 行情前置
	char  QBROKER_ID[8];
	char  QUSER_ID[64];
	char  QPASSWORD[64];
	char  TRADER_FRONT[64];	//交易前置
	char  TBROKER_ID[8];
	char  TUSER_ID[64];
	char  TPASSWORD[64];
	char *INSTRUMENTS[64];
	int   INSTRUMENT_COUNT;
};