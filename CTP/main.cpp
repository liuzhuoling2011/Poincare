#include <fstream>
#include <sstream>
#include "utils/log.h"		 
#include "core/Quote_Handler.h"
#include "utils/json11.hpp"
#include "utils/utils.h"

using namespace json11;

#define CONFIG_FILE    "./config.json"

void read_config_file(std::string& content)
{
	std::ifstream fin(CONFIG_FILE);
	std::stringstream buffer;
	buffer << fin.rdbuf();
	content = buffer.str();
	fin.close();
}

bool read_json_config(TraderConfig& trader_config) {
	std::string json_config;
	read_config_file(json_config);
	std::string err_msg;
	Json l_json = json11::Json::parse(json_config, err_msg, JsonParse::COMMENTS);
	if (err_msg.length() > 0) {
		PRINT_ERROR("Json parse fail, please check your setting!");
		return false;
	}

	strlcpy(trader_config.QUOTE_FRONT, l_json["QUOTE_FRONT"].string_value().c_str(), 64);
	strlcpy(trader_config.QBROKER_ID, l_json["QBROKER_ID"].string_value().c_str(), 8);
	strlcpy(trader_config.QUSER_ID, l_json["QUSER_ID"].string_value().c_str(), 64);
	strlcpy(trader_config.QPASSWORD, l_json["QPASSWORD"].string_value().c_str(), 64);
	strlcpy(trader_config.TRADER_FRONT, l_json["TRADER_FRONT"].string_value().c_str(), 64);
	strlcpy(trader_config.TBROKER_ID, l_json["TBROKER_ID"].string_value().c_str(), 8);
	strlcpy(trader_config.TUSER_ID, l_json["TUSER_ID"].string_value().c_str(), 64);
	strlcpy(trader_config.TPASSWORD, l_json["TPASSWORD"].string_value().c_str(), 64);
	int instr_count = 0;
	for (auto &l_instr : l_json["INSTRUMENTS"].array_items()) {
		trader_config.INSTRUMENTS[instr_count] = (char *)malloc(64 * sizeof(char));
		strlcpy(trader_config.INSTRUMENTS[instr_count], l_instr.string_value().c_str(), 64);
		instr_count++;
	}
	trader_config.INSTRUMENT_COUNT = instr_count;
}

void free_config(TraderConfig& trader_config) {
	for(int i = 0; i < trader_config.INSTRUMENT_COUNT; i++) {
		free(trader_config.INSTRUMENTS[i]);
	}
}

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
