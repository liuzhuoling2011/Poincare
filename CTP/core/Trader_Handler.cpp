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


char INSTRUMENT_ID[] = "rb1801";	// ��Լ����
TThostFtdcPriceType	LIMIT_PRICE = 15350;	// �۸�
TThostFtdcDirectionType	DIRECTION1 = THOST_FTDC_D_Buy;	// ��������
int iRequestID = 0; // ������

					   // �Ự����
TThostFtdcFrontIDType	FRONT_ID;	//ǰ�ñ��
TThostFtdcSessionIDType	SESSION_ID;	//�Ự���
TThostFtdcOrderRefType	ORDER_REF;	//��������

									// �����ж�
bool IsFlowControl(int iResult)
{
	return ((iResult == -2) || (iResult == -3));
}

Trader_Handler::Trader_Handler(CThostFtdcTraderApi* TraderApi, TraderConfig* trader_config)
{
	m_trader_config = trader_config;
	m_trader_api = TraderApi;
	m_trader_api->RegisterSpi(this);			// ע���¼���
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
		// ����Ự����
		FRONT_ID = pRspUserLogin->FrontID;
		SESSION_ID = pRspUserLogin->SessionID;
		int iNextOrderRef = atoi(pRspUserLogin->MaxOrderRef);
		iNextOrderRef++;
		sprintf(ORDER_REF, "%d", iNextOrderRef);
		///��ȡ��ǰ������
		cerr << "--->>> Trading day = " << m_trader_api->GetTradingDay() << endl;
		
		///Ͷ���߽�����ȷ��
		CThostFtdcSettlementInfoConfirmField req = { 0 };
		strcpy(req.BrokerID, m_trader_config->TBROKER_ID);
		strcpy(req.InvestorID, m_trader_config->TUSER_ID);
		int iResult = m_trader_api->ReqSettlementInfoConfirm(&req, ++iRequestID);
		if (iResult == 0)
			PRINT_SUCCESS("Send settlement request successful!");

		//�����ѯ�ʽ��˻�
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

		  ///�����ѯͶ���ֲ߳�
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
		///�����ѯ��Լ
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
		///����¼������
		//ReqOrderInsert();
	}
}

void Trader_Handler::ReqOrderInsert()
{
	CThostFtdcInputOrderField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, m_trader_config->TBROKER_ID);
	///Ͷ���ߴ���
	strcpy(req.InvestorID, m_trader_config->TUSER_ID);
	///��Լ����
	strcpy(req.InstrumentID, INSTRUMENT_ID);
	///��������
	strcpy(req.OrderRef, ORDER_REF);
	///�û�����
	//	TThostFtdcUserIDType	UserID;
	///�����۸�����: �޼�
	req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	//    req.OrderPriceType = THOST_FTDC_OPT_AnyPrice; //shijia
	///��������: 
	req.Direction = DIRECTION1;
	///��Ͽ�ƽ��־: ����
	// req.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	req.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;//  THOST_FTDC_OF_Close
													 ///���Ͷ���ױ���־
	req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///�۸�
	req.LimitPrice = LIMIT_PRICE;
	//req.LimitPrice = 0; // shijia
	///����: 1
	req.VolumeTotalOriginal = 1;
	///��Ч������: ������Ч
	req.TimeCondition = THOST_FTDC_TC_GFD;
	///GTD����
	//	TThostFtdcDateType	GTDDate;
	///�ɽ�������: �κ�����
	req.VolumeCondition = THOST_FTDC_VC_AV;
	///��С�ɽ���: 1
	req.MinVolume = 1;
	///��������: ����
	req.ContingentCondition = THOST_FTDC_CC_Immediately;
	///ֹ���
	//	TThostFtdcPriceType	StopPrice;
	///ǿƽԭ��: ��ǿƽ
	req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///�Զ������־: ��
	req.IsAutoSuspend = 0;
	///ҵ��Ԫ
	//	TThostFtdcBusinessUnitType	BusinessUnit;
	///������
	//	TThostFtdcRequestIDType	RequestID;
	///�û�ǿ����־: ��
	req.UserForceClose = 0;

	//int iResult = m_trader_api->ReqOrderInsert(&req, ++iRequestID);
	//cerr << "--->>> ����¼������: " << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
}

void Trader_Handler::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "--->>> " << "OnRspOrderInsert" << endl;
	IsErrorRspInfo(pRspInfo, bIsLast);
}

void Trader_Handler::ReqOrderAction(CThostFtdcOrderField *pOrder)
{
	static bool ORDER_ACTION_SENT = false;		//�Ƿ����˱���
	if (ORDER_ACTION_SENT)
		return;

	CThostFtdcInputOrderActionField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, pOrder->BrokerID);
	///Ͷ���ߴ���
	strcpy(req.InvestorID, pOrder->InvestorID);
	///������������
	//	TThostFtdcOrderActionRefType	OrderActionRef;
	///��������
	strcpy(req.OrderRef, pOrder->OrderRef);
	///������
	//	TThostFtdcRequestIDType	RequestID;
	///ǰ�ñ��
	req.FrontID = FRONT_ID;
	///�Ự���
	req.SessionID = SESSION_ID;
	///����������
	//	TThostFtdcExchangeIDType	ExchangeID;
	///�������
	//	TThostFtdcOrderSysIDType	OrderSysID;
	///������־
	req.ActionFlag = THOST_FTDC_AF_Delete;
	///�۸�
	//	TThostFtdcPriceType	LimitPrice;
	///�����仯
	//	TThostFtdcVolumeType	VolumeChange;
	///�û�����
	//	TThostFtdcUserIDType	UserID;
	///��Լ����
	strcpy(req.InstrumentID, pOrder->InstrumentID);

	//int iResult = m_trader_api->ReqOrderAction(&req, ++iRequestID);
	//cerr << "--->>> ������������: " << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
	ORDER_ACTION_SENT = true;
}

void Trader_Handler::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "--->>> " << "OnRspOrderAction" << endl;
	IsErrorRspInfo(pRspInfo, bIsLast);
}

///����֪ͨ
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

///�ɽ�֪ͨ
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