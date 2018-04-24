#include <string.h>
#include "utils/log.h"
#include "utils/json11.hpp"
#include "Trader_Handler.h"

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#define sleep Sleep
#endif

using namespace json11;

static bool SIMPLE_LOCK = true;
static char TEMP_MSG[512] = "";
static int  RETURN_CODE = 0;
static int  ERROR_CODE = 0;
static char ERROR_MSG[512] = "";

#define GETLOCK() do{\
	while(SIMPLE_LOCK == true){sleep(1);} \
	SIMPLE_LOCK = true; \
}while (0)

#define LOCK() do{\
	SIMPLE_LOCK = true; \
}while (0)

#define UNLOCK() do{\
	SIMPLE_LOCK = false; \
}while (0)

static MyHash<bool> g_selected_contracts;
static MyHash<CThostFtdcInstrumentField> g_all_contracts_info(1024);

Json::object g_output_info;
bool complete_data_flag = false;

void update_trader_info(TraderInfo& info, CThostFtdcRspUserLoginField *pRspUserLogin) {
	info.FrontID = pRspUserLogin->FrontID;
	info.SessionID = pRspUserLogin->SessionID;
	int iNextOrderRef = atoi(pRspUserLogin->MaxOrderRef);
	info.MaxOrderRef = iNextOrderRef++;

	strlcpy(info.TradingDay, pRspUserLogin->TradingDay, TRADING_DAY_LEN);
	strlcpy(info.LoginTime, pRspUserLogin->LoginTime, TRADING_DAY_LEN);
}

Trader_Handler::Trader_Handler(CThostFtdcTraderApi* TraderApi, TraderConfig* trader_config)
{
	m_trader_config = trader_config;
	m_trader_api = TraderApi;
	m_trader_api->RegisterSpi(this);			// 注册事件类
	m_trader_api->RegisterFront(m_trader_config->FRONT);		

	m_trader_api->Init();
	// 必须在Init函数之后调用
	m_trader_api->SubscribePublicTopic(THOST_TERT_QUICK);				// 注册公有流
	m_trader_api->SubscribePrivateTopic(THOST_TERT_QUICK);				// 注册私有流

	// 在查询账号之后操作会解锁
	ReqManager();
}

Trader_Handler::~Trader_Handler()
{
	flush_log();
}

void Trader_Handler::OnFrontConnected()
{
	CThostFtdcReqUserLoginField req = { 0 };
	strcpy(req.BrokerID, m_trader_config->BROKER_ID);
	strcpy(req.UserID, m_trader_config->USER_ID);
	strcpy(req.Password, m_trader_config->PASSWORD);
	m_trader_api->ReqUserLogin(&req, ++m_request_id);
}

void Trader_Handler::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL && pRspInfo->ErrorID == 0) {
		update_trader_info(m_trader_info, pRspUserLogin);
		ReqSettlementInfo();
	} else {
		PRINT_LOG_INFO("Login Failed!");
		ERROR_CODE = pRspInfo->ErrorID;
		sprintf(ERROR_MSG, "Account %s Login Failed! %s", m_trader_config->USER_ID, pRspInfo->ErrorMsg);
		EndProcess();
	}
}

void Trader_Handler::ReqSettlementInfo()
{
	CThostFtdcSettlementInfoConfirmField req = { 0 };
	strcpy(req.BrokerID, m_trader_config->BROKER_ID);
	strcpy(req.InvestorID, m_trader_config->USER_ID);
	do {
		RETURN_CODE = m_trader_api->ReqSettlementInfoConfirm(&req, ++m_request_id);
		sleep(1);
	} while (RETURN_CODE != 0);
}

void Trader_Handler::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	ReqTradingAccount();
}

void Trader_Handler::ReqTradingAccount()
{
	CThostFtdcQryTradingAccountField requ = { 0 };
	strcpy(requ.BrokerID, m_trader_config->BROKER_ID);
	strcpy(requ.InvestorID, m_trader_config->USER_ID);
	strcpy(requ.CurrencyID, "CNY");
	do {
		RETURN_CODE = m_trader_api->ReqQryTradingAccount(&requ, ++m_request_id);
		if (RETURN_CODE == 0) PRINT_LOG_INFO("Send query account: %s success", requ.InvestorID);
		sleep(1);
	} while (RETURN_CODE != 0);
}

void Trader_Handler::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pTradingAccount == NULL) {
		PRINT_LOG_INFO("Query Account Failed!");
		ERROR_CODE = pRspInfo->ErrorID;
		sprintf(ERROR_MSG, "Query Account %s Failed! %s", m_trader_config->USER_ID, pRspInfo->ErrorMsg);
		EndProcess();
	}

	PRINT_INFO("Account: %s %f %f %f %f %f", pTradingAccount->AccountID, pTradingAccount->PositionProfit, pTradingAccount->CloseProfit, pTradingAccount->Commission, pTradingAccount->Available, pTradingAccount->ExchangeMargin);
	
	Json::object account_info;
	account_info["AccountID"] = pTradingAccount->AccountID;
	account_info["PositionProfit"] = pTradingAccount->PositionProfit;
	account_info["CloseProfit"] = pTradingAccount->CloseProfit;
	account_info["PreBalance"] = pTradingAccount->PreBalance;
	account_info["Balance"] = pTradingAccount->Balance;
	account_info["Withdraw"] = pTradingAccount->Withdraw;
	account_info["Deposit"] = pTradingAccount->Deposit;
	account_info["Commission"] = pTradingAccount->Commission;
	account_info["Available"] = pTradingAccount->Available;
	account_info["WithdrawQuota"] = pTradingAccount->WithdrawQuota;
	account_info["TradingDay"] = pTradingAccount->TradingDay;
	account_info["CurrMargin"] = pTradingAccount->CurrMargin;
	account_info["CurrencyID"] = pTradingAccount->CurrencyID;

	if(complete_data_flag) {
		account_info["BrokerID"] = pTradingAccount->BrokerID;
		account_info["ExchangeMargin"] = pTradingAccount->ExchangeMargin;
		account_info["PreMortgage"] = pTradingAccount->PreMortgage;
		account_info["PreCredit"] = pTradingAccount->PreCredit;
		account_info["PreDeposit"] = pTradingAccount->PreDeposit;
		account_info["PreMargin"] = pTradingAccount->PreMargin;
		account_info["InterestBase"] = pTradingAccount->InterestBase;
		account_info["Interest"] = pTradingAccount->Interest;
		account_info["FrozenMargin"] = pTradingAccount->FrozenMargin;
		account_info["FrozenCash"] = pTradingAccount->FrozenCash;
		account_info["FrozenCommission"] = pTradingAccount->FrozenCommission;
		account_info["CashIn"] = pTradingAccount->CashIn;
		account_info["Reserve"] = pTradingAccount->Reserve;
		account_info["SettlementID"] = pTradingAccount->SettlementID;
		account_info["Credit"] = pTradingAccount->Credit;
		account_info["Mortgage"] = pTradingAccount->Mortgage;
		account_info["DeliveryMargin"] = pTradingAccount->DeliveryMargin;
		account_info["ExchangeDeliveryMargin"] = pTradingAccount->ExchangeDeliveryMargin;
		account_info["ReserveBalance"] = pTradingAccount->ReserveBalance;
		account_info["PreFundMortgageIn"] = pTradingAccount->PreFundMortgageIn;
		account_info["PreFundMortgageOut"] = pTradingAccount->PreFundMortgageOut;
		account_info["FundMortgageIn"] = pTradingAccount->FundMortgageIn;
		account_info["FundMortgageOut"] = pTradingAccount->FundMortgageOut;
		account_info["FundMortgageAvailable"] = pTradingAccount->FundMortgageAvailable;
		account_info["MortgageableFund"] = pTradingAccount->MortgageableFund;
		account_info["SpecProductMargin"] = pTradingAccount->SpecProductMargin;
		account_info["SpecProductFrozenMargin"] = pTradingAccount->SpecProductFrozenMargin;
		account_info["SpecProductCommission"] = pTradingAccount->SpecProductCommission;
		account_info["SpecProductFrozenCommission"] = pTradingAccount->SpecProductFrozenCommission;
		account_info["SpecProductPositionProfit"] = pTradingAccount->SpecProductPositionProfit;
		account_info["SpecProductCloseProfit"] = pTradingAccount->SpecProductCloseProfit;
		account_info["SpecProductPositionProfitByAlg"] = pTradingAccount->SpecProductPositionProfitByAlg;
		account_info["SpecProductExchangeMargin"] = pTradingAccount->SpecProductExchangeMargin;
	}

	g_output_info["account"] = account_info;
	UNLOCK();
}

void Trader_Handler::ReqInstrument() {
	CThostFtdcQryInstrumentField req_contract = { 0 };
	do {
		RETURN_CODE = m_trader_api->ReqQryInstrument(&req_contract, ++m_request_id);
		if (RETURN_CODE == 0) PRINT_LOG_INFO("Start query contracts...");
		sleep(1);
	} while (RETURN_CODE != 0);
}

void Trader_Handler::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pInstrument == NULL) return;
	CThostFtdcInstrumentField& instr = g_all_contracts_info.get_next_free_node();
	instr = *pInstrument; // deep copy
	g_all_contracts_info.insert_current_node(pInstrument->InstrumentID);

	if (bIsLast) UNLOCK();
}

void Trader_Handler::ReqQryInvestorPositionDetail()
{
	CThostFtdcQryInvestorPositionDetailField req_pos = { 0 };
	strcpy(req_pos.BrokerID, m_trader_config->BROKER_ID);
	strcpy(req_pos.InvestorID, m_trader_config->USER_ID);
	do {
		RETURN_CODE = m_trader_api->ReqQryInvestorPositionDetail(&req_pos, ++m_request_id);
		if (RETURN_CODE == 0) PRINT_LOG_INFO("Start query position...");
		sleep(1);
	} while (RETURN_CODE != 0);
}

Json::array position_info;
void Trader_Handler::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField * pInvestorPositionDetail, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	CThostFtdcInvestorPositionDetailField *pos = pInvestorPositionDetail;
	if (pos) {
		if (pos->Volume > 0) {
			g_selected_contracts[pos->InstrumentID] = true;

			Json::object pos_info;
			pos_info["InstrumentID"] = pos->InstrumentID;
			pos_info["ExchangeID"] = pos->ExchangeID;
			pos_info["HedgeFlag"] = pos->HedgeFlag;
			pos_info["Direction"] = pos->Direction;
			pos_info["Volume"] = pos->Volume;
			pos_info["OpenPrice"] = pos->OpenPrice;
			pos_info["Margin"] = pos->Margin;
			pos_info["OpenDate"] = pos->OpenDate;

			if (complete_data_flag) {
				pos_info["TradingDay"] = pos->TradingDay;
				pos_info["SettlementID"] = pos->SettlementID;
				pos_info["TradeID"] = pos->TradeID;
				pos_info["TradeType"] = pos->TradeType;
				pos_info["BrokerID"] = pos->BrokerID;
				pos_info["InvestorID"] = pos->InvestorID;
				pos_info["CombInstrumentID"] = pos->CombInstrumentID;
				pos_info["CloseProfitByDate"] = pos->CloseProfitByDate;
				pos_info["CloseProfitByTrade"] = pos->CloseProfitByTrade;
				pos_info["PositionProfitByDate"] = pos->PositionProfitByDate;
				pos_info["PositionProfitByTrade"] = pos->PositionProfitByTrade;
				pos_info["ExchMargin"] = pos->ExchMargin;
				pos_info["MarginRateByMoney"] = pos->MarginRateByMoney;
				pos_info["MarginRateByVolume"] = pos->MarginRateByVolume;
				pos_info["LastSettlementPrice"] = pos->LastSettlementPrice;
				pos_info["SettlementPrice"] = pos->SettlementPrice;
				pos_info["CloseVolume"] = pos->CloseVolume;
				pos_info["CloseAmount"] = pos->CloseAmount;
			}
			position_info.push_back(pos_info);
		}
	}

	if (!bIsLast) return;

	if (!position_info.empty()) {
		g_output_info["positions"] = position_info;
	}

	UNLOCK();
}

void Trader_Handler::ReqQryOrder()
{
	CThostFtdcQryOrderField req = { 0 };
	strcpy(req.InvestorID, m_trader_config->USER_ID);
	do {
		RETURN_CODE = m_trader_api->ReqQryOrder(&req, ++m_request_id);
		if (RETURN_CODE == 0) PRINT_LOG_INFO("Start query order list...");
		sleep(1);
	} while (RETURN_CODE != 0);
}

Json::array orderlist_info;
void Trader_Handler::OnRspQryOrder(CThostFtdcOrderField * pOrder, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pOrder == NULL) {
		UNLOCK();
		return;
	}

	g_selected_contracts[pOrder->InstrumentID] = true;
	PRINT_LOG_DEBUG("%s %s %s %d %d %d@%f %s", pOrder->InstrumentID, pOrder->InsertDate, pOrder->InsertTime,
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
	order_log["ExchangeID"] = pOrder->ExchangeID;
	order_log["OrderType"] = pOrder->OrderType;
	order_log["CombHedgeFlag"] = pOrder->CombHedgeFlag;
	order_log["OrderPriceType"] = pOrder->OrderPriceType;
	order_log["OrderLocalID"] = pOrder->OrderLocalID;

	if (complete_data_flag) {
		order_log["StatusMsg"] = pOrder->StatusMsg;
		order_log["BrokerID"] = pOrder->BrokerID;
		order_log["InvestorID"] = pOrder->InvestorID;
		order_log["OrderRef"] = pOrder->OrderRef;
		order_log["UserID"] = pOrder->UserID;
		order_log["TimeCondition"] = pOrder->TimeCondition;
		order_log["GTDDate"] = pOrder->GTDDate;
		order_log["VolumeCondition"] = pOrder->VolumeCondition;
		order_log["MinVolume"] = pOrder->MinVolume;
		order_log["ContingentCondition"] = pOrder->ContingentCondition;
		order_log["StopPrice"] = pOrder->StopPrice;
		order_log["ForceCloseReason"] = pOrder->ForceCloseReason;
		order_log["IsAutoSuspend"] = pOrder->IsAutoSuspend;
		order_log["BusinessUnit"] = pOrder->BusinessUnit;
		order_log["RequestID"] = pOrder->RequestID;
		order_log["ParticipantID"] = pOrder->ParticipantID;
		order_log["ClientID"] = pOrder->ClientID;
		order_log["ExchangeInstID"] = pOrder->ExchangeInstID;
		order_log["TraderID"] = pOrder->TraderID;
		order_log["InstallID"] = pOrder->InstallID;
		order_log["OrderSubmitStatus"] = pOrder->OrderSubmitStatus;
		order_log["NotifySequence"] = pOrder->NotifySequence;
		order_log["TradingDay"] = pOrder->TradingDay;
		order_log["SettlementID"] = pOrder->SettlementID;
		order_log["OrderSource"] = pOrder->OrderSource;
		order_log["VolumeTraded"] = pOrder->VolumeTraded;
		order_log["VolumeTotal"] = pOrder->VolumeTotal;
		order_log["ActiveTime"] = pOrder->ActiveTime;
		order_log["SuspendTime"] = pOrder->SuspendTime;
		order_log["UpdateTime"] = pOrder->UpdateTime;
		order_log["CancelTime"] = pOrder->CancelTime;
		order_log["ActiveTraderID"] = pOrder->ActiveTraderID;
		order_log["ClearingPartID"] = pOrder->ClearingPartID;
		order_log["SequenceNo"] = pOrder->SequenceNo;
		order_log["FrontID"] = pOrder->FrontID;
		order_log["SessionID"] = pOrder->SessionID;
		order_log["UserProductInfo"] = pOrder->UserProductInfo;
		order_log["UserForceClose"] = pOrder->UserForceClose;
		order_log["ActiveUserID"] = pOrder->ActiveUserID;
		order_log["BrokerOrderSeq"] = pOrder->BrokerOrderSeq;
		order_log["RelativeOrderSysID"] = pOrder->RelativeOrderSysID;
		order_log["ZCETotalTradedVolume"] = pOrder->ZCETotalTradedVolume;
		order_log["IsSwapOrder"] = pOrder->IsSwapOrder;
		order_log["BranchID"] = pOrder->BranchID;
		order_log["InvestUnitID"] = pOrder->InvestUnitID;
		order_log["AccountID"] = pOrder->AccountID;
		order_log["CurrencyID"] = pOrder->CurrencyID;
		order_log["IPAddress"] = pOrder->IPAddress;
		order_log["MacAddress"] = pOrder->MacAddress;
	}
	orderlist_info.push_back(order_log);

	if (!bIsLast) return;

	if (!orderlist_info.empty()) {
		g_output_info["orderlist"] = orderlist_info;
	}

	UNLOCK();
}

void Trader_Handler::ReqQryTrade()
{
	CThostFtdcQryTradeField req = { 0 };
	strcpy(req.InvestorID, m_trader_config->USER_ID);
	do {
		RETURN_CODE = m_trader_api->ReqQryTrade(&req, ++m_request_id);
		if (RETURN_CODE == 0) PRINT_LOG_INFO("Start query trade list...");
		sleep(1);
	} while (RETURN_CODE != 0);
}

Json::array tradelist_info = Json::array();
void Trader_Handler::OnRspQryTrade(CThostFtdcTradeField * pTrade, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pTrade == NULL) {
		UNLOCK();
		return;
	}

	g_selected_contracts[pTrade->InstrumentID] = true;
	PRINT_LOG_DEBUG("%s %s %s %d %d %d@%f %s %s", pTrade->InstrumentID, pTrade->TradeDate, pTrade->TradeTime,
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
	trade_log["ExchangeID"] = pTrade->ExchangeID;
	trade_log["HedgeFlag"] = pTrade->HedgeFlag;
	trade_log["OrderLocalID"] = pTrade->OrderLocalID;

	if (complete_data_flag) {
		trade_log["BrokerID"] = pTrade->BrokerID;
		trade_log["InvestorID"] = pTrade->InvestorID;
		trade_log["OrderRef"] = pTrade->OrderRef;
		trade_log["UserID"] = pTrade->UserID;
		trade_log["ParticipantID"] = pTrade->ParticipantID;
		trade_log["ClientID"] = pTrade->ClientID;
		trade_log["TradingRole"] = pTrade->TradingRole;
		trade_log["ExchangeInstID"] = pTrade->ExchangeInstID;
		trade_log["TradeType"] = pTrade->TradeType;
		trade_log["PriceSource"] = pTrade->PriceSource;
		trade_log["ClearingPartID"] = pTrade->ClearingPartID;
		trade_log["BusinessUnit"] = pTrade->BusinessUnit;
		trade_log["SequenceNo"] = pTrade->SequenceNo;
		trade_log["TradingDay"] = pTrade->TradingDay;
		trade_log["SettlementID"] = pTrade->SettlementID;
		trade_log["BrokerOrderSeq"] = pTrade->BrokerOrderSeq;
		trade_log["TradeSource"] = pTrade->TradeSource;
	}
	tradelist_info.push_back(trade_log);

	if (!bIsLast) return;

	if (!tradelist_info.empty()) {
		g_output_info["tradelist"] = tradelist_info;
	}

	UNLOCK();
}

void fill_contract_json_object(Json::object &contracts, const char* symbol) {
	CThostFtdcInstrumentField& info = g_all_contracts_info[symbol];

	Json::object contr;
	contr["ExchangeID"] = info.ExchangeID;
	//code_convert(info.InstrumentName, strlen(info.InstrumentName), TEMP_MSG, 4096);
	contr["InstrumentName"] = info.InstrumentName;
	contr["ExchangeInstID"] = info.ExchangeInstID;
	contr["ProductID"] = info.ProductID;
	contr["ProductClass"] = info.ProductClass;
	contr["DeliveryYear"] = info.DeliveryYear;
	contr["DeliveryMonth"] = info.DeliveryMonth;
	contr["MaxMarketOrderVolume"] = info.MaxMarketOrderVolume;
	contr["MinMarketOrderVolume"] = info.MinMarketOrderVolume;
	contr["MaxLimitOrderVolume"] = info.MaxLimitOrderVolume;
	contr["MinLimitOrderVolume"] = info.MinLimitOrderVolume;
	contr["VolumeMultiple"] = info.VolumeMultiple;
	contr["PriceTick"] = info.PriceTick;
	contr["CreateDate"] = info.CreateDate;
	contr["OpenDate"] = info.OpenDate;
	contr["ExpireDate"] = info.ExpireDate;
	contr["StartDelivDate"] = info.StartDelivDate;
	contr["EndDelivDate"] = info.EndDelivDate;
	contr["InstLifePhase"] = info.InstLifePhase;
	contr["IsTrading"] = info.IsTrading;
	contr["PositionType"] = info.PositionType;
	contr["PositionDateType"] = info.PositionDateType;
	contr["LongMarginRatio"] = info.LongMarginRatio;
	contr["ShortMarginRatio"] = info.ShortMarginRatio;
	contr["MaxMarginSideAlgorithm"] = info.MaxMarginSideAlgorithm;
	contr["UnderlyingInstrID"] = info.UnderlyingInstrID;
	contr["StrikePrice"] = info.StrikePrice;
	contr["OptionsType"] = info.OptionsType;
	contr["UnderlyingMultiple"] = info.UnderlyingMultiple;
	contr["CombinationType"] = info.CombinationType;

	contracts[symbol] = contr;
}

void Trader_Handler::EndProcess() {
	g_output_info["error_code"] = ERROR_CODE;
	g_output_info["error_msg"] = ERROR_MSG;
	Json output_info = g_output_info;
	std::string temp_result = output_info.dump();
	printf("%s\n", temp_result.c_str());
	LOG_LN("%s", temp_result.c_str());
	
	fprintf(stderr, "\n"); // user friendly
	flush_log();
	m_trader_api->Release();
	exit(0);
}

void Trader_Handler::ReqManager() {
	if (m_trader_config->POSITION_FLAG) {
		GETLOCK();
		ReqQryInvestorPositionDetail();
	}

	if (m_trader_config->ORDER_INFO_FLAG) {
		GETLOCK();
		ReqQryOrder();
	}

	if (m_trader_config->TRADE_INFO_FLAG) {
		GETLOCK();
		ReqQryTrade();
	}

	if (m_trader_config->CONTRACT_INFO_FLAG) {
		GETLOCK();
		ReqInstrument();
	
		GETLOCK();
		Json::object contracts_info;
		if (m_trader_config->ALL_CONTRACT_INFO_FLAG) {
			for (auto iter = g_all_contracts_info.begin(); iter != g_all_contracts_info.end(); iter++) {
				fill_contract_json_object(contracts_info, iter->first);
			}
		}
		else {
			for (auto iter = g_selected_contracts.begin(); iter != g_selected_contracts.end(); iter++) {
				fill_contract_json_object(contracts_info, iter->first);
			}
		}
		g_output_info["contracts_info"] = contracts_info;
		UNLOCK();
	}

	GETLOCK();
	EndProcess();
}

void Trader_Handler::OnFrontDisconnected(int nReason)
{
	LOG_LN("Trader front disconnected, code: %d", nReason);
}

void Trader_Handler::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL && pRspInfo->ErrorID != 0) {
		code_convert(pRspInfo->ErrorMsg, strlen(pRspInfo->ErrorMsg), TEMP_MSG, 4096);
		LOG_LN("ErrorID = %d, ErrorMsg = %s", pRspInfo->ErrorID, TEMP_MSG);
	}
	flush_log();
}

