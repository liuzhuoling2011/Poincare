#include <time.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>

#include "core/Quote_Handler.h"
#include "core/ThostFtdcUserApiStruct.h"
//#include "utils/MyArray.h"
#include "utils/utils.h"
#include "utils/log.h"
//#include "utils/MyHash.h"
#include "strategy_interface.h"

#define MAX_QUOTE_SIZE 100000

//typedef MyArray<CThostFtdcDepthMarketDataField> QuoteArray;
//static MyHash<QuoteArray*> Quotes;

static Futures_Internal_Book g_f_book;
static st_data_t g_data_t;
static char ERROR_MSG[4096];

Quote_Handler::Quote_Handler(CThostFtdcMdApi *md_api_, Trader_Handler *trader_api_, TraderConfig *trader_config)
{
	m_trader_config = trader_config;
	m_trader_handler = trader_api_;
	g_data_t.info = &g_f_book;

	m_md_api = md_api_;
	m_md_api->RegisterSpi(this);
	m_md_api->RegisterFront(m_trader_config->QUOTE_FRONT);
}

Quote_Handler::~Quote_Handler()
{
	m_md_api->Release();
	/*for(auto iter = Quotes.begin(); iter != Quotes.end(); iter++) {
		delete iter->second;
	}*/
}

void Quote_Handler::OnFrontConnected()
{
	CThostFtdcReqUserLoginField req = { 0 };
	strcpy(req.BrokerID, m_trader_config->QBROKER_ID);
	strcpy(req.UserID, m_trader_config->QUSER_ID);
	strcpy(req.Password, m_trader_config->QPASSWORD);
	m_md_api->ReqUserLogin(&req, ++requestId);
}

void Quote_Handler::OnRspError(CThostFtdcRspInfoField *pRspInfo,
                       int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL && pRspInfo->ErrorID != 0) {
		code_convert(pRspInfo->ErrorMsg, strlen(pRspInfo->ErrorMsg), ERROR_MSG, 4096);
		PRINT_ERROR("ErrorID = %d, ErrorMsg = %s", pRspInfo->ErrorID, ERROR_MSG);
		LOG_LN("ErrorID = %d, ErrorMsg = %s", pRspInfo->ErrorID, ERROR_MSG);
	}
}

void Quote_Handler::OnHeartBeatWarning(int nTimeLapse)
{
    PRINT_WARN("OnHeartBeatWarning: nTimerLapse = %d", nTimeLapse);
}

void Quote_Handler::OnRspUserLogin(
    CThostFtdcRspUserLoginField *pRspUserLogin,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo != NULL && pRspInfo->ErrorID != 0) {
		code_convert(pRspInfo->ErrorMsg, strlen(pRspInfo->ErrorMsg), ERROR_MSG, 4096);
		PRINT_ERROR("ErrorID = %d, ErrorMsg = %s", pRspInfo->ErrorID, ERROR_MSG);
		LOG_LN("ErrorID = %d, ErrorMsg = %s", pRspInfo->ErrorID, ERROR_MSG);
	}

	PRINT_SUCCESS("Login quote front successful!");
}

void Quote_Handler::OnRspSubMarketData(
    CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(pSpecificInstrument) PRINT_SUCCESS("Subscribe %s", pSpecificInstrument->InstrumentID);
}

void Quote_Handler::print(CThostFtdcDepthMarketDataField *pDepthMarketData){
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



void Quote_Handler::OnRtnDepthMarketData(
    CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	//char* symbol = pDepthMarketData->InstrumentID;
	//if (!Quotes.exist(symbol)) return;
	    
    /*CThostFtdcDepthMarketDataField& l_quote = Quotes[symbol]->get_next_free_node();
    l_quote = *pDepthMarketData;*/
	convert_quote(pDepthMarketData, &g_f_book);
    
	PRINT_INFO("time: %d symbol: %s ap1: %f av1: %d bp1: %f bv1:%d",
		g_f_book.int_time, g_f_book.symbol, g_f_book.ap_array[0], g_f_book.av_array[0], g_f_book.bp_array[0], g_f_book.bv_array[0]);
    //print(pDepthMarketData);
    //PRINT_INFO("%s quote size: %d\n", symbol, Quotes[symbol]->size());
	
	my_on_book(DEFAULT_FUTURE_QUOTE, sizeof(st_data_t), &g_data_t);
}

void Quote_Handler::OnFrontDisconnected(int nReason)
{
    std::cout << "OnFrontDisconnected: " << nReason << std::endl;
}

