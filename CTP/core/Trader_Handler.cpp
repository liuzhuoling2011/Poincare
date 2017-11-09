#include <iostream>
#include <string.h>
#include "utils/utils.h"
#include "utils/log.h"
using namespace std;

#include "Trader_Handler.h"
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#define sleep Sleep
#endif


char INSTRUMENT_ID[] = "rb1801";	// 合约代码
TThostFtdcPriceType	LIMIT_PRICE = 15350;	// 价格
TThostFtdcDirectionType	DIRECTION1 = THOST_FTDC_D_Buy;	// 买卖方向
int iRequestID = 0; // 请求编号

					   // 会话参数
TThostFtdcFrontIDType	FRONT_ID;	//前置编号
TThostFtdcSessionIDType	SESSION_ID;	//会话编号
TThostFtdcOrderRefType	ORDER_REF;	//报单引用

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
}

void Trader_Handler::OnFrontConnected()
{
	CThostFtdcReqUserLoginField req = { 0 };
	strcpy(req.BrokerID, m_trader_config->TBROKER_ID);
	strcpy(req.UserID, m_trader_config->TUSER_ID);
	strcpy(req.Password, m_trader_config->TPASSWORD);
	m_trader_api->ReqUserLogin(&req, ++iRequestID);
}


void Trader_Handler::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo, bIsLast))
	{
		PRINT_SUCCESS("Login trader front successful!");
		// 保存会话参数
		FRONT_ID = pRspUserLogin->FrontID;
		SESSION_ID = pRspUserLogin->SessionID;
		int iNextOrderRef = atoi(pRspUserLogin->MaxOrderRef);
		iNextOrderRef++;
		sprintf(ORDER_REF, "%d", iNextOrderRef);
		///获取当前交易日
		cerr << "--->>> Trading day = " << m_trader_api->GetTradingDay() << endl;
		
		///投资者结算结果确认
		CThostFtdcSettlementInfoConfirmField req = { 0 };
		strcpy(req.BrokerID, m_trader_config->TBROKER_ID);
		strcpy(req.InvestorID, m_trader_config->TUSER_ID);
		int iResult = m_trader_api->ReqSettlementInfoConfirm(&req, ++iRequestID);
		if (iResult == 0)
			PRINT_SUCCESS("Send settlement request successful!");

		//请求查询资金账户
		CThostFtdcQryTradingAccountField requ = { 0 };
		strcpy(requ.BrokerID, m_trader_config->TBROKER_ID);
		strcpy(requ.InvestorID, m_trader_config->TUSER_ID);
		while (true) {
			int iResult = m_trader_api->ReqQryTradingAccount(&requ, ++iRequestID);
			if (!IsFlowControl(iResult)) {
				PRINT_INFO("send query account: %s %s", requ.InvestorID, iResult == 0 ? "success" : "fail");
				break;
			} else {
				PRINT_WARN("query account %s fail, wait!", requ.InvestorID);
				sleep(1);
			}
		} // while

		  ///请求查询投资者持仓
		CThostFtdcQryInvestorPositionField req1 = { 0 };
		strcpy(req1.BrokerID, m_trader_config->TBROKER_ID);
		strcpy(req1.InvestorID, m_trader_config->TUSER_ID);
		for (int i = 0; i < m_trader_config->INSTRUMENT_COUNT; i++) {
			strcpy(req1.InstrumentID, m_trader_config->INSTRUMENTS[i]);
			while (true) {
				int iResult = m_trader_api->ReqQryInvestorPosition(&req1, ++iRequestID);
				if (!IsFlowControl(iResult)) {
					PRINT_INFO("send query position: %s %s", req1.InstrumentID, iResult == 0 ? "success" : "fail");
					break;
				} else {
					PRINT_WARN("query position %s fail, wait!", req1.InstrumentID);
					sleep(1);
				}
			} // while 
		}
	}
}

void Trader_Handler::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo, bIsLast)) {
		///请求查询合约
		CThostFtdcQryInstrumentField req = { 0 };
		for(int i=0; i < m_trader_config->INSTRUMENT_COUNT; i++) {
			while(true) {
				strcpy(req.InstrumentID, m_trader_config->INSTRUMENTS[i]);
				int iResult = m_trader_api->ReqQryInstrument(&req, ++iRequestID);
				if (!IsFlowControl(iResult)) {
					PRINT_INFO("send query contract: %s %s", req.InstrumentID, iResult == 0 ? "success" : "fail");
					break;
				} else {
					PRINT_WARN("query contract %s fail, wait!", req.InstrumentID);
					sleep(1);
				}
			}
		}
	}
}

void Trader_Handler::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo, bIsLast))
	{
		PRINT_DEBUG("%s %s %s %s %s %f", pInstrument->InstrumentID, pInstrument->ExchangeID, pInstrument->ProductID, pInstrument->CreateDate, pInstrument->ExpireDate, pInstrument->PriceTick);
	}
}


void Trader_Handler::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo, bIsLast))
	{
		PRINT_DEBUG("%s %f %f %f %f %f %f %f", pTradingAccount->AccountID, pTradingAccount->Interest, pTradingAccount->Deposit, pTradingAccount->Withdraw, pTradingAccount->CurrMargin, pTradingAccount->CloseProfit, pTradingAccount->PositionProfit, pTradingAccount->Available);

		
	}
}

void Trader_Handler::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo, bIsLast))
	{
		PRINT_DEBUG("%s %d %d %d %f", pInvestorPosition->InstrumentID, pInvestorPosition->Position, pInvestorPosition->TodayPosition, pInvestorPosition->YdPosition, pInvestorPosition->UseMargin);
		///报单录入请求
		//ReqOrderInsert();
	}
}

void Trader_Handler::ReqOrderInsert()
{
	CThostFtdcInputOrderField req;
	memset(&req, 0, sizeof(req));
	///经纪公司代码
	strcpy(req.BrokerID, m_trader_config->TBROKER_ID);
	///投资者代码
	strcpy(req.InvestorID, m_trader_config->TUSER_ID);
	///合约代码
	strcpy(req.InstrumentID, INSTRUMENT_ID);
	///报单引用
	strcpy(req.OrderRef, ORDER_REF);
	///用户代码
	//	TThostFtdcUserIDType	UserID;
	///报单价格条件: 限价
	req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	//    req.OrderPriceType = THOST_FTDC_OPT_AnyPrice; //shijia
	///买卖方向: 
	req.Direction = DIRECTION1;
	///组合开平标志: 开仓
	// req.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	req.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;//  THOST_FTDC_OF_Close
													 ///组合投机套保标志
	req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///价格
	req.LimitPrice = LIMIT_PRICE;
	//req.LimitPrice = 0; // shijia
	///数量: 1
	req.VolumeTotalOriginal = 1;
	///有效期类型: 当日有效
	req.TimeCondition = THOST_FTDC_TC_GFD;
	///GTD日期
	//	TThostFtdcDateType	GTDDate;
	///成交量类型: 任何数量
	req.VolumeCondition = THOST_FTDC_VC_AV;
	///最小成交量: 1
	req.MinVolume = 1;
	///触发条件: 立即
	req.ContingentCondition = THOST_FTDC_CC_Immediately;
	///止损价
	//	TThostFtdcPriceType	StopPrice;
	///强平原因: 非强平
	req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///自动挂起标志: 否
	req.IsAutoSuspend = 0;
	///业务单元
	//	TThostFtdcBusinessUnitType	BusinessUnit;
	///请求编号
	//	TThostFtdcRequestIDType	RequestID;
	///用户强评标志: 否
	req.UserForceClose = 0;

	//int iResult = m_trader_api->ReqOrderInsert(&req, ++iRequestID);
	//cerr << "--->>> 报单录入请求: " << iResult << ((iResult == 0) ? ", 成功" : ", 失败") << endl;
}

void Trader_Handler::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "--->>> " << "OnRspOrderInsert" << endl;
	IsErrorRspInfo(pRspInfo, bIsLast);
}

void Trader_Handler::ReqOrderAction(CThostFtdcOrderField *pOrder)
{
	static bool ORDER_ACTION_SENT = false;		//是否发送了报单
	if (ORDER_ACTION_SENT)
		return;

	CThostFtdcInputOrderActionField req;
	memset(&req, 0, sizeof(req));
	///经纪公司代码
	strcpy(req.BrokerID, pOrder->BrokerID);
	///投资者代码
	strcpy(req.InvestorID, pOrder->InvestorID);
	///报单操作引用
	//	TThostFtdcOrderActionRefType	OrderActionRef;
	///报单引用
	strcpy(req.OrderRef, pOrder->OrderRef);
	///请求编号
	//	TThostFtdcRequestIDType	RequestID;
	///前置编号
	req.FrontID = FRONT_ID;
	///会话编号
	req.SessionID = SESSION_ID;
	///交易所代码
	//	TThostFtdcExchangeIDType	ExchangeID;
	///报单编号
	//	TThostFtdcOrderSysIDType	OrderSysID;
	///操作标志
	req.ActionFlag = THOST_FTDC_AF_Delete;
	///价格
	//	TThostFtdcPriceType	LimitPrice;
	///数量变化
	//	TThostFtdcVolumeType	VolumeChange;
	///用户代码
	//	TThostFtdcUserIDType	UserID;
	///合约代码
	strcpy(req.InstrumentID, pOrder->InstrumentID);

	//int iResult = m_trader_api->ReqOrderAction(&req, ++iRequestID);
	//cerr << "--->>> 报单操作请求: " << iResult << ((iResult == 0) ? ", 成功" : ", 失败") << endl;
	ORDER_ACTION_SENT = true;
}

void Trader_Handler::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "--->>> " << "OnRspOrderAction" << endl;
	IsErrorRspInfo(pRspInfo, bIsLast);
}

///报单通知
void Trader_Handler::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	cerr << "--->>> " << "OnRtnOrder" << endl;
	if (IsMyOrder(pOrder))
	{
		if (IsTradingOrder(pOrder))
			ReqOrderAction(pOrder);
		else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
			cout << "--->>> cancel" << endl;
	}
}

///成交通知
void Trader_Handler::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	cerr << "--->>> " << "OnRtnTrade" << endl;
}

void Trader_Handler::OnFrontDisconnected(int nReason)
{
	cerr << "--->>> " << "OnFrontDisconnected" << endl;
	cerr << "--->>> Reason = " << nReason << endl;
}

void Trader_Handler::OnHeartBeatWarning(int nTimeLapse)
{
	cerr << "--->>> " << "OnHeartBeatWarning" << endl;
	cerr << "--->>> nTimerLapse = " << nTimeLapse << endl;
}

void Trader_Handler::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	IsErrorRspInfo(pRspInfo, bIsLast);
}

bool Trader_Handler::IsMyOrder(CThostFtdcOrderField *pOrder)
{
	return ((pOrder->FrontID == FRONT_ID) &&
		(pOrder->SessionID == SESSION_ID) &&
		(strcmp(pOrder->OrderRef, ORDER_REF) == 0));
}

bool Trader_Handler::IsTradingOrder(CThostFtdcOrderField *pOrder)
{
	return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
		(pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
		(pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}