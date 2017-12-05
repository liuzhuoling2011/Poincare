#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include "utils/log.h"
#include "strategy_interface.h"

char g_strategy_path[256];

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
	}
	st_init = (st_data_func_t)dlsym(strategy_handler, "my_st_init");
	on_book = (st_data_func_t)dlsym(strategy_handler, "my_on_book");
	on_response = (st_data_func_t)dlsym(strategy_handler, "my_on_response");
	on_timer = (st_data_func_t)dlsym(strategy_handler, "my_on_timer");
	st_destroy = (st_none_func_t)dlsym(strategy_handler, "my_destroy");
}

int my_st_init(int type, int length, void *cfg) {
	load_strategy();
	st_config_t* config = (st_config_t*)cfg;
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
	return st_init(type, length, cfg);
}

int my_on_book(int type, int length, void *book) {
	return on_book(type, length, book);
}

int my_on_response(int type, int length, void *resp) {
	return on_response(type, length, resp);
}

int my_on_timer(int type, int length, void *info) {
	LOG_LN("It is on_timer!");
	return on_timer(type, length, info);
}

void my_destroy() {
	LOG_LN("It is finished!");
	st_destroy();
	dlclose(strategy_handler);
}