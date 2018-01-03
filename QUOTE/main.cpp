#include "Quote_Handler.h"
#include "utils/log.h"		 
#include "utils/utils.h"

int main(int argc, char **argv)
{
	PRINT_INFO("Welcome to Poincare Quote!");

	TraderConfig trader_config = { 0 };
	read_json_config(trader_config);

	CThostFtdcMdApi *QuoteApi = CThostFtdcMdApi::CreateFtdcMdApi("tmp/marketdata", false);
	Quote_Handler quote_handler(QuoteApi, &trader_config);
	QuoteApi->Init(); 
	QuoteApi->Join();

	free_config(trader_config);
	return 0;
}
