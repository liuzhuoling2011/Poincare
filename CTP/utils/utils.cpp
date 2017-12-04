﻿#include <iconv.h>
#include <stdio.h>
#include <ctype.h>
#include <ctime>
#include <fstream>
#include <sstream>
#include "utils.h"
#include "utils/log.h"
#include "utils/json11.hpp"
#include "core/ThostFtdcTraderApi.h"

using namespace json11;

static const std::map<std::string, char> EXCH_CHAR =
{ { "SZSE", '0' },{ "SSE", '1' },{ "HKEX", '2' },{ "SHFE", 'A' },{ "CFFEX", 'G' },{ "DCE", 'B' },
{ "CZCE", 'C' },{ "FUT_EXCH", 'X' },{ "SGX", 'S' },{ "SGE", 'D' },{ "CBOT", 'F' },{ "CME", 'M' },
{ "LME", 'L' },{ "COMEX", 'O' },{ "NYMEX", 'N' },{ "BLANK_EXCH", '\0' },{ "UNDEFINED_EXCH", 'u' } };

char get_exch_by_name(const char *name)
{
	auto need = EXCH_CHAR.find(name);
	if (need != EXCH_CHAR.end())
		return need->second;
	else
		return '\0';
}

int convert_open_close_flag(char openclose)
{
	if(openclose == '0' || openclose == '1') {
		return openclose - '0';
	}
	else if (openclose == '3' || openclose == '4') {
		return openclose - '1';
	}
	return UNDEFINED_OPEN_CLOSE;
}

ORDER_STATUS convert_status(char status)
{
	switch(status) {
		case 'a': {
			return UNDEFINED_STATUS;
		}
		case '0': {
			return SIG_STATUS_SUCCEED;
		}
		case '1': {
			return SIG_STATUS_PARTED;
		}
		case '3': {
			return SIG_STATUS_ENTRUSTED;
		}
		case '4': {
			return SIG_STATUS_REJECTED;
		}
		case '2':
		case '5': {
			return SIG_STATUS_CANCELED;
		}
	}
	return UNDEFINED_STATUS;
}

ORDER_STATUS get_final_status(ORDER_STATUS pre, ORDER_STATUS cur)
{
	if (cur == SIG_STATUS_SUCCEED || cur == SIG_STATUS_PARTED || cur == SIG_STATUS_ENTRUSTED
		|| cur == SIG_STATUS_PARTED)
		return cur;
	if (cur == SIG_STATUS_CANCELED && pre != SIG_STATUS_ENTRUSTED)
		return SIG_STATUS_REJECTED;
}

#define CHAR_EQUAL_ZERO(a, b, c) do{\
	if (a != b) goto end;\
	if (a == 0) {c = 0; goto end;}\
}while(0)

int my_strcmp(const char * s1, const char * s2)
{
	int ret = -1;
	CHAR_EQUAL_ZERO(s1[0], s2[0], ret);
	CHAR_EQUAL_ZERO(s1[1], s2[1], ret);
	CHAR_EQUAL_ZERO(s1[2], s2[2], ret);
	CHAR_EQUAL_ZERO(s1[3], s2[3], ret);
	CHAR_EQUAL_ZERO(s1[4], s2[4], ret);
	CHAR_EQUAL_ZERO(s1[5], s2[5], ret);
	CHAR_EQUAL_ZERO(s1[6], s2[6], ret);
	CHAR_EQUAL_ZERO(s1[7], s2[7], ret);

	ret = strcmp(s1 + 8, s2 + 8);
end:
	return ret;
}

int strcmp_case(const char * one, const char * two)
{
	char *pOne = (char *)one;
	char *pTwo = (char *)two;
	if (strlen(one) < strlen(two))
		return -1;
	else if (strlen(one) > strlen(two))
		return 1;
	while (*pOne != '\0') {
		if (tolower(*pOne) != tolower(*pTwo))
			return -1;
		pOne++;
		pTwo++;
	}
	return 0;
}

uint64_t my_hash_value(const char *str_key)
{
	char *pt = (char *)str_key;
	unsigned int hash = 1315423911;

	if (pt[0] == 0) goto end;
	hash ^= ((hash << 5) + pt[0] + (hash >> 2));
	if (pt[1] == 0) goto end;
	hash ^= ((hash << 5) + pt[1] + (hash >> 2));
	if (pt[2] == 0) goto end;
	hash ^= ((hash << 5) + pt[2] + (hash >> 2));
	if (pt[3] == 0) goto end;
	hash ^= ((hash << 5) + pt[3] + (hash >> 2));

	if (pt[4] == 0) goto end;
	hash ^= ((hash << 5) + pt[4] + (hash >> 2));
	if (pt[5] == 0) goto end;
	hash ^= ((hash << 5) + pt[5] + (hash >> 2));
	if (pt[6] == 0) goto end;
	hash ^= ((hash << 5) + pt[6] + (hash >> 2));
	if (pt[7] == 0) goto end;
	hash ^= ((hash << 5) + pt[7] + (hash >> 2));

	pt = (char *)(str_key + 8);
	while (*pt != 0) {
		hash ^= ((hash << 5) + *pt + (hash >> 2));
		pt++;
	}

end:
	return hash;
}

int
double_compare(double a, double b, double epsilon)
{
	if (a - b > epsilon)
		return 1;
	else if (a - b < -epsilon)
		return -1;
	else
		return 0;
}

int float_compare(float a, float b, float epsilon)
{
	if (a - b > epsilon)
		return 1;
	else if (a - b < -epsilon)
		return -1;
	else
		return 0;
}

size_t
strlcpy(char * dst, const char * src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';              /* NUL-terminate dst */
		while (*s++);
	}

	return(s - src - 1);    /* count does not include NUL */
}

void
my_strncpy(char *dst, const char *src, size_t siz)
{
	dst[0] = src[0]; if (src[0] == 0) goto end;
	dst[1] = src[1]; if (src[1] == 0) goto end;
	dst[2] = src[2]; if (src[2] == 0) goto end;
	dst[3] = src[3]; if (src[3] == 0) goto end;
	dst[4] = src[4]; if (src[4] == 0) goto end;
	dst[5] = src[5]; if (src[5] == 0) goto end;
	dst[6] = src[6]; if (src[6] == 0) goto end;
	dst[7] = src[7]; if (src[7] == 0) goto end;
	dst[8] = src[8]; if (src[8] == 0) goto end;
	dst[9] = src[9]; if (src[9] == 0) goto end;
	dst[10] = src[10]; if (src[10] == 0) goto end;
	dst[11] = src[11]; if (src[11] == 0) goto end;
	strlcpy(dst + 12, src + 12, siz - 12);
end:
	return;
}

void duplicate_contract(Contract *src, Contract *dest)
{
	dest->exch = src->exch;
	dest->max_pos = src->max_pos;
	dest->max_accum_open_vol = src->max_accum_open_vol;
	dest->single_side_max_pos = src->single_side_max_pos;
	strlcpy(dest->symbol, src->symbol, SYMBOL_LEN);
}

//----------positions related-------
double
avg_px(Position &pos)
{
	if (pos.qty > 0)
		return pos.notional / (double)pos.qty;
	else
		return 0.0;
}

int
position(Contract *cont)
{
	return long_position(cont) - short_position(cont);
}

int 
position(DIRECTION side, Contract * cont)
{
	if (side == ORDER_BUY)
		return long_position(cont);
	else
		return short_position(cont);
}

int
long_position(Contract *cont)
{
	return cont->positions[LONG_OPEN].qty - cont->positions[SHORT_CLOSE].qty;
}

int
short_position(Contract *cont)
{
	return cont->positions[SHORT_OPEN].qty - cont->positions[LONG_CLOSE].qty;
}

double
long_notional(Contract *cont)
{
	return cont->positions[LONG_OPEN].notional + cont->positions[LONG_CLOSE].notional;
}

double
short_notional(Contract *cont)
{
	return cont->positions[SHORT_OPEN].notional + cont->positions[SHORT_CLOSE].notional;
}

double
avg_buy_price(Contract *cont)
{
	return (cont->positions[LONG_OPEN].notional + cont->positions[LONG_CLOSE].notional)
		/ (cont->positions[LONG_OPEN].qty + cont->positions[LONG_CLOSE].qty);
}

double
avg_sell_price(Contract *cont)
{
	return (cont->positions[SHORT_OPEN].notional + cont->positions[SHORT_CLOSE].notional)
		/ (cont->positions[SHORT_OPEN].qty + cont->positions[SHORT_CLOSE].qty);
}

double
get_transaction_fee(Contract *cont, int size, double price, bool flag_close_yes)
{
	double l_fee = 0.0;
	double exchange_fee = 0.0;
	if (flag_close_yes) {
		exchange_fee = cont->yes_exchange_fee;
	}
	else {
		exchange_fee = cont->exchange_fee;
	}
	if (cont->fee_by_lot)
		l_fee = size * (exchange_fee + cont->broker_fee); // Caution, for futures right now, broker fee is 0.0
	else
		l_fee = size * price * (exchange_fee + cont->broker_fee);
	if (cont->exch == SSE)
		l_fee += size * price * cont->acc_transfer_fee;

	return l_fee;
}

double
get_realized_PNL(Contract *cont)
{
	double long_side_PNL = 0.0;
	double short_side_PNL = 0.0;

	int long_pos = MIN(cont->positions[LONG_OPEN].qty, cont->positions[SHORT_CLOSE].qty);
	int short_pos = MIN(cont->positions[SHORT_OPEN].qty, cont->positions[LONG_CLOSE].qty);

	if (long_pos > 0)
		long_side_PNL = long_pos * (avg_px(cont->positions[SHORT_CLOSE]) - avg_px(cont->positions[LONG_OPEN]));

	if (short_pos > 0)
		short_side_PNL = short_pos * (avg_px(cont->positions[SHORT_OPEN]) - avg_px(cont->positions[LONG_CLOSE]));

	return long_side_PNL + short_side_PNL;
}

static uint64_t order_count = 0;

void reset_order_count_for_test() {
	order_count = 0;
}

static char ERROR_MSG[1024];

bool IsErrorRspInfo(CThostFtdcRspInfoField * pRspInfo)
{
	if (pRspInfo == NULL) return true;

	// 如果ErrorID != 0, 说明收到了错误的响应
	if (pRspInfo->ErrorID != 0) {
		code_convert(pRspInfo->ErrorMsg, strlen(pRspInfo->ErrorMsg), ERROR_MSG, 1024);
		PRINT_ERROR("ErrorID = %d, ErrorMsg = %s", pRspInfo->ErrorID, ERROR_MSG);
		return true;
	}
	else
		return false;
}

int process_debug_info(int type, int length, void *data) {
	switch (type) {
	case S_STRATEGY_DEBUG_LOG: {
		printf("[CHECK] LOG: %s\n", (char*)data);
		break;
	}
	case S_PLACE_ORDER_DEFAULT: {
		order_t *l_ord = (order_t *)((st_data_t *)data)->info;
		l_ord->order_id = ++order_count * 10000000000 + l_ord->st_id;
		printf("[CHECK] Send Order: %c %s %d %f %d %d %d %d %d %lld %lld %lld\n", l_ord->exch, l_ord->symbol,
			l_ord->volume, l_ord->price, l_ord->direction, l_ord->open_close,
			l_ord->investor_type, l_ord->order_type, l_ord->time_in_force,
			l_ord->st_id, l_ord->order_id, l_ord->org_ord_id);
		LOG("Send Order: %c %s %d %f %d %d %d %d %d %lld %lld %lld\n", l_ord->exch, l_ord->symbol,
			l_ord->volume, l_ord->price, l_ord->direction, l_ord->open_close,
			l_ord->investor_type, l_ord->order_type, l_ord->time_in_force,
			l_ord->st_id, l_ord->order_id, l_ord->org_ord_id);
		break;
	}
	case S_CANCEL_ORDER_DEFAULT: {
		order_t *l_ord = (order_t *)((st_data_t *)data)->info;
		printf("[CHECK] Cancel Order: %c %s %d %f %d %d %d %d %d %lld %lld %lld\n", l_ord->exch, l_ord->symbol,
			l_ord->volume, l_ord->price, l_ord->direction, l_ord->open_close,
			l_ord->investor_type, l_ord->order_type, l_ord->time_in_force,
			l_ord->st_id, l_ord->order_id, l_ord->org_ord_id);
		LOG("Cancel Order: %c %s %d %f %d %d %d %d %d %lld %lld %lld\n", l_ord->exch, l_ord->symbol,
			l_ord->volume, l_ord->price, l_ord->direction, l_ord->open_close,
			l_ord->investor_type, l_ord->order_type, l_ord->time_in_force,
			l_ord->st_id, l_ord->order_id, l_ord->org_ord_id);
		break;
	}
	case S_STRATEGY_PASS_RSP: {
		st_response_t *l_ord = (st_response_t *)((st_data_t *)data)->info;
		printf("[CHECK] Order Resp: %lld %s %d %d %f %d %d %d %s\n", l_ord->order_id, l_ord->symbol,
			l_ord->direction, l_ord->open_close, l_ord->exe_price, l_ord->exe_volume,
			l_ord->status, l_ord->error_no, l_ord->error_info);
		LOG("Order Resp: %lld %s %d %d %f %d %d %d %s\n", l_ord->order_id, l_ord->symbol,
			l_ord->direction, l_ord->open_close, l_ord->exe_price, l_ord->exe_volume,
			l_ord->status, l_ord->error_no, l_ord->error_info);
		break;
	}
	}
	return 0;
}

int get_time_diff(int time1, int time2)
{
	int time1_hour = time1 / 10000000;
	int time1_min = (time1 / 100000) % 100;
	int time1_sec = ((time1 / 1000) % 100);
	int time2_hour = time2 / 10000000;
	int time2_min = (time2 / 100000) % 100;
	int time2_sec = ((time2 / 1000) % 100);

	int hour_diff = time2_hour - time1_hour;
	int min_diff = time2_min - time1_min;
	int sec_diff = time2_sec - time1_sec;
	return (hour_diff * 3600 + min_diff * 60 + sec_diff);
}

int add_time(int int_time, int seconds)
{
	int time_hour = int_time / 10000000;
	int time_min = (int_time / 100000) % 100;
	int time_sec = ((int_time / 1000) % 100);

	int current_sec = time_hour * 3600 + time_min * 60 + time_sec;
	int sum_sec = current_sec + seconds;

	int time1_hour = sum_sec / 3600;
	int time1_min = (sum_sec % 3600) / 60;
	int time1_sec = ((sum_sec % 3600) % 60);
	return (time1_hour * 10000 + time1_min * 100 + time1_sec) * 1000;
}

int code_convert(char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open("utf-8", "gb2312");
	if (cd == 0) return -1;
	memset(outbuf, 0, outlen);
	if (iconv(cd, pin, &inlen, pout, &outlen) == -1) 
		return -1;
	iconv_close(cd);
	return 0;
}

#define CONFIG_FILE    "./config.json"

void read_config_file(std::string& content)
{
	std::ifstream fin(CONFIG_FILE);
	std::stringstream buffer;
	buffer << fin.rdbuf();
	content = buffer.str();
	fin.close();
}

bool read_json_config(TraderConfig& trader_config) {
	std::string json_config;
	read_config_file(json_config);
	std::string err_msg;
	Json l_json = json11::Json::parse(json_config, err_msg, JsonParse::COMMENTS);
	if (err_msg.length() > 0) {
		PRINT_ERROR("Json parse fail, please check your setting!");
		return false;
	}

	strlcpy(trader_config.QUOTE_FRONT, l_json["QUOTE_FRONT"].string_value().c_str(), 64);
	strlcpy(trader_config.QBROKER_ID, l_json["QBROKER_ID"].string_value().c_str(), 8);
	strlcpy(trader_config.QUSER_ID, l_json["QUSER_ID"].string_value().c_str(), 64);
	strlcpy(trader_config.QPASSWORD, l_json["QPASSWORD"].string_value().c_str(), 64);
	strlcpy(trader_config.TRADER_FRONT, l_json["TRADER_FRONT"].string_value().c_str(), 64);
	strlcpy(trader_config.TBROKER_ID, l_json["TBROKER_ID"].string_value().c_str(), 8);
	strlcpy(trader_config.TUSER_ID, l_json["TUSER_ID"].string_value().c_str(), 64);
	strlcpy(trader_config.TPASSWORD, l_json["TPASSWORD"].string_value().c_str(), 64);
	for (int i = 0; i < 64; i++) {
		trader_config.INSTRUMENTS[i] = (char *)malloc(64 * sizeof(char));
	}
	int instr_count = 0;
	for (auto &l_instr : l_json["INSTRUMENTS"].array_items()) {
		strlcpy(trader_config.INSTRUMENTS[instr_count], l_instr.string_value().c_str(), 64);
		instr_count++;
	}
	
	trader_config.INSTRUMENT_COUNT = instr_count;

	trader_config.STRAT_ID = l_json["STRAT_ID"].int_value();
	strlcpy(trader_config.STRAT_PATH, l_json["STRAT_PATH"].string_value().c_str(), 64);
	strlcpy(trader_config.STRAT_EV, l_json["STRAT_EV"].string_value().c_str(), 64);
}

void free_config(TraderConfig& trader_config) {
	for (int i = 0; i < trader_config.INSTRUMENT_COUNT; i++) {
		free(trader_config.INSTRUMENTS[i]);
	}
}

void convert_quote(CThostFtdcDepthMarketDataField * ctp_quote, Futures_Internal_Book * internal_book) {
	//correct
	internal_book->feed_type = 0;
	strlcpy(internal_book->symbol, ctp_quote->InstrumentID, SYMBOL_LEN);
	//correct
	internal_book->exchange = ctp_quote->ExchangeID[0];
	internal_book->int_time = atoi(ctp_quote->UpdateTime);
	internal_book->pre_close_px = ctp_quote->PreClosePrice;
	internal_book->pre_settle_px = ctp_quote->PreSettlementPrice;
	internal_book->pre_open_interest = ctp_quote->PreOpenInterest;
	internal_book->open_interest = ctp_quote->OpenInterest;
	internal_book->open_px = ctp_quote->OpenPrice;
	internal_book->high_px = ctp_quote->HighestPrice;
	internal_book->low_px = ctp_quote->LowestPrice;
	internal_book->avg_px = ctp_quote->AveragePrice;
	internal_book->last_px = ctp_quote->LastPrice;
	internal_book->bp_array[0] = ctp_quote->BidPrice1;
	internal_book->bp_array[1] = ctp_quote->BidPrice2;
	internal_book->bp_array[2] = ctp_quote->BidPrice3;
	internal_book->bp_array[3] = ctp_quote->BidPrice4;
	internal_book->bp_array[4] = ctp_quote->BidPrice5;

	internal_book->ap_array[0] = ctp_quote->AskPrice1;
	internal_book->ap_array[1] = ctp_quote->AskPrice2;
	internal_book->ap_array[2] = ctp_quote->AskPrice3;
	internal_book->ap_array[3] = ctp_quote->AskPrice4;
	internal_book->ap_array[4] = ctp_quote->AskPrice5;

	internal_book->bv_array[0] = ctp_quote->AskVolume1;
	internal_book->bv_array[1] = ctp_quote->AskVolume2;
	internal_book->bv_array[2] = ctp_quote->AskVolume3;
	internal_book->bv_array[3] = ctp_quote->AskVolume4;
	internal_book->bv_array[4] = ctp_quote->AskVolume5;

	internal_book->av_array[0] = ctp_quote->AskVolume1;
	internal_book->av_array[1] = ctp_quote->AskVolume2;
	internal_book->av_array[2] = ctp_quote->AskVolume3;
	internal_book->av_array[3] = ctp_quote->AskVolume4;
	internal_book->av_array[4] = ctp_quote->AskVolume5;

	internal_book->upper_limit_px = ctp_quote->UpperLimitPrice;
	internal_book->lower_limit_px = ctp_quote->LowerLimitPrice;
	internal_book->close_px = ctp_quote->ClosePrice;
	internal_book->settle_px = ctp_quote->SettlementPrice;

	//internal_book->total_buy_ordsize =
	//internal_book->total_sell_ordsize=
	//internal_book->total_notional=
	//internal_book->total_vol = ctp_quote->

	//internal_book->weighted_buy_px =
	//internal_book->weighted_sell_px=
	//DCE?
	//internal_book->implied_ask_size[0]=
	//internal_book->implied_bid_size[0]=

}

void get_time_record(char *time_str) {
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep); //取得当地时间
	sprintf(time_str, "%d%02d%02d %02d:%02d:%02d", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
}

int get_seconds_from_char_time(char * time_str)
{
	/*Convert int_time e.g 134523 to char time e.g "13:45:23"*/
	int hour = (time_str[0] - '0') * 10 + (time_str[1] - '0');
	int minute = (time_str[3] - '0') * 10 + (time_str[4] - '0');
	int second = (time_str[6] - '0') * 10 + (time_str[7] - '0');
	return hour * 3600 + minute * 60 + second;
}

int reverse_index(uint64_t ord_id) {
	return ord_id / 10000000000;
}