#include <stdio.h>
#include <string.h>
#include "strategy_interface.h"
#include "quote_format_define.h"
#include "core/sdp_handler.h"
#include "utils/k.h"
#include "utils/log.h"
#include "utils/utils.h"
#include "utils/json11.hpp"

using namespace json11;

static SDPHandler *sdp_handler = NULL;
static st_config_t *g_config = NULL;
static ST_BASE_CONFIG g_base_config = { 0 };

static int count = 0;
static int tick_time = 0;
static int int_time = 0; 
static double last_ask_price; 
static double last_bid_price;
static char local_time[64];
static char kdb_sql[1024];
static int kdb_handler_list[16] = { 0 };

void process_kdb_all(){
    for(int i = 0; i < g_base_config.KDB_PORT_NUM; i++)
        k(kdb_handler_list[i], kdb_sql, (K)0);
}

int my_st_init(int type, int length, void *cfg) {
	if (sdp_handler == NULL) {
		g_config = (st_config_t *)cfg;
		sdp_handler = new SDPHandler(type, length, cfg);

		/* write your logic here */
		char* init_kdb_sql = "int2time:{\"T\"$-9#\"00000000\",string x};evparam:%f;if[not `quoteData in tables `.;ibAskpx:0f;ibBidpx:0f;res:([]a:enlist 0i;b:enlist 0f;c:enlist 0f;d:enlist 0i);quoteData:();FinalSignal2:();"
									"quote:([]Date:();`float$LegOneBid1:();`float$LegOneAsk1:();`float$LegTwoBid1:();`float$LegTwoAsk1:());`quote insert (.z.D+.z.t;3333.0;3330.5;3.05;3.1)]";

		if (!read_json_config(g_base_config, g_config->param_file_path)) 
			return -1;

		for (int i = 0; i < g_base_config.KDB_PORT_NUM; i++) {
			kdb_handler_list[i] = khpu(g_base_config.KDB_IP, g_base_config.KDB_PORT_LIST[i], "");
			sprintf(kdb_sql, init_kdb_sql, g_base_config.KDB_EV_LIST[i]);
			k(-kdb_handler_list[i], kdb_sql, (K)0);
		}

		LOG_LN("Strategy Init!");
	}
	return 0;
}

inline bool not_working_time(int atime) {
	if (((int_time >= 10000000 && int_time <= 90005000)) || ((int_time >= 101500000) && (int_time <= 102005000)) || ((int_time >= 113000000) && (int_time <= 133005000)) || ((int_time >= 150000000) && (int_time <= 210005000)))
		return true;
	return false;
}

inline bool is_working_time(int atime) {
	if (((int_time > 85800000) && (int_time < 101600000)) || ((int_time > 102800000) && (int_time < 113100000)) || ((int_time > 132800000) && (int_time < 150100000)) || ((int_time > 205800000) && (int_time < 250100000)))
		return true;
	return false;
}

int run_strategy(int kdb_handle){
    K table = k(kdb_handle, "select from res ", (K)0);
    K values = kK(table->k)[1];
                                                                                                                                     
    int signal = kI(kK(values)[3])[0];  //short or long
	if (signal == 1 || signal == -1)
		return signal;
	return 0;
}

int my_on_book(int type, int length, void *book) {
	if (sdp_handler == NULL) return -1;
	sdp_handler->on_book(type, length, book);

	Futures_Internal_Book *f_book = (Futures_Internal_Book *)((st_data_t*)book)->info;
	int_time = f_book->int_time;

	//6秒撤单机制
	tick_time = get_seconds_from_int_time(f_book->int_time);
	sdp_handler->cancel_old_order(tick_time);

	get_time_record(local_time);

	//当行情时间和本地时间相差5分钟，认为是无效行情，注意这里的时间已做过0点处理。
	int l_local_time = get_seconds_from_char_time(local_time);
	//PRINT_INFO("tick_time:%d,local_time:%d", tick_time, l_local_time);
	if (abs(l_local_time - tick_time) > 5 * 60) {
		PRINT_ERROR("No Use Quote, QUOTE TIME: %d, LOCAL TIME:%s, delta:%d", int_time, local_time, abs(l_local_time - tick_time));
		return -1;
	}

	//过滤一些极端时间段
	if (not_working_time(int_time)) return -1;
	if (int_time < 40000000) int_time += 240000000;
	if (is_working_time(int_time) == false)	return -1;

	last_ask_price = Round(f_book->ap_array[0]);
	last_bid_price = Round(f_book->bp_array[0]);
	int int_time_tmp = int_time;
	if (int_time_tmp > 240000000) {
		int_time_tmp -= 240000000;
	}
    sprintf(kdb_sql, "ctpAskpx:%lf;ctpBidpx:%lf;quote:update Date:.z.D+int2time %d,LegOneBid1:%s,LegOneAsk1:%s,LegTwoBid1:%s,LegTwoAsk1:%s from quote;system \"l strategy.q\"",
        last_ask_price, last_bid_price, int_time_tmp, "ibBidpx", "ibAskpx", "ctpBidpx", "ctpAskpx" );
	LOG_LN("kdb_sql: %s", kdb_sql);
	process_kdb_all();

    //为防止kdb中行情表格过大，每600个tick清理一次kdb表格。只保留2000行。
    if (++count % 600 == 0){
          strcpy(kdb_sql, "quoteData: select [-2000] from quoteData;");
          process_kdb_all();
    }

	int signal_sum = 0;
	for (int i = 0; i < g_base_config.KDB_PORT_NUM; i++) {
		LOG_LN("Strategy %d run...", i);
		signal_sum += run_strategy(kdb_handler_list[i]);
	}

	Contract *instr = sdp_handler->find_contract(f_book->symbol);
	sdp_handler->long_short(instr, signal_sum * g_base_config.MAX_VOL, last_ask_price, last_bid_price, 2); //目前的容忍度是2个tick_size

	return 0;
}



int my_on_response(int type, int length, void *resp) {
	if (sdp_handler == NULL) return -1;
	st_response_t* rsp = (st_response_t*)((st_data_t*)resp)->info;
	sdp_handler->on_response(type, length, resp);
	switch (rsp->status) {
	case SIG_STATUS_PARTED:
		break;
	case SIG_STATUS_SUCCEED:
		break;
	case SIG_STATUS_CANCEL_REJECTED:
	case SIG_STATUS_REJECTED:
		break;
	case SIG_STATUS_CANCELED:
		break;
	case SIG_STATUS_INTRREJECTED:
		break;
	case SIG_STATUS_INIT:
		break;
	case SIG_STATUS_ENTRUSTED:
		break;
	}
	return 0;
}

int my_on_timer(int type, int length, void *info) {
	if (sdp_handler != NULL) {
		sdp_handler->on_timer(type, length, info);
		/* write your logic here */
		printf("on timer!\n");
		return 0;
	}
	return -1;
}

void my_destroy() {
	if (sdp_handler != NULL) {
		delete sdp_handler;
		sdp_handler = NULL;

		PRINT_ERROR("Haozai > ERROR or DISCONNECT,Come into error_deal, save kdb tables...");
		LOG_LN("Haozai > ERROR or DISCONNECT,Come into error_deal, save kdb tables...");
		k(-kdb_handler_list[0], "{save hsym `$\"tbls/\",string x } each  tables `.", (K)0);
	}
}
