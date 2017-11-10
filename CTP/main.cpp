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
	//// ������Init����֮�����
	//TraderApi->SubscribePublicTopic(THOST_TERT_QUICK);				// ע�ṫ����
	//TraderApi->SubscribePrivateTopic(THOST_TERT_QUICK);				// ע��˽����

	MdUserApi->Join();
	MdUserApi->Release();
	/*TraderApi->Join();
	TraderApi->Release();*/

	free_config(trader_config);
	return 0;
}
