#pragma once
#include "core/ThostFtdcTraderApi.h"
#include "core/CTP_define.h"
#include "core/base_define.h"
#include "utils/MyArray.h"
#include "utils/MyHash.h"

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

	//����֪ͨ
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);

	//�ɽ�֪ͨ
	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);

private:
	//Ͷ���߽�����ȷ��
	void ReqSettlementInfo();
	//�����ѯ�ʽ��˻�
	void ReqTradingAccount();
	//�����ѯ��Լ
	void ReqInstrument(char* symbol);
	//�����ѯ��Լ��������
	void ReqQryInstrumentCommissionRate(char* symbol);
	//�����ѯͶ���ֲ߳�
	void ReqQryInvestorPositionDetail();
	void ReqQryInvestorPosition();
	//��ʼ������
	void init_strategy();
	//����¼������
	int send_single_order(order_t *order);
	//������������
	int cancel_single_order(order_t *order);
	//���Զ�ʱ����
	int st_idle();

	CThostFtdcInputOrderField& get_order_info(uint64_t order_id);

public:
	TraderConfig *m_trader_config;
private:
	CThostFtdcTraderApi* m_trader_api = NULL;
	TraderInfo m_trader_info;

	int m_request_id = 0;

	MyHash<CThostFtdcInputOrderField> *m_orders;

	CThostFtdcInputOrderField m_cancel;
	CThostFtdcQryInstrumentField m_req_contract;
};
