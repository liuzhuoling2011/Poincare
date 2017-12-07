#include "stdio.h"
#include "strategy_interface.h"
#include "core/base_define.h"
#include "core/check_handler.h"

st_config_t g_config = { 0 };
order_t empty_order = { 0 };
st_response_t empty_resp = { 0 };

void testcase1() { //close val check
	st_data_t data1, data2;
	order_t ord1 = empty_order;
	my_strncpy(ord1.symbol, "pp1601", SYMBOL_LEN);
	ord1.direction = ORDER_BUY;
	ord1.open_close = ORDER_OPEN;
	ord1.volume = 10;
	ord1.price = 99.99;
	ord1.st_id = 99;
	ord1.order_id = 10000000099;
	data1.info = (void*)&ord1;

	order_t ord2 = ord1;
	ord2.direction = ORDER_SELL;
	ord2.open_close = ORDER_CLOSE;
	ord2.volume = 11;
	ord2.order_id = 20000000099;
	data2.info = (void*)&ord2;

	my_on_book(S_PLACE_ORDER_DEFAULT, 0, (void*)(&data1));
	my_on_book(S_PLACE_ORDER_DEFAULT, 0, (void*)(&data2));
}

void testcase2() { //side && price check
	st_data_t data1, data2;
	order_t ord1 = empty_order;
	my_strncpy(ord1.symbol, "pp1601", SYMBOL_LEN);
	ord1.direction = ORDER_BUY;
	ord1.open_close = ORDER_OPEN;
	ord1.volume = 10;
	ord1.price = 99.99;
	ord1.st_id = 99;
	ord1.order_id = 10000000099;
	data1.info = (void*)&ord1;

	order_t ord2 = ord1;
	ord2.direction = ORDER_SELL;
	ord2.open_close = ORDER_OPEN;
	ord1.order_id = 20000000099;
	data2.info = (void*)&ord2;

	my_on_book(S_PLACE_ORDER_DEFAULT, 0, (void*)(&data1));
	my_on_book(S_PLACE_ORDER_DEFAULT, 0, (void*)(&data2));
}

void testcase3() { //max open val check
	st_data_t data;
	order_t ord1 = empty_order;
	my_strncpy(ord1.symbol, "pp1601", SYMBOL_LEN);
	ord1.direction = ORDER_BUY;
	ord1.open_close = ORDER_OPEN;
	ord1.volume = 5001;
	ord1.price = 99.99;
	ord1.st_id = 99;
	ord1.order_id = 10000000099;
	data.info = (void*)&ord1;

	my_on_book(S_PLACE_ORDER_DEFAULT, 0, (void*)(&data));
	my_on_book(S_PLACE_ORDER_DEFAULT, 0, (void*)(&data));
}

void testcase4() { //cancel order fail, not entrust
	st_data_t data;
	order_t ord1 = empty_order;
	my_strncpy(ord1.symbol, "pp1601", SYMBOL_LEN);
	ord1.direction = ORDER_BUY;
	ord1.open_close = ORDER_OPEN;
	ord1.volume = 5001;
	ord1.price = 99.99;
	ord1.st_id = 99;
	data.info = (void*)&ord1;

	my_on_book(S_PLACE_ORDER_DEFAULT, 0, (void*)(&data));
	ord1.org_ord_id = ord1.order_id;
	my_on_book(S_CANCEL_ORDER_DEFAULT, 0, (void*)(&data));
}

void testcase5() { //cancel order fail, already sussess
	st_data_t data;
	order_t ord1 = empty_order;
	my_strncpy(ord1.symbol, "pp1601", SYMBOL_LEN);
	ord1.direction = ORDER_BUY;
	ord1.open_close = ORDER_OPEN;
	ord1.volume = 5001;
	ord1.price = 99.99;
	ord1.st_id = 99;
	data.info = (void*)&ord1;

	my_on_book(S_PLACE_ORDER_DEFAULT, 0, (void*)(&data));

	st_response_t l_resp = empty_resp;
	l_resp.order_id = ord1.order_id;
	my_strncpy(l_resp.symbol, "pp1601", SYMBOL_LEN);
	l_resp.direction = ord1.direction;
	l_resp.open_close = ord1.open_close;
	l_resp.exe_price = ord1.price;
	l_resp.exe_volume = ord1.volume;
	l_resp.status = SIG_STATUS_SUCCEED;
	l_resp.error_no = 0;
	data.info = (void*)&l_resp;
	my_on_response(S_STRATEGY_PASS_RSP, 0, (void*)(&data));

	ord1.org_ord_id = ord1.order_id;
	data.info = (void*)&ord1;
	my_on_book(S_CANCEL_ORDER_DEFAULT, 0, (void*)(&data));
}

void testcase6() { //cash is not enough
	st_data_t data;
	order_t ord1 = empty_order;
	my_strncpy(ord1.symbol, "000001", SYMBOL_LEN);
	ord1.exch = SSE;
	ord1.direction = ORDER_BUY;
	ord1.open_close = ORDER_OPEN;
	ord1.volume = 5000;
	ord1.price = 999999;
	ord1.st_id = 99;
	data.info = (void*)&ord1;

	my_on_book(S_PLACE_ORDER_DEFAULT, 0, (void*)(&data));
}

void testcase7() { //pre long pos is not enough
	st_data_t data;
	order_t ord1 = empty_order;
	my_strncpy(ord1.symbol, "000001", SYMBOL_LEN);
	ord1.exch = SSE;
	ord1.direction = ORDER_SELL;
	ord1.open_close = ORDER_CLOSE;
	ord1.volume = 5;
	ord1.price = 9999;
	ord1.st_id = 99;
	data.info = (void*)&ord1;

	my_on_book(S_PLACE_ORDER_DEFAULT, 0, (void*)(&data));
}

void testcase8() { //cancel order fail, reach max limit
	st_data_t data;
	order_t ord1 = empty_order;
	my_strncpy(ord1.symbol, "000001", SYMBOL_LEN);
	ord1.exch = SSE;
	ord1.direction = ORDER_BUY;
	ord1.open_close = ORDER_OPEN;
	ord1.volume = 5;
	ord1.price = 999;
	ord1.st_id = 99;
	ord1.order_id = 10000000099;
	data.info = (void*)&ord1;
	
	my_on_book(S_PLACE_ORDER_DEFAULT, 0, (void*)(&data));

	order_t ord2 = ord1;
	data.info = (void*)&ord2;
	ord2.order_id = 20000000099;
	my_on_book(S_PLACE_ORDER_DEFAULT, 0, (void*)(&data));

	st_response_t resp = { 0 };
	data.info = (void*)&resp;
	resp.order_id = 10000000099;
	resp.status = SIG_STATUS_ENTRUSTED;
	my_on_response(S_STRATEGY_PASS_RSP, 0, (void*)(&data));
	resp.order_id = 20000000099;
	my_on_response(S_STRATEGY_PASS_RSP, 0, (void*)(&data));

	data.info = (void*)&ord1;
	ord1.org_ord_id = 10000000099;
	my_on_book(S_CANCEL_ORDER_DEFAULT, 0, (void*)(&data));

	data.info = (void*)&resp;
	resp.order_id = 10000000099;
	resp.status = SIG_STATUS_CANCELED;
	my_on_response(S_STRATEGY_PASS_RSP, 0, (void*)(&data));
	
	data.info = (void*)&ord1;
	ord1.org_ord_id = 20000000099;
	my_on_book(S_CANCEL_ORDER_DEFAULT, 0, (void*)(&data));
}

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
	l_contract.max_cancel_limit = 1;
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
	strlcpy(g_config.output_file_path, "./out_file.txt", PATH_LEN);
	strlcpy(g_config.st_name, "test_strategy", PATH_LEN);
	g_config.vst_id = 564335;

	g_config.send_info_hdl = process_debug_info;
	g_config.proc_order_hdl = process_debug_info;
	g_config.pass_rsp_hdl = process_debug_info;
}

void lt_prepare_config() {
	account_t& l_account = g_config.accounts[0];
	strlcpy(l_account.account, "zoran", ACCOUNT_LEN);
	l_account.currency = 0;
	l_account.exch_rate = 1.0;
	l_account.cash_asset = 99999999;
	l_account.cash_available = 100000000;

	contract_t& l_contract = g_config.contracts[0];	
	contract_t& l_contract1 = g_config.contracts[1];
	strlcpy(l_contract.symbol, "TA801", SYMBOL_LEN);
	strlcpy(l_contract.account, "zoran", ACCOUNT_LEN);
	l_contract.exch = 'C';
	l_contract.max_accum_open_vol = 6;
	l_contract.max_cancel_limit = 480;
	l_contract.expiration_date = 20200808;
	l_contract.tick_size = 10.0;
	l_contract.multiple = 10.0;

	l_contract.yesterday_pos.long_volume = 0;
	l_contract.yesterday_pos.long_price = 0;
	l_contract.yesterday_pos.short_volume = 0;
	l_contract.yesterday_pos.short_price = 0;
	l_contract.today_pos.long_volume = 0;
	l_contract.today_pos.long_price = 0;
	l_contract.today_pos.short_volume = 0;
	l_contract.today_pos.short_price = 0;

	l_contract.fee.fee_by_lot = 0;
	l_contract.fee.exchange_fee = 10;
	l_contract.fee.yes_exchange_fee = 5;
	l_contract.fee.broker_fee = 0.0002;
	l_contract.fee.stamp_tax = 0.001;
	l_contract.fee.acc_transfer_fee = 0.00002;

	l_contract1 = l_contract;
	//strlcpy(l_contract1.symbol, "000001", SYMBOL_LEN);
	//l_contract1.exch = SSE;

	//strlcpy(g_config.param_file_path, "./param_file.txt", PATH_LEN);
	//strlcpy(g_config.output_file_path, "./out_file.txt", PATH_LEN);
	strlcpy(g_config.st_name, "test_strategy", PATH_LEN);
	g_config.vst_id = 564335;

	g_config.send_info_hdl = process_debug_info;
	g_config.proc_order_hdl = process_debug_info;
	g_config.pass_rsp_hdl = process_debug_info;
}

void lt_testcase() {
    st_data_t data1;
	order_t ord1 = empty_order;
	my_strncpy(ord1.symbol, "TA801", SYMBOL_LEN);
	ord1.direction = ORDER_BUY;
	ord1.open_close = ORDER_OPEN;
    ord1.order_id = 10022000291;
    ord1.org_ord_id = 0;
	ord1.volume = 1;
	ord1.price = 5490.0;
	ord1.st_id = 99;
	data1.info = (void*)&ord1;
	my_on_book(S_PLACE_ORDER_DEFAULT, 0, (void*)(&data1));

    st_response_t l_resp = empty_resp;
	l_resp.order_id = 10022000291;
	my_strncpy(l_resp.symbol, "TA801", SYMBOL_LEN);
	l_resp.direction = ord1.direction;
	l_resp.open_close = ord1.open_close;
	l_resp.exe_price = ord1.price;
	l_resp.exe_volume = ord1.volume;
	l_resp.status = SIG_STATUS_ENTRUSTED;
	l_resp.error_no = 0;
	data1.info = (void*)&l_resp;
	my_on_response(S_STRATEGY_PASS_RSP, 0, (void*)(&data1));

    ord1 = empty_order;
	my_strncpy(ord1.symbol, "TA801", SYMBOL_LEN);
	ord1.direction = ORDER_BUY;
	ord1.open_close = ORDER_OPEN;
    ord1.order_id = 2002000291;
    ord1.org_ord_id = 10022000291;
	ord1.volume = 1;
	ord1.price = 5490.0;
	ord1.st_id = 99;
	data1.info = (void*)&ord1;
	my_on_book(S_CANCEL_ORDER_DEFAULT, 0, (void*)(&data1));

    ord1 = empty_order;
	my_strncpy(ord1.symbol, "TA801", SYMBOL_LEN);
	ord1.direction = ORDER_BUY;
	ord1.open_close = ORDER_OPEN;
    ord1.order_id = 30022000291;
    ord1.org_ord_id = 0;
	ord1.volume = 1;
	ord1.price = 5492.0;
	ord1.st_id = 99;
	data1.info = (void*)&ord1;
	my_on_book(S_PLACE_ORDER_DEFAULT, 0, (void*)(&data1));

    l_resp = empty_resp;
	l_resp.order_id = 30022000291;
	my_strncpy(l_resp.symbol, "TA801", SYMBOL_LEN);
	l_resp.direction = ord1.direction;
	l_resp.open_close = ord1.open_close;
	l_resp.exe_price = ord1.price;
	l_resp.exe_volume = ord1.volume;
	l_resp.status = SIG_STATUS_ENTRUSTED;
	l_resp.error_no = 0;
	data1.info = (void*)&l_resp;
	my_on_response(S_STRATEGY_PASS_RSP, 0, (void*)(&data1));

    l_resp = empty_resp;
	l_resp.order_id = 10022000291;
	my_strncpy(l_resp.symbol, "TA801", SYMBOL_LEN);
	l_resp.direction = ord1.direction;
	l_resp.open_close = ord1.open_close;
	l_resp.exe_price = ord1.price;
	l_resp.exe_volume = ord1.volume;
	l_resp.status = SIG_STATUS_CANCELED;
	l_resp.error_no = 0;
	data1.info = (void*)&l_resp;
	my_on_response(S_STRATEGY_PASS_RSP, 0, (void*)(&data1));
}

int main() {
	prepare_config();
	my_st_init(0, 0, (void*)(&g_config));
	testcase1(); //close val check
	my_destroy();

	my_st_init(0, 0, (void*)(&g_config));
	testcase2(); //side && price check
	my_destroy();

	my_st_init(0, 0, (void*)(&g_config));
	testcase3(); //max open val check
	my_destroy();

	my_st_init(0, 0, (void*)(&g_config));
	testcase4(); //cancel order fail, not entrust
	my_destroy();

	my_st_init(0, 0, (void*)(&g_config));
	testcase5(); //cancel order fail, already success
	my_destroy();

	my_st_init(0, 0, (void*)(&g_config));
	testcase6(); //cash is not enough
	my_destroy();

	my_st_init(0, 0, (void*)(&g_config));
	testcase7(); //pre long pos is not enough
	my_destroy();

	my_st_init(0, 0, (void*)(&g_config));
	testcase8(); //cancel order fail, reach max limit
	my_destroy();

    lt_prepare_config();
    my_st_init(0, 0, (void*)(&g_config));
    lt_testcase();
	my_destroy();
	return 0;
}
