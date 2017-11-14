#include "strategy.h"
#include "utils/utils.h"
#include "utils/log.h"

Trader_Handler *s_trader_handler = NULL;

order_t default_order = { 0 };
static int i = 0;
void on_book(CThostFtdcDepthMarketDataField * pDepthMarketData)
{
	PRINT_SUCCESS("strategy on book");
	strlcpy(default_order.symbol, pDepthMarketData->InstrumentID, SYMBOL_LEN);
	default_order.direction = ORDER_BUY;
	default_order.open_close = ORDER_OPEN;
	default_order.price = pDepthMarketData->AskPrice1;
	default_order.volume = 1;

	if(++i % 50 == 0 && strcmp(default_order.symbol,"rb1805") == 0)
		s_trader_handler->send_single_order(&default_order);
}

void on_response(CThostFtdcTradeField * pTrade)
{
	printf("we are here!\n");
}
