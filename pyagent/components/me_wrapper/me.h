#if !defined(_ME_H_DDCC932B_83E4_4D5E_BC8A_A8D12002D92A)
#define  _ME_H_DDCC932B_83E4_4D5E_BC8A_A8D12002D92A

#include "my_trade.h"
#include "order_types_def.h"

typedef TradeHandler trade_t;

/**
static void *
trader_init(int me_type);

static int
trader_load_product(void* trade, void *fee_field, int len);

static int
trader_load_config(void* trade, struct match_param_t *conf, int len);

static void
trader_destroy(void *trade);

static void
trader_clear(void* trade);

static void
trader_cancel_order(void* trade, order_request* cl_ord, signal_response *ord_rtn);

static void
trader_place_order(void *trade, order_request* pl_ord, signal_response *ord_rtn);

static void
trader_feed_quote(void *trade, void* quote, int q_type, signal_response** trd_rtn_ar,
                  int* rtn_cnt, int market, bool is_bar=false);
**/

static void *
trader_init(int me_type)
{
        trade_t *trade = new TradeHandler(me_type);
        return trade;
}

static int
trader_load_product(void *trade, product_info *pi, int cnt)
{
        int ret = ((trade_t*)trade)->load_product(pi, cnt);
#if (MESW >0)
        int i;
        for (i = 0; i < cnt; i++) {
            printf("[M] PR %s, %s %d %.3f,%.3f,%d, %c\n",
                       pi[i].name, pi[i].prefix,
                       pi[i].fee_fixed,
                       pi[i].rate, pi[i].unit, pi[i].multiple,
                       pi[i].xchg_code);
        }
#endif
        return ret;
}

static int
trader_load_config(void* trade, mode_config *conf, int cnt)
{
        int ret = ((trade_t *)trade)->load_config(conf, cnt);
#if (MESW >0)
        int i;
        for (i = 0; i < cnt; i++) {
            printf("[M] CF %s, %c %d %.3f\n",
                    conf[i].product, conf[i].exch,
                    conf[i].mode, conf[i].trade_ratio);
        }
#endif
        return ret;
}


static void
trader_destroy(void *trade)
{
        delete (trade_t*)trade;
}

/*
static void
trader_clear(void *trade)
{
        ((trade_t*)trade)->clear();
}
*/

static void
trader_cancel_order(void *trade, order_request* cl_ord, signal_response *ord_rtn)
{
        ((trade_t*)trade)->cancel_order(cl_ord, ord_rtn);

#if (MESW >0)
        printf("[M] CO %ld,%ld,%s,%d,%d,%d\n",
                        cl_ord->order_id, cl_ord->org_order_id,
                        cl_ord->symbol,
                        cl_ord->direction, cl_ord->open_close,
                        cl_ord->investor_type
              );

        printf("[M] COR %d,%d,%lu,%s,%d,%d,%.3f,%d,%d\n",
                        ord_rtn->entrust_no, ord_rtn->strat_id,
                        ord_rtn->serial_no, ord_rtn->symbol,
                        ord_rtn->direction, ord_rtn->open_close,
                        ord_rtn->exe_price, ord_rtn->exe_volume,
                        ord_rtn->status);
#endif
}

static void
trader_place_order(void* trade, order_request* pl_ord, signal_response *ord_rtn)
{
        ((trade_t*)trade)->place_order(pl_ord, ord_rtn);
#if (MESW >0)
        printf("[M] PO %lu,%s,%.3f,%d,%d,%d,%ld,%d,%d\n",
                        pl_ord->order_id, pl_ord->symbol,
                        pl_ord->limit_price, pl_ord->direction,
                        pl_ord->open_close, pl_ord->investor_type,
                        pl_ord->order_size,pl_ord->time_in_force,
                        pl_ord->order_type);

        printf("[M] POR %d,%d,%lu,%s,%d,%d,%.3f,%d,%d\n",
                        ord_rtn->entrust_no, ord_rtn->strat_id,
                        ord_rtn->serial_no, ord_rtn->symbol,
                        ord_rtn->direction, ord_rtn->open_close,
                        ord_rtn->exe_price, ord_rtn->exe_volume,
                        ord_rtn->status);
#endif
}

#if (MESW >1)
/* print quote */
static void
print_ib(void *quote_data)
{
        //struct st_quote_internal_book *quote = (struct st_quote_internal_book*)quote_data;
        struct InternalBook *quote = (struct InternalBook*)quote_data;

        printf("[M] QU %d,%d,%.3f,%ld,%.3f\n",
                        quote->book_type,quote->int_time,
                        quote->total_notional, quote->total_vol,
                        quote->last_px);
        printf("[M] QUB %.2f,%.2f,%.2f,%d,%d,%d\n",
                        quote->bp_array[0], quote->bp_array[1], quote->bp_array[2],
                        quote->bv_array[0], quote->bv_array[1], quote->bv_array[2]);
        printf("[M] QUA %.2f,%.2f,%.2f,%d,%d,%d\n",
                        quote->ap_array[0], quote->ap_array[1], quote->ap_array[2],
                        quote->av_array[0], quote->av_array[1], quote->av_array[2]);
}
#endif

static void
trader_feed_quote(void* trade, void* quote, int q_type, signal_response** trd_rtn_ar,
                  int* rtn_cnt, int market, bool is_bar)
{
        ((trade_t*)trade)->feed_quote(quote, q_type, trd_rtn_ar, rtn_cnt, market, is_bar);
#if (MESW >1)
        print_ib(quote);
#endif

#if (MESW >0)
        int i;
        for(i=0;i<*rtn_cnt; ++i) {
                signal_response *tr= &((*trd_rtn_ar)[i]);
                printf("[M] TR(%d) %lu,%d,%d,%.3f,%s,%d,%d,%d\n",
                                i,
                                tr->serial_no, tr->entrust_no,
                                tr->exe_volume, tr->exe_price,
                                tr->symbol, tr->direction,
                                tr->open_close,tr->status);
        }

#endif
}

#endif // _ME_H_DDCC932B_83E4_4D5E_BC8A_A8D12002D92A
