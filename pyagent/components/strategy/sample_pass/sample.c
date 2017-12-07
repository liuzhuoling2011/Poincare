#include <string.h>
#include <stdio.h>

#include "strategy_interface.h"
#include "quote_format_define.h"

static send_data   g_proc_order;
static send_data   g_send_info;
static send_data   g_recv_info;


int
my_st_init(int type, int length, void *cfg)
{
    switch (type)  {
        case DEFAULT_CONFIG:
            st_config_t *st_cfg = (st_config_t *)cfg;
            g_proc_order = st_cfg->proc_order_hdl;
            g_send_info = st_cfg->send_info_hdl;
            g_recv_info = st_cfg->pass_rsp_hdl;
            // do nothing else
            break;
    }
    return 0;
}

int
my_on_book(int type, int length, void *book)
{
    switch (type) {
        case DEFAULT_STOCK_QUOTE:
            printf("pass on_book stock\n");
            break;
        case DEFAULT_FUTURE_QUOTE:
            printf("pass on_book future\n");
            g_proc_order(type, length, book);
            break;
        case S_PLACE_ORDER_DEFAULT:
        case S_CANCEL_ORDER_DEFAULT:
            g_proc_order(type, length, book);
            break;
        default:
            g_send_info(type, length, book);
    }
    return 0;
}

int
my_on_response(int type, int length, void *resp)
{
    printf("pass before call recv father\n");
    g_recv_info(type, length, resp);
    printf("pass after call recv father\n");
    return 0;
}

int
my_on_timer(int type, int length, void *resp)
{
    //printf("on_timer api\n");
    return 0;
}

void
my_destroy()
{
    // do nothing
}
