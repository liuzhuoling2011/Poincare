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

	//���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
	virtual void OnFrontConnected();

	//��¼������Ӧ
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//Ͷ���߽�����ȷ����Ӧ
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//�����ѯ�ʽ��˻���Ӧ
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
	//�����ѯ��Լ��Ӧ
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//�����ѯͶ���ֲ߳���Ӧ
	virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
	//�����ѯ������Ӧ
	virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//�����ѯ�ɽ���Ӧ
	virtual void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
	//���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ���÷��������á���������������API���Զ��������ӣ��ͻ��˿ɲ�������
	virtual void OnFrontDisconnected(int nReason);

	//����Ӧ��
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

private:
	//Ͷ���߽�����ȷ��
	void ReqSettlementInfo();
	//�����ѯ�ʽ��˻�
	void ReqTradingAccount();
	//���������߼�
	void ReqManager();
	//�����ѯ��Լ
	void ReqInstrument();
	//�����ѯͶ���ֲ߳�
	void ReqQryInvestorPositionDetail();
	//�����ѯ����
	void ReqQryOrder();
	//�����ѯ����
	void ReqQryTrade();
	//��������
	void EndProcess();
	

public:
	TraderConfig *m_trader_config;

private:
	CThostFtdcTraderApi* m_trader_api = NULL;
	TraderInfo m_trader_info;
	int m_request_id = 0;
	bool m_init_flag = false;
};
