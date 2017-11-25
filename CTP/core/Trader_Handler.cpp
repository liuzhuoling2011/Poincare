#include <iostream>
#include <string.h>
#include "utils/utils.h"
#include "utils/log.h"
#include "strategy_interface.h"
#include "strategy.h"
using namespace std;

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#define sleep Sleep
#endif

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
	for(int i = 0; i < 6; i++) {
		if(info.LoginTime[i] != ':') {
			hour = 10 * hour + info.LoginTime[i];
		}
		break;
	}
	if (hour > 8 && hour < 19)
		g_config_t.day_night = DAY;
	else
		g_config_t.day_night = NIGHT;
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

		

		//请求下单
		//ReqOrderInsert();
		//ReqOrderAction(&m_cancel);
	} else {
		PRINT_ERROR("Login Failed!");
		exit(-1);
	}
}

void Trader_Handler::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	PRINT_SUCCESS("Comform settlement!");
	//请求查询资金账户
	ReqTradingAccount();
}

void Trader_Handler::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField * pInstrumentMarginRate, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pInstrumentMarginRate == NULL) return;
	//todo 填写合约保证金率相关信息
	PRINT_INFO("%s", pInstrumentMarginRate->InstrumentID);
    //g_config_t.contracts[0]
	//g_config_t.contracts[0].fee.broker_fee = ;

	
}

void Trader_Handler::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField * pInstrumentCommissionRate, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pInstrumentCommissionRate == NULL) return;
	//todo 填写手续费率相关信息
	g_config_t.contracts[0].fee.broker_fee = 1;
	g_config_t.contracts[0].fee.exchange_fee = pInstrumentCommissionRate->OpenRatioByMoney;
//	g_config_t.contracts[0].fee.acc_transfer_fee= ;
//	g_config_t.contracts[0].fee.fee_by_lot= ;
//	g_config_t.contracts[0].fee.yes_exchange_fee=;
//	g_config_t.contracts[0].fee.stamp_tax = ;

	PRINT_INFO("%s", pInstrumentCommissionRate->InstrumentID);
	//请求查询合约保证金率
	//ReqQryInstrumentMarginRate(pInstrumentCommissionRate->InstrumentID);
	//查询最大报单数量请求
	ReqQueryMaxOrderVolume(pInstrumentCommissionRate->InstrumentID);
}

void Trader_Handler::OnRspQueryMaxOrderVolume(CThostFtdcQueryMaxOrderVolumeField * pQueryMaxOrderVolume, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pQueryMaxOrderVolume == NULL) return;
	//todo 填写最大开仓量
	PRINT_INFO("%s", pQueryMaxOrderVolume->InstrumentID);
	g_config_t.contracts[0].max_accum_open_vol = pQueryMaxOrderVolume->MaxVolume;

	//请求查询投资者持仓
	ReqQryInvestorPositionDetail();
}

void Trader_Handler::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pInstrument == NULL) return;
	PRINT_DEBUG("%s %s %s %s %s %f", pInstrument->InstrumentID, pInstrument->ExchangeID, pInstrument->ProductID, pInstrument->CreateDate, pInstrument->ExpireDate, pInstrument->PriceTick);
	//todo 补全合约信息，先考虑一个合约

	strlcpy(g_config_t.contracts[0].symbol, pInstrument->InstrumentID, SYMBOL_LEN);
	//correct
	g_config_t.contracts[0].exch=pInstrument->ExchangeID[0];
	//correct
	g_config_t.contracts[0].expiration_date=(int)pInstrument->EndDelivDate;
	g_config_t.contracts[0].tick_size=pInstrument->PriceTick;
//  g_config_t.contracts[0].multiple
//	g_config_t.contracts[0].account
//	g_config_t.contracts[0].max_cancel_limit
//	g_config_t.contracts[0].reserved_data
//	g_config_t.contracts[0].today_pos
//	g_config_t.contracts[0].yesterday_pos

	if(bIsLast == true) {
		//请求查询合约手续费率
		ReqQryInstrumentCommissionRate(pInstrument->InstrumentID);
	}
}


void Trader_Handler::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pTradingAccount == NULL) return;
	PRINT_DEBUG("%s %f %f %f %f %f %f %f", pTradingAccount->AccountID, pTradingAccount->Interest, pTradingAccount->Deposit, pTradingAccount->Withdraw, pTradingAccount->CurrMargin, pTradingAccount->CloseProfit, pTradingAccount->PositionProfit, pTradingAccount->Available);
	//todo 补全账户信息
	strlcpy(g_config_t.accounts[0].account, pTradingAccount->AccountID, ACCOUNT_LEN);
	g_config_t.accounts[0].cash_asset = pTradingAccount->CashIn;
	g_config_t.accounts[0].cash_available = pTradingAccount->Available;
	//g_config_t.accounts[0].currency = pTradingAccount->CurrencyID;
	g_config_t.accounts[0].exch_rate = pTradingAccount->PreBalance;

	//请求查询合约
	for (int i = 0; i < m_trader_config->INSTRUMENT_COUNT; i++) {
		ReqInstrument(m_trader_config->INSTRUMENTS[i]);
	}
}

void Trader_Handler::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	PRINT_INFO("is_last: %d", bIsLast); //todo 过滤重复的
	if (pInvestorPosition) {
		printf("\nInstrumentID: %s, "
			"PosiDirection: %c, "
			"HedgeFlag: %c, "
			"YdPosition: %d, "
			"Position: %d, "
			"LongFrozen: %d, "
			"ShortFrozen: %d, "
			"LongFrozenAmount: %f, "
			"ShortFrozenAmount: %f, "
			"OpenVolume: %d, "
			"CloseVolume: %d, "
			"OpenAmount: %f, "
			"CloseAmount: %f, "
			"PositionCost: %f, "
			"UseMargin: %f, "
			"FrozenMargin: %f, "
			"FrozenCash: %f, "
			"FrozenCommission: %f, "
			"CashIn: %f, "
			"Commission: %f, "
			"CloseProfit: %f, "
			"PositionProfit: %f, "
			"OpenCost: %f, \n"
			"ExchangeMargin: %f, "
			"CloseProfitByDate: %f, "
			"CloseProfitByTrade: %f, "
			"PositionProfit: %f, "
			"TodayPosition: %d, "
			"MarginRateByMoney: %f, "
			"MarginRateByVolume: %f, "
			"StrikeFrozenAmount: %f \n",
			pInvestorPosition->InstrumentID,
			pInvestorPosition->PosiDirection,
			pInvestorPosition->HedgeFlag,
			pInvestorPosition->YdPosition,
			pInvestorPosition->Position,
			pInvestorPosition->LongFrozen,
			pInvestorPosition->ShortFrozen,
			pInvestorPosition->LongFrozenAmount,
			pInvestorPosition->ShortFrozenAmount,
			pInvestorPosition->OpenVolume,
			pInvestorPosition->CloseVolume,
			pInvestorPosition->OpenAmount,
			pInvestorPosition->CloseAmount,
			pInvestorPosition->PositionCost,
			pInvestorPosition->UseMargin,
			pInvestorPosition->FrozenMargin,
			pInvestorPosition->FrozenCash,
			pInvestorPosition->FrozenCommission,
			pInvestorPosition->CashIn,
			pInvestorPosition->Commission,
			pInvestorPosition->CloseProfit,
			pInvestorPosition->PositionProfit,
			pInvestorPosition->OpenCost,
			pInvestorPosition->ExchangeMargin,
			pInvestorPosition->CloseProfitByDate,
			pInvestorPosition->CloseProfitByTrade,
			pInvestorPosition->PositionProfit,
			pInvestorPosition->TodayPosition,
			pInvestorPosition->MarginRateByMoney,
			pInvestorPosition->MarginRateByVolume,
			pInvestorPosition->StrikeFrozenAmount);
	}
	//	PRINT_DEBUG("%s %d %d %d %f", pInvestorPosition->InstrumentID, pInvestorPosition->Position, pInvestorPosition->TodayPosition, pInvestorPosition->YdPosition, pInvestorPosition->UseMargin);
		///报单录入请求
		//ReqOrderInsert();
	//}
}

void Trader_Handler::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField * pInvestorPositionDetail, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pInvestorPositionDetail) {
		//对于所有合约，不保存已平仓的，只保存未平仓的
		if (pInvestorPositionDetail->Volume > 0) {
			if (pInvestorPositionDetail->Direction == '0')
				m_contracts_long.push_back(pInvestorPositionDetail);
			else if (pInvestorPositionDetail->Direction == '1')
				m_contracts_short.push_back(pInvestorPositionDetail);

			bool find_instId = false;
			for(int i = 0; i< m_trader_config->INSTRUMENT_COUNT; i++) {
				if (strcmp(m_trader_config->INSTRUMENTS[i], pInvestorPositionDetail->InstrumentID) == 0) {	//合约已存在，已订阅过行情
					find_instId = true;
					break;
				}
			}
			if (find_instId == false) {
				strlcpy(m_trader_config->INSTRUMENTS[m_trader_config->INSTRUMENT_COUNT], pInvestorPositionDetail->InstrumentID, SYMBOL_LEN);
				m_trader_config->INSTRUMENT_COUNT++;
			}
		}

		if (bIsLast) {
			//ReqQryInvestorPosition();
			//todo 根据日期合并填入contract的yes_pos, today_pos
			/*
			 * long remained contract:3 short remained contract:1
				InvestorID: 106409
				 InstrumentID: rb1805
				 ExchangeID: SHFE
				 Direction: 0
				 OpenPrice: 3711
				 Volume: 1
				 TradingDay: 20171123
				InvestorID: 106409
				 InstrumentID: rb1805
				 ExchangeID: SHFE
				 Direction: 0
				 OpenPrice: 3692
				 Volume: 1
				 TradingDay: 20171123
				InvestorID: 106409
				 InstrumentID: rb1805
				 ExchangeID: SHFE
				 Direction: 0
				 OpenPrice: 3711
				 Volume: 1
				 TradingDay: 20171123
				InvestorID: 106409
				 InstrumentID: rb1805
				 ExchangeID: SHFE
				 Direction: 1
				 OpenPrice: 3793
				 Volume: 1
				 TradingDay: 20171123
			 */
			cout << "long remained contract:" << m_contracts_long.size() << " short remained contract:" << m_contracts_short.size() << endl;
			for (int i = 0; i < m_contracts_long.size(); i++) {
				cout << "InvestorID: " << m_contracts_long[i].InvestorID << endl
					<< " InstrumentID: " << m_contracts_long[i].InstrumentID << endl
					<< " ExchangeID: " << m_contracts_long[i].ExchangeID << endl
					<< " Direction: " << m_contracts_long[i].Direction << endl
					<< " OpenPrice: " << m_contracts_long[i].OpenPrice << endl
					<< " Volume: " << m_contracts_long[i].Volume << endl
					<< " TradingDay: " << m_contracts_long[i].TradingDay << endl;
			}

			for (int i = 0; i < m_contracts_short.size(); i++) {
				cout << "InvestorID: " << m_contracts_short[i].InvestorID << endl
					<< " InstrumentID: " << m_contracts_short[i].InstrumentID << endl
					<< " ExchangeID: " << m_contracts_short[i].ExchangeID << endl
					<< " Direction: " << m_contracts_short[i].Direction << endl
					<< " OpenPrice: " << m_contracts_short[i].OpenPrice << endl
					<< " Volume: " << m_contracts_short[i].Volume << endl
					<< " TradingDay: " << m_contracts_short[i].TradingDay << endl;
			}

			//todo 在这里我们结束了config的配置，开始初始化策略
		}
	}
}

void Trader_Handler::ReqSettlementInfo()
{
	CThostFtdcSettlementInfoConfirmField req = { 0 };
	strcpy(req.BrokerID, m_trader_config->TBROKER_ID);
	strcpy(req.InvestorID, m_trader_config->TUSER_ID);
	int ret = m_trader_api->ReqSettlementInfoConfirm(&req, ++m_request_id);
	if (ret)
		PRINT_ERROR("Send settlement request fail!");
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
		} else {
			sleep(1);
		}
	} // while
}

void Trader_Handler::ReqInstrument(char* symbol) {
	while (true) {
		strcpy(m_req_contract.InstrumentID, symbol);
		int iResult = m_trader_api->ReqQryInstrument(&m_req_contract, ++m_request_id);
		if (!IsFlowControl(iResult)) {
			PRINT_INFO("send query contract: %s %s", m_req_contract.InstrumentID, iResult == 0 ? "success" : "fail");
			break;
		} else {
			sleep(1);
		}
	}
}

void Trader_Handler::ReqQryInstrumentMarginRate(char * symbol)
{
	CThostFtdcQryInstrumentMarginRateField ReqMarginRate = { 0 };
	strcpy(ReqMarginRate.BrokerID, m_trader_config->TBROKER_ID);
	strcpy(ReqMarginRate.InvestorID, m_trader_config->TUSER_ID);
	strcpy(ReqMarginRate.InstrumentID, symbol);
	while (true) {
		//strcpy(m_req_pos.InstrumentID, symbol);
		int iResult = m_trader_api->ReqQryInstrumentMarginRate(&ReqMarginRate, ++m_request_id);
		if (!IsFlowControl(iResult)) {
			PRINT_INFO("send query margin rate %s", iResult == 0 ? "success" : "fail");
			break;
		}
		else {
			sleep(1);
		}
	}
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
		} else {
			sleep(1);
		}
	}
}

void Trader_Handler::ReqQueryMaxOrderVolume(char* symbol)
{
	CThostFtdcQueryMaxOrderVolumeField ReqMaxOrdSize = { 0 };
	strcpy(ReqMaxOrdSize.BrokerID, m_trader_config->TBROKER_ID);
	strcpy(ReqMaxOrdSize.InvestorID, m_trader_config->TUSER_ID);
	strcpy(ReqMaxOrdSize.InstrumentID, symbol);
	while (true) {
		//strcpy(m_req_pos.InstrumentID, symbol);
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
	if(order->order_type == ORDER_TYPE_LIMIT)
		g_order_t.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	else
		g_order_t.OrderPriceType = THOST_FTDC_OPT_AnyPrice; //市价单
	///买卖方向: 
	g_order_t.Direction = order->direction == ORDER_BUY ? '0':'1';
	///组合开平标志: 开仓
	if(order->open_close == ORDER_OPEN)
		g_order_t.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	else if(order->open_close == ORDER_CLOSE)
		g_order_t.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
	else if (order->open_close == ORDER_CLOSE_YES)
		g_order_t.CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday;

	///组合投机套保标志
	if(order->investor_type == ORDER_SPECULATOR)
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
	if(order->time_in_force == ORDER_TIF_DAY || order->time_in_force == ORDER_TIF_GTD)
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

void Trader_Handler::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cout << "--->>> " << "OnRspOrderInsert" << endl;
	IsErrorRspInfo(pRspInfo);
}

void Trader_Handler::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cout << "--->>> " << "OnRspOrderAction" << endl;
	IsErrorRspInfo(pRspInfo);
}

///报单通知
void Trader_Handler::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	cout << "--->>> " << "OnRtnOrder" << endl;
	if (IsMyOrder(pOrder))
	{
		if (pOrder) {
			printf("[OnRtnOrder]  Inv_ID: %s,symbol:%s ,Ref: %d, LocalID: %d, O_Sys_ID: %d, OrderStatus: %c,InsertTime:%s,VolumeTraded:%d,VolumeTotal:%d, Dir:%c, open_close:%c,price:%f ,vol:%d,Broker_Seq:%d \n",
				pOrder->InvestorID,
				pOrder->InstrumentID,
				atoi(pOrder->OrderRef),
				atoi(pOrder->OrderLocalID),
				atoi(pOrder->OrderSysID),
				pOrder->OrderStatus,
				pOrder->InsertTime,
				pOrder->VolumeTraded,
				pOrder->VolumeTotal,
				pOrder->Direction,
				pOrder->CombOffsetFlag[0],
				pOrder->LimitPrice,
				pOrder->VolumeTotalOriginal,
				pOrder->BrokerOrderSeq);
		}
		/*if (IsTradingOrder(pOrder))
			ReqOrderAction(pOrder);
		else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
			cout << "--->>> cancel" << endl;
	*/}
}

///成交通知
void Trader_Handler::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	if (pTrade) {
		on_response(pTrade);
		printf("[OnRtnTrade] Inv_ID:%s ,Ref: %d, symbol:%s ,exhg_ID:%s,LocalID: %d,O_Sys_ID:%d,Dir:%c,open_close:%c, price:%f,deal_vol:%d, TradeTime:%s, TradeID:%s,Broker_Seq:%d \n",
			pTrade->InvestorID,
			atoi(pTrade->OrderRef),
			pTrade->InstrumentID,
			pTrade->ExchangeID,
			atoi(pTrade->OrderLocalID),
			atoi(pTrade->OrderSysID),
			pTrade->Direction,
			pTrade->OffsetFlag,
			pTrade->Price,
			pTrade->Volume,
			pTrade->TradeTime,
			pTrade->TradeID,
			pTrade->BrokerOrderSeq);
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
