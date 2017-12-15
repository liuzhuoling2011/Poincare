#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <execinfo.h>
#include <sys/wait.h>
#include "utils/log.h"		 
#include "core/Quote_Handler.h"
#include "core/Trader_Handler.h"
#include "utils/utils.h"

#define LOG_DIR './logs'
extern FILE* log_handle;

void DumpBacktrace() {
	pid_t dying_pid = getpid();
	pid_t child_pid = fork();
	if (child_pid < 0) {
		perror("fork() while collecting backtrace:");
	}
	else if (child_pid == 0) {
		char buf[1024];
		sprintf(buf, "gdb -p %d -batch -ex bt 2>/dev/null | "
			"sed '0,/<signal handler/d'", dying_pid);
		const char* argv[] = { "sh", "-c", buf, NULL };
		execve("/bin/sh", (char**)argv, NULL);
	}
	else {
		waitpid(child_pid, NULL, 0);
	}
}

Trader_Handler *trader_handler;

void recv_signal(int sig)
{
	DumpBacktrace();
	flush_log();
	fclose(log_handle);
	delete trader_handler;
	exit(0);
}

int main(int argc, char **argv)
{
	PRINT_INFO("Welcome to Poincare!");
	//signal(SIGSEGV, recv_signal);
	//signal(SIGABRT, recv_signal);
	//signal(SIGINT, recv_signal);

	TraderConfig trader_config = { 0 };
	read_json_config(trader_config);

	if (log_handle == NULL) {
		log_handle = fopen(trader_config.TRADER_LOG, "w");
	}

	CThostFtdcMdApi *QuoteApi = CThostFtdcMdApi::CreateFtdcMdApi("tmp/marketdata", false);
	Quote_Handler quote_handler(QuoteApi, &trader_config);

	CThostFtdcTraderApi* TraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi("tmp/trader");
	trader_handler = new Trader_Handler(TraderApi, QuoteApi, &trader_config);

	free_config(trader_config);
	flush_log();
	fclose(log_handle);
	return 0;
}
