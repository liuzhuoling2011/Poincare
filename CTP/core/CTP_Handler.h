#pragma once
#include "core/ThostFtdcMdApi.h"

struct TraderConfig
{
	char  MD_FRONT[64];
	char  BROKER_ID[8];
	char  USER_ID[64];
	char  PASSWORD[64];
	char *INSTRUMENTS[64];
	int   INSTRUMENT_COUNT;
};

class CTP_Handler : public CThostFtdcMdSpi
{
  public:
	CTP_Handler(CThostFtdcMdApi *api_, TraderConfig *trader_config);
	~CTP_Handler();

	///���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
	virtual void OnFrontConnected();

	///���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ���÷��������á���������������API���Զ��������ӣ��ͻ��˿ɲ�������
	///@param nReason ����ԭ��
	///        0x1001 �����ʧ��
	///        0x1002 ����дʧ��
	///        0x2001 ����������ʱ
	///        0x2002 ��������ʧ��
	///        0x2003 �յ�������
	virtual void OnFrontDisconnected(int nReason);

	///������ʱ���档����ʱ��δ�յ�����ʱ���÷��������á�
	///@param nTimeLapse �����ϴν��ձ��ĵ�ʱ��
	virtual void OnHeartBeatWarning(int nTimeLapse);

	///��¼������Ӧ
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
								CThostFtdcRspInfoField *pRspInfo, int nRequestID,
								bool bIsLast);

	///�ǳ�������Ӧ
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout,
								CThostFtdcRspInfoField *pRspInfo, int nRequestID,
								bool bIsLast){};

	///����Ӧ��
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID,
							bool bIsLast);

	///��������Ӧ��
	virtual void
	OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
						CThostFtdcRspInfoField *pRspInfo, int nRequestID,
						bool bIsLast);

	///ȡ����������Ӧ��
	virtual void
	OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
						CThostFtdcRspInfoField *pRspInfo, int nRequestID,
						bool bIsLast){};

	///����ѯ��Ӧ��
	virtual void
	OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
						CThostFtdcRspInfoField *pRspInfo, int nRequestID,
						bool bIsLast){};

	///ȡ������ѯ��Ӧ��
	virtual void
	OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
						CThostFtdcRspInfoField *pRspInfo, int nRequestID,
						bool bIsLast){};

	///�������֪ͨ
	virtual void
	OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);

	///ѯ��֪ͨ
	virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp){};

	void start_trading();

  private:
    CThostFtdcMdApi *api;
    int requestId = 0;
	TraderConfig *m_trader_config;
    void print(CThostFtdcDepthMarketDataField *pDepthMarketData);
};
