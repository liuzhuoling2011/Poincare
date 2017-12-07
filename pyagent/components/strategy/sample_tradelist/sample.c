#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "strategy_interface.h"
#include "quote_format_define.h"
#include "msp_intf.h"

// open it for temporary test
#define WITHDATE

static send_data   g_proc_order;
static send_data   g_send_info;
static send_data   g_recv_info;
static int         f_tick;
static int         g_date;

#ifdef WITHDATE
typedef struct
{
    int date;
    char symbol[64];
    double weight;
    int size;
    double limit_price;
    char algo[64];
    char start[64];
    char end[64];
} trade_cmd_t;
#else
typedef struct
{
    char symbol[64];
    double weight;
    int size;
    double limit_price;
    char algo[64];
    char start[64];
    char end[64];
} trade_cmd_t;
#endif

typedef struct
{
    int cnt;
    trade_cmd_t cmds[1024];
} tradelist_t;

static tradelist_t s_tradelist;

static int
read_tradelist(const char *file)
{
    if (access(file, F_OK) != 0) {
        return -1;
    }
    FILE * fp = fopen(file, "r");
    if (fp == NULL) {
        return -1;
    }
    char rline[150], *line;
    char temp[64];
    size_t len = 0;
    ssize_t read;
    s_tradelist.cnt = 0;
    // bypass the title
    read = getline(&line, &len, fp);
    if (read == -1) {
        return -1;
    }
    while(1) {
        if (fgets(rline, 150, fp) == NULL) break;
        line = rline;
        trade_cmd_t *it = &s_tradelist.cmds[s_tradelist.cnt++];
#ifdef WITHDATE
        len = strcspn(line, ",");
        sprintf(temp, "%.*s", (int)len, line);
        it->date = atoi(temp);
        if (it->date != g_date) {
            // skip task not send trade date
            s_tradelist.cnt--;
            continue;
        }
        line += len + 1;
#endif
        len = strcspn(line, ",");
        sprintf(temp, "%.*s", (int)len, line);
        strcpy(it->symbol, temp);
        line += len + 1;
        len = strcspn(line, ",");
        if (len == 0) {
            it->weight = -1;
        } else {
            sprintf(temp, "%.*s", (int)len, line);
            it->weight = atof(temp);
        }
        line += len + 1;
        len = strcspn(line, ",");
        sprintf(temp, "%.*s", (int)len, line);
        it->size = atoi(temp);

        line += len + 1;
        len = strcspn(line, ",");
        if (len == 0) {
            it->limit_price = -1;
        } else {
            sprintf(temp, "%.*s", (int)len, line);
            it->limit_price = atof(temp);
        }

        line += len + 1;
        len = strcspn(line, ",");
        sprintf(temp, "%.*s", (int)len, line);
        strcpy(it->algo, temp);

        line += len + 1;
        len = strcspn(line, ",");
        sprintf(temp, "%.*s", (int)len, line);
        strcpy(it->start, temp);
        line += len + 1;
        len = strcspn(line, ",");
        sprintf(temp, "%.*s", (int)len, line);
        strcpy(it->end, temp);
    }
    fclose(fp);
    return 0;
}

int
my_st_init(int type, int length, void *cfg)
{
    switch (type)  {
        case DEFAULT_CONFIG:
            st_config_t *st_cfg = (st_config_t *)cfg;
            g_proc_order = st_cfg->proc_order_hdl;
            g_send_info = st_cfg->send_info_hdl;
            g_recv_info = st_cfg->pass_rsp_hdl;
            // read and parse the config file
            g_date = st_cfg->trading_date;
            read_tradelist(st_cfg->param_file_path);
            break;
    }
    return 0;
}

int
time_convert(const char *time)
{
    char x[64], *it;
    int ret = 0;
    strcpy(x, time);
    it = x;
    int len = strcspn(it, ":");
    ret = atoi(it);
    it += 1 + len;
    len = strcspn(it, ":");
    ret *= 100; 
    ret += atoi(it);
    it += 1 + len;
    ret *= 1000;
    if (*it != '\0') {
        len = strcspn(it, ":");
        ret += atoi(it);
    }
    ret *= 100;
    return ret;
}

int construct_order(special_order_t *ord, int type, st_data_t *data)
{
#if 0
        char        exch;                                   /* exchange enumeration value. */
        char        symbol[64];                             /* symbol name */
        int         direction;                              /* see @enum direction */
        int         volume;                                 /* target order volume */
        double      price;                                  /* order price */
        int         start_time;                             /* algorithm start time, TWAP/VWAP use only */
        int         end_time;                               /* algorithm end time, TWAP/VWAP use only */
        int         status;                                 /* return 0 if success */
        int         sync_cancel;                            /* recieve cancel resp and then send order flag, DMA use only, 1-yes, 0-no */
#endif
    static int num = 0;
    if (num >= s_tradelist.cnt) {
        return -2;
    }
    trade_cmd_t *it = &s_tradelist.cmds[num++];
    ord->exch = '0';
    strcpy(ord->symbol, it->symbol);
    if (strncmp(ord->symbol, "000", 3) == 0 || 
        strncmp(ord->symbol, "200", 3) == 0 ||
        strncmp(ord->symbol, "001", 3) == 0 || 
        strncmp(ord->symbol, "002", 3) == 0 ||
        strncmp(ord->symbol, "300", 3) == 0) {
        ord->exch = '0';
    } else if (strncmp(ord->symbol, "600", 3) == 0 ||
                strncmp(ord->symbol, "601", 3) == 0||
                strncmp(ord->symbol, "603", 3) == 0 ||
                strncmp(ord->symbol, "900", 3) == 0) {
        ord->exch = '1';
    } else {
        assert(0);
    }
    ord->direction = 0;
    ord->volume = it->size;
    ord->price = it->limit_price;
    ord->start_time = time_convert(it->start); 
    ord->end_time = time_convert(it->end);
    if (ord->volume == 0) {
        // TODO: convert cash and weight
        ord->volume = 1;
    }
    if (strcmp(it->algo, "twap") == 0) {
        return S_PLACE_ORDER_TWAP;
    }
    if (strcmp(it->algo, "vwap") == 0) {
        return S_PLACE_ORDER_VWAP;
    }
    if (strcmp(it->algo, "dma") == 0) {
        return S_PLACE_ORDER_DMA;
    }
    return -1;
}

int
my_on_book(int type, int length, void *book)
{
    f_tick++;
    special_order_t ord;
    // send all order when tick == 1000
    int ret;
    if (f_tick == 1000) {
        while((ret = construct_order(&ord, type, (st_data_t *)book)) != -2) {
            st_data_t order_data = {0};
            order_data.info = &ord;
            if (ret != -1) {
                if (ord.volume == 0) {
                    ord.price = 0.1;
                } else {
                    ord.price = 1000000;
                }
                printf("send order: type %d, len %d\n", ret, sizeof(order_data));
                g_proc_order(ret, sizeof(order_data), (void *)&order_data);
                char msg[128] = {0};
                sprintf(msg, "cmd: %d, place order symbol:%s %d@%lf direction: %d, start: %d, end: %d\n", 
                        ret, ord.symbol, ord.volume, ord.price, ord.direction, ord.start_time, ord.end_time);
                printf("the return status: %d\n", ord.status);
                printf("%s", msg);
                g_send_info(S_STRATEGY_DEBUG_LOG, sizeof(msg), msg);
            }
        }
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
