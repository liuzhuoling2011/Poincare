#include <stdio.h>
#include "utils/utils.h"
#include "strategy_interface.h"
#include "quote_format_define.h"

static st_config_t g_config = { 0 };
static st_data_t g_st_data = { 0 };
static special_order_t g_special_order = { 0 };
static Stock_Internal_Book g_stock_internal_book = { 0 };

void prepare_config() {
	account_t& l_account = g_config.accounts[0];
	strlcpy(l_account.account, "zoran", ACCOUNT_LEN);
	l_account.currency = 0;
	l_account.exch_rate = 1.0;
	l_account.cash_asset = 99999999;
	l_account.cash_available = 100000000;

	contract_t& l_contract = g_config.contracts[0];
	contract_t& l_contract1 = g_config.contracts[1];
	strlcpy(l_contract.symbol, "pp1601", SYMBOL_LEN);
	strlcpy(l_contract.account, "zoran", ACCOUNT_LEN);
	l_contract.exch = 'B';
	l_contract.max_accum_open_vol = 10000;
	l_contract.expiration_date = 20200808;
	l_contract.tick_size = 0.01;
	l_contract.multiple = 5;

	l_contract.yesterday_pos.long_volume = 1;
	l_contract.yesterday_pos.long_price = 2;
	l_contract.yesterday_pos.short_volume = 3;
	l_contract.yesterday_pos.short_price = 4;
	l_contract.today_pos.long_volume = 5;
	l_contract.today_pos.long_price = 6;
	l_contract.today_pos.short_volume = 7;
	l_contract.today_pos.short_price = 8;

	l_contract.fee.fee_by_lot = 1;
	l_contract.fee.exchange_fee = 10;
	l_contract.fee.yes_exchange_fee = 5;
	l_contract.fee.broker_fee = 0.0002;
	l_contract.fee.stamp_tax = 0.001;
	l_contract.fee.acc_transfer_fee = 0.00002;

	l_contract1 = l_contract;
	strlcpy(l_contract1.symbol, "000001", SYMBOL_LEN);
	l_contract1.exch = SSE;

	strlcpy(g_config.param_file_path, "./param_file.txt", PATH_LEN);
	strlcpy(g_config.output_file_path, "./output", PATH_LEN);
	strlcpy(g_config.st_name, "test_strategy", PATH_LEN);
	g_config.vst_id = 564335;
	g_config.trading_date = 20160808;
	g_config.day_night = 0;

	g_config.send_info_hdl = process_debug_info;
	g_config.proc_order_hdl = process_debug_info;
	g_config.pass_rsp_hdl = process_debug_info;
}

void format_order() {
	/*g_special_order.exch = SSE;
	strlcpy(g_special_order.symbol, "000001", SYMBOL_LEN);*/
	g_special_order.exch = 'B';
	strlcpy(g_special_order.symbol, "pp1601", SYMBOL_LEN);
	g_special_order.direction = ORDER_BUY;
	g_special_order.price = 1000;
	g_special_order.volume = 10;
	g_special_order.start_time = 90000000;
	g_special_order.end_time = 180000000;
	g_special_order.sync_cancel = false;
}

void format_quote() {
	g_stock_internal_book.exch_time = 85800000;
	strlcpy(g_stock_internal_book.ticker, "000001", SYMBOL_LEN);
}

int main() {
	prepare_config();
	my_st_init(0, 0, &g_config);
	format_quote();
	/*g_st_data.info = (void*)&g_stock_internal_book;
	my_on_book(DEFAULT_STOCK_QUOTE, sizeof(g_st_data), &g_st_data);
*/
	format_order();
	g_st_data.info = (void*)&g_special_order;
	my_on_book(S_PLACE_ORDER_DMA, sizeof(g_st_data), &g_st_data);
	g_special_order.volume = 1;
	g_special_order.direction = ORDER_SELL;
	g_special_order.price = 1001;
	my_on_book(S_PLACE_ORDER_DMA, sizeof(g_st_data), &g_st_data);

	return 0;
}