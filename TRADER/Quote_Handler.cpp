#include "Quote_Handler.h"
#include "utils/utils.h"
#include "utils/log.h"
#include "utils/redis_handler.h"
#include "quote_format_define.h"
#include "strategy_interface.h"

static Futures_Internal_Book g_f_book;
static char ERROR_MSG[4096];

extern RedisHandler *g_redis_handler;
RedisList *g_redis_multi_quote = NULL;

static st_data_t g_data;

unsigned long g_start_time, g_end_time;

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
	
	g_data.info = &g_f_book;
}

Quote_Handler::~Quote_Handler()
{
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

	m_md_api->SubscribeMarketData(m_trader_config->INSTRUMENTS, m_trader_config->INSTRUMENT_COUNT);
	if (m_trader_config->QUOTE_TYPE == 2) {
		g_redis_multi_quote = new RedisList(g_redis_handler, m_trader_config->REDIS_MULTI_QUOTE_CACHE);
	}
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
	HP_TIMING_NOW(g_start_time);
	convert_quote(pDepthMarketData, &g_f_book);
    
	PRINT_INFO("time: %d symbol: %s ap1: %f av1: %d bp1: %f bv1:%d",
		g_f_book.int_time, g_f_book.symbol, g_f_book.ap_array[0], g_f_book.av_array[0], g_f_book.bp_array[0], g_f_book.bv_array[0]);
   
	if(m_trader_config->QUOTE_TYPE == 2) {
		char* multi_quote = g_redis_multi_quote->last_item();
		if (multi_quote != NULL) {
			update_future_quote(multi_quote);
		}
		g_redis_multi_quote->freeStr();
	}

	my_on_book(DEFAULT_FUTURE_QUOTE, sizeof(st_data_t), &g_data);
	HP_TIMING_NOW(g_end_time);
	PRINT_INFO("Total time: %ld", g_end_time - g_start_time);
}

void Quote_Handler::OnFrontDisconnected(int nReason)
{
	PRINT_WARN("Quote front disconnected, code: %d", nReason);
}

