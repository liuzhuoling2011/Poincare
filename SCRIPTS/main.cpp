#include <signal.h>
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
char g_core_dump_msg[8192];

void recv_signal(int sig) {
	dump_backtrace(g_core_dump_msg);
	PRINT_ERROR("%s", g_core_dump_msg);
	LOG_LN("%s", g_core_dump_msg);
	if (trader_handler != NULL) delete trader_handler;
	exit(1);
}

int main(int argc, char **argv)
{
	PRINT_INFO("Welcome to Poincare Assist!");
	signal(SIGSEGV, recv_signal);
	signal(SIGABRT, recv_signal);
	//signal(SIGINT, recv_signal);

	TraderConfig trader_config = { 0 };
	read_json_config(trader_config);

	if (log_handle == NULL) {
		log_handle = fopen(trader_config.TRADER_LOG, "w");
	}

	CThostFtdcTraderApi* TraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi("tmp/trader");
	trader_handler = new Trader_Handler(TraderApi, &trader_config);

	TraderApi->Join();

	free_config(trader_config);
	flush_log();
	fclose(log_handle);
	return 0;
}
