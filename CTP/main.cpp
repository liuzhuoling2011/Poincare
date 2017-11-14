#include "utils/log.h"		 
#include "core/Quote_Handler.h"
#include "core/Trader_Handler.h"
#include "utils/utils.h"


int main(int argc, char **argv)
{
	PRINT_INFO("Welcome to CTP demo!");
	TraderConfig trader_config = { 0 };
	read_json_config(trader_config);

	CThostFtdcTraderApi* TraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi("tmp/tr");
	Trader_Handler* trader_handler = new Trader_Handler(TraderApi, &trader_config);
	TraderApi->Init();
	// ������Init����֮�����
	TraderApi->SubscribePublicTopic(THOST_TERT_QUICK);				// ע�ṫ����
	TraderApi->SubscribePrivateTopic(THOST_TERT_QUICK);				// ע��˽����

	CThostFtdcMdApi *MdUserApi = CThostFtdcMdApi::CreateFtdcMdApi("tmp/md", false);
	Quote_Handler quote_handler(MdUserApi, trader_handler, &trader_config);
	MdUserApi->Init();

	TraderApi->Join();
	TraderApi->Release();
	MdUserApi->Join();
	MdUserApi->Release();

	free_config(trader_config);
	return 0;
}
