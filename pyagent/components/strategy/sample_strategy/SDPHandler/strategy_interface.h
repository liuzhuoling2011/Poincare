#ifndef _LF_STRATEGY_API_H_
#define _LF_STRATEGY_API_H_

#define SYMBOL_MAX       (4096)                                         /* 模型支持的合约最大个数 */
#define PATH_LEN         (2048)                                         /* 模型so/ev文件存放绝对路径最大长度 */
#define STRAT_NAME_LEN   (256)                                          /* 策略名最大长度 */
#define SYMBOL_LEN       (64)                                           /* 合约名最大长度 */
#define ACCOUNT_MAX      (64)                                           /* 模型支持的最大账户个数 */
#define ACCOUNT_LEN      (64)                                           /* 账户名最大长度 */

#ifndef _WIN32
        #define EXPORT_SYMBOL __attribute((visibility("default")))
#else
        #define EXPORT_SYMBOL 
#endif

#pragma pack(push)
#pragma pack(8)


typedef enum _CHECK_ERROR
{       
        /* error_no would be 0 when none check error found */
        ERROR_OPEN_POS_IS_NOT_ENOUGH = -1,
        ERROR_SELF_MATCHING = -2,
        ERROR_REARCH_MAX_ACCUMULATE_OPEN_VAL = -3,
        ERROR_CANCEL_ORDER_FAIL = -4,
        ERROR_CANCEL_ORDER_REARCH_LIMIT = -5,
        ERROR_CASH_IS_NOT_ENOUGH = -6,
        ERROR_PRE_POS_IS_NOT_ENOUGH_TO_CLOSE = -7,
} CHECK_ERROR;

typedef enum _DAY_NIGHT
{
        DAY = 0,
        NIGHT = 1,
} DAY_NIGHT;

typedef enum _EXCHANGE
{
        SZSE = '0',                                                     /* SHENZHEN STOCK EXCHANGE */
        SSE = '1',                                                      /* SHANGHAI STOCK EXCHANGE*/
        HKEX = '2',                                                     /* FIXME make this consistent with trader */
        SHFE = 'A',//XSGE = 'A',                                        /* SHANGHAI FUTURES EXCHANGE */
        CFFEX = 'G',//CCFX = 'G',                                       /* CHINA FINANCIAL FUTURES EXCHANGE */
        DCE = 'B', //XDCE = 'B',                                        /* DALIAN COMMODITY EXCHANGE */
        CZCE = 'C', //XZCE = 'C',                                       /* ZHENGZHOU COMMODITY EXCHANGE */
        FUT_EXCH = 'X',
        SGX = 'S',                                                      /* FIXME make this consistent with trader */
        SGE = 'D',                                                      /* shanghai gold exchange */
        CBOT = 'F',                                                     /* make this consistent with trader, eSunny Foreign */
        CME = 'M',
        LME = 'L',
        COMEX = 'O',
        NYMEX = 'N',                                                    /* TODO: Change to NYMX */
        BLANK_EXCH = '\0',
        UNDEFINED_EXCH = 'u'
} EXCHANGE;

typedef enum _CURRENCY {
        CNY = 0,                                                        /* Chinese Yuan */
        USD = 1,                                                        /* US Dollar */
        HKD = 2,                                                        /* HongKong Dollar */
        EUR = 3,                                                        /* Euro */
        JPY = 4,                                                        /* Japanese Yen */
        GBP = 5,                                                        /* Puond Sterling */
        AUD = 6,                                                        /* Australian Dollar */
        CAD = 7,                                                        /* Canadian Dollar */
        SEK = 8,                                                        /* Swedish Krona */
        NZD = 9,                                                        /* New Zealand Dollar */
        MXN = 10,                                                       /* Mexican Peso */
        SGD = 11,                                                       /* Singapore Dollar */
        NOK = 12,                                                       /* Norwegian Krone */
        KRW = 13,                                                       /* South Korean Won */
        TRY = 14,                                                       /* Turkish Lira */
        RUB = 15,                                                       /* Russian Ruble */
        INR = 16,                                                       /* Indian Rupee */
        BRL = 17,                                                       /* Brazilian Real */
        ZAR = 18,                                                       /* South African Rand */
        CNH = 19,                                                       /* Offshore Chinese Yuan */
} CURRENCY;


typedef enum _COMMAND_RET                                               /* send_info, pass_resp, proc_order return */
{
        SEND_CMD_FAILED = -1,
        SEND_CMD_SUCCESS = 0,
} COMMAND_RET;

typedef enum _DIRECTION {
        ORDER_BUY = 0,
        ORDER_SELL = 1,
        UNDEFINED_SIDE = 2,
} DIRECTION;

typedef enum _OPEN_CLOSE {
        ORDER_OPEN = 0,
        ORDER_CLOSE = 1,
        ORDER_CLOSE_TOD = 2,                                            /* For SHFE, we have to tell tunnel close today. not used now */
        ORDER_CLOSE_YES = 3,                                            /* For SHFE, we have to tell tunnel close yesterday. */
        UNDEFINED_OPEN_CLOSE = 4,
} OPEN_CLOSE;

typedef enum _INVESTOR_TYPE {
        ORDER_SPECULATOR = 0,
        ORDER_HEDGER = 1,
        ORDER_ARBITRAGEURS = 2,
} INVESTOR_TYPE;

typedef enum _ORDER_TYPE {
        ORDER_TYPE_LIMIT = 0,                                           /* limit order */
        ORDER_TYPE_MARKET = 1,                                          /* market order */
        ORDER_TYPE_STOP = 2,
} ORDER_TYPE;

typedef enum _TIME_IN_FORCE {
        ORDER_TIF_DAY = 0,                                              /* Nomal */
        ORDER_TIF_FAK = 1,                                              /* Fill And Kill */
        ORDER_TIF_IOC = 2,                                              /* Immediate Or Cancel */
        ORDER_TIF_FOK = 3,                                              /* Fill Or Kill */
        ORDER_TIF_GTD = 4,                                              /* Good Till Day */
        ORDER_TIF_GTC = 5,                                              /* Good Till Cancel */
} TIME_IN_FORCE;

typedef enum _ORDER_STATUS {
        SIG_STATUS_INIT = -1,                                           /* new order */
        SIG_STATUS_SUCCEED,
        SIG_STATUS_ENTRUSTED,                                           /* Acknowledge by exchange */
        SIG_STATUS_PARTED,                                              /* Partial Fill  */
        SIG_STATUS_CANCELED,
        SIG_STATUS_REJECTED,                                            /* Rejected by exchange or broker */
        SIG_STATUS_CANCEL_REJECTED,
        SIG_STATUS_INTRREJECTED,                                        /* Rejected by trading system */
        UNDEFINED_STATUS
} ORDER_STATUS;

typedef enum _STRATEGY_OPT_TYPE {
        S_PLACE_ORDER_DEFAULT = 100,                                    /* info = order_t */
        S_CANCEL_ORDER_DEFAULT,                                         /* info = order_t */
        S_STRATEGY_DEBUG_LOG,                                           /* char * */
        S_STRATEGY_PASS_RSP,                                            /* info = st_response_t */
        S_PLACE_ORDER_DMA,                                              /* direct market access info = special_order_t */
        S_PLACE_ORDER_VWAP,                                             /* volume weighted average price info = special_order_t  */
        S_PLACE_ORDER_TWAP,                                             /* time weighted average price info = special_order_t */
        S_MANUAL_ORDER_HEDGE_RSP,                                       /* info = void*, used for manual order rsp */
        S_STRATEGY_PARENT_ORDER_INFO,                                   /* t/vwap strategy parent order information */
        S_STRATEGY_CHILD_ORDER_INFO,                                    /* t/vwap strategy child order information */
        S_STRATEGY_ALERT_INFO,                                          /* alert info send by strategy */
} STRATEGY_OPT_TYPE;

typedef enum _CONFIG_TYPE {
        DEFAULT_CONFIG,                                                 /* st_config_t */
} CONFIG_TYPE;

typedef enum _FEED_DATA_TYPE {
        DEFAULT_FUTURE_QUOTE = 0,                                       /* info = future quote data */
        DEFAULT_STOCK_QUOTE,                                            /* info = stock quote data */
        MANUAL_ORDER,                                                   /* info = manual order data */
} FEED_DATA_TYPE;

typedef enum _RESPONSE_TYPE {
        DEFAULT_RSP,                                                    /* info = st_response_t */
} RESPONSE_TYPE;

enum TIMER_TYPE {
        DEFAULT_TIMER,                                                  /* info = NULL */
};

typedef struct {
        char        exch;                                   /* exchange enumeration value. */
        char        symbol[64];                             /* symbol name */
        int         volume;                                 /* order volume */
        double      price;                                  /* order price */
        int         direction;                              /* see @enum direction */
        int         open_close;                             /* see @enum open_close */
        int         investor_type;                          /* see @enum investor_type */
        int         order_type;                             /* see @enum order_type*/
        int         time_in_force;                          /* see @enum time_in_force */
        long long   st_id;
        unsigned long long  order_id;                       /* Order id filled by trading system. */
        unsigned long long  org_ord_id;                     /* Original order_id for order cancelling. */
} order_t;


typedef struct {
        char        exch;                                   /* exchange enumeration value. */
        char        symbol[64];                             /* symbol name */
        int         direction;                              /* see @enum direction */
        int         volume;                                 /* target order volume */
        double      price;                                  /* order price */
        int         start_time;                             /* algorithm start time, TWAP/VWAP use only */
        int         end_time;                               /* algorithm end time, TWAP/VWAP use only */
        int         status;                                 /* return 0 if success */
        int         sync_cancel;                            /* recieve cancel resp and then send order flag, DMA use only, 1-yes, 0-no */
} special_order_t;

/**
 * @param type: @refer to  strategy_opt_type 
 * @param length: length of data.
 * @param data: data content decided by type.
 */
typedef int (*send_data)(int type, int length, void *data);

typedef struct {
        int         long_volume;
        double      long_price;
        int         short_volume;
        double      short_price;
} position_t;

typedef struct {
        char        account[ACCOUNT_LEN];                   /* account name */
        int         currency;                               /* cash currency type */
        double      exch_rate;                              /* currency exchange rate */
        double      cash_asset;                             /* cash for pnl counting. */
        double      cash_available;                         /* cash for order pre-sending checking. */
} account_t;

typedef struct {
        int         fee_by_lot;                             /* True - Fee by Lot, False - Fee by Notional */
        double      exchange_fee;
        double      yes_exchange_fee;
        double      broker_fee;                             /* 中介费 */
        double      stamp_tax;                              /* 印花税 */
        double      acc_transfer_fee;                       /* Account Transfer Fee - only for SSE */
} fee_t;

typedef struct {
        char        symbol[SYMBOL_LEN];                     /* 合约名称 */
        char        exch;                                   /* see @enum exchange */
        int         max_accum_open_vol;                     /* max vol allowed to open in a single day */
        int         max_cancel_limit;                       /* max cancel order limit number */
        int         expiration_date;                        /* 合约到期日期，例如：20161125  */
        double      tick_size;                              /* smallest price movement an asset can make */
        double      multiple;                               /* A constant set by an exchange, which when multiplied by the futures price gives the dollar value of a stock index futures contract */
        char        account[ACCOUNT_LEN];                   /* account name */
        position_t  yesterday_pos;                          /* 昨日持仓 */
        position_t  today_pos;                              /* 今日持仓 */
        fee_t       fee;                                    /* 合约费率 */
        void        *reserved_data;
} contract_t;

typedef struct {
        send_data   proc_order_hdl;                         /* 发/撤单接口 */
        send_data   send_info_hdl;                          /* 内部消息接口，比如 debug 信息传递 */
        send_data   pass_rsp_hdl;  //add 20171019           /* 通用接口，策略开发者无需使用 */

        int         trading_date;                           /* trading date format like YYYYMMDD */
        int         day_night;                              /* see @enum DAY_NIGHT */
        account_t   accounts[ACCOUNT_MAX];                  /* 交易账户信息配置 */
        contract_t  contracts[SYMBOL_MAX];                  /* 交易合约信息配置 */
        char        param_file_path[PATH_LEN];              /* 外部变量记录文件名，含目录 */
        char        output_file_path[PATH_LEN];             /* 策略可写路径 */
        long long   vst_id;                                 /* Virtual Strategy ID, strategy send order should carry this id */
        char        st_name[STRAT_NAME_LEN];                /* Strategy NAME */
        void        *reserved_data;
} st_config_t;

typedef struct {
        unsigned long long      order_id;
        char                    symbol[SYMBOL_LEN];
        unsigned short          direction;                  /* see @enum direction */
        unsigned short          open_close;                 /* see @enum open_close */
        double                  exe_price;
        int                     exe_volume;
        int                     status;                     /* order_status */
        int                     error_no;
        char                    error_info[512];
        void                    *reserved_data;
} st_response_t;

/* Shannon 行情，回报，自定义信息接口, void * info 可以传任意行情 */
typedef struct {
        void                *__trade;                       /* trade private use, feed into operation api */
        void                  *info;                        /* quote data, response, user defined data */
} st_data_t;

#pragma pack(pop)


#if defined(__cplusplus)
extern "C" {
#endif

/**
 * 该初始化参数进程运行生命周期有效
 * 
 * @param type:    config_type
 * @param length:  the data length
 * @param cfg:     determined by type
 * data map {
 *  DEFAULT_CONFIG <-> st_config_t
 * }
 */
EXPORT_SYMBOL int
my_st_init(int type, int length, void *cfg);

/**
 * 单次 my_on_book 调用完成后传入的数据无效
 * 
 * @param type:    feed_data_type
 * @param length:  the data length
 * @param book:    determined by type
 * data map {
 *  DEFAULT_FUTURE_QUOTE <-> st_data_t(info is my_futures_spot_t)
 *  DEFAULT_STOCK_QUOTE  <-> st_data_t(info is my_stock_t)
 * }
 */
EXPORT_SYMBOL int
my_on_book(int type, int length, void *book);

/**
 * 单次 my_on_response 调用完成后传入的数据无效
 * 
 * @param type:     response_type
 * @param length:   the data length
 * @param resp:     determined by type
 * data map {
 *  DEFAULT_RSP <-> st_data_t(info is st_response_t)
 * }
 */
EXPORT_SYMBOL int
my_on_response(int type, int length, void *resp);


/**
 * 单次 my_on_timer 调用完成后传入的数据无效
 * 
 * @param type:     timer_type
 * @param length:   the data length
 * @param resp:     determined by type
 * data map {
 *  DEFAULT_TIMER <-> st_data_t(info is NULL)
 * }
 */
EXPORT_SYMBOL int
my_on_timer(int type, int length, void *info);

/**
 * release all resources allocated in strategy
 */
EXPORT_SYMBOL void
my_destroy();



#if defined(__cplusplus)
}
#endif

#endif // _LF_STRATEGY_API_H_

