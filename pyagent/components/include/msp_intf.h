#if !defined(_MSP_H_56A4D8AD_4951_4E9D_B329_4540D620E3E6)
#define _MSP_H_56A4D8AD_4951_4E9D_B329_4540D620E3E6
#include <stdint.h>

#ifndef MAX_NAME_SIZE
#define MAX_NAME_SIZE (64)
#endif 

struct InternalBook {
        int    contract_id;     /* Contract Id */
        char   symbol[64];      /* Changed from char cTicker[8] */
        char   Wind_equity_code[32];
        int    exchange;
        char   char_time[10];
        int    int_time;        /* 090059000, HourMintueSecondMilliSeconds 1125*/
        int    feed_status;
        double pre_settle_px;
        double last_px;
        int    last_trade_size;
        double open_interest;
        unsigned long long total_vol;
        double total_notional;   /* Trade Notional, Turnover */
        double upper_limit_px;
        double lower_limit_px;
        double open_px;
        double high_px;
        double low_px;
        double pre_close_px;
        double close_px;    /* Today's close price */
        double pre_open_interest;
        double avg_px;
        double settle_px;
        double bp1;         /* Best Bid */
        double ap1;         /* Best Ask */
        int    bv1;
        int    av1;
        double bp_array[30];
        double ap_array[30];
        int    bv_array[30];
        int    av_array[30];
        int    interval_trade_vol;  /* TradeVolume between each Snapshot Updates */
        double interval_notional;   /* Trade Notional between Snapshots */
        double interval_avg_px;     /* Avg Price between Snapshot updates */
        double interval_oi_change;  /* Open Interest Change between Snapshot updates */

        int    shfe_data_flag;          /* SHFE 1-Valid Market Data, 2-Valid Entrust, 3-Both Market/Entrust are valid*/
        int    implied_bid_size[30];    /*DCE implied Bid Ask Size */
        int    implied_ask_size[30];
        int    total_buy_ordsize;     /* Total Open Buy Order Size */
        int    total_sell_ordsize;    /* Total Open Sell Order Size */
        double weighted_buy_px;       /* Weighted Buy Order Price */
        double weighted_sell_px;      /* Weighted Sell Order Price */
        double life_high;             /* LifeTime High */
        double life_low;              /* LifeTime Low */

        unsigned int num_trades;    /* Stock Lv2 Number of trades */
        long long    total_bid_vol;
        long long    total_ask_vol;
        int /*mi_type*/ book_type; /* type of the data source - mainly used to identify level 1 quote */
};

typedef struct{
	int     serial;
	int /*MI_TYPE*/ mi_type;
	int64_t local_time;
	int64_t exchange_time;
	struct InternalBook quote;
}numpy_book_data;

/* Cancel order of internal. */
typedef struct {
    char symbol[MAX_NAME_SIZE];
    uint64_t order_id;
    uint64_t org_order_id;
    uint64_t st_id; /* Strategy id */
}int_cancel_order_t;

/* Place order of internal. */
typedef struct {
    char symbol[MAX_NAME_SIZE];
    int64_t volume;
    char direction;
    char open_close;
    double limit_price;
    uint64_t order_id;
    uint64_t st_id;
    char close_dir; /* close today, close yestorday */
}int_place_order_t;


typedef struct {
    int type;
    union {
        int_cancel_order_t cancel;
        int_place_order_t place;
    };
}int_order_t;

typedef struct {
    int type;
    int_cancel_order_t cancel;
}int_order_c_t;

typedef struct {
    int type;
    int_place_order_t place;
}int_order_p_t;


#endif



