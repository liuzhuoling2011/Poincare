#include "strategy_interface.h"
#include "core/sdp_handler.h"
#include "utils/log.h"
#include "utils/utils.h"
#include <quote_format_define.h>

static SDPHandler *sdp_handler = NULL;
static st_config_t *g_config = NULL;

#include "utils/k.h"
#include <string>

#define KDBLEN 1024
#define SIMULATION 

static int count = 0;

static int insert_time = 0;
static int tick_time = 0;
static int int_time = 0;
static double last_ask_price;
static double last_bid_price;
static char local_time[64];


static int kdb_handle = khpu("localhost", 5000, "");
static char kdb_sql[KDBLEN];
static int RoundCount = 0;
static int cuSignalcount = 0;

int my_st_init(int type, int length, void *cfg) {
	if (sdp_handler == NULL) {
		sdp_handler = new SDPHandler(type, length, cfg);

		/* write your logic here */
		char init_kdb_sql[KDBLEN] = "if[not `quoteData in tables `.;ibAskpx:0f;ibBidpx:0f;res:([]a:enlist 0i;b:enlist 0f;c:enlist 0f;d:enlist 0i);quoteData:();FinalSignal2:();"
									"quote:([]Date:();`float$LegOneBid1:();`float$LegOneAsk1:();`float$LegTwoBid1:();`float$LegTwoAsk1:());`quote insert (.z.D+.z.t;3333.0;3330.5;3.05;3.1)]";
		k(-kdb_handle, init_kdb_sql, (K)0);

		LOG_LN("Strategy Init!");
		g_config = (st_config_t *)cfg;
		char* account = g_config->accounts[0].account;
		printf("Your accont cash is: %f\n", sdp_handler->get_account_cash(account));
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

int my_on_book(int type, int length, void *book) {
	if (sdp_handler == NULL) return -1;
	sdp_handler->on_book(type, length, book);

	Futures_Internal_Book *f_book = (Futures_Internal_Book *)((st_data_t*)book)->info;
	int_time = f_book->int_time;
    LOG("int_time:%d,ap1 = %f\n",int_time,f_book->ap_array[0]);

	tick_time = get_seconds_from_int_time(f_book->int_time);

	//6�볷������
	if (insert_time > 0 && tick_time >= insert_time + 6) {
		//if 6 secs in orderlist:cancel order
		PRINT_ERROR("%d Order in OrderList Waiting over 6 Secs;CanCelling...\n", int_time);
		sdp_handler->cancel_all_orders();
	}

#ifndef SIMULATION 
	get_time_record(local_time);

	//������ʱ��ͱ���ʱ�����5���ӣ���Ϊ����Ч���飬ע�������ʱ��������0�㴦��
	int l_local_time = get_seconds_from_char_time(local_time);
	PRINT_INFO("tick_time:%d,local_time:%d", tick_time, l_local_time);
	if (abs(l_local_time - tick_time) > 5 * 60) {
		PRINT_ERROR("No Use Quote,QUOTE TIME: %d ,LOCAL TIME:%s, delta:%d", int_time, local_time, abs(l_local_time - tick_time));
		return -1;
	}
#endif

	//����һЩ����ʱ���
	if (not_working_time(int_time)) return -1;
	if (int_time < 40000000) int_time += 240000000;
	if (is_working_time(int_time) == false)	return -1;

	last_ask_price = f_book->ap_array[0];
	last_bid_price = f_book->bp_array[0];

	//�����µ������ƽ�kdbͬʱִ��kdb���Դ���
	sprintf(kdb_sql, "ctpAskpx:%lf;ctpBidpx:%lf;", last_ask_price, last_bid_price);
	k(kdb_handle, kdb_sql, (K)0);
	sprintf(kdb_sql, "quote:update Date:.z.D+.z.N,LegOneBid1:%s,LegOneAsk1:%s,LegTwoBid1:%s,LegTwoAsk1:%s from quote;system \"l strategy.q\"",
		"ibBidpx",
		"ibAskpx",
		"ctpBidpx",
		"ctpAskpx"
	);
	k(-kdb_handle, kdb_sql, (K)0);

	//Ϊ��ֹkdb�����������ÿ600��tick����һ��kdb���ֻ����2000�С�
	if (++count % 600 == 0)
		k(-kdb_handle, "quoteData: select [-2000] from quoteData;", (K)0);
	PRINT_SUCCESS("i:%d\n", count);

	//��kdb��ȡ�źš��źŰ����ĸ�����
	K table = k(kdb_handle, "select from res ", (K)0);
	K values = kK(table->k)[1];
	K col1_length = kK(values)[0];
	K col2_bid1 = kK(values)[1];
	K col3_ask1 = kK(values)[2];
	K col4_signal = kK(values)[3];

	Contract *instr = sdp_handler->find_contract(f_book->symbol);

	for (int i = 0; i < col1_length->n; i++) {
		//���Ǵ�kdb��ȡ���ĸ�������
		int length = kI(col1_length)[i]; //���ȣ�����⧷��Ƿ�����ź�
		double bid1 = kF(col2_bid1)[i];  //bid price
		double ask1 = kF(col3_ask1)[i];   //ask price
		int signal = kI(col4_signal)[i];  //short or long
											//printf("%d %f %f %d\n", kI(col1_length)[i], kF(col2_bid1)[i],kF(col3_ask1)[i],kI(col4_signal)[i]); 
		if (RoundCount == 0) {
			cuSignalcount = length;
			RoundCount++;
		}
		else if (length > cuSignalcount) {
			cuSignalcount = length;
			//����Short�ź�
			if (signal == -1) {
				if(instr->pre_long_position > 0)
					sdp_handler->send_single_order(instr, instr->exch, bid1, 1, ORDER_SELL, ORDER_CLOSE_YES);
				if(long_position(instr) > 0)
					sdp_handler->send_single_order(instr, instr->exch, bid1, 1, ORDER_SELL, ORDER_CLOSE);
				sdp_handler->send_single_order(instr, instr->exch, bid1, 1, ORDER_SELL, ORDER_OPEN);
			}
			else if (signal == 1) {
				if (instr->pre_short_position > 0)
					sdp_handler->send_single_order(instr, instr->exch, ask1, 1, ORDER_BUY, ORDER_CLOSE_YES);
				if (short_position(instr) > 0)
					sdp_handler->send_single_order(instr, instr->exch, ask1, 1, ORDER_BUY, ORDER_CLOSE);
				sdp_handler->send_single_order(instr, instr->exch, ask1, 1, ORDER_BUY, ORDER_OPEN);
			}
		}
	}
	//�ͷ� �ڴ棬��kdb��õ�res �źű�
	r0(table);
	PRINT_SUCCESS("FINISHED...");
	return 0;
}

int my_on_response(int type, int length, void *resp) {
	if (sdp_handler != NULL) {
		sdp_handler->on_response(type, length, resp);

		st_response_t* rsp = (st_response_t*)((st_data_t*)resp)->info;
		Contract *instr = sdp_handler->find_contract(rsp->symbol);

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
		case SIG_STATUS_CANCEL_REJECTED:
			LOG_LN("OrderID: %lld Strategy received CancelRejected", rsp->order_id);
		case SIG_STATUS_CANCELED:
			LOG_LN("OrderID: %lld Strategy received Canceled", rsp->order_id);
			PRINT_WARN("[%d]Cancel Succeed!\n",int_time);
		case SIG_STATUS_REJECTED:
			LOG_LN("OrderID: %lld Strategy received Rejected, Error: %s",
				rsp->order_id, rsp->error_info);

			PRINT_SUCCESS("[%d]Not into OrderList\n",int_time);
			
			PRINT_ERROR("Send again\n");
			if(rsp->direction == ORDER_BUY)
				sdp_handler->send_single_order(instr, instr->exch, last_ask_price, 1, (DIRECTION)rsp->direction, (OPEN_CLOSE)rsp->open_close);
			else
				sdp_handler->send_single_order(instr, instr->exch, last_bid_price, 1, (DIRECTION)rsp->direction, (OPEN_CLOSE)rsp->open_close);

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

			PRINT_ERROR("[%d]Has Been in OrderList;Waiting Deal...",int_time);
			PRINT_WARN("%s,%s\n", rsp->direction == ORDER_SELL ? "ORDER_SELL" : "ORDER_BUY",
				rsp->open_close == ORDER_CLOSE ? "ORDER_CLOSE" : "ORDER_OPEN");
			insert_time = get_seconds_from_int_time(int_time);
			break;
		}
		return 0;
	}
	return -1;
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

		PRINT_ERROR("[%d]Aaron> ERROR or DISCONNECT,Come into error_deal\n",int_time);
		PRINT_ERROR("[%d]save kdb tables:",int_time);
		char sql[KDBLEN] = "{save hsym `$\"tbls/\",string x } each  tables `.";
		k(-kdb_handle, sql, (K)0);
	}
}
