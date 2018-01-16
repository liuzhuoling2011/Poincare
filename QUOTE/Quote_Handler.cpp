#include "Quote_Handler.h"
#include "utils/utils.h"
#include "utils/log.h"
#include "utils/redis_handler.h"

static Futures_Internal_Book g_f_book;
static char ERROR_MSG[4096];

RedisHandler *g_redis_handler = NULL;
RedisList *g_redis_contract = NULL;
RedisSubPub *g_redis_subpub = NULL;
RedisList *g_redis_quote = NULL;
RedisList *g_redis_multi_quote = NULL;

bool update_future_quote(void* multi_quote) {
	Futures_Internal_Book *quote = (Futures_Internal_Book *)multi_quote;
	printf("int_time: %d, symbol: %s, feed_type: %d, exch: %d, pre_close_px: %f, pre_settle_px: %f, pre_open_interest: %f, open_interest: %f "
		"open_px: %f, high_px: %f, low_px: %f, avg_px: %f, last_px: %f "
		"ap1: %f, av1: %d, ap2: %f, av2: %d, ap3: %f, av3: %d, ap4: %f, av4: %d, ap5: %f, av5: %d "
		"bp1: %f, bv1: %d, bp2: %f, bv2: %d, bp3: %f, bv3: %d, bp4: %f, bv4: %d, bp5: %f, bv5: %d "
		"total_vol: %lld, total_notional: %f, upper_limit_px: %f, lower_limit_px: %f, close_px: %f, settle_px: %f\n",
		quote->int_time, quote->symbol, quote->feed_type, quote->exchange, quote->pre_close_px, quote->pre_settle_px, quote->pre_open_interest, quote->open_interest,
		quote->open_px, quote->high_px, quote->low_px, quote->avg_px, quote->last_px,
		quote->ap_array[0], quote->av_array[0], quote->ap_array[1], quote->av_array[1], quote->ap_array[2], quote->av_array[2], quote->ap_array[3], quote->av_array[3], quote->ap_array[4], quote->av_array[4],
		quote->bp_array[0], quote->bv_array[0], quote->bp_array[1], quote->bv_array[1], quote->bp_array[2], quote->bv_array[2], quote->bp_array[3], quote->bv_array[3], quote->bp_array[4], quote->bv_array[4],
		quote->total_vol, quote->total_notional, quote->upper_limit_px, quote->lower_limit_px, quote->close_px, quote->settle_px);

	for (int i = 0; i < 5; i++) {
		g_f_book.bp_array[i] = quote->bp_array[i];
		g_f_book.bv_array[i] = quote->bv_array[i];
		g_f_book.ap_array[i] = quote->ap_array[i];
		g_f_book.av_array[i] = quote->av_array[i];
	}
	return true;
}

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
	g_redis_multi_quote = new RedisList(g_redis_handler, m_trader_config->REDIS_MULTI_QUOTE_CACHE);
}

Quote_Handler::~Quote_Handler()
{
	delete g_redis_handler;
	delete g_redis_contract;
	delete g_redis_subpub;
	delete g_redis_quote;
	delete g_redis_multi_quote;
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
   
	if(m_trader_config->QUOTE_TYPE == 2) {
		char* multi_quote = g_redis_multi_quote->last_item();
		if (multi_quote != NULL) {
			update_future_quote(multi_quote);
		}
	}//else if(m_trader_config->QUOTE_TYPE == 1) { }
	g_redis_quote->rpush_binary((char*)&g_f_book, sizeof(Futures_Internal_Book));
	g_redis_subpub->publish_binary((char*)&g_f_book, sizeof(Futures_Internal_Book));
}

void Quote_Handler::OnFrontDisconnected(int nReason)
{
	PRINT_WARN("Quote front disconnected, code: %d", nReason);
}

