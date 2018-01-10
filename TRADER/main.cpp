#include "Trader_Handler.h"
#include "utils/log.h"		 
#include "utils/utils.h"

#ifndef _WIN32
	#include <unistd.h>
#else
	#include <windows.h>
	#define sleep Sleep
#endif

#define LOG_DIR './logs'
extern FILE* log_handle;

Trader_Handler *trader_handler;

int main(int argc, char **argv)
{
	PRINT_INFO("Welcome to Poincare Trader!");

	TraderConfig trader_config = { 0 };
	read_json_config(trader_config);

	if (log_handle == NULL) {
		log_handle = fopen(trader_config.TRADER_LOG, "w");
	}

	CThostFtdcTraderApi* TraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi("tmp/trader");
	trader_handler = new Trader_Handler(TraderApi, &trader_config);

	while(trader_handler->m_init_flag == false) {
		PRINT_DEBUG("Main thread is waiting...");
		sleep(1);
	}

	trader_handler->init_strategy();
	TraderApi->Join();

	free_config(trader_config);
	flush_log();
	fclose(log_handle);
	return 0;
}
