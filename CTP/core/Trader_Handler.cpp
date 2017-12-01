#include <iostream>
#include <string.h>
#include "utils/utils.h"
#include "utils/log.h"
#include "strategy_interface.h"
#include "strategy.h"
#include "Trader_Handler.h"
#include "strategy_interface.h"
#include "ThostFtdcUserApiStruct.h"

using namespace std;

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#define sleep Sleep
#endif

#define BROKER_FEE 0.0002
#define STAMP_TAX 0.001
#define ACC_TRANSFER_FEE 0.00002

static CThostFtdcInputOrderField g_order_t = { 0 };
static CThostFtdcInputOrderActionField g_order_action_t = { 0 };
static st_config_t g_config_t = { 0 };

int process_strategy_order(int type, int length, void *data){
	PRINT_INFO("lalal");
	return 0;
}

int process_strategy_info(int type, int length, void *data) {
	PRINT_INFO("lalal");
	return 0;
}

int process_strategy_resp(int type, int length, void *data) {
	PRINT_INFO("lalal");
	return 0;
}


// 流控判断
bool IsFlowControl(int iResult)
{
	return ((iResult == -2) || (iResult == -3));
}

void update_trader_info(TraderInfo& info, CThostFtdcRspUserLoginField *pRspUserLogin) {
	// 保存会话参数
	info.FrontID = pRspUserLogin->FrontID;
	info.SessionID = pRspUserLogin->SessionID;
	int iNextOrderRef = atoi(pRspUserLogin->MaxOrderRef);
	info.MaxOrderRef = iNextOrderRef++;
	//sprintf(info.MaxOrderRef, "%d", iNextOrderRef);
	strlcpy(info.TradingDay, pRspUserLogin->TradingDay, TRADING_DAY_LEN);
	strlcpy(info.LoginTime, pRspUserLogin->LoginTime, TRADING_DAY_LEN);

	g_config_t.trading_date = atoi(info.TradingDay);
	int hour = 0;
	for (int i = 0; i < 6; i++) {
		if (info.LoginTime[i] != ':') {
			hour = 10 * hour + info.LoginTime[i];
		}
		break;
	}
	if (hour > 8 && hour < 19)
		g_config_t.day_night = DAY;
	else
		g_config_t.day_night = NIGHT;
}


Trader_Handler::Trader_Handler(CThostFtdcTraderApi* TraderApi, TraderConfig* trader_config)
{
	m_trader_config = trader_config;
	m_trader_api = TraderApi;
	m_trader_api->RegisterSpi(this);			// 注册事件类
	m_trader_api->RegisterFront(m_trader_config->TRADER_FRONT);		// connect
	m_orders = new MyArray<CThostFtdcInputOrderField>(2000);

	g_config_t.proc_order_hdl = process_strategy_order;
	g_config_t.send_info_hdl = process_strategy_info;
	g_config_t.pass_rsp_hdl = process_strategy_resp;
}

Trader_Handler::~Trader_Handler()
{
	delete m_orders;
}

void Trader_Handler::OnFrontConnected()
{
	CThostFtdcReqUserLoginField req = { 0 };
	strcpy(req.BrokerID, m_trader_config->TBROKER_ID);
	strcpy(req.UserID, m_trader_config->TUSER_ID);
	strcpy(req.Password, m_trader_config->TPASSWORD);
	m_trader_api->ReqUserLogin(&req, ++m_request_id);
}

void Trader_Handler::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL && pRspInfo->ErrorID == 0) {
		m_is_ready = true;
		update_trader_info(m_trader_info, pRspUserLogin);
		PRINT_SUCCESS("TradingDay: %d DayNight: %d", g_config_t.trading_date, g_config_t.day_night);

		//投资者结算结果确认
		ReqSettlementInfo();
	} else {
		PRINT_ERROR("Login Failed!");
		exit(-1);
	}
}

void Trader_Handler::ReqSettlementInfo()
{
	CThostFtdcSettlementInfoConfirmField req = { 0 };
	strcpy(req.BrokerID, m_trader_config->TBROKER_ID);
	strcpy(req.InvestorID, m_trader_config->TUSER_ID);
	int ret = m_trader_api->ReqSettlementInfoConfirm(&req, ++m_request_id);
	if (ret != 0)
		PRINT_ERROR("Send settlement request fail!");
}

void Trader_Handler::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	PRINT_SUCCESS("Comform settlement!");
	//请求查询资金账户
	ReqTradingAccount();
}

void Trader_Handler::ReqTradingAccount()
{
	CThostFtdcQryTradingAccountField requ = { 0 };
	strcpy(requ.BrokerID, m_trader_config->TBROKER_ID);
	strcpy(requ.InvestorID, m_trader_config->TUSER_ID);
	strcpy(requ.CurrencyID, "CNY");
	while (true) {
		int iResult = m_trader_api->ReqQryTradingAccount(&requ, ++m_request_id);
		if (!IsFlowControl(iResult)) {
			PRINT_INFO("send query account: %s %s", requ.InvestorID, iResult == 0 ? "success" : "fail");
			break;
		}
		else {
			sleep(1);
		}
	} // while
}

void Trader_Handler::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pTradingAccount == NULL) return;
	PRINT_DEBUG("%s %f %f %f %f %f %f %f", pTradingAccount->AccountID, pTradingAccount->Interest, pTradingAccount->Deposit, pTradingAccount->Withdraw, pTradingAccount->CurrMargin, pTradingAccount->CloseProfit, pTradingAccount->PositionProfit, pTradingAccount->Available);
	strlcpy(g_config_t.accounts[0].account, pTradingAccount->AccountID, ACCOUNT_LEN);
	g_config_t.accounts[0].cash_asset = pTradingAccount->Available; //to change
	g_config_t.accounts[0].cash_available = pTradingAccount->Available;
	g_config_t.accounts[0].currency = CNY;
	g_config_t.accounts[0].exch_rate = 1.0;

	//请求查询合约
	for (int i = 0; i < m_trader_config->INSTRUMENT_COUNT; i++) {
		ReqInstrument(m_trader_config->INSTRUMENTS[i]);
	}
}

void Trader_Handler::ReqInstrument(char* symbol) {
	while (true) {
		strcpy(m_req_contract.InstrumentID, symbol);
		int iResult = m_trader_api->ReqQryInstrument(&m_req_contract, ++m_request_id);
		if (!IsFlowControl(iResult)) {
			PRINT_INFO("send query contract: %s %s", m_req_contract.InstrumentID, iResult == 0 ? "success" : "fail");
			break;
		}
		else {
			sleep(1);
		}
	}
}

void Trader_Handler::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pInstrument == NULL) return;
	PRINT_DEBUG("%s %s %s %s %s %f", pInstrument->InstrumentID, pInstrument->ExchangeID, pInstrument->ProductID, pInstrument->CreateDate, pInstrument->ExpireDate, pInstrument->PriceTick);
	//todo 补全合约信息，先考虑一个合约

	strlcpy(g_config_t.contracts[0].symbol, pInstrument->InstrumentID, SYMBOL_LEN);
	g_config_t.contracts[0].exch = get_exch_by_name(pInstrument->ExchangeID);
	g_config_t.contracts[0].max_accum_open_vol = 10000;
	g_config_t.contracts[0].max_cancel_limit = 1000;
	g_config_t.contracts[0].expiration_date = atoi(pInstrument->EndDelivDate); // to correct
	g_config_t.contracts[0].tick_size = pInstrument->PriceTick;
	g_config_t.contracts[0].multiple = pInstrument->VolumeMultiple; // to correct
	strlcpy(g_config_t.contracts[0].account, g_config_t.accounts[0].account, ACCOUNT_LEN);

	if (bIsLast == true) {
		//查询最大报单数量请求
		ReqQueryMaxOrderVolume(pInstrument->InstrumentID);
	}
}

void Trader_Handler::ReqQueryMaxOrderVolume(char* symbol)
{
	CThostFtdcQueryMaxOrderVolumeField ReqMaxOrdSize = { 0 };
	strcpy(ReqMaxOrdSize.BrokerID, m_trader_config->TBROKER_ID);
	strcpy(ReqMaxOrdSize.InvestorID, m_trader_config->TUSER_ID);
	strcpy(ReqMaxOrdSize.InstrumentID, symbol);
	while (true) {
		int iResult = m_trader_api->ReqQueryMaxOrderVolume(&ReqMaxOrdSize, ++m_request_id);
		if (!IsFlowControl(iResult)) {
			PRINT_INFO("send query max order size %s", iResult == 0 ? "success" : "fail");
			break;
		}
		else {
			sleep(1);
		}
	}
}

void Trader_Handler::OnRspQueryMaxOrderVolume(CThostFtdcQueryMaxOrderVolumeField * pQueryMaxOrderVolume, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pQueryMaxOrderVolume == NULL) return;
	g_config_t.contracts[0].max_accum_open_vol = pQueryMaxOrderVolume->MaxVolume;

	//请求查询合约手续费率
	ReqQryInstrumentCommissionRate(pQueryMaxOrderVolume->InstrumentID);
}

void Trader_Handler::ReqQryInstrumentCommissionRate(char * symbol)
{
	CThostFtdcQryInstrumentCommissionRateField ReqCommissionRate = { 0 };
	strcpy(ReqCommissionRate.BrokerID, m_trader_config->TBROKER_ID);
	strcpy(ReqCommissionRate.InvestorID, m_trader_config->TUSER_ID);
	strcpy(ReqCommissionRate.InstrumentID, symbol);
	while (true) {
		//strcpy(m_req_pos.InstrumentID, symbol);
		int iResult = m_trader_api->ReqQryInstrumentCommissionRate(&ReqCommissionRate, ++m_request_id);
		if (!IsFlowControl(iResult)) {
			PRINT_INFO("send query commission rate %s", iResult == 0 ? "success" : "fail");
			break;
		}
		else {
			sleep(1);
		}
	}
}

void Trader_Handler::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField * pInstrumentCommissionRate, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pInstrumentCommissionRate == NULL) return;
	g_config_t.contracts[0].fee.exchange_fee = pInstrumentCommissionRate->OpenRatioByMoney;
	g_config_t.contracts[0].fee.fee_by_lot = false;
	g_config_t.contracts[0].fee.yes_exchange_fee = pInstrumentCommissionRate->CloseRatioByMoney;
	g_config_t.contracts[0].fee.acc_transfer_fee = ACC_TRANSFER_FEE;
	g_config_t.contracts[0].fee.stamp_tax = STAMP_TAX;
	g_config_t.contracts[0].fee.broker_fee = BROKER_FEE;

	PRINT_INFO("%s", pInstrumentCommissionRate->InstrumentID);
	
	//请求查询投资者持仓
	ReqQryInvestorPositionDetail();
}

void Trader_Handler::ReqQryInvestorPositionDetail()
{
	CThostFtdcQryInvestorPositionDetailField req_pos = { 0 };
	strcpy(req_pos.BrokerID, m_trader_config->TBROKER_ID);
	strcpy(req_pos.InvestorID, m_trader_config->TUSER_ID);
	while (true) {
		//strcpy(m_req_pos.InstrumentID, symbol);
		int iResult = m_trader_api->ReqQryInvestorPositionDetail(&req_pos, ++m_request_id);
		if (!IsFlowControl(iResult)) {
			PRINT_INFO("send query contract position %s", iResult == 0 ? "success" : "fail");
			break;
		}
		else {
			sleep(1);
		}
	}
}

void Trader_Handler::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField * pInvestorPositionDetail, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pInvestorPositionDetail) {
		//对于所有合约，不保存已平仓的，只保存未平仓的
		if (pInvestorPositionDetail->Volume > 0) {
			if (pInvestorPositionDetail->Direction == TRADER_BUY)
				m_contracts_long.push_back(pInvestorPositionDetail);
			else if (pInvestorPositionDetail->Direction == TRADER_SELL)
				m_contracts_short.push_back(pInvestorPositionDetail);

			bool find_instId = false;
			for (int i = 0; i< m_trader_config->INSTRUMENT_COUNT; i++) {
				if (strcmp(m_trader_config->INSTRUMENTS[i], pInvestorPositionDetail->InstrumentID) == 0) {	//合约已存在，已订阅过行情
					find_instId = true;
					break;
				}
			}
			if (find_instId == false) {
				strlcpy(m_trader_config->INSTRUMENTS[m_trader_config->INSTRUMENT_COUNT++], pInvestorPositionDetail->InstrumentID, SYMBOL_LEN);
			}
		}

		if (bIsLast) {
			int long_size = 0, short_size = 0;
			double long_price = 0, short_price = 0;

			int yes_long_size = 0, yes_short_size = 0;
			double yes_long_price = 0, yes_short_price = 0;

			for (int i = 0; i < m_contracts_long.size(); i++) {
				if (atoi(m_contracts_long[i].OpenDate) != g_config_t.trading_date) {
					yes_long_price += m_contracts_long[i].OpenPrice * m_contracts_long[i].Volume;
					yes_long_size += m_contracts_long[i].Volume;
				}
				long_price += m_contracts_long[i].OpenPrice * m_contracts_long[i].Volume;
				long_size += m_contracts_long[i].Volume;
			}
			if(long_size == 0) {
				g_config_t.contracts[0].today_pos.long_price = 0;
				g_config_t.contracts[0].today_pos.long_volume = 0;
			} else {
				g_config_t.contracts[0].today_pos.long_price = long_price / long_size;
				g_config_t.contracts[0].today_pos.long_volume = long_size;
			}
			
			if(yes_long_size == 0) {
				g_config_t.contracts[0].yesterday_pos.long_price = 0;
				g_config_t.contracts[0].yesterday_pos.long_volume = 0;
			} else {
				g_config_t.contracts[0].yesterday_pos.long_price = yes_long_price / yes_long_size;
				g_config_t.contracts[0].yesterday_pos.long_volume = yes_long_size;
			}

			for (int i = 0; i < m_contracts_short.size(); i++) {
				if (atoi(m_contracts_short[i].OpenDate) != g_config_t.trading_date) {
					yes_short_price += m_contracts_short[i].OpenPrice * m_contracts_short[i].Volume;
					yes_short_size += m_contracts_short[i].Volume;
				}
				short_price += m_contracts_short[i].OpenPrice * m_contracts_short[i].Volume;
				short_size += m_contracts_short[i].Volume;
			}
			
			if (short_size == 0) {
				g_config_t.contracts[0].today_pos.short_price = 0;
				g_config_t.contracts[0].today_pos.short_volume = 0;
			} else {
				g_config_t.contracts[0].today_pos.short_price = short_price / short_size;
				g_config_t.contracts[0].today_pos.short_volume = short_size;
			}

			if (yes_short_size == 0) {
				g_config_t.contracts[0].yesterday_pos.short_price = 0;
				g_config_t.contracts[0].yesterday_pos.short_volume = 0;
			} else {
				g_config_t.contracts[0].yesterday_pos.short_price = yes_short_price / yes_short_size;
				g_config_t.contracts[0].yesterday_pos.short_volume = yes_short_size;
			}
			
			// 在这里我们结束了config的配置，开始初始化策略

			for (int i = 0; i < ACCOUNT_MAX; i++) {
				if (g_config_t.accounts[i].account[0] == '\0') break;
				account_t& l_account = g_config_t.accounts[i];
				PRINT_SUCCESS("account: %s, cash_available: %f, cash_asset: %f, exch_rate: %f, currency: %d",
					l_account.account, l_account.cash_available, l_account.cash_asset, l_account.exch_rate, l_account.currency);
			}

			for (int i = 0; i < SYMBOL_MAX; i++) {
				if (g_config_t.contracts[i].symbol[0] == '\0') break;
				contract_t& l_config_instr = g_config_t.contracts[i];
				PRINT_SUCCESS("symbol: %s, account: %s, exch: %c, max_accum_open_vol: %d, max_cancel_limit: %d, expiration_date: %d, "
					"today_long_pos: %d, today_long_price: %f, today_short_pos: %d, today_short_price: %f, "
					"yesterday_long_pos: %d, yesterday_long_price: %f, yesterday_short_pos: %d, yesterday_short_price: %f, "
					"fee_by_lot: %d, exchange_fee : %f, yes_exchange_fee : %f, broker_fee : %f, stamp_tax : %f, acc_transfer_fee : %f, tick_size : %f, multiplier : %f",
					l_config_instr.symbol, l_config_instr.account, l_config_instr.exch, l_config_instr.max_accum_open_vol, l_config_instr.max_cancel_limit, l_config_instr.expiration_date,
					l_config_instr.today_pos.long_volume, l_config_instr.today_pos.long_price, l_config_instr.today_pos.short_volume, l_config_instr.today_pos.short_price,
					l_config_instr.yesterday_pos.long_volume, l_config_instr.yesterday_pos.long_price, l_config_instr.yesterday_pos.short_volume, l_config_instr.yesterday_pos.short_price,
					l_config_instr.fee.fee_by_lot, l_config_instr.fee.exchange_fee, l_config_instr.fee.yes_exchange_fee, l_config_instr.fee.broker_fee, l_config_instr.fee.stamp_tax, l_config_instr.fee.acc_transfer_fee, l_config_instr.tick_size, l_config_instr.multiple);
			}
		}
	}
}

void Trader_Handler::ReqQryInvestorPosition()
{
	CThostFtdcQryInvestorPositionField req_pos = { 0 };
	strcpy(req_pos.BrokerID, m_trader_config->TBROKER_ID);
	strcpy(req_pos.InvestorID, m_trader_config->TUSER_ID);
	while (true) {
		//strcpy(m_req_pos.InstrumentID, symbol);
		int iResult = m_trader_api->ReqQryInvestorPosition(&req_pos, ++m_request_id);
		if (!IsFlowControl(iResult)) {
			PRINT_INFO("send query contract position %s", iResult == 0 ? "success" : "fail");
			break;
		}
		else {
			sleep(1);
		}
	}
}

void Trader_Handler::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	PRINT_INFO("is_last: %d", bIsLast); //todo 过滤重复的
	if (pInvestorPosition) {
		PRINT_INFO("%s %d %d %d %f", pInvestorPosition->InstrumentID, pInvestorPosition->Position, pInvestorPosition->TodayPosition, pInvestorPosition->YdPosition, pInvestorPosition->UseMargin);
	}
}

void Trader_Handler::send_single_order(order_t *order)
{
	///经纪公司代码
	strcpy(g_order_t.BrokerID, m_trader_config->TBROKER_ID);
	///投资者代码
	strcpy(g_order_t.InvestorID, m_trader_config->TUSER_ID);
	///报单引用
	sprintf(g_order_t.OrderRef, "%d", m_trader_info.MaxOrderRef);
	//strcpy(g_order_t.OrderRef, m_trader_info.MaxOrderRef);
	///用户代码
	strcpy(g_order_t.UserID, m_trader_config->TUSER_ID);

	///合约代码
	strcpy(g_order_t.InstrumentID, order->symbol);
	///报单价格条件: 限价
	if (order->order_type == ORDER_TYPE_LIMIT)
		g_order_t.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	else
		g_order_t.OrderPriceType = THOST_FTDC_OPT_AnyPrice; //市价单
															///买卖方向: 
	g_order_t.Direction = order->direction == ORDER_BUY ? '0' : '1';
	///组合开平标志: 开仓
	if (order->open_close == ORDER_OPEN)
		g_order_t.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	else if (order->open_close == ORDER_CLOSE)
		g_order_t.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
	else if (order->open_close == ORDER_CLOSE_YES)
		g_order_t.CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday;

	///组合投机套保标志
	if (order->investor_type == ORDER_SPECULATOR)
		g_order_t.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	else if (order->investor_type == ORDER_HEDGER)
		g_order_t.CombHedgeFlag[0] = THOST_FTDC_HF_Hedge;
	else if (order->investor_type == ORDER_ARBITRAGEURS)
		g_order_t.CombHedgeFlag[0] = THOST_FTDC_HF_Arbitrage;
	///价格
	g_order_t.LimitPrice = order->price;
	///数量: 1
	g_order_t.VolumeTotalOriginal = order->volume;
	///有效期类型: 当日有效
	if (order->time_in_force == ORDER_TIF_DAY || order->time_in_force == ORDER_TIF_GTD)
		g_order_t.TimeCondition = THOST_FTDC_TC_GFD;
	else if (order->time_in_force == ORDER_TIF_IOC || order->time_in_force == ORDER_TIF_FOK
		|| order->time_in_force == ORDER_TIF_FAK)
		g_order_t.TimeCondition = THOST_FTDC_TC_IOC;
	else if (order->time_in_force == ORDER_TIF_GTC)
		g_order_t.TimeCondition = THOST_FTDC_TC_GTC;
	///GTD日期
	//	TThostFtdcDateType	GTDDate;
	///成交量类型: 任何数量
	g_order_t.VolumeCondition = THOST_FTDC_VC_AV;
	///最小成交量: 1
	g_order_t.MinVolume = 1;
	///触发条件: 立即
	g_order_t.ContingentCondition = THOST_FTDC_CC_Immediately;
	///止损价
	//	TThostFtdcPriceType	StopPrice;
	///强平原因: 非强平
	g_order_t.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///自动挂起标志: 否
	g_order_t.IsAutoSuspend = 0;
	///业务单元
	//	TThostFtdcBusinessUnitType	BusinessUnit;
	///请求编号
	//	TThostFtdcRequestIDType	RequestID;
	///用户强评标志: 否
	g_order_t.UserForceClose = 0;

	CThostFtdcInputOrderField& order_record = (*m_orders)[m_trader_info.MaxOrderRef];
	order_record = g_order_t;
	order->order_id = m_trader_info.MaxOrderRef;
	m_trader_info.MaxOrderRef++;
	int ret = m_trader_api->ReqOrderInsert(&g_order_t, ++m_request_id);
	PRINT_DEBUG("Send order %s", ret == 0 ? ", success" : ", fail");
}

void Trader_Handler::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(pInputOrder) {
		PRINT_SUCCESS("--->>> OnRspOrderInsert");
		cout << "经纪公司代码 " << pInputOrder->BrokerID << endl;
		cout << "投资者代码 " << pInputOrder->InvestorID << endl;
		cout << "合约代码 " << pInputOrder->InstrumentID << endl;
		cout << "报单引用 " << pInputOrder->OrderRef << endl;
		cout << "用户代码 " << pInputOrder->UserID << endl;
		cout << "报单价格条件 " << pInputOrder->OrderPriceType << endl;
		cout << "买卖方向 " << pInputOrder->Direction << endl;
		cout << "组合开平标志 " << pInputOrder->CombOffsetFlag << endl;
		cout << "组合投机套保标志 " << pInputOrder->CombHedgeFlag << endl;
		cout << "价格 " << pInputOrder->LimitPrice << endl;
		cout << "数量 " << pInputOrder->VolumeTotalOriginal << endl;
		cout << "有效期类型 " << pInputOrder->TimeCondition << endl;
		cout << "成交量类型 " << pInputOrder->VolumeCondition << endl;
		cout << "最小成交量 " << pInputOrder->MinVolume << endl;
		cout << "触发条件 " << pInputOrder->ContingentCondition << endl;
		cout << "请求编号 " << pInputOrder->RequestID << endl;
		cout << "交易所代码 " << pInputOrder->ExchangeID << endl;
	}
	IsErrorRspInfo(pRspInfo);
}

void Trader_Handler::cancel_single_order(order_t * order)
{
	CThostFtdcInputOrderField& order_record = (*m_orders)[order->order_id];
	///经纪公司代码
	strcpy(g_order_action_t.BrokerID, order_record.BrokerID);
	///投资者代码
	strcpy(g_order_action_t.InvestorID, order_record.InvestorID);
	///报单操作引用
	//	TThostFtdcOrderActionRefType	OrderActionRef;
	///报单引用
	strcpy(g_order_action_t.OrderRef, order_record.OrderRef);
	///请求编号
	g_order_action_t.RequestID = m_request_id;
	///前置编号
	g_order_action_t.FrontID = m_trader_info.FrontID;
	///会话编号
	g_order_action_t.SessionID = m_trader_info.SessionID;
	///交易所代码
	//	TThostFtdcExchangeIDType	ExchangeID;
	///报单编号
	//	TThostFtdcOrderSysIDType	OrderSysID;
	///操作标志
	g_order_action_t.ActionFlag = THOST_FTDC_AF_Delete;
	///价格
	//	TThostFtdcPriceType	LimitPrice;
	///数量变化
	//	TThostFtdcVolumeType	VolumeChange;
	///用户代码
	//	TThostFtdcUserIDType	UserID;
	///合约代码
	strcpy(g_order_action_t.InstrumentID, order_record.InstrumentID);

	int ret = m_trader_api->ReqOrderAction(&g_order_action_t, ++m_request_id);
	PRINT_DEBUG("Cancel order %s", ret == 0 ? ", success" : ", fail");
}

void Trader_Handler::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(pInputOrderAction) {
		PRINT_SUCCESS("--->>> OnRspOrderAction");
		cout << "经纪公司代码 " << pInputOrderAction->BrokerID << endl;
		cout << "投资者代码 " << pInputOrderAction->InvestorID << endl;
		cout << "合约代码 " << pInputOrderAction->InstrumentID << endl;
		cout << "报单编号 " << pInputOrderAction->OrderSysID << endl;
		cout << "报单引用 " << pInputOrderAction->OrderRef << endl;
		cout << "用户代码 " << pInputOrderAction->UserID << endl;
		cout << "价格 " << pInputOrderAction->LimitPrice << endl;
		cout << "数量变化 " << pInputOrderAction->VolumeChange << endl;
		cout << "操作标志 " << pInputOrderAction->ActionFlag << endl;
		cout << "交易所代码 " << pInputOrderAction->ExchangeID << endl;
		cout << "请求编号 " << pInputOrderAction->RequestID << endl;
		cout << "前置编号 " << pInputOrderAction->FrontID << endl;
		cout << "会话编号 " << pInputOrderAction->SessionID << endl;
	}
	IsErrorRspInfo(pRspInfo);
}

void Trader_Handler::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	if (pOrder) {
		PRINT_SUCCESS("--->>> OnRtnOrder %s", pOrder->InsertTime);
		cout << "经纪公司代码 "	<< pOrder->BrokerID << endl;
		cout << "投资者代码 "		<< pOrder->InvestorID << endl;
		cout << "合约代码 "		<< pOrder->InstrumentID << endl;
		cout << "报单引用 "		<< pOrder->OrderRef << endl;
		cout << "用户代码 "		<< pOrder->UserID << endl;
		cout << "报单价格条件 "	<< pOrder->OrderPriceType << endl;
		cout << "买卖方向 "		<< pOrder->Direction << endl;
		cout << "组合开平标志 "	<< pOrder->CombOffsetFlag << endl;
		cout << "组合投机套保标志 " << pOrder->CombHedgeFlag << endl;
		cout << "价格 "			<< pOrder->LimitPrice << endl;
		cout << "数量 "			<< pOrder->VolumeTotalOriginal << endl;
		cout << "有效期类型 "		<< pOrder->TimeCondition << endl;
		cout << "成交量类型 "		<< pOrder->VolumeCondition << endl;
		cout << "最小成交量 "		<< pOrder->MinVolume << endl;
		cout << "触发条件 "		<< pOrder->ContingentCondition << endl;
		cout << "请求编号 "		<< pOrder->RequestID << endl;
		cout << "本地报单编号 "	<< pOrder->OrderLocalID << endl;
		cout << "交易所代码 "		<< pOrder->ExchangeID << endl;
		cout << "合约在交易所的代码 "	<< pOrder->ExchangeInstID << endl;
		cout << "报单提交状态 "	<< pOrder->OrderSubmitStatus << endl;
		cout << "报单提示序号 "	<< pOrder->NotifySequence << endl;
		cout << "交易日 "		<< pOrder->TradingDay << endl;
		cout << "报单编号 "		<< pOrder->OrderSysID << endl;
		cout << "报单状态 "		<< pOrder->OrderStatus << endl;
		cout << "今成交数量 "		<< pOrder->VolumeTraded << endl;
		cout << "剩余数量 "		<< pOrder->VolumeTotal << endl;
		cout << "报单日期 "		<< pOrder->InsertDate << endl;
		cout << "报单时间 "		<< pOrder->InsertTime << endl;
		cout << "最后修改时间 "	<< pOrder->UpdateTime << endl;
		cout << "撤销时间 "		<< pOrder->CancelTime << endl;
		cout << "序号 "			<< pOrder->SequenceNo << endl;
		cout << "前置编号 "		<< pOrder->FrontID << endl;
		cout << "会话编号 "		<< pOrder->SessionID << endl;
	}
}

void Trader_Handler::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	if (pTrade) {
		on_response(pTrade);
		PRINT_SUCCESS("--->>> OnRtnTrade %s", pTrade->TradeTime);
		cout << "经纪公司代码 "	<< pTrade->BrokerID << endl;
		cout << "投资者代码 "		<< pTrade->InvestorID << endl;
		cout << "合约代码 "		<< pTrade->InstrumentID << endl;
		cout << "报单引用 "		<< pTrade->OrderRef << endl;
		cout << "用户代码 "		<< pTrade->UserID << endl;
		cout << "交易所代码 "		<< pTrade->ExchangeID << endl;
		cout << "成交编号 "		<< pTrade->TradeID << endl;
		cout << "买卖方向 "		<< pTrade->Direction << endl;
		cout << "报单编号 "		<< pTrade->OrderSysID << endl;
		cout << "会员代码 "		<< pTrade->ParticipantID << endl;
		cout << "客户代码 "		<< pTrade->ClientID << endl;
		cout << "合约在交易所的代码 " << pTrade->ExchangeInstID << endl;
		cout << "开平标志 "		<< pTrade->OffsetFlag << endl;
		cout << "投机套保标志 "	<< pTrade->HedgeFlag << endl;
		cout << "价格 "			<< pTrade->Price << endl;
		cout << "数量 "			<< pTrade->Volume << endl;
		cout << "成交时期 "		<< pTrade->TradeDate << endl;
		cout << "成交时间 "		<< pTrade->TradeTime << endl;
		cout << "本地报单编号 "	<< pTrade->OrderLocalID << endl;
		cout << "序号 "			<< pTrade->SequenceNo << endl;
		cout << "交易日 "		<< pTrade->TradingDay << endl;
	}
}

void Trader_Handler::OnFrontDisconnected(int nReason)
{
	cout << "--->>> " << "OnFrontDisconnected" << endl;
	cout << "--->>> Reason = " << nReason << endl;
}

void Trader_Handler::OnHeartBeatWarning(int nTimeLapse)
{
	cout << "--->>> " << "OnHeartBeatWarning" << endl;
	cout << "--->>> nTimerLapse = " << nTimeLapse << endl;
}

void Trader_Handler::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	IsErrorRspInfo(pRspInfo);
}

bool Trader_Handler::IsMyOrder(CThostFtdcOrderField *pOrder)
{
	return ((pOrder->FrontID == m_trader_info.FrontID) &&
		(pOrder->SessionID == m_trader_info.SessionID) &&
		atoi(pOrder->OrderRef) == m_trader_info.MaxOrderRef);
}

bool Trader_Handler::IsTradingOrder(CThostFtdcOrderField *pOrder)
{
	return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
		(pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
		(pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}

