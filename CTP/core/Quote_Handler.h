#pragma once
#include "core/ThostFtdcMdApi.h"
#include "core/CTP_define.h"
#include "core/Trader_Handler.h"

class Quote_Handler : public CThostFtdcMdSpi
{
  public:
	Quote_Handler(CThostFtdcMdApi *md_api_, Trader_Handler *trader_api_, TraderConfig *trader_config);
	~Quote_Handler();

	//当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
	virtual void OnFrontConnected();

	//当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	//@param nReason 错误原因
	//        0x1001 网络读失败
	//        0x1002 网络写失败
	//        0x2001 接收心跳超时
	//        0x2002 发送心跳失败
	//        0x2003 收到错误报文
	virtual void OnFrontDisconnected(int nReason);

	//心跳超时警告。当长时间未收到报文时，该方法被调用。
	//@param nTimeLapse 距离上次接收报文的时间
	virtual void OnHeartBeatWarning(int nTimeLapse);

	//登录请求响应
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
								CThostFtdcRspInfoField *pRspInfo, int nRequestID,
								bool bIsLast);

	//登出请求响应
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout,
								CThostFtdcRspInfoField *pRspInfo, int nRequestID,
								bool bIsLast){};

	//错误应答
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID,
							bool bIsLast);

	//订阅行情应答
	virtual void
	OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
						CThostFtdcRspInfoField *pRspInfo, int nRequestID,
						bool bIsLast);

	//取消订阅行情应答
	virtual void
	OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
						CThostFtdcRspInfoField *pRspInfo, int nRequestID,
						bool bIsLast){};

	//订阅询价应答
	virtual void
	OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
						CThostFtdcRspInfoField *pRspInfo, int nRequestID,
						bool bIsLast){};

	//取消订阅询价应答
	virtual void
	OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
						CThostFtdcRspInfoField *pRspInfo, int nRequestID,
						bool bIsLast){};

	//深度行情通知
	virtual void
	OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);

	//询价通知
	virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp){};

  private:
	int requestId = 0;
	CThostFtdcMdApi *m_md_api;
	Trader_Handler *m_trader_handler;
	TraderConfig *m_trader_config;
    void print(CThostFtdcDepthMarketDataField *pDepthMarketData);
};
