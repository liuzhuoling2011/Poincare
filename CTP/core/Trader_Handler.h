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

	//���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
	virtual void OnFrontConnected();

	//��¼������Ӧ
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//Ͷ���߽�����ȷ����Ӧ
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//�����ѯ��Լ����������Ӧ
	virtual void OnRspQryInstrumentCommissionRate(
		CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//��ѯ��󱨵�������Ӧ
	virtual void OnRspQueryMaxOrderVolume(
		CThostFtdcQueryMaxOrderVolumeField *pQueryMaxOrderVolume,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//�����ѯ��Լ��Ӧ
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//�����ѯ�ʽ��˻���Ӧ
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//�����ѯͶ���ֲ߳���Ӧ
	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//����¼��������Ӧ
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//��������������Ӧ
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//����Ӧ��
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ���÷��������á���������������API���Զ��������ӣ��ͻ��˿ɲ�������
	virtual void OnFrontDisconnected(int nReason);

	//������ʱ���档����ʱ��δ�յ�����ʱ���÷��������á�
	virtual void OnHeartBeatWarning(int nTimeLapse);

	//����֪ͨ
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);

	//�ɽ�֪ͨ
	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);

	//����¼������
	void send_single_order(order_t *order);

	//������������
	void cancel_single_order(order_t *order);

private:
	//Ͷ���߽�����ȷ��
	void ReqSettlementInfo();
	//�����ѯ�ʽ��˻�
	void ReqTradingAccount();
	//�����ѯ��Լ
	void ReqInstrument(char* symbol);
	//��ѯ��󱨵���������
	void ReqQueryMaxOrderVolume(char* symbol);
	//�����ѯ��Լ��������
	void ReqQryInstrumentCommissionRate(char* symbol);
	//�����ѯͶ���ֲ߳�
	void ReqQryInvestorPositionDetail();
	void ReqQryInvestorPosition();
	
	// �Ƿ��ҵı����ر�
	bool IsMyOrder(CThostFtdcOrderField *pOrder);
	// �Ƿ����ڽ��׵ı���
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
