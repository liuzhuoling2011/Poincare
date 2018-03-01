#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include "utils/log.h"
#include "utils/redis_handler.h"
#include "utils/json11.hpp"
#include "Trader_Handler.h"
#include <ctime>
#include <iostream>

using namespace std;
using namespace json11;

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#define sleep Sleep
#endif

#define BROKER_FEE 0.0002
#define STAMP_TAX 0.001
#define ACC_TRANSFER_FEE 0.00002
#define SERIAL_NO_MULTI 10000000000

static int hand_index = 100; //手工下单

//extern char g_strategy_path[256];

static CThostFtdcInputOrderActionField g_order_action_t = { 0 };

static contract_t empty_contract_t = { 0 };

//shannon 接口用于初始化的结构，盘前查好，更新初始化信息，如资金仓位，还有发单函数等。
static st_config_t g_config_t = { 0 };

//shannon 结构用于将回报传给策略的结构。本代码将ctp回报包装成这个结构打包给strategy
static st_response_t g_resp_t = { 0 };

//shannon 接口用于传递数据的结构
static st_data_t g_data_t = { 0 };

static int  g_sig_count = 0;
static char ERROR_MSG[512];
static char BUFFER_MSG[2048];

//订单状态
const static char STATUS[][64] = { 
    "SUCCEED",  //成交
    "ENTRUSTED", //挂单成功
    "PARTED",  //部分成交
    "CANCELED",  //已撤单
    "REJECTED",  //被拒
    "CANCEL_REJECTED", //撤单被拒
    "INTRREJECTED",  //
    "UNDEFINED_STATUS" //
};

//CTP返回的仓位信息的Array
typedef MyArray<CThostFtdcInvestorPositionDetailField> ContractPositionArray;

struct ContractPosition
{
	ContractPositionArray long_pos;
	ContractPositionArray short_pos;
};


//key: 合约代码字符串：value: contract_t 结构。
static MyHash<contract_t> g_contract_config_hash;

//key: 合约代码字符串：value:   ContractPosition结构。
static MyHash<ContractPosition*> g_contract_pos_hash;

MyHash<contract_t>::Iterator g_iter;

typedef void(*feed_quote_func)(char *data);

time_t rawtime;
tm timeinfo = { 0 };
timeval current_time = { 0 };

Json::object g_output_info;

//TraderInfo 结构用于保存 登陆ctp后的信息如FrontId,sessionID等。这些在后面发单，或者
//查询其他都会用到。
//此外：完善了 g_config_t 的 交易日 和 日夜盘。
void update_trader_info(TraderInfo& info, CThostFtdcRspUserLoginField *pRspUserLogin) {
	// 保存会话参数
	info.FrontID = pRspUserLogin->FrontID;
	info.SessionID = pRspUserLogin->SessionID;
	int temp_front = pRspUserLogin->FrontID < 0 ? -1 * pRspUserLogin->FrontID : pRspUserLogin->FrontID;
	int SessionID = pRspUserLogin->SessionID < 0 ? -1 * pRspUserLogin->SessionID : pRspUserLogin->SessionID;
	info.SELF_CODE = (temp_front + SessionID) % 99;

	LOG_LN("FrontID: %d SessionID: %d SpecialCode: %d", info.FrontID, info.SessionID, info.SELF_CODE);
	int iNextOrderRef = atoi(pRspUserLogin->MaxOrderRef);
	info.MaxOrderRef = iNextOrderRef++;

	strlcpy(info.TradingDay, pRspUserLogin->TradingDay, TRADING_DAY_LEN);
	strlcpy(info.LoginTime, pRspUserLogin->LoginTime, TRADING_DAY_LEN);
	g_config_t.trading_date = atoi(info.TradingDay);

	int hour = 0;
	for (int i = 0; i < 6; i++) {
		if (info.LoginTime[i] != ':') {
			hour = 10 * hour + info.LoginTime[i] - '0';
		} else
			break;
	}
	time(&rawtime);
	tm * tmp_time = localtime(&rawtime);
	timeinfo = *tmp_time;
	timeinfo.tm_min = 0;
	timeinfo.tm_sec = 0;
	if (hour >= 8 && hour < 19) {
        //填写日夜盘
		g_config_t.day_night = DAY;
		timeinfo.tm_hour = 9;
	}
	else {
		g_config_t.day_night = NIGHT;
		timeinfo.tm_hour = 21;
	}
    //开盘时间
	info.START_TIME_STAMP = mktime(&timeinfo);
}


// m_trader_config  是 读取 config.json中的信息。
Trader_Handler::Trader_Handler(CThostFtdcTraderApi* TraderApi, TraderConfig* trader_config)
{
	m_trader_config = trader_config;
	m_trader_api = TraderApi;
	m_trader_api->RegisterSpi(this);			// 注册事件类
	m_trader_api->RegisterFront(m_trader_config->ACCOUNT.FRONT);		// connect
	m_orders = new MyHash<CThostFtdcInputOrderField>(2000);

	m_trader_api->Init();
	// 必须在Init函数之后调用
	m_trader_api->SubscribePublicTopic(THOST_TERT_QUICK);				// 注册公有流
	m_trader_api->SubscribePrivateTopic(THOST_TERT_QUICK);				// 注册私有流
}

Trader_Handler::~Trader_Handler()
{
	if (m_init_flag) {
		//my_destroy();
	} else
		flush_log();
	delete m_orders;
	m_trader_api->Release();
}

// 完善了 g_config_t 的 策略id,ev路径，策略名字，输出路径。（都是从config读取）
// 然后登陆用户
void Trader_Handler::OnFrontConnected()
{
	memset(&g_config_t, 0, sizeof(st_config_t));
	
	CThostFtdcReqUserLoginField req = { 0 };
	strcpy(req.BrokerID, m_trader_config->ACCOUNT.BROKER_ID);
	strcpy(req.UserID, m_trader_config->ACCOUNT.USER_ID);
	strcpy(req.Password, m_trader_config->ACCOUNT.PASSWORD);
	m_trader_api->ReqUserLogin(&req, ++m_request_id);
}



//一旦登陆成功后，就调用update_trader_info  将登陆的信息存放TraderInfo m_trader_info
void Trader_Handler::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL && pRspInfo->ErrorID == 0) {
		update_trader_info(m_trader_info, pRspUserLogin);
		fprintf(stderr, "TradingDay: %d DayNight: %s Time: %s\n",
			g_config_t.trading_date, g_config_t.day_night == 0 ? "Day":"Night", m_trader_info.LoginTime);

		//投资者结算结果确认
		ReqSettlementInfo();
	} else {
		LOG_LN("Login Failed!");
		exit(-1);
	}
}


//从 json中取得基本信息 查询 投资者结算
void Trader_Handler::ReqSettlementInfo()
{
	CThostFtdcSettlementInfoConfirmField req = { 0 };
	strcpy(req.BrokerID, m_trader_config->ACCOUNT.BROKER_ID);
	strcpy(req.InvestorID, m_trader_config->ACCOUNT.USER_ID);
	int ret = m_trader_api->ReqSettlementInfoConfirm(&req, ++m_request_id);
	if (ret != 0)
		LOG_LN("Send settlement request fail!");
}


//查询资金，账户
void Trader_Handler::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	LOG_LN("Comform settlement!");
	//请求查询资金账户
	ReqTradingAccount();
}


//查询资金账户
void Trader_Handler::ReqTradingAccount()
{
	CThostFtdcQryTradingAccountField requ = { 0 };
	strcpy(requ.BrokerID, m_trader_config->ACCOUNT.BROKER_ID);
	strcpy(requ.InvestorID, m_trader_config->ACCOUNT.USER_ID);
	strcpy(requ.CurrencyID, "CNY");
	int iResult = m_trader_api->ReqQryTradingAccount(&requ, ++m_request_id);
	LOG_LN("send query account: %s %s", requ.InvestorID, iResult == 0 ? "success" : "fail");
}

// 将资金账户的币中，费率 ，放进g_config_t  ，目前由于资金管理还暂时没有用，所以先随便写死
void Trader_Handler::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pTradingAccount == NULL) return;
	fprintf(stderr, "Account: %s %f %f %f %f %f\n", pTradingAccount->AccountID, pTradingAccount->PositionProfit, pTradingAccount->CloseProfit, pTradingAccount->Commission, pTradingAccount->Available, pTradingAccount->ExchangeMargin);
	Json::object account_info;
	account_info["AccountID"] = pTradingAccount->AccountID;
	account_info["PositionProfit"] = pTradingAccount->PositionProfit;
	account_info["CloseProfit"] = pTradingAccount->CloseProfit;
	account_info["PreBalance"] = pTradingAccount->PreBalance;
	account_info["Withdraw"] = pTradingAccount->Withdraw;
	account_info["Deposit"] = pTradingAccount->Deposit;
	account_info["Commission"] = pTradingAccount->Commission;
	account_info["Available"] = pTradingAccount->Available;
	account_info["ExchangeMargin"] = pTradingAccount->ExchangeMargin;

	g_output_info["account"] = account_info;
	
	strlcpy(g_config_t.accounts[0].account, pTradingAccount->AccountID, ACCOUNT_LEN);
	g_config_t.accounts[0].cash_asset = 1000000;// pTradingAccount->Available; //to change
	g_config_t.accounts[0].cash_available = 1000000;// pTradingAccount->Available;
	g_config_t.accounts[0].currency = CNY;
	g_config_t.accounts[0].exch_rate = 1.0;

	//请求查询投资者持仓
	ReqQryInvestorPositionDetail();
}
//从 m_trader_config 取得配置，发送仓位请求
void Trader_Handler::ReqQryInvestorPositionDetail()
{
	fprintf(stderr, "\nStart query position...\n");
	CThostFtdcQryInvestorPositionDetailField req_pos = { 0 };
	strcpy(req_pos.BrokerID, m_trader_config->ACCOUNT.BROKER_ID);
	strcpy(req_pos.InvestorID, m_trader_config->ACCOUNT.USER_ID);
	int ret = 0;
	do {
		ret = m_trader_api->ReqQryInvestorPositionDetail(&req_pos, ++m_request_id);
		sleep(1);
	} while (ret != 0);
}

void Trader_Handler::ReqQryTrade()
{
	fprintf(stderr, "\nStart query trade list...\n");
	CThostFtdcQryTradeField req = { 0 };
	strcpy(req.InvestorID, m_trader_config->ACCOUNT.USER_ID);
	int ret = 0;
	do {
		ret = m_trader_api->ReqQryTrade(&req, ++m_request_id);
		sleep(1);
	} while (ret != 0);
}

void Trader_Handler::ReqQryOrder()
{
	fprintf(stderr, "\nStart query order list...\n");
	CThostFtdcQryOrderField req = { 0 };
	strcpy(req.InvestorID, m_trader_config->ACCOUNT.USER_ID);
	m_trader_api->ReqQryOrder(&req, ++m_request_id);
}

void Trader_Handler::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField * pInvestorPositionDetail, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pInvestorPositionDetail) {
		// 对于所有合约，不保存已平仓的，只保存未平仓的
        // 如果 config.json 中 的ONLY_RECEIVE_SUBSCRIBE_INSTRUMENTS_POSITION ＝ ture
        // 那么会 只查找 订阅行情的仓位。
        // false 就会 增加订阅行情。把账户有仓位的行情都订阅。
		if (pInvestorPositionDetail->Volume > 0) {
            //初始化  g_contract_pos_hash  用于后面的仓位管理
			ContractPosition* &contract_position = g_contract_pos_hash[pInvestorPositionDetail->InstrumentID];
			if (contract_position == NULL) contract_position = new ContractPosition();

            // 如果这条回报的方向是buy,就 更新g_contract_pos_hash 中对应的long_pos 
            // 如果这条回报的方向是sell,就 更新g_contract_pos_hash 中对应的short_pos
			if (pInvestorPositionDetail->Direction == TRADER_BUY)
				contract_position->long_pos.push_back(*pInvestorPositionDetail);
			else if (pInvestorPositionDetail->Direction == TRADER_SELL)
				contract_position->short_pos.push_back(*pInvestorPositionDetail);

            //前面的代码已经维持了  g_contract_pos_hash 的结构，并且把每个合约的long short都装入其中了。

			bool find_instId = false;
			for (int i = 0; i < m_trader_config->INSTRUMENT_COUNT; i++) {
				if (my_strcmp(m_trader_config->INSTRUMENTS[i], pInvestorPositionDetail->InstrumentID) == 0) {	//合约已存在，已订阅过行情
					find_instId = true;
					break;
				}
			}
			if (find_instId == false) {
                //增加订阅的行情。
				strlcpy(m_trader_config->INSTRUMENTS[m_trader_config->INSTRUMENT_COUNT++], pInvestorPositionDetail->InstrumentID, SYMBOL_LEN);
			}
		}
	}

	if (bIsLast) {
            //前面的代码已经维持了  g_contract_pos_hash 的结构，
            //并且把每个合约的long short都装入其中了。
            //目的：遍历g_contract_pos_hash 来完善  g_contract_config_hash.
            //所以  g_contract_config_hash 才是最终的。 
		for (auto iter = g_contract_pos_hash.begin(); iter != g_contract_pos_hash.end(); iter++) {

            //初始化，这个 g_contract_config_hash
			contract_t& contract_config = g_contract_config_hash[iter->first];

            //取得iter 这个合约的 long 和 short pos
			ContractPositionArray &contracts_long = iter->second->long_pos;
			ContractPositionArray &contracts_short = iter->second->short_pos;

			int long_size = 0, short_size = 0, yes_long_size = 0, yes_short_size = 0;
			double long_price = 0, short_price = 0, yes_long_price = 0, yes_short_price = 0;

            // 更新昨仓，今仓
			for (int i = 0; i < iter->second->long_pos.size(); i++) {
                //如果是昨仓
				if (atoi(contracts_long[i].OpenDate) != g_config_t.trading_date) {
					yes_long_price += contracts_long[i].OpenPrice * contracts_long[i].Volume;
					yes_long_size += contracts_long[i].Volume;
				}
                //如果是今仓
				long_price += contracts_long[i].OpenPrice * contracts_long[i].Volume;
				long_size += contracts_long[i].Volume;
			}
			if (long_size == 0) {
				contract_config.today_pos.long_price = 0;
				contract_config.today_pos.long_volume = 0;
			}
			else {
                //今天仓位的均价和仓位。
				contract_config.today_pos.long_price = long_price / long_size;
				contract_config.today_pos.long_volume = long_size;
			}

			if (yes_long_size == 0) {
				contract_config.yesterday_pos.long_price = 0;
				contract_config.yesterday_pos.long_volume = 0;
			}
			else {
                //昨天仓位的均价和仓位。
				contract_config.yesterday_pos.long_price = yes_long_price / yes_long_size;
				contract_config.yesterday_pos.long_volume = yes_long_size;
			}

            //short 同理。
			for (int i = 0; i < contracts_short.size(); i++) {
				if (atoi(contracts_short[i].OpenDate) != g_config_t.trading_date) {
					yes_short_price += contracts_short[i].OpenPrice * contracts_short[i].Volume;
					yes_short_size += contracts_short[i].Volume;
				}
				short_price += contracts_short[i].OpenPrice * contracts_short[i].Volume;
				short_size += contracts_short[i].Volume;
			}
            //short 同理。
			if (short_size == 0) {
				contract_config.today_pos.short_price = 0;
				contract_config.today_pos.short_volume = 0;
			}
			else {
				contract_config.today_pos.short_price = short_price / short_size;
				contract_config.today_pos.short_volume = short_size;
			}

			if (yes_short_size == 0) {
				contract_config.yesterday_pos.short_price = 0;
				contract_config.yesterday_pos.short_volume = 0;
			}
			else {
				contract_config.yesterday_pos.short_price = yes_short_price / yes_short_size;
				contract_config.yesterday_pos.short_volume = yes_short_size;
			}
		}
		//如果订阅的合约，在仓位查询中没有返回回报，那么就插入空的	 empty_contract_t
		for (int i = 0; i < m_trader_config->INSTRUMENT_COUNT; i++) {
			char* l_symbol = m_trader_config->INSTRUMENTS[i];
			if (!g_contract_config_hash.exist(l_symbol)) {
				g_contract_config_hash.insert(l_symbol, empty_contract_t); 
			}
		}

		if(!g_contract_config_hash.empty()) {
			Json::array contracts_info = Json::array();
			for (auto iter = g_contract_config_hash.begin(); iter != g_contract_config_hash.end(); iter++) {
				fprintf(stderr, "%s %d@%f %d@%f %d@%f %d@%f\n", iter->first, iter->second.today_pos.long_volume, iter->second.today_pos.long_price,
					iter->second.today_pos.short_volume, iter->second.today_pos.short_price, 
					iter->second.yesterday_pos.long_volume, iter->second.yesterday_pos.long_price,
					iter->second.yesterday_pos.short_volume, iter->second.yesterday_pos.short_price);
				Json::object contract_info;
				contract_info["symbol"] = iter->first;
				contract_info["tpos_long_val"] = iter->second.today_pos.long_volume;
				contract_info["tpos_long_price"] = iter->second.today_pos.long_price;
				contract_info["tpos_short_val"] = iter->second.today_pos.short_volume;
				contract_info["tpos_short_price"] = iter->second.today_pos.short_price;
				contract_info["ypos_long_val"] = iter->second.yesterday_pos.long_volume;
				contract_info["ypos_long_price"] = iter->second.yesterday_pos.long_price;
				contract_info["ypos_short_val"] = iter->second.yesterday_pos.short_volume;
				contract_info["ypos_short_price"] = iter->second.yesterday_pos.short_price;
				contracts_info.push_back(contract_info);
			}
			g_output_info["contracts"] = contracts_info;
		}
        
		ReqQryOrder();
	}
}

Json::array tradelist_info = Json::array();
void end_process() {
	if (!tradelist_info.empty()) {
		g_output_info["tradelist"] = tradelist_info;
	}
	Json output_info = g_output_info;
	std::string temp_result = output_info.dump();
	printf("%s\n", temp_result.c_str());
	exit(0);
}

void Trader_Handler::OnRspQryTrade(CThostFtdcTradeField * pTrade, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pTrade == NULL) 
		end_process();

	fprintf(stderr, "%s %s %s %d %d %d@%f %s %s\n", pTrade->InstrumentID, pTrade->TradeDate, pTrade->TradeTime,
		pTrade->Direction - '0', pTrade->OffsetFlag - '0', pTrade->Volume, pTrade->Price, pTrade->OrderSysID, pTrade->TradeID);

	Json::object trade_log;
	trade_log["InstrumentID"] = pTrade->InstrumentID;
	trade_log["Price"] = pTrade->Price;
	trade_log["Volume"] = pTrade->Volume;
	trade_log["TradeID"] = pTrade->TradeID;
	trade_log["OrderSysID"] = pTrade->OrderSysID;
	trade_log["Direction"] = pTrade->Direction;
	trade_log["OffsetFlag"] = pTrade->OffsetFlag;
	trade_log["TradeDate"] = pTrade->TradeDate;
	trade_log["TradeTime"] = pTrade->TradeTime;
	tradelist_info.push_back(trade_log);

	if (!bIsLast)
		return;

	end_process();
}

Json::array orderlist_info = Json::array();
void Trader_Handler::OnRspQryOrder(CThostFtdcOrderField * pOrder, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pOrder == NULL) {
		ReqQryTrade();
		return;
	}

	fprintf(stderr, "%s %s %s %d %d %d@%f %s\n", pOrder->InstrumentID, pOrder->InsertDate, pOrder->InsertTime,
		pOrder->Direction - '0', pOrder->CombOffsetFlag[0] - '0', pOrder->VolumeTotalOriginal, pOrder->LimitPrice, pOrder->OrderSysID);

	Json::object order_log;
	order_log["InstrumentID"] = pOrder->InstrumentID;
	order_log["Price"] = pOrder->LimitPrice;
	order_log["Volume"] = pOrder->VolumeTotalOriginal;
	order_log["OrderSysID"] = pOrder->OrderSysID;
	order_log["Direction"] = pOrder->Direction;
	order_log["OrderStatus"] = pOrder->OrderStatus;
	order_log["OffsetFlag"] = pOrder->CombOffsetFlag[0];
	order_log["InsertDate"] = pOrder->InsertDate;
	order_log["InsertTime"] = pOrder->InsertTime;
	orderlist_info.push_back(order_log);

	if (!bIsLast)
		return;

	if (!orderlist_info.empty()) {
		g_output_info["orderlist"] = orderlist_info;
	}

	ReqQryTrade();
}

// 发单失败，被拒，挂单失败才会被调用。
void Trader_Handler::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(pInputOrder) {
		if (!m_orders->exist(pInputOrder->OrderRef)) return;
		
		sprintf(BUFFER_MSG, "\n--->>> OnRspOrderInsert\n经纪公司代码 %s\n投资者代码 %s\n合约代码 %s\n报单引用 %s\n用户代码 %s\n"
			"报单价格条件 %c\n买卖方向 %c\n组合开平标志 %s\n组合投机套保标志 %s\n"
			"价格 %f\n数量 %d\n有效期类型 %c\n成交量类型 %c\n"
			"最小成交量 %d\n触发条件 %c\n请求编号 %d\n交易所代码 %s",
			pInputOrder->BrokerID, pInputOrder->InvestorID, pInputOrder->InstrumentID, pInputOrder->OrderRef, pInputOrder->UserID,
			pInputOrder->OrderPriceType, pInputOrder->Direction, pInputOrder->CombOffsetFlag, pInputOrder->CombHedgeFlag,
			pInputOrder->LimitPrice, pInputOrder->VolumeTotalOriginal, pInputOrder->TimeCondition, pInputOrder->VolumeCondition,
			pInputOrder->MinVolume, pInputOrder->ContingentCondition, pInputOrder->RequestID, pInputOrder->ExchangeID
		);
		LOG_LN("%s", BUFFER_MSG);
	}
	flush_log();
}


//撤单请求回报
void Trader_Handler::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(pInputOrderAction) {
        //如果不是自己进程的Front id 和 session id 跳过。
		if (!is_my_order(pInputOrderAction->FrontID, pInputOrderAction->SessionID)) return;

		sprintf(BUFFER_MSG, "\n--->>> OnRspOrderAction\n经纪公司代码 %s\n投资者代码 %s\n合约代码 %s\n报单引用 %s\n"
			"价格 %f\n数量变化 %d\n操作标志 %c\n交易所代码 %s\n"
			"请求编号 %d\n前置编号 %d\n会话编号 %d",
			g_order_action_t.BrokerID, g_order_action_t.InvestorID, g_order_action_t.InstrumentID, g_order_action_t.OrderRef,
			pInputOrderAction->LimitPrice, pInputOrderAction->VolumeChange, pInputOrderAction->ActionFlag, pInputOrderAction->ExchangeID,
			g_order_action_t.RequestID, g_order_action_t.FrontID, g_order_action_t.SessionID
		);

		LOG_LN("%s", BUFFER_MSG);
	}
	if (pRspInfo != NULL && pRspInfo->ErrorID != 0) {
		code_convert(pRspInfo->ErrorMsg, strlen(pRspInfo->ErrorMsg), ERROR_MSG, 4096);
		LOG_LN("ErrorID = %d, ErrorMsg = %s", pRspInfo->ErrorID, ERROR_MSG);
	}
	flush_log();
}

void Trader_Handler::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	if (pOrder) {
		if (!is_my_order(pOrder->FrontID, pOrder->SessionID)) return;

		sprintf(BUFFER_MSG, "\n--->>> OnRtnOrder\n经纪公司代码 %s\n投资者代码 %s\n合约代码 %s\n报单引用 %s\n用户代码 %s\n"
			"报单价格条件 %c\n买卖方向 %c\n组合开平标志 %s\n组合投机套保标志 %s\n"
			"价格 %f\n数量 %d\n有效期类型 %c\n成交量类型 %c\n"
			"最小成交量 %d\n触发条件 %c\n请求编号 %d\n交易所代码 %s\n"
			"本地报单编号 %s\n报单提交状态 %c\n报单编号 %s\n报单状态 %c\n"
			"今成交数量 %d\n剩余数量 %d\n报单日期 %s\n报单时间 %s\n"
			"最后修改时间 %s\n撤销时间 %s\n前置编号 %d\n会话编号 %d",
			pOrder->BrokerID, pOrder->InvestorID, pOrder->InstrumentID, pOrder->OrderRef, pOrder->UserID,
			pOrder->OrderPriceType, pOrder->Direction, pOrder->CombOffsetFlag, pOrder->CombHedgeFlag,
			pOrder->LimitPrice, pOrder->VolumeTotalOriginal, pOrder->TimeCondition, pOrder->VolumeCondition,
			pOrder->MinVolume, pOrder->ContingentCondition, pOrder->RequestID, pOrder->ExchangeID,
			pOrder->OrderLocalID, pOrder->OrderSubmitStatus, pOrder->OrderSysID, pOrder->OrderStatus, 
			pOrder->VolumeTraded, pOrder->VolumeTotal, pOrder->InsertDate, pOrder->InsertTime,
			pOrder->UpdateTime, pOrder->CancelTime, pOrder->FrontID, pOrder->SessionID
		);

		LOG_LN("%s", BUFFER_MSG);
	}
	flush_log();
}

void Trader_Handler::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	if (pTrade) {
		if (!m_orders->exist(pTrade->OrderRef)) return;
		
		sprintf(BUFFER_MSG, "\n--->>> OnRtnOrder\n经纪公司代码 %s\n投资者代码 %s\n合约代码 %s\n报单引用 %s\n用户代码 %s\n"
			"买卖方向 %c\n开平标志 %c\n机套保标志 %c\n价格 %f\n数量 %d\n"
			"成交编号 %s\n交易所代码 %s\n本地报单编号 %s\n报单编号 %s\n成交日期 %s\n成交时间 %s",
			pTrade->BrokerID, pTrade->InvestorID, pTrade->InstrumentID, pTrade->OrderRef, pTrade->UserID,
			pTrade->Direction, pTrade->OffsetFlag, pTrade->HedgeFlag, pTrade->Price, pTrade->Volume, 
			pTrade->TradeID, pTrade->ExchangeID, pTrade->OrderLocalID, pTrade->OrderSysID, pTrade->TradeDate, pTrade->TradeTime
		);

		LOG_LN("%s", BUFFER_MSG);
	}
	flush_log();
}

bool Trader_Handler::is_my_order(int front_id, int session_id)
{
	return (front_id == m_trader_info.FrontID && session_id == m_trader_info.SessionID);
}

void Trader_Handler::OnFrontDisconnected(int nReason)
{
	LOG_LN("Trader front disconnected, code: %d", nReason);
}

void Trader_Handler::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL && pRspInfo->ErrorID != 0) {
		code_convert(pRspInfo->ErrorMsg, strlen(pRspInfo->ErrorMsg), ERROR_MSG, 4096);
		LOG_LN("ErrorID = %d, ErrorMsg = %s", pRspInfo->ErrorID, ERROR_MSG);
	}
	flush_log();
}

void Trader_Handler::OnErrRtnOrderInsert(CThostFtdcInputOrderField * pInputOrder, CThostFtdcRspInfoField * pRspInfo)
{
	if (pRspInfo != NULL && pRspInfo->ErrorID != 0) {
		code_convert(pRspInfo->ErrorMsg, strlen(pRspInfo->ErrorMsg), ERROR_MSG, 4096);
		LOG_LN("ErrorID = %d, ErrorMsg = %s", pRspInfo->ErrorID, ERROR_MSG);
	}
	flush_log();
}

void Trader_Handler::OnErrRtnOrderAction(CThostFtdcOrderActionField * pOrderAction, CThostFtdcRspInfoField * pRspInfo)
{
	if (pRspInfo != NULL && pRspInfo->ErrorID != 0) {
		code_convert(pRspInfo->ErrorMsg, strlen(pRspInfo->ErrorMsg), ERROR_MSG, 4096);
		LOG_LN("ErrorID = %d, ErrorMsg = %s", pRspInfo->ErrorID, ERROR_MSG);
	}
	flush_log();
}

