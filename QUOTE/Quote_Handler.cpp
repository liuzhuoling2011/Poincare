#include "Quote_Handler.h"
#include "utils/utils.h"
#include "utils/log.h"
#include "utils/redis_handler.h"
#include "strategy_interface.h"

static Futures_Internal_Book g_f_book;
static char ERROR_MSG[4096];

RedisHandler *g_redis_handler = NULL;
RedisList *g_redis_contract = NULL;
RedisSubPub *g_redis_subpub = NULL;
RedisList *g_redis_quote = NULL;

Quote_Handler::Quote_Handler(CThostFtdcMdApi *md_api_, TraderConfig *trader_config)
{
	m_trader_config = trader_config;

	m_md_api = md_api_;
	m_md_api->RegisterSpi(this);
	m_md_api->RegisterFront(m_trader_config->QUOTE_FRONT);
	
	g_redis_handler = new RedisHandler(m_trader_config->REDIS_IP, m_trader_config->REDIS_PORT);
	g_redis_contract = new RedisList(g_redis_handler, m_trader_config->REDIS_CONTRACT);
	g_redis_subpub = new RedisSubPub(g_redis_handler, m_trader_config->REDIS_QUOTE);
	g_redis_quote = new RedisList(g_redis_handler, m_trader_config->REDIS_QUOTE_CACHE);
}

Quote_Handler::~Quote_Handler()
{
	delete g_redis_handler;
	delete g_redis_contract;
	delete g_redis_quote;
	m_md_api->Release();
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
	
	printf("Wait for contract info from redis...\n");
	char* contract = g_redis_contract->blpop();
	int begin = 0, instr_count = 0;
	for (int i = 0; i < strlen(contract); i++) {
		if(contract[i] == ',') {
			m_trader_config->INSTRUMENTS[instr_count] = (char *)malloc(64 * sizeof(char));
			strlcpy(m_trader_config->INSTRUMENTS[instr_count], contract + begin, i - begin + 1);
			instr_count++;
			begin = i + 1;
		}
	}

	m_trader_config->INSTRUMENTS[instr_count] = (char *)malloc(64 * sizeof(char));
	strlcpy(m_trader_config->INSTRUMENTS[instr_count], contract + begin, 64);
	instr_count++;

	m_trader_config->INSTRUMENT_COUNT = instr_count;
	g_redis_contract->freeStr();

	/*for(int i=0; i<m_trader_config->INSTRUMENT_COUNT; i++) {
		printf("%s\n", m_trader_config->INSTRUMENTS[i]);
	}*/

	m_md_api->SubscribeMarketData(m_trader_config->INSTRUMENTS, m_trader_config->INSTRUMENT_COUNT);
}

void Quote_Handler::OnRspSubMarketData(
    CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(pSpecificInstrument) PRINT_SUCCESS("Subscribe %s market data success!", pSpecificInstrument->InstrumentID);
}

void Quote_Handler::OnRtnDepthMarketData(
    CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	convert_quote(pDepthMarketData, &g_f_book);
    
	PRINT_INFO("time: %d symbol: %s ap1: %f av1: %d bp1: %f bv1:%d",
		g_f_book.int_time, g_f_book.symbol, g_f_book.ap_array[0], g_f_book.av_array[0], g_f_book.bp_array[0], g_f_book.bv_array[0]);
   
	g_redis_quote->rpush_binary((char*)&g_f_book, sizeof(Futures_Internal_Book));
	g_redis_subpub->publish_binary((char*)&g_f_book, sizeof(Futures_Internal_Book));
	//my_on_book(DEFAULT_FUTURE_QUOTE, sizeof(st_data_t), &g_data_t);
}

void Quote_Handler::OnFrontDisconnected(int nReason)
{
	PRINT_WARN("Quote front disconnected, code: %d", nReason);
}

