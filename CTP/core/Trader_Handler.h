#pragma once
#include "core/ThostFtdcTraderApi.h"
#include "core/CTP_define.h"
#include "core/base_define.h"
#include "utils/MyArray.h"

class Trader_Handler : public CThostFtdcTraderSpi
{
public:
	Trader_Handler(CThostFtdcTraderApi* TraderApi, TraderConfig* trader_config);
	~Trader_Handler();

	//当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
	virtual void OnFrontConnected();

	//登录请求响应
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//投资者结算结果确认响应
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//请求查询合约保证金率响应
	virtual void OnRspQryInstrumentMarginRate(
		CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//请求查询合约手续费率响应
	virtual void OnRspQryInstrumentCommissionRate(
		CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//查询最大报单数量响应
	virtual void OnRspQueryMaxOrderVolume(
		CThostFtdcQueryMaxOrderVolumeField *pQueryMaxOrderVolume,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//请求查询合约响应
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//请求查询资金账户响应
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//请求查询投资者持仓响应
	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//报单录入请求响应
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//报单操作请求响应
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//错误应答
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	virtual void OnFrontDisconnected(int nReason);

	//心跳超时警告。当长时间未收到报文时，该方法被调用。
	virtual void OnHeartBeatWarning(int nTimeLapse);

	//报单通知
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);

	//成交通知
	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);

	//报单录入请求
	void send_single_order(order_t *order);

	//报单撤销请求
	void cancel_single_order(order_t *order);

	

private:
	//投资者结算结果确认
	void ReqSettlementInfo();
	//请求查询资金账户
	void ReqTradingAccount();
	//请求查询合约
	void ReqInstrument(char* symbol);
	//请求查询合约保证金率
	void ReqQryInstrumentMarginRate(char* symbol);
	//请求查询合约手续费率
	void ReqQryInstrumentCommissionRate(char* symbol);
	//请求查询投资者持仓
	void ReqQryInvestorPositionDetail();
	void ReqQryInvestorPosition();
	//查询最大报单数量请求
	void ReqQueryMaxOrderVolume(char* symbol);
	
	// 是否我的报单回报
	bool IsMyOrder(CThostFtdcOrderField *pOrder);
	// 是否正在交易的报单
	bool IsTradingOrder(CThostFtdcOrderField *pOrder);

private:
	CThostFtdcTraderApi* m_trader_api = NULL;
	TraderConfig *m_trader_config;
	TraderInfo m_trader_info;

	int m_request_id = 0;
	bool m_is_ready = false;

	MyArray<CThostFtdcInvestorPositionDetailField> m_contracts_long;
	MyArray<CThostFtdcInvestorPositionDetailField> m_contracts_short;
	MyArray<CThostFtdcInputOrderField> *m_orders;

	CThostFtdcInputOrderField m_cancel;
	CThostFtdcQryInstrumentField m_req_contract;
};
