#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include "utils/log.h"
#include "strategy_interface.h"
#include "quote_format_define.h"
#include "core/Trader_Handler.h"

char g_strategy_path[256];
Trader_Handler* g_trader_handler;

const static char STATUS[][64] = { "SUCCEED", "ENTRUSTED", "PARTED", "CANCELED", "REJECTED", "CANCEL_REJECTED", "INTRREJECTED", "UNDEFINED_STATUS" };
const static char OPEN_CLOSE_STR[][16] = { "OPEN", "CLOSE", "CLOSE_TOD", "CLOSE_YES" };
const static char BUY_SELL_STR[][8] = { "BUY", "SELL" };

#define MAX_LOG_BUFF_SIZE 1024 * 1024 * 8
static FILE*      st_log_handle = NULL;
static char       st_log_buffer[MAX_LOG_BUFF_SIZE + 1];
static char*      st_start_point = st_log_buffer;
static int        st_log_buffer_len = 0;
extern int        int_time;

void st_flush_log()
{
	if (st_log_handle != NULL) {
		fwrite(st_log_buffer, st_log_buffer_len, 1, st_log_handle);
		fflush(st_log_handle);
	}
	st_log_buffer[0] = '\0';
	st_start_point = st_log_buffer;
	st_log_buffer_len = 0;

	flush_log(); // flush treder log
}

int process_strategy_order(int type, int length, void *data) {
	int ret = 0;
	switch (type) {
		case S_PLACE_ORDER_DEFAULT: {
			order_t *ord = (order_t*)((st_data_t*)data)->info;

			LOG_LN("Send Order: %c %s %d %f %s %s %d %d %d %lld %lld %lld", ord->exch, ord->symbol,
				ord->volume, ord->price, BUY_SELL_STR[ord->direction], OPEN_CLOSE_STR[ord->open_close],
				ord->investor_type, ord->order_type, ord->time_in_force,
				ord->st_id, ord->order_id, ord->org_ord_id);

			ret = g_trader_handler->send_single_order(ord);
			break;
		}
		case S_CANCEL_ORDER_DEFAULT: {
			order_t *ord = (order_t*)((st_data_t*)data)->info;

			LOG_LN("Cancel Order: %c %s %d %f %s %s %d %d %d %lld %lld %lld", ord->exch, ord->symbol,
				ord->volume, ord->price, BUY_SELL_STR[ord->direction], OPEN_CLOSE_STR[ord->open_close],
				ord->investor_type, ord->order_type, ord->time_in_force,
				ord->st_id, ord->order_id, ord->org_ord_id);

			ret = g_trader_handler->cancel_single_order(ord);
			break;
		}
	}
	st_flush_log();
	return ret;
}

int process_strategy_info(int type, int length, void *data) {
	switch (type) {
		case S_STRATEGY_DEBUG_LOG: {
			if (st_log_handle == NULL) return 1;
			if (length + st_log_buffer_len >= MAX_LOG_BUFF_SIZE)
				st_flush_log();

			strncpy(st_start_point, (char*)data, length);
			if (length > 0) {
				st_start_point += length;
				st_log_buffer_len += length;
			}
			break;
		}
	}
	return 0;
}

// top process resp level do nothing
int process_strategy_resp(int type, int length, void *data) {
	return 0;
}

typedef int(*st_data_func_t)(int type, int length, void *data);
typedef int(*st_none_func_t)();

void          *strategy_handler;
st_data_func_t st_init;
st_data_func_t on_book;
st_data_func_t on_response;
st_data_func_t on_timer;
st_none_func_t st_destroy;

void load_strategy() {
	strategy_handler = dlopen(g_strategy_path, RTLD_LAZY);
	if (strategy_handler == NULL) {
		char err_msg[256];
		snprintf(err_msg, sizeof(err_msg), "Strategy dlopen failed: %s, %s", g_strategy_path, dlerror());
		PRINT_ERROR("load strategy failed! %s", err_msg);
	}
	st_init = (st_data_func_t)dlsym(strategy_handler, "my_st_init");
	on_book = (st_data_func_t)dlsym(strategy_handler, "my_on_book");
	on_response = (st_data_func_t)dlsym(strategy_handler, "my_on_response");
	on_timer = (st_data_func_t)dlsym(strategy_handler, "my_on_timer");
	st_destroy = (st_none_func_t)dlsym(strategy_handler, "my_destroy");
}

void log_config(st_config_t* config) {
	LOG_LN("trading_date: %d, day_night: %d, param_file_path: %s, output_file_path: %s, vst_id: %d, st_name: %s",
		config->trading_date, config->day_night, config->param_file_path, config->output_file_path, config->vst_id, config->st_name);

	for (int i = 0; i < ACCOUNT_MAX; i++) {
		if (config->accounts[i].account[0] == '\0') break;
		account_t& l_account = config->accounts[i];
		LOG_LN("account: %s, cash_available: %f, cash_asset: %f, exch_rate: %f, currency: %d",
			l_account.account, l_account.cash_available, l_account.cash_asset, l_account.exch_rate, l_account.currency);
	}
	for (int i = 0; i < SYMBOL_MAX; i++) {
		if (config->contracts[i].symbol[0] == '\0') break;
		contract_t& l_config_instr = config->contracts[i];
		LOG_LN("symbol: %s, account: %s, exch: %c, max_accum_open_vol: %d, max_cancel_limit: %d, expiration_date: %d, "
			"today_long_pos: %d, today_long_price: %f, today_short_pos: %d, today_short_price: %f, "
			"yesterday_long_pos: %d, yesterday_long_price: %f, yesterday_short_pos: %d, yesterday_short_price: %f, "
			"fee_by_lot: %d, exchange_fee : %f, yes_exchange_fee : %f, broker_fee : %f, stamp_tax : %f, acc_transfer_fee : %f, tick_size : %f, multiplier : %f",
			l_config_instr.symbol, l_config_instr.account, l_config_instr.exch, l_config_instr.max_accum_open_vol, l_config_instr.max_cancel_limit, l_config_instr.expiration_date,
			l_config_instr.today_pos.long_volume, l_config_instr.today_pos.long_price, l_config_instr.today_pos.short_volume, l_config_instr.today_pos.short_price,
			l_config_instr.yesterday_pos.long_volume, l_config_instr.yesterday_pos.long_price, l_config_instr.yesterday_pos.short_volume, l_config_instr.yesterday_pos.short_price,
			l_config_instr.fee.fee_by_lot, l_config_instr.fee.exchange_fee, l_config_instr.fee.yes_exchange_fee, l_config_instr.fee.broker_fee, l_config_instr.fee.stamp_tax, l_config_instr.fee.acc_transfer_fee, l_config_instr.tick_size, l_config_instr.multiple);
	}
}

void log_quote(Futures_Internal_Book* quote) {
	LOG_LN("int_time: %d, symbol: %s, feed_type: %d, exch: %d, pre_close_px: %f, pre_settle_px: %f, pre_open_interest: %f, open_interest: %f \n"
		"open_px: %f, high_px: %f, low_px: %f, avg_px: %f, last_px: %f \n"
		"ap1: %f, av1: %d, ap2: %f, av2: %d, ap3: %f, av3: %d, ap4: %f, av4: %d, ap5: %f, av5: %d \n"
		"bp1: %f, bv1: %d, bp2: %f, bv2: %d, bp3: %f, bv3: %d, bp4: %f, bv4: %d, bp5: %f, bv5: %d \n"
		"total_vol: %lld, total_notional: %f, upper_limit_px: %f, lower_limit_px: %f, close_px: %f, settle_px: %f",
		quote->int_time, quote->symbol, quote->feed_type, quote->exchange, quote->pre_close_px, quote->pre_settle_px, quote->pre_open_interest, quote->open_interest,
		quote->open_px, quote->high_px, quote->low_px, quote->avg_px, quote->last_px,
		quote->ap_array[0], quote->av_array[0], quote->ap_array[1], quote->av_array[1], quote->ap_array[2], quote->av_array[2], quote->ap_array[3], quote->av_array[3], quote->ap_array[4], quote->av_array[4],
		quote->bp_array[0], quote->bv_array[0], quote->bp_array[1], quote->bv_array[1], quote->bp_array[2], quote->bv_array[2], quote->bp_array[3], quote->bv_array[3], quote->bp_array[4], quote->bv_array[4],
		quote->total_vol, quote->total_notional, quote->upper_limit_px, quote->lower_limit_px, quote->close_px, quote->settle_px);
}

void log_resp(st_response_t* resp) {
	LOG_LN("Order Resp: %lld %s %s %s %f %d %s %d %s", resp->order_id, resp->symbol,
		BUY_SELL_STR[resp->direction], OPEN_CLOSE_STR[resp->open_close], resp->exe_price, resp->exe_volume,
		STATUS[resp->status], resp->error_no, resp->error_info);
}

int my_st_init(int type, int length, void *cfg) {
	if (st_log_handle == NULL) {
		st_log_handle = fopen(g_trader_handler->m_trader_config->STRAT_LOG, "w");
		if (st_log_handle == NULL) {
			PRINT_ERROR("Can't open writing log file: %s", g_trader_handler->m_trader_config->STRAT_LOG);
			return NULL;
		}
	}
	load_strategy();
	st_config_t* config = (st_config_t*)cfg;
	config->proc_order_hdl = process_strategy_order;
	config->send_info_hdl = process_strategy_info;
	config->pass_rsp_hdl = process_strategy_resp;
	log_config(config);
	int ret = st_init(type, length, cfg);
	st_flush_log();
	return ret;
}

int my_on_book(int type, int length, void *book) {
	Futures_Internal_Book *f_book = (Futures_Internal_Book *)((st_data_t*)book)->info;
	int_time = f_book->int_time;
	log_quote(f_book);
	return on_book(type, length, book);
}

int my_on_response(int type, int length, void *resp) {
	st_response_t* l_resp = (st_response_t*)((st_data_t*)resp)->info;
	log_resp(l_resp);
	st_flush_log();
	return on_response(type, length, resp);
}

int my_on_timer(int type, int length, void *info) {
	LOG_LN("It is on_timer!");
	return on_timer(type, length, info);
}

void my_destroy() {
	LOG_LN("It is finished!");
	st_destroy();
	if (st_log_handle != NULL) {
		fwrite(st_log_buffer, st_log_buffer_len, 1, st_log_handle);
		fclose(st_log_handle);
		st_log_handle = NULL;
		st_start_point = st_log_buffer;
		st_log_buffer_len = 0;
	}
	dlclose(strategy_handler);
}