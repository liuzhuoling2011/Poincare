#pragma once
#include "ThostFtdcMdApi.h"
#include "ThostFtdcTraderApi.h"
#include "CTP_define.h"
#include "base_define.h"
#include "utils/MyArray.h"
#include "utils/MyHash.h"

class Trader_Handler : public CThostFtdcTraderSpi
{
public:
	Trader_Handler(CThostFtdcTraderApi* TraderApi, TraderConfig* trader_config);
	virtual ~Trader_Handler();

	//当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
	virtual void OnFrontConnected();

	//登录请求响应
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//投资者结算结果确认响应
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//请求查询资金账户响应
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
	//请求查询投资者持仓响应
	virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
	//请求查询成交响应
	virtual void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//请求查询合约响应
	//virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//报单录入请求响应
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//报单操作请求响应
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//报单通知
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);

	//成交通知
	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);
	
	//当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	virtual void OnFrontDisconnected(int nReason);

	//错误应答
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//报单录入错误回报
	virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder,
		CThostFtdcRspInfoField *pRspInfo);

	//报单操作错误回报
	virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction,
		CThostFtdcRspInfoField *pRspInfo);

	
private:
	//投资者结算结果确认
	void ReqSettlementInfo();
	//请求查询资金账户
	void ReqTradingAccount();
	//请求查询合约
	void ReqInstrument(char* symbol);
	//请求查询投资者持仓
	void ReqQryInvestorPositionDetail();
	//请求查询交易
	void ReqQryTrade();

	bool is_my_order(int front_id, int session_id);

public:
	TraderConfig *m_trader_config;
	bool m_init_flag = false;

private:
	CThostFtdcTraderApi* m_trader_api = NULL;
	TraderInfo m_trader_info;
	MyHash<CThostFtdcInputOrderField> *m_orders;
	int m_request_id = 0;
};
