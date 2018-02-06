#include "strategy_interface.h"
#include "sdp_handler/core/sdp_handler.h"
#include "sdp_handler/utils/log.h"

static SDPHandler *sdp_handler = NULL;
static st_config_t *g_config = NULL;

int my_st_init(int type, int length, void *cfg) {
	if (sdp_handler == NULL) {
		sdp_handler = new SDPHandler(type, length, cfg);

		/* write your logic here */
		LOG_LN("Strategy Init!");
		g_config = (st_config_t *)cfg;
		char* account = g_config->accounts[0].account;
		printf("Your accont cash is: %f\n", sdp_handler->get_account_cash(account));
	}
	return 0;
}

static bool first_order_flag = true;

int my_on_book(int type, int length, void *book) {
	if (sdp_handler != NULL) {
		sdp_handler->on_book(type, length, book);

		/* write your logic here */
		switch (type) {
		case DEFAULT_STOCK_QUOTE: {
			Stock_Internal_Book *s_book = (Stock_Internal_Book *)((st_data_t*)book)->info;
			//sample logic
			if (first_order_flag) {
				Contract *instr = sdp_handler->find_contract(s_book->ticker);
				if (instr->ap1 == 0 || instr->bp1 == 0) return 0;
				sdp_handler->send_twap_order(instr, 1000, ORDER_BUY, 10000, 90000000, 110000000);
				first_order_flag = false;
			}
			break;
		}
		case DEFAULT_FUTURE_QUOTE: {
			Futures_Internal_Book *f_book = (Futures_Internal_Book *)((st_data_t*)book)->info;
			//sample logic
			if (first_order_flag) {
				Contract *instr = sdp_handler->find_contract(f_book->symbol);
				if (instr->ap1 == 0 || instr->bp1 == 0) return 0;
				sdp_handler->send_single_order(instr, instr->exch, instr->ap1 + 3 * instr->tick_size, 1, ORDER_BUY, ORDER_OPEN);
				first_order_flag = false;
			}/* else {
			 sdp_handler->cancel_all_orders();
			 }*/
			break;
		}
		}
		return 0;
	}
	return -1;
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
		delete sdp_handler;
		sdp_handler = NULL;
	}
}