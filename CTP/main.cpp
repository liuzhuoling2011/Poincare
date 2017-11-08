#include <fstream>
#include <sstream>
#include "utils/log.h"		 
#include "core/CTP_Handler.h"
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

	strlcpy(trader_config.MD_FRONT, l_json["MD_FRONT"].string_value().c_str(), 64);
	strlcpy(trader_config.BROKER_ID, l_json["BROKER_ID"].string_value().c_str(), 8);
	strlcpy(trader_config.USER_ID, l_json["USER_ID"].string_value().c_str(), 64);
	strlcpy(trader_config.PASSWORD, l_json["PASSWORD"].string_value().c_str(), 64);
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
	CThostFtdcMdApi *api = CThostFtdcMdApi::CreateFtdcMdApi("tmp/md", false);
	TraderConfig trader_config = { 0 };
	//trader_config.INSTRUMENTS = (char*)malloc(64 * sizeof(64));
	read_json_config(trader_config);
	CTP_Handler ctp_handler(api, &trader_config);
	ctp_handler.start_trading();
	free_config(trader_config);
	return 0;
}
