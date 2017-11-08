#include <time.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>

#include "core/CTP_Handler.h"
#include "core/ThostFtdcUserApiStruct.h"
#include "utils/MyArray.h"
#include "utils/utils.h"
#include "utils/log.h"
#include "utils/MyHash.h"

#define MAX_QUOTE_SIZE 100000

typedef MyArray<CThostFtdcDepthMarketDataField> QuoteArray;
static MyHash<QuoteArray*> Quotes;


CTP_Handler::CTP_Handler(CThostFtdcMdApi * api_, TraderConfig *trader_config)
{
	api = api_;
	m_trader_config = trader_config;
	api->RegisterSpi(this);
	api->RegisterFront(m_trader_config->MD_FRONT);
}

CTP_Handler::~CTP_Handler()
{
	QuoteArray* qarray;
	Quotes.init_iterator();
	while (Quotes.next_used_node(&qarray)) {
		delete qarray;
	}
}

void CTP_Handler::OnFrontConnected()
{
	CThostFtdcReqUserLoginField req = { 0 };
	strcpy(req.BrokerID, m_trader_config->BROKER_ID);
	strcpy(req.UserID, m_trader_config->USER_ID);
	strcpy(req.Password, m_trader_config->PASSWORD);
	int ret = api->ReqUserLogin(&req, ++requestId);
	if (ret == 0)
		PRINT_SUCCESS("User Login Successful!");
}

void CTP_Handler::OnRspError(CThostFtdcRspInfoField *pRspInfo,
                       int nRequestID, bool bIsLast)
{
	bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
	if (bResult)
		PRINT_ERROR("ErrorID = %d, ErrorMsg = %s", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
}

void CTP_Handler::OnHeartBeatWarning(int nTimeLapse)
{
    PRINT_WARN("OnHeartBeatWarning: nTimerLapse = %d", nTimeLapse);
}

void CTP_Handler::OnRspUserLogin(
    CThostFtdcRspUserLoginField *pRspUserLogin,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    std::cout << "OnRspUserLogin>>>|" << std::endl;
    std::cout << "ErrorID>>>|: " << pRspInfo->ErrorID << std::endl;
    std::cout << "TradingDay>>>|: " << pRspUserLogin->TradingDay << std::endl;
    std::cout << "SessionID>>>|: " << pRspUserLogin->SessionID << std::endl;
    std::cout << "FrontID>>>|: " << pRspUserLogin->FrontID << std::endl;
    std::cout << "MaxOrderRef>>>|: " << pRspUserLogin->MaxOrderRef << std::endl;

    if (pRspInfo->ErrorID == 0 && bIsLast) {
		int ret = api->SubscribeMarketData(m_trader_config->INSTRUMENTS, m_trader_config->INSTRUMENT_COUNT);
		if (ret == 0) {
			for(int i = 0; i < m_trader_config->INSTRUMENT_COUNT; i++) {
				char* symbol = m_trader_config->INSTRUMENTS[i];
				if (!Quotes.exist(symbol)) {
					QuoteArray* qarray = new QuoteArray(MAX_QUOTE_SIZE);
					Quotes.insert(symbol, qarray);
					PRINT_SUCCESS("Subscribe %s", symbol);
				}
			}
		}
    }
}

void CTP_Handler::OnRspSubMarketData(
    CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
}

void CTP_Handler::print(CThostFtdcDepthMarketDataField *pDepthMarketData){
     std::cout << pDepthMarketData->TradingDay<<", ";
     std::cout << pDepthMarketData->InstrumentID<<", ";
     std::cout << pDepthMarketData->ExchangeID<<", ";
     std::cout << pDepthMarketData->ExchangeInstID<<", ";
     std::cout << pDepthMarketData->LastPrice<<", ";
     std::cout << pDepthMarketData->PreSettlementPrice<<", ";
     std::cout << pDepthMarketData->PreClosePrice<<", ";
     std::cout << pDepthMarketData->PreOpenInterest<<", ";
     std::cout << pDepthMarketData->OpenPrice<<", ";
     std::cout << pDepthMarketData->HighestPrice<<", ";
     std::cout << pDepthMarketData->LowestPrice<<", ";
     std::cout << pDepthMarketData->Volume<<", ";
     //std::cout << pDepthMarketData->Turnover<<", ";
     //std::cout << pDepthMarketData->OpenInterest<<", ";
     //std::cout << pDepthMarketData->ClosePrice<<", ";
     //std::cout << pDepthMarketData->SettlementPrice<<", ";
     //std::cout << pDepthMarketData->UpperLimitPrice<<", ";
     //std::cout << pDepthMarketData->LowerLimitPrice<<", ";
     //std::cout << pDepthMarketData->PreDelta<<", ";
     //std::cout << pDepthMarketData->CurrDelta<<", ";
     //std::cout << pDepthMarketData->UpdateTime<<", ";
     //std::cout << pDepthMarketData->UpdateMillisec<<", ";
     std::cout << pDepthMarketData->BidPrice1<<", ";
     std::cout << pDepthMarketData->BidVolume1<<", ";
     std::cout << pDepthMarketData->AskPrice1<<", ";
     std::cout << pDepthMarketData->AskVolume1<<", ";
     //std::cout << pDepthMarketData->BidPrice2<<", ";
     //std::cout << pDepthMarketData->BidVolume2<<", ";
     //std::cout << pDepthMarketData->AskPrice2<<", ";
     //std::cout << pDepthMarketData->AskVolume2<<", ";
     //std::cout << pDepthMarketData->BidPrice3<<", ";
     //std::cout << pDepthMarketData->AskPrice3<<", ";
     //std::cout << pDepthMarketData->AskVolume3<<", ";
     //std::cout << pDepthMarketData->BidPrice4<<", ";
     //std::cout << pDepthMarketData->BidVolume4<<", ";
     //std::cout << pDepthMarketData->AskPrice4<<", ";
     //std::cout << pDepthMarketData->AskVolume4<<", ";
     //std::cout << pDepthMarketData->BidPrice5<<", ";
     //std::cout << pDepthMarketData->BidVolume5<<", ";
     //std::cout << pDepthMarketData->AskPrice5<<", ";
     //std::cout << pDepthMarketData->AskVolume5<<", ";
}
void CTP_Handler::OnRtnDepthMarketData(
    CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	char* symbol = pDepthMarketData->InstrumentID;
	if (!Quotes.exist(symbol)) return;
	    
    CThostFtdcDepthMarketDataField& l_quote = Quotes[symbol]->next();
    l_quote = *pDepthMarketData;

    std::cout << "OnRtnDepthMarketData:|| " << l_quote.InstrumentID << ", " << l_quote.UpdateTime << ", " << l_quote.UpdateMillisec << ", " <<
    l_quote.LastPrice <<  std::endl;
    //print(pDepthMarketData);
    PRINT_INFO("%s quote size: %d\n", symbol, Quotes[symbol]->size());
}

void CTP_Handler::start_trading()
{
	api->Init();
	api->Join();
}

void CTP_Handler::OnFrontDisconnected(int nReason)
{
    std::cout << "OnFrontDisconnected: " << nReason << std::endl;
}

