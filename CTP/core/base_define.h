#ifndef _BASE_DEFINE_H_
#define _BASE_DEFINE_H_

#include <stdint.h>
#include "utils/list.h"

#define TURNOVER_DIRECTION(dir) ((dir)^1)

#define SYMBOL_MAX       (4096)                                         /* 模型支持的合约最大个数 */
#define PATH_LEN         (2048)                                         /* 模型so/ev文件存放绝对路径最大长度 */
#define STRAT_NAME_LEN   (256)                                          /* 策略名最大长度 */
#define SYMBOL_LEN       (64)                                           /* 合约名最大长度 */
#define ACCOUNT_MAX      (64)                                           /* 模型支持的最大账户个数 */
#define ACCOUNT_LEN      (64)                                           /* 账户名最大长度 */

#pragma pack(push)
#pragma pack(8)


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
	ORDER_CLOSE_YES = 2,                                            /* For SHFE, we have to tell tunnel close today or yes. */
	UNDEFINED_OPEN_CLOSE = 3,
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
	ORDER_TIF_IOC = 1,                                              /* Immediate Or Cancel */
	ORDER_TIF_FOK = 2,                                              /* Fill Or Kill */
	ORDER_TIF_GTD = 3,                                              /* Good Till Day */
	ORDER_TIF_GTC = 4,                                              /* Good Till Cancel */
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
	S_PLACE_DMA_ORDER,
	S_PLACE_VWAP_ORDER,
	S_PLACE_TWAP_ORDER,
	S_CANCEL_ORDER_DEFAULT,                                         /* info = order_t */
	S_STRATEGY_DEBUG_LOG,                                           /* char * */
	S_STRATEGY_PASS_RSP,                                            /* info = st_response_t */
	S_TUNNEL_TRADE_LOG,
	S_STRATEGY_PARENT_ORDER_INFO,
	S_STRATEGY_CHILD_ORDER_INFO,
	S_PROGRAM_ALERT_INFO,
	S_PROGRAM_CMD_RSP,
	S_PROGRAM_AGENT_CMD,
} STRATEGY_OPT_TYPE;

typedef enum _CONFIG_TYPE {
	DEFAULT_CONFIG,                                                 /* st_config_t */
} CONFIG_TYPE;

typedef enum _FEED_DATA_TYPE {
	DEFAULT_FUTURE_QUOTE = 0,                                       /* info = future quote data */
	DEFAULT_STOCK_QUOTE,                                            /* info = stock quote data */
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
	int         order_type;                             /* @enum order_type*/
	int         time_in_force;                          /* @enum time_in_force */
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
	bool        sync_cancel;                            /* recieve cancel resp and then send order flag, DMA use only */
} special_order_t;

/**
* @param type: @refer to  strategy_opt_type
* @param length: length of data.
* @param data: data content decided by type.
*/
typedef int(*send_data)(int type, int length, void *data);

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
	int         max_pos;                                /* 最大持仓(运营配置，表示策略每次下单的最大手数，与开仓量检查无关) */
	char        exch;                                   /* see @enum exchange */
	int         max_accum_open_vol;                     /* max vol allowed to open in a single day */
	int         max_cancel_limit;                       /* max cancel order limit number */
	int         single_side_max_pos;                    /* 最大单边仓位上限，可以多次开平仓，一般情况仓位不允许超过 max_pos就行 */
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
	long long   st_id;                                  /* Strategy ID */
	char        st_name[STRAT_NAME_LEN];                /* Used in Turing strategy setup */
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


enum CHECK_ERROR
{
	ERROR_OPEN_POS_IS_NOT_ENOUGH = -1,
	ERROR_SELF_MATCHING = -2,
	ERROR_REARCH_MAX_ACCUMULATE_OPEN_VAL = -3,
	ERROR_CANCEL_ORDER_FAIL = -4,
	ERROR_CASH_IS_NOT_ENOUGH = -5,
	ERROR_PRE_LONG_POS_IS_NOT_ENOUGH = -6,
};

struct hash_bucket {
	list_t head;
};

struct Position {
	int     qty;
	double  notional;   /* A cost of current position, including fees in points */
};

enum POSITION_FLAG {
	LONG_OPEN = 0,
	LONG_CLOSE = 1,
	SHORT_OPEN = 2,
	SHORT_CLOSE = 3
};

struct Contract {
	char      account[ACCOUNT_LEN];    // Related Account, used to calculate cash.
	char      symbol[SYMBOL_LEN];
	int       expiration_date;        /* 20161020 */
	EXCHANGE  exch;

	int       init_today_long_qty;   /* Remaining Today's long position, including yesterday's pos */
	int       init_today_short_qty;	 /* Remaining Today's short position, including yesterday's pos */
	int       pre_long_qty_remain;   /* Remaining pre long position */
	int       pre_short_qty_remain;  /* Remaining pre short position */
	Position  positions[4];          // POSITION_FLAG, 1-4 LONG_OPEN = 1,LONG_CLOSE = 2,SHORT_OPEN = 3,SHORT_CLOSE = 4

	/* Risk Related */
	int       max_pos;
	int       max_accum_open_vol;     /* 交易所规定，最大双边累计开仓上限，例如上期 rb 7700, 目前图灵会传入，但SDP 不检查. Read from rss config in simulation */
	int       single_side_max_pos;    /* 最大单边仓位上限，用于锁仓功能 */
	int       order_cum_open_vol;

	bool      fee_by_lot;	          // True - Fee by Lot, False - Fee by Notional
	double    exchange_fee;
	double    yes_exchange_fee;
	double    broker_fee;
	double    stamp_tax;
	double    acc_transfer_fee;      // Account Transfer Fee - only for SSE
	double    tick_size;
	double    multiplier;

	int       bv1;
	int       av1;
	double    bp1;
	double    ap1;
	int       pending_buy_open_size;
	int       pending_sell_open_size;
	int       pending_buy_close_size;
	int       pending_sell_close_size;
	int       order_no_entrusted_num;
	int       pre_data_time;
	int       m_order_no_entrusted_num;
};

struct Order {
	uint64_t signal_id;
	EXCHANGE exch;
	Contract *contr;
	double price;
	int volume;
	DIRECTION side;
	OPEN_CLOSE openclose;
	INVESTOR_TYPE invest_type;
	ORDER_TYPE ord_type;
	TIME_IN_FORCE time_in_force;
	char symbol[SYMBOL_LEN];

	ORDER_STATUS status;

	double last_px;
	int last_qty;
	int cum_qty;
	double cum_amount;

	long orig_cl_ord_id;
	long orig_ord_id;

	bool pending_cancel;

	list_t hs_link; // hash link
	list_t pd_link; // pending link
	list_t fr_link; // free unused link
	list_t search_link; // used to place in the search list
};

struct Stock_Internal_Book
{
	char wind_code[32];         //600001.SH
	char ticker[32];             //原始Code
	int action_day;             //业务发生日(自然日)
	int trading_day;            //交易日
	int exch_time;				//时间(HHMMSSmmm)
	int status;				    //股票状态
	float pre_close_px;				//前收盘价 
	float open_px;					//开盘价 
	float high_px;					//最高价 
	float low_px;					//最低价 
	float last_px;				//最新价 
	float ap_array[10];			//申卖价 
	uint32_t av_array[10];			//申卖量
	float bp_array[10];			//申买价 
	uint32_t bv_array[10];			//申买量
	uint32_t num_of_trades;			//成交笔数
	int64_t total_vol;				//成交总量
	int64_t total_notional;		    //成交总额准确值,Turnover
	int64_t total_bid_vol;			//委托买入总量
	int64_t total_ask_vol;			//委托卖出总量
	float weighted_avg_bp;	//加权平均委买价格 
	float weighted_avg_ap;  //加权平均委卖价格
	float IOPV;					//IOPV净值估值  （基金）
	float yield_to_maturity;		//到期收益率	（债券）
	float upper_limit_px;			//涨停价 
	float lower_limit_px;			//跌停价 
	char prefix[4];			//证券信息前缀
	int PE1;					//市盈率1	未使用（当前值为0）
	int PE2;					//市盈率2	未使用（当前值为0）
	int change;					//升跌2（对比上一笔）	未使用（当前值为0）
};

#endif
