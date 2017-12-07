#ifndef _MATCH_RECORDER_H_
#define _MATCH_RECORDER_H_

#include "quote_convert.h"

#define RSS_RESULT_FILE_SIZE 16777216	// 8M
#define CUR_VERSION "1.0.0"
#define UINT16 unsigned short
#define UINT32 unsigned int
#define MAX_ORD_PER_TICK 64

#define MAX_VERSION_LEN 16
#define MAX_QUOTE_DATE 16
#define MAX_RSS_DATE 16
#define MAX_CONTR_LEN 32
#define MAX_ITEM_LEN 32
#define MAX_TICK_SIZE 100000	// about 100k
#define ST_NAME_LEN 128

/* trade assumption structure */
struct res_trade_assume {
        double trade_pct;
};

struct res_st_unit {
        char st_name[64];
        UINT32 st_id;
        UINT32 st_vol;
};

struct res_st_data {
        UINT32 count;

        /* vector of struct res_st_unit */
        struct my_vector *st_vec;
};

struct res_rec_idx_data {
        UINT32 count;

        /* vector of UINT32 */
        struct my_vector *rec_off_vec;
};

enum {
        DATA_T_QUOTE = 0,
        DATA_T_PL_ORDER,
        DATA_T_CL_ORDER,
        DATA_T_MY_TRADE,
        DATA_T_PNL,
        DATA_T_MKT_TRADE,
        DATA_T_SIZE,
};

struct res_rec_data_def {
        UINT32 id_val;
        UINT32 off_data;
};

struct res_record_header {
        UINT32 count;

        /* vector of struct rec_data_def */
        struct my_vector *data_vec;
};

struct res_quote_unit {
        double bp;
        UINT32 bv;
        double sp;
        UINT32 sv;
};

struct res_quote_data {
        UINT32 count;
        double amount;
        UINT32 volume;
        double last_price;

       //struct res_quote_unit bspv_ar[0];
};

struct res_pl_order_unit {
        UINT32 sn;
        UINT32 dir;
        double price;
        UINT32 vol;
};

struct res_cl_order_unit {
        UINT32 sn;
        UINT32 org_sn;
        UINT32 dir;
        double price;
};

struct res_my_trade_unit {
        UINT32 sn;
        double price;
        UINT32 vol;
        UINT32 dir;
};

struct res_mkt_trade_unit {
        double price;
        UINT32 vol;
};

struct res_pnl_unit {
        UINT32 st_id;
        UINT32 tot_b_vol;
        UINT32 tot_s_vol;
        UINT32 pl_ord_count;
        UINT32 cl_ord_count;
        double fee;
        double net_pnl;
};

struct trade_info {
        //	int st_id;
        int total_vol;
        int b_total_vol;
        int s_total_vol;
        int pl_ord_count;
        int cl_ord_count;
        double fee;
        double income;
};

struct st_cfg {
        char contr[MAX_CONTR_LEN];
        char st_name[ST_NAME_LEN];
        unsigned int st_id;
        int vol;
};

struct st_info {
        int id;
        struct st_cfg cfg;
        struct trade_info trade;
};


struct res_basic_info {
        char version[MAX_VERSION_LEN];
        char quote_date[MAX_QUOTE_DATE];
        char rss_date[MAX_RSS_DATE];
        char contr[MAX_CONTR_LEN];
        int day_night; /* day --> 0 , night--->1 */

        /* item config */
        char item[MAX_ITEM_LEN];
        double min_change;
        unsigned int multiple;
        unsigned int fee_fixed;
        double fee_rate;
};


struct tick_data {
        int quote_idx;

        int pl_ord_cnt;
        int pl_ord_idx_ar[MAX_ORD_PER_TICK];

        int cl_ord_cnt;
        int cl_ord_idx_ar[MAX_ORD_PER_TICK];

        int mkt_trd_cnt;
        int mkt_trd_idx_ar[MAX_ORD_PER_TICK];

        int my_trd_cnt;
        int my_trd_idx_ar[MAX_ORD_PER_TICK];
};

struct rss_res_mgnt {
        unsigned int magic;

        /* basic info */
        char version[16];
        char quote_date[16];
        char rss_date[16];
        char contr[32];
        int day_night; /* day-->0 , night--->1 */

        /* item config */
        char item[32];
        double min_change;
        unsigned int multiple;
        unsigned int fee_fixed;
        double fee_rate;

        /* quote */
        double last_price;
        double high_limit;
        double low_limit;

        /* binary data structure */

        unsigned int st_cnt;
        unsigned int off_st_data;

        unsigned int tick_cnt;
        unsigned int off_tick_idx_data;

        unsigned int quote_cnt;
        unsigned int off_quote_data;

        unsigned int pl_ord_cnt;
        unsigned int off_pl_ord_data;

        unsigned int cl_ord_cnt;
        unsigned int off_cl_ord_data;

        unsigned int mkt_trd_cnt;
        unsigned int off_mkt_trd_rtn_data;

        unsigned int my_trd_cnt;
        unsigned int off_my_trd_data;

        /* trade assume info */
        unsigned int off_trade_asm;
};

struct rss_res_tick_index {
        /* file offset of this structure. */
        unsigned int off_tick_data;

        unsigned short pl_ord_cnt;
        unsigned short cl_ord_cnt;
        unsigned short my_trd_cnt;
        unsigned short mkt_trd_cnt;
        unsigned short reserved;
};

struct rss_res_tick {
        /* relative offset */
        unsigned short off_pl_ord;
        unsigned short off_cl_ord;
        unsigned short off_my_trd;
        unsigned short off_mkt_trd;
};

/* Size: 106,001,336 Bytes */
struct rss_recorder {
        struct rss_res_mgnt mgnt;
        char log_dir[1024];

        int cur_tick_num;
        struct rss_res_tick_index *cur_tick_index;
        struct tick_data *cur_tick_data;
        struct tick_data tick_data_ar[MAX_TICK_SIZE];
        struct rss_res_tick_index tick_idx_ar[MAX_TICK_SIZE];

        struct my_vector *vec_st;
        struct my_vector *vec_quote;
        struct my_vector *vec_pl_ord;
        struct my_vector *vec_cl_ord;
        struct my_vector *vec_mkt_trd;
        struct my_vector *vec_my_trd;

        struct res_trade_assume tra_asm;
        /* memory buffer as result output.*/
        void *res_mem;
};

struct res_header {
        UINT32 magic;
        char version[16];
        char quote_date[16];
        char rss_date[16];
        char item[32];
        char contr[32];

        double min_change;
        UINT32 multiple;
        UINT32 fee_fixed;
        double fee_rate;

        double last_price;
        double high_limit;
        double low_limit;
        UINT32 off_trade_assume_data;
        UINT32 off_st_data;
        UINT32 off_rec_idx_data;
        UINT32 off_pnl_data;
};


#endif
