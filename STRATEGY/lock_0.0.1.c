#include "strategy_interface.h"
#include "core/sdp_handler.h"
#include "utils/log.h"
#include "utils/utils.h"
#include <quote_format_define.h>
#include <stdio.h>
#include <math.h>
static SDPHandler *sdp_handler = NULL;
static st_config_t *g_config = NULL;

#include "utils/k.h"
#include <string>

#define KDBLEN 1024

//#define SIMULATION 
//#define BREAK_GO_ON 
//

#define MAX_DEAL_NUM 20
#define PORT 5050


static int CountTodayDeal = 0;



static int count = 0;

static int tick_time = 0;
static int int_time = 0;
static double last_ask_price;
static double last_bid_price;
static char local_time[64];


static int kdb_handle = khpu("localhost",PORT, "");
static char kdb_sql[KDBLEN];
static int RoundCount = 0;
static int cuSignalcount = 0;



//debug:

//static int flag1 = 1;
//static int flag2 = 0;




int my_st_init(int type, int length, void *cfg) {
	if (sdp_handler == NULL) {
		sdp_handler = new SDPHandler(type, length, cfg);

#ifdef BREAK_GO_ON
		k(-kdb_handle, "ibAskpx:0f;ibBidpx:0f;system \"l tbls\"; system \"cd ..\"", (K)0);
#endif

		/* write your logic here */
		char init_kdb_sql[KDBLEN] = "int2time:{\"T\"$-9#\"00000000\",string x};if[not `quoteData in tables `.;ibAskpx:0f;ibBidpx:0f;res:([]a:enlist 0i;b:enlist 0f;c:enlist 0f;d:enlist 0i);quoteData:();FinalSignal2:();"
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

void cancel_old_order(int tick_time) {
	list_t *pos, *n;
	Order *l_ord;

	list_t *ord_list = sdp_handler->m_orders->get_order_by_side(ORDER_BUY);
	list_for_each_prev_safe(pos, n, ord_list) {
		l_ord = list_entry(pos, Order, pd_link);
		if (tick_time >= get_seconds_from_int_time(l_ord->insert_time) + 1) {
			PRINT_ERROR("Order in OrderList Waiting over 6 Secs; cur time %d, insert time %d, Cancelling...\n", int_time, get_seconds_from_int_time(l_ord->insert_time));
			LOG_LN("Order in OrderList Waiting over 6 Secs; cur time %d, insert time %d, Cancelling...\n", int_time, get_seconds_from_int_time(l_ord->insert_time));
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

int my_on_book(int type, int length, void *book) {
	if (sdp_handler == NULL) return -1;
	sdp_handler->on_book(type, length, book);

	Futures_Internal_Book *f_book = (Futures_Internal_Book *)((st_data_t*)book)->info;
	int_time = f_book->int_time;
    LOG("int_time:%d, ap1 = %f\n",int_time,f_book->ap_array[0]);

	//6秒撤单机制
	tick_time = get_seconds_from_int_time(f_book->int_time);
	cancel_old_order(tick_time);

#ifndef SIMULATION 
	get_time_record(local_time);

	//当行情时间和本地时间相差5分钟，认为是无效行情，注意这里的时间已做过0点处理。
	int l_local_time = get_seconds_from_char_time(local_time);
	//PRINT_INFO("tick_time:%d,local_time:%d", tick_time, l_local_time);
//	if (abs(l_local_time - tick_time) > 5 * 60) {
//		PRINT_ERROR("No Use Quote,QUOTE TIME: %d ,LOCAL TIME:%s, delta:%d", int_time, local_time, abs(l_local_time - tick_time));
//		return -1;
//	}
#endif

	//过滤一些极端时间段
//	if (not_working_time(int_time)) return -1;
//	if (int_time < 40000000) int_time += 240000000;
//	if (is_working_time(int_time) == false)	return -1;

	last_ask_price = Round(f_book->ap_array[0]);
	last_bid_price = Round(f_book->bp_array[0]);

    printf("IH askprice:%lf, bidprice:%lf\n",last_ask_price,last_bid_price);
	//将最新的行情推进kdb同时执行kdb策略代码
	sprintf(kdb_sql, "ctpAskpx:%lf;ctpBidpx:%lf;", last_ask_price, last_bid_price);
	k(kdb_handle, kdb_sql, (K)0);
	sprintf(kdb_sql, "quote:update Date:.z.D+int2time %d,LegOneBid1:%s,LegOneAsk1:%s,LegTwoBid1:%s,LegTwoAsk1:%s from quote;system \"l strategy.q\"",
        int_time,
		"ibBidpx",
		"ibAskpx",
		"ctpBidpx",
		"ctpAskpx"
	);
    LOG_LN("kdb_sql:%s\n",kdb_sql);
	k(-kdb_handle, kdb_sql, (K)0);

	//为防止kdb中行情表格过大，每600个tick清理一次kdb表格。只保留2000行。
	if (++count % 600 == 0)
		k(-kdb_handle, "quoteData: select [-2000] from quoteData;", (K)0);

	//从kdb获取信号。信号包括四个东西
	K table = k(kdb_handle, "select from res ", (K)0);
	K values = kK(table->k)[1];
	K col1_length = kK(values)[0];
	K col2_bid1 = kK(values)[1];
	K col3_ask1 = kK(values)[2];
	K col4_signal = kK(values)[3];

	Contract *instr = sdp_handler->find_contract(f_book->symbol);

	for (int i = 0; i < col1_length->n; i++) {
		//这是从kdb获取的四个东西。
		int length = kI(col1_length)[i]; //长度，用于猝发是否产生信号
		double bid1 = Round(kF(col2_bid1)[i]);  //bid price
		double ask1 = Round( kF(col3_ask1)[i]);   //ask price
		int signal = kI(col4_signal)[i];  //short or long
		LOG_LN("KDB: %d %f %f %d", kI(col1_length)[i], kF(col2_bid1)[i],kF(col3_ask1)[i],kI(col4_signal)[i]);

        //bid1 = last_bid_price; 
        //ask1  = last_ask_price; 

        int today_long_pos = long_position(instr) - instr->pre_long_position;
        if (today_long_pos<0) today_long_pos=0;
        int today_short_pos = short_position(instr) - instr->pre_short_position  ;
        if (today_short_pos<0) today_short_pos=0;
        LOG_LN("POSITION long:%d,short:%d,all:%d",today_long_pos,today_short_pos,today_long_pos + today_short_pos);

		if (RoundCount == 0) {
			LOG_LN("enter if, length %d, RoundCount %d", length, RoundCount);
			cuSignalcount = length;
			RoundCount++;
		}
		else if (length > cuSignalcount /*|| flag1*/ ) 
        {
			LOG_LN("enter elseif, length %d", length);
			cuSignalcount = length;
			//产生Short信号
            if (signal == -1 /*count %30 ==0 && flag2== 0*/) {
               // flag2 = 1;
                LOG_LN("SHORT %d", int_time);

                if(today_long_pos+today_short_pos == MAX_DEAL_NUM-1){
                    LOG_LN("A1 %d", int_time);
                    sdp_handler->send_single_order(instr, instr->exch, bid1, 1, ORDER_SELL, ORDER_OPEN);
                }
                else if(today_long_pos+ today_short_pos <MAX_DEAL_NUM-1){
                    LOG_LN("A2 %d", int_time);
                    if(today_long_pos >0){
                        LOG_LN("A3 %d", int_time);
                        sdp_handler->send_single_order(instr, instr->exch, bid1, 1, ORDER_SELL, ORDER_OPEN);
                        sdp_handler->send_single_order(instr, instr->exch, bid1, 1, ORDER_SELL, ORDER_OPEN);
                    }
                    else if(today_long_pos ==0 && instr->pre_long_position >=2){
                        LOG_LN("A4 %d", int_time);
                        if(CountTodayDeal == 0){
                            LOG_LN("A5 %d", int_time);
                            CountTodayDeal=1;
                            sdp_handler->send_single_order(instr, instr->exch, bid1, 1, ORDER_SELL, ORDER_CLOSE_YES);
                        }
                        else{
                            LOG_LN("A6 %d", int_time);
                            sdp_handler->send_single_order(instr, instr->exch, bid1, 1, ORDER_SELL, ORDER_CLOSE_YES);
                            sdp_handler->send_single_order(instr, instr->exch, bid1, 1, ORDER_SELL, ORDER_CLOSE_YES);
                        }
                    }
                    else if(today_long_pos == 0 && instr->pre_long_position ==1){
                        LOG_LN("A7 %d", int_time);
                        if(CountTodayDeal ==0){
                            LOG_LN("A8 %d", int_time);
                            CountTodayDeal = 1; 
                            sdp_handler->send_single_order(instr, instr->exch, bid1, 1, ORDER_SELL, ORDER_CLOSE_YES);
                        }
                        else{
                            LOG_LN("A9 %d", int_time);
                            sdp_handler->send_single_order(instr, instr->exch, bid1, 1, ORDER_SELL, ORDER_CLOSE_YES);
                            sdp_handler->send_single_order(instr, instr->exch, bid1, 1, ORDER_SELL, ORDER_OPEN);
                        }

                    }
                    else if(today_long_pos ==0 && instr->pre_long_position == 0){
                        LOG_LN("A10 %d", int_time);
                        if(CountTodayDeal == 0) {
                            LOG_LN("A11 %d", int_time);
                            CountTodayDeal =1;
                            sdp_handler->send_single_order(instr, instr->exch, bid1, 1, ORDER_SELL, ORDER_OPEN);
                        }
                        else{
                            LOG_LN("A12 %d", int_time);
                            sdp_handler->send_single_order(instr, instr->exch, bid1, 1, ORDER_SELL, ORDER_OPEN);
                            sdp_handler->send_single_order(instr, instr->exch, bid1, 1, ORDER_SELL, ORDER_OPEN);
                        }
                    }
                }
                else if(today_long_pos + today_short_pos >MAX_DEAL_NUM-1){
                   LOG_LN("FILLED TODAY");
                }
            }
            else if (signal == 1  /*count%30==0 && flag2 ==1*/) {
                //flag2 =0;
                LOG_LN("LONG %d", int_time);
                if(today_long_pos+today_short_pos == MAX_DEAL_NUM-1){

                    LOG_LN("B1 %d", int_time);
                    sdp_handler->send_single_order(instr, instr->exch, ask1, 1, ORDER_BUY, ORDER_OPEN);
                }
                else if(today_long_pos+ today_short_pos <MAX_DEAL_NUM-1){
                    LOG_LN("B2 %d", int_time);
                    if(today_short_pos >0){
                        LOG_LN("B3 %d", int_time);
                        sdp_handler->send_single_order(instr, instr->exch, ask1, 1, ORDER_BUY, ORDER_OPEN);
                        sdp_handler->send_single_order(instr, instr->exch, ask1, 1, ORDER_BUY, ORDER_OPEN);
                    }
                    else if(today_short_pos ==0 && instr->pre_short_position >=2){
                        LOG_LN("B4 %d", int_time);
                        if(CountTodayDeal == 0){
                            LOG_LN("B5 %d", int_time);
                            CountTodayDeal=1;
                            sdp_handler->send_single_order(instr, instr->exch,ask1, 1, ORDER_BUY, ORDER_CLOSE_YES);
                        }
                        else{
                            LOG_LN("B6 %d", int_time);
                            sdp_handler->send_single_order(instr, instr->exch,ask1, 1, ORDER_BUY, ORDER_CLOSE_YES);
                            sdp_handler->send_single_order(instr, instr->exch,ask1, 1, ORDER_BUY, ORDER_CLOSE_YES);
                        }
                    }
                    else if(today_short_pos == 0 && instr->pre_short_position ==1){
                        LOG_LN("B7 %d", int_time);
                        if(CountTodayDeal ==0){
                            LOG_LN("B8 %d", int_time);
                            CountTodayDeal =1;
                            sdp_handler->send_single_order(instr, instr->exch, ask1, 1, ORDER_BUY, ORDER_CLOSE_YES);
                        }
                        else{
                            LOG_LN("B9 %d", int_time);
                        sdp_handler->send_single_order(instr, instr->exch, ask1, 1, ORDER_BUY, ORDER_CLOSE_YES);
                        sdp_handler->send_single_order(instr, instr->exch, ask1, 1, ORDER_BUY, ORDER_OPEN);
                        }
                    }
                    else if(today_short_pos ==0 && instr->pre_short_position ==0){
                        LOG_LN("B10 %d", int_time);
                        if(CountTodayDeal ==  0) {
                            LOG_LN("B11 %d", int_time);
                            CountTodayDeal = 1;
                            sdp_handler->send_single_order(instr, instr->exch, ask1, 1, ORDER_BUY, ORDER_OPEN);
                        }
                        else{
                            LOG_LN("B12 %d", int_time);
                            sdp_handler->send_single_order(instr, instr->exch, ask1, 1, ORDER_BUY, ORDER_OPEN);
                            sdp_handler->send_single_order(instr, instr->exch, ask1, 1, ORDER_BUY, ORDER_OPEN);
                        }
                    
                    }
                }
                else if(today_long_pos + today_short_pos >MAX_DEAL_NUM-1){
                    LOG_LN("FILLED TODAY");
                }

            }
        }
	}
	//释放 内存，从kdb获得的res 信号表。
	r0(table);
	return 0;

}



int my_on_response(int type, int length, void *resp) {
	if (sdp_handler == NULL) return -1;
	sdp_handler->on_response(type, length, resp);

	st_response_t* rsp = (st_response_t*)((st_data_t*)resp)->info;
	Contract *instr = sdp_handler->find_contract(rsp->symbol);
    Order* l_ord = NULL;

	switch (rsp->status) {
	case SIG_STATUS_PARTED:
		break;
	case SIG_STATUS_SUCCEED:
		break;
	case SIG_STATUS_CANCEL_REJECTED:
	case SIG_STATUS_CANCELED:
		PRINT_WARN("[%d]Cancel Succeed!", int_time);
		LOG_LN("[%d]Cancel Succeed!", int_time);
	case SIG_STATUS_REJECTED:
		PRINT_ERROR("Send order again");
		LOG_LN("Send order again");
        //l_ord = sdp_handler->m_orders->query_order(rsp->order_id);
		if(rsp->direction == ORDER_BUY)
			sdp_handler->send_single_order(instr, instr->exch, last_ask_price,rsp->exe_volume, (DIRECTION)rsp->direction, (OPEN_CLOSE)rsp->open_close);
		else
			sdp_handler->send_single_order(instr, instr->exch, last_bid_price,rsp->exe_volume, (DIRECTION)rsp->direction, (OPEN_CLOSE)rsp->open_close);
		break;
	case SIG_STATUS_INTRREJECTED:
		break;
	case SIG_STATUS_INIT:
		break;
	case SIG_STATUS_ENTRUSTED:
		PRINT_ERROR("[%d]Has Been in OrderList;Waiting Deal...",int_time);
		LOG_LN("[%d]Has Been in OrderList;Waiting Deal...", int_time);
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

		PRINT_ERROR("[%d]Aaron> ERROR or DISCONNECT,Come into error_deal\n",int_time);
		PRINT_ERROR("[%d]save kdb tables:", int_time);
		LOG_LN("[%d]save kdb tables:",int_time);
		char sql[KDBLEN] = "{save hsym `$\"tbls/\",string x } each  tables `.";
		k(-kdb_handle, sql, (K)0);
	}
}
