#include "strategy_interface.h"
#include "sdp_handler/core/sdp_handler.h"
#include "sdp_handler/core/bar.h"
#include "sdp_handler/utils/log.h"

static SDPHandler *sdp_handler = NULL;
static st_config_t *g_config = NULL;

static double last_ask_price;
static double last_bid_price;
int my_on_bar(Internal_Bar* bar_quote); // declare bar function

//sample strategy: 遇到两个阳线就做多，遇到两个阴线就做空的反手策略。

int my_st_init(int type, int length, void *cfg) {
	if (sdp_handler == NULL) {
		sdp_handler = new SDPHandler(type, length, cfg);

		/* write your logic here */
		LOG_LN("Strategy Init!");
		g_config = (st_config_t *)cfg;
		activate_bar(1, my_on_bar);
	}
	return 0;
}

static bool first_order_flag = true;

int my_on_book(int type, int length, void *book) {
	if (sdp_handler != NULL) {
		sdp_handler->on_book(type, length, book);

		/* write your logic here */
		Futures_Internal_Book *f_book = (Futures_Internal_Book *)((st_data_t*)book)->info;
		last_ask_price = f_book->ap_array[0];
		last_bid_price = f_book->bp_array[0];
		
		//k线 跳入。
		process_bar_data(f_book);
	}
	return -1;
}

int bar_up_count = 0;
int bar_down_count = 0;
int my_on_bar(Internal_Bar* bar_quote) {
	printf("Bar data: symbol %s, int_time %d, open %f, close %f, high %f, low %f\n",
		bar_quote->symbol, bar_quote->int_time, bar_quote->open, bar_quote->close, bar_quote->high, bar_quote->low);
	LOG_LN("Bar data: symbol %s, int_time %d, open %f, close %f, high %f, low %f",
		bar_quote->symbol, bar_quote->int_time, bar_quote->open, bar_quote->close, bar_quote->high, bar_quote->low);
	
	//write your bar logic here.
	if( bar_quote->close > bar_quote->open ) {
		if(bar_up_count == 1) {
			Contract *instr = sdp_handler->find_contract(bar_quote->symbol);
			sdp_handler->long_short(instr, 5, last_ask_price, last_bid_price, 2);
		}
		bar_up_count++;
		bar_down_count = 0;
	}

	if (bar_quote->close < bar_quote->open) {
		if (bar_down_count == 1) {
			Contract *instr = sdp_handler->find_contract(bar_quote->symbol);
			sdp_handler->long_short(instr, -5, last_ask_price, last_bid_price, 2);
		}
		bar_down_count++;
		bar_up_count = 0;
	}
	return 0;
}

int my_on_response(int type, int length, void *resp) {
	if (sdp_handler != NULL) {
		sdp_handler->on_response(type, length, resp);

		/* write your logic here */
		st_response_t* rsp = (st_response_t*)((st_data_t*)resp)->info;
		switch (rsp->status) {
		case SIG_STATUS_PARTED:
			LOG_LN("OrderID: %lld Strategy received Partial Filled: %s %s %d@%f",
				rsp->order_id,
				rsp->symbol, (rsp->direction == ORDER_BUY ? "Bought" : "Sold"),
				rsp->exe_volume, rsp->exe_price);
			break;
		case SIG_STATUS_SUCCEED:
			LOG_LN("OrderID: %lld Strategy received Filled: %s %s %d@%f",
				rsp->order_id,
				rsp->symbol, (rsp->direction == ORDER_BUY ? "Bought" : "Sold"),
				rsp->exe_volume, rsp->exe_price);
			break;
		case SIG_STATUS_CANCELED:
			LOG_LN("OrderID: %lld Strategy received Canceled", rsp->order_id);
			break;
		case SIG_STATUS_CANCEL_REJECTED:
			LOG_LN("OrderID: %lld Strategy received CancelRejected", rsp->order_id);
			break;
		case SIG_STATUS_REJECTED:
			LOG_LN("OrderID: %lld Strategy received Rejected, Error: %s",
				rsp->order_id, rsp->error_info);
			break;
		case SIG_STATUS_INTRREJECTED:
			LOG_LN("OrderID: %lld Strategy received Rejected, Error: %s",
				rsp->order_id, rsp->error_info);
			break;
		case SIG_STATUS_INIT:
			LOG_LN("OrderID: %lld Strategy received PendingNew", rsp->order_id);
			break;
		case SIG_STATUS_ENTRUSTED:
			LOG_LN("OrderID: %lld Strategy received Entrusted, %s %s",
				rsp->order_id, rsp->symbol,
				(rsp->direction == ORDER_BUY ? "Buy" : "Sell"));
			break;
		}
		return 0;
	}
	return -1;
}

int my_on_timer(int type, int length, void *info) {
	if (sdp_handler != NULL) {
		sdp_handler->on_book(type, length, info);

		/* write your logic here */
		printf("on timer!\n");
		return 0;
	}
	return -1;
}

void my_destroy() {
	if (sdp_handler != NULL) {
		destory_bar();
		delete sdp_handler;
		sdp_handler = NULL;
	}
}