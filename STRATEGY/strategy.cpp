#include <stdio.h>
#include <math.h>
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

static int count = 0;
static int tick_time = 0;
static int int_time = 0; 
static double last_ask_price; 
static double last_bid_price;
static char local_time[64];

#define KDBLEN 1024
#define SIGNAL_COUNT 4
#define MY_VOL 5

static char kdb_sql[KDBLEN];

static char* KDB_IP = "localhost";
static double ev_list[SIGNAL_COUNT] = { 0.25, 0.5, 0.75, 1.0 };
static int port_list[SIGNAL_COUNT] = {5006, 5007, 5008, 5009};
static int kdb_handler_list[SIGNAL_COUNT] = { 0 };

void process_kdb_all(){
    for(int i = 0; i < SIGNAL_COUNT; i++)
        k(kdb_handler_list[i], kdb_sql, (K)0);
}

int my_st_init(int type, int length, void *cfg) {
	if (sdp_handler == NULL) {
		g_config = (st_config_t *)cfg;
		sdp_handler = new SDPHandler(type, length, cfg);

		/* write your logic here */
		char* init_kdb_sql = "int2time:{\"T\"$-9#\"00000000\",string x};evparam:%f;if[not `quoteData in tables `.;ibAskpx:0f;ibBidpx:0f;res:([]a:enlist 0i;b:enlist 0f;c:enlist 0f;d:enlist 0i);quoteData:();FinalSignal2:();"
									"quote:([]Date:();`float$LegOneBid1:();`float$LegOneAsk1:();`float$LegTwoBid1:();`float$LegTwoAsk1:());`quote insert (.z.D+.z.t;3333.0;3330.5;3.05;3.1)]";

		for (int i = 0; i < SIGNAL_COUNT; i++) {
			kdb_handler_list[i] = khpu(KDB_IP, port_list[i], "");
			sprintf(kdb_sql, init_kdb_sql, ev_list[i]);
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

void cancel_old_order(int tick_time) {
	list_t *pos, *n;
	Order *l_ord;

	list_t *ord_list = sdp_handler->m_orders->get_order_by_side(ORDER_BUY);
	list_for_each_prev_safe(pos, n, ord_list) {
		l_ord = list_entry(pos, Order, pd_link);
		if (tick_time >= get_seconds_from_int_time(l_ord->insert_time) + 1) {
			PRINT_ERROR("Order in OrderList Waiting over 1 Secs; cur time %d, insert time %d, Cancelling...\n", int_time, get_seconds_from_int_time(l_ord->insert_time));
			LOG_LN("Order in OrderList Waiting over 1 Secs; cur time %d, insert time %d, Cancelling...\n", int_time, get_seconds_from_int_time(l_ord->insert_time));
			sdp_handler->cancel_single_order(l_ord);
		}
	}

	ord_list = sdp_handler->m_orders->get_order_by_side(ORDER_SELL);
	list_for_each_prev_safe(pos, n, ord_list) {
		l_ord = list_entry(pos, Order, pd_link);
		if (tick_time >= get_seconds_from_int_time(l_ord->insert_time) + 1) {
			PRINT_ERROR("Order in OrderList Waiting over 1 Secs; cur time %d, insert time %d, Cancelling...\n", int_time, get_seconds_from_int_time(l_ord->insert_time));
			LOG_LN("Order in OrderList Waiting over 1 Secs; cur time %d, insert time %d, Cancelling...\n", int_time, get_seconds_from_int_time(l_ord->insert_time));
			sdp_handler->cancel_single_order(l_ord);
		}
	}
}

double Round(double price)
{
    double res = round(price*10)/10.0;
    printf("before:%lf, after:%lf\n",price,res);
    return res;
}

int run_strategy(int kdb_handle){
    K table = k(kdb_handle, "select from res ", (K)0);
    K values = kK(table->k)[1];
                                                                                                                                     
    //int length = kI(kK(values)[0])[0]; //长度，用于猝发是否产生信号
    //double bid1 = Round(kF(kK(values)[1])[0]);  //bid price
    //double ask1 = Round(kF(kK(values)[2])[0]);   //ask price
    int signal = kI(kK(values)[3])[0];  //short or long
	return signal;
}

void long_short(Contract * instr, int desired_pos, double ap, double bp)
{
	LOG_LN("Enter long_short, symbol: %s, desired_pos: %d, ap: %f, bp: %f",
		instr->symbol, desired_pos, ap, bp);

	int current_pos = position(instr);

	if (current_pos == desired_pos) {
		sdp_handler->cancel_all_orders();
		return;
	}

	int a_size = abs(desired_pos - current_pos);
	DIRECTION a_side;
	double price;
	int same_side_pos;
	int opposite_side_pos;
	int available_for_close;
	int available_for_close_yes;
	int total_pending_buy_close_size = instr->pending_buy_close_size;
	int total_pending_sell_close_size = instr->pending_sell_close_size;

	if (desired_pos > current_pos) {
		a_side = ORDER_BUY;
		price = ap;
		same_side_pos = long_position(instr);
		opposite_side_pos = short_position(instr);
		available_for_close_yes = instr->pre_short_qty_remain;
		available_for_close = opposite_side_pos - total_pending_buy_close_size;
	}
	else {
		a_side = ORDER_SELL;
		price = bp;
		same_side_pos = short_position(instr);
		opposite_side_pos = long_position(instr);
		available_for_close_yes = instr->pre_long_qty_remain;
		available_for_close = opposite_side_pos - total_pending_sell_close_size;
	}

	LOG_LN("side: %d, price %f, same_side_pos: %d, opposite_side_pos: %d, available_for_close_yes: %d, available_for_close: %d",
		a_side, price, same_side_pos, opposite_side_pos, available_for_close_yes, available_for_close);

	int pending_open_size = 0;
	int pending_close_size = 0;
	int pending_close_yes_size = 0;
	sdp_handler->cancel_orders_with_dif_px(instr, a_side, price, pending_open_size, pending_close_size, pending_close_yes_size);
	sdp_handler->cancel_orders_by_side(instr, switch_side(a_side));

	LOG_LN("pending_open_size: %d, pending_close_size %d, pending_close_yes_size: %d",
		pending_open_size, pending_close_size, pending_close_yes_size);

	if (pending_close_size > opposite_side_pos || available_for_close < 0 || available_for_close < available_for_close_yes)
	{
		LOG_LN("Something is wrong with close_size, close_yes_size, opposite_side_pos.");
		sdp_handler->cancel_orders_by_side(instr, a_side, ORDER_CLOSE);
		return;
	}

	if (pending_open_size + pending_close_size + pending_close_yes_size > 0) {
		LOG_LN("Skip current signal since pending_open_size + pending_close_size + pending_close_yes_size > 0 for same side and price.");
		return;
	}

	int need_to_close_yes = MIN(available_for_close_yes, a_size);
	int need_to_close_today = MIN(available_for_close - available_for_close_yes, a_size - need_to_close_yes);
	int need_to_open = a_size - need_to_close_yes - need_to_close_today;

	if (need_to_close_yes > 0)
		sdp_handler->send_single_order(instr, instr->exch, price, need_to_close_yes, a_side, ORDER_CLOSE_YES);
	if (need_to_close_today > 0)
		sdp_handler->send_single_order(instr, instr->exch, price, need_to_close_today, a_side, ORDER_CLOSE);
	if (need_to_open > 0)
		sdp_handler->send_single_order(instr, instr->exch, price, need_to_open, a_side, ORDER_OPEN);
}

int my_on_book(int type, int length, void *book) {
	if (sdp_handler == NULL) return -1;
	sdp_handler->on_book(type, length, book);

	Futures_Internal_Book *f_book = (Futures_Internal_Book *)((st_data_t*)book)->info;
	int_time = f_book->int_time;

	//6秒撤单机制
	tick_time = get_seconds_from_int_time(f_book->int_time);
	cancel_old_order(tick_time);

	get_time_record(local_time);

	//当行情时间和本地时间相差5分钟，认为是无效行情，注意这里的时间已做过0点处理。
	int l_local_time = get_seconds_from_char_time(local_time);
	//PRINT_INFO("tick_time:%d,local_time:%d", tick_time, l_local_time);
	if (abs(l_local_time - tick_time) > 5 * 60) {
		PRINT_ERROR("No Use Quote,QUOTE TIME: %d ,LOCAL TIME:%s, delta:%d", int_time, local_time, abs(l_local_time - tick_time));
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
	LOG_LN("kdb_sql: %s\n", kdb_sql);
	process_kdb_all();

    //为防止kdb中行情表格过大，每600个tick清理一次kdb表格。只保留2000行。
    if (++count % 600 == 0){
          strcpy(kdb_sql, "quoteData: select [-2000] from quoteData;");
          process_kdb_all();
    }

	int signal_sum = 0;
	for (int i = 0; i < SIGNAL_COUNT; i++) {
		LOG_LN("\nStrategy %d run...", i);
		signal_sum += run_strategy(kdb_handler_list[i]);
	}

	Contract *instr = sdp_handler->find_contract(f_book->symbol);
	long_short(instr, signal_sum * MY_VOL, last_ask_price, last_bid_price);

	return 0;
}



int my_on_response(int type, int length, void *resp) {
	if (sdp_handler == NULL) return -1;
	st_response_t* rsp = (st_response_t*)((st_data_t*)resp)->info;
    Order* l_ord = NULL;
    l_ord = sdp_handler->m_orders->query_order(rsp->order_id);
	int leaves_temp = leaves_qty(l_ord);
	sdp_handler->on_response(type, length, resp);
	Contract *instr = sdp_handler->find_contract(rsp->symbol);
	switch (rsp->status) {
	case SIG_STATUS_PARTED:
		break;
	case SIG_STATUS_SUCCEED:
		break;
	case SIG_STATUS_CANCEL_REJECTED:
	case SIG_STATUS_REJECTED:
		break;
	case SIG_STATUS_CANCELED:
		//PRINT_WARN("Cancel Succeed!, Send order again");
		//LOG_LN("Cancel Succeed!, Send order again");
		//if(rsp->direction == ORDER_BUY)
		//	sdp_handler->send_single_order(instr, instr->exch, last_ask_price,leaves_temp, (DIRECTION)rsp->direction, (OPEN_CLOSE)rsp->open_close);
		//else
		//	sdp_handler->send_single_order(instr, instr->exch, last_bid_price,leaves_temp, (DIRECTION)rsp->direction, (OPEN_CLOSE)rsp->open_close);
		break;
	case SIG_STATUS_INTRREJECTED:
		break;
	case SIG_STATUS_INIT:
		break;
	case SIG_STATUS_ENTRUSTED:
		PRINT_ERROR("Has Been in OrderList; Waiting Deal...");
		LOG_LN("Has Been in OrderList; Waiting Deal...");
		l_ord = sdp_handler->m_orders->query_order(rsp->order_id);
		if(l_ord != NULL)
			l_ord->insert_time = int_time;
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
