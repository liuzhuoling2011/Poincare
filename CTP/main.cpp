#include "utils/log.h"		 
#include "core/Quote_Handler.h"
#include "utils/utils.h"


int main(int argc, char **argv)
{
	PRINT_INFO("Welcome to CTP demo!");
	TraderConfig trader_config = { 0 };
	read_json_config(trader_config);

	CThostFtdcMdApi *MdUserApi = CThostFtdcMdApi::CreateFtdcMdApi("tmp/md", false);
	Quote_Handler quote_handler(MdUserApi, &trader_config);
	MdUserApi->Init();

	//CThostFtdcTraderApi* TraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi("tmp/tr");
	//Trader_Handler* trader_handler = new Trader_Handler(TraderApi, &trader_config);
	//TraderApi->Init();
	//// 必须在Init函数之后调用
	//TraderApi->SubscribePublicTopic(THOST_TERT_QUICK);				// 注册公有流
	//TraderApi->SubscribePrivateTopic(THOST_TERT_QUICK);				// 注册私有流

	MdUserApi->Join();
	MdUserApi->Release();
	/*TraderApi->Join();
	TraderApi->Release();*/

	free_config(trader_config);
	return 0;
}
