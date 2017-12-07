#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "strategy_interface.h"
#include "quote_format_define.h"
#include "msp_intf.h"


static send_data   g_proc_order;
static send_data   g_send_info;
static send_data   g_recv_info;
static int         f_tick;
static int         do_flag = -1;


int
my_st_init(int type, int length, void *cfg)
{
    switch (type)  {
        case DEFAULT_CONFIG:
            st_config_t *st_cfg = (st_config_t *)cfg;
            g_proc_order = st_cfg->proc_order_hdl;
            g_send_info = st_cfg->send_info_hdl;
            g_recv_info = st_cfg->pass_rsp_hdl;
            char msg[128] = {0};
            sprintf(msg, "Do sample strategy init\n");
            g_send_info(S_STRATEGY_DEBUG_LOG, strlen(msg), msg);
            // do nothing else
            break;
    }
    return 0;
}

int construct_order(order_t *ord, int type, st_data_t *data)
{
    ord->org_ord_id = 0;
    if (type == DEFAULT_FUTURE_QUOTE) {
        Futures_Internal_Book *fq = (Futures_Internal_Book *)data->info;
        //ord->__trade = data->__trade;
        ord->exch = 'B';
        strcpy(ord->symbol, fq->symbol);
        //strcpy(ord->symbol, "pp1801");
        ord->volume = 1;
        ord->price = fq->last_px;
        //ord->price = 1000;
        ord->direction = ORDER_BUY;
        ord->investor_type = ORDER_SPECULATOR;
        ord->open_close = ORDER_OPEN;
        ord->order_type = ORDER_TYPE_LIMIT;
        ord->time_in_force = ORDER_TIF_DAY;
        ord->time_in_force = ORDER_TIF_DAY;
        ord->st_id = 1;
    }
    else if (type == DEFAULT_STOCK_QUOTE){
        Stock_Internal_Book *sq = (Stock_Internal_Book *)data->info; 
        //ord->__trade = data->__trade;
        ord->exch = 'B';
        //strcpy(ord->symbol, "pp1801");
        strcpy(ord->symbol, sq->ticker);
        ord->volume = 1;
        ord->price = sq->last_px;
        //ord->price = 1000;
        ord->direction = ORDER_BUY;
        ord->investor_type = ORDER_SPECULATOR;
        ord->order_type = ORDER_TYPE_LIMIT;
        ord->open_close = ORDER_OPEN;
        ord->order_type = ORDER_TYPE_LIMIT;
        ord->time_in_force = ORDER_TIF_DAY;
        ord->st_id = 1;
    } 
    return 0;
}

void
debug_future(st_data_t *book)
{
    //static int tick = 1;
    //Futures_Internal_Book *fu = (Futures_Internal_Book *)book->info;
    //printf("tick %d,symbol: %s, last_px: %lf\n", tick++, fu->symbol, fu->last_px);
}

int
my_on_book(int type, int length, void *book)
{
    order_t ord;
    debug_future((st_data_t *)book);
    f_tick++;

    if (f_tick < 2000 && do_flag == -1) {
        int ret = construct_order(&ord, type, (st_data_t *)book);
        st_data_t order_data = {0};
        order_data.info = &ord;
        if (ret == 0) {
            do_flag = 0;
            g_proc_order(S_PLACE_ORDER_DEFAULT, sizeof(order_data), (void *)&order_data);
            char msg[128] = {0};
            sprintf(msg, "place order %d@%lf direction: %d, open_close: %d\n", ord.volume, ord.price, ord.direction, ord.open_close);
            printf("%s", msg);
            g_send_info(S_STRATEGY_DEBUG_LOG, strlen(msg), msg);
            //exit(0);
        }
        //unsigned long long ord_id = ord.order_id;
    }
    return 0;
}

int
my_on_response(int type, int length, void *resp)
{
    // do nothing 
    if (type == S_STRATEGY_PASS_RSP) {
        st_data_t *td = (st_data_t *)resp;
        st_response_t *sr = (st_response_t *)td->info;
        printf("error msg is %s\n", sr->error_info);
    }
    printf("easy we receive the on_response easy\n");
    return 0;
}

int
my_on_timer(int type, int length, void *resp)
{
    // do nothing
    //printf("easy on timer api\n");
    return 0;
}

void
my_destroy()
{
    // do nothing
}
