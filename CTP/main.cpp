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

char local_time[1024];

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


CThostFtdcMdApi *MdUserApi;	// after strategy init finished, call MdUserApi->Init();
extern Trader_Handler *g_trader_handler;

void recv_signal(int sig)
{
	DumpBacktrace();
	flush_log();
	fclose(log_handle);
	delete g_trader_handler;
	exit(0);
}

int main(int argc, char **argv)
{
	PRINT_INFO("Welcome to CTP demo!");
	//signal(SIGSEGV, recv_signal);
	//signal(SIGABRT, recv_signal);
	//signal(SIGINT, recv_signal);

	TraderConfig trader_config = { 0 };
	read_json_config(trader_config);

	if (log_handle == NULL) {
		log_handle = fopen(trader_config.TRADER_LOG, "w");
	}

	CThostFtdcTraderApi* TraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi("tmp/tr");
	Trader_Handler* trader_handler = new Trader_Handler(TraderApi, &trader_config);
	TraderApi->Init();
	// 必须在Init函数之后调用
	TraderApi->SubscribePublicTopic(THOST_TERT_QUICK);				// 注册公有流
	TraderApi->SubscribePrivateTopic(THOST_TERT_QUICK);				// 注册私有流

	MdUserApi = CThostFtdcMdApi::CreateFtdcMdApi("tmp/md", false);
	Quote_Handler quote_handler(MdUserApi, trader_handler, &trader_config);
	
	TraderApi->Join();
	TraderApi->Release();
	MdUserApi->Join();
	MdUserApi->Release();

	free_config(trader_config);
	flush_log();
	fclose(log_handle);
	return 0;
}
