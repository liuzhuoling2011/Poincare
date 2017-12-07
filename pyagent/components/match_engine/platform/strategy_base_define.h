/*
* strategy_base_defines.h
*
* API data structures provided to user
*
* Copyright(C) by MY Capital Inc. 2007-2999
*/
#pragma once
#ifndef __STRATEGY_BASE_DEFINE_H__
#define __STRATEGY_BASE_DEFINE_H__

#include <stdint.h>
#include "list.h"
#include "mi_type.h"

#define SYMBOL_MAX       4096
#define PATH_LEN         256
#define EV_PATH_LEN      2048
#define SYMBOL_LEN       64
#define ACCOUNT_MAX      64
#define ACCOUNT_LEN      64
#define MAX_SIGNAL_SIZE 1000
#define MAX_CAN_TRADE_SIZE   64
#define MAX_PATH_SIZE        256
#define MAX_ORDER_COUNT 256

/*Contract related*/
#define MAX_GROUP_SIZE     16
#define MAX_FEED_TYPE_SIZE  8
//for Hash 
#define HASH_TABLE_SIZE	  (8192)
#define MAX_ITEM_SIZE	  (4096)

/************************************************************************
 Enums
*************************************************************************/
enum STRAT_SO_TYPE {
	SO_TYPE_CPP = 98,
	SO_TYPE_PYTHON = 99
};

enum PRINT_LOG_LEVEL {
	PRINT_LOG_LEVEL_DEBUG = 0,
	PRINT_LOG_LEVEL_INFO = 1,
	PRINT_LOG_LEVEL_WARN = 2,
	PRINT_LOG_LEVEL_ERROR = 3,
	PRINT_LOG_LEVEL_NOLOG = 4
};

/* Currency definition */
enum ENUM_CURRENCY {
	CNY = 0,   // Chinese Yuan
	USD = 1,   // US Dollar
	HKD = 2,   // HongKong Dollar
	EUR = 3,   // Euro
	JPY = 4,   // Japanese Yen
	GBP = 5,   // Puond Sterling
	AUD = 6,   // Australian Dollar
	CAD = 7,   // Canadian Dollar
	SEK = 8,   // Swedish Krona
	NZD = 9,   // New Zealand Dollar
	MXN = 10,  // Mexican Peso
	SGD = 11,  // Singapore Dollar
	NOK = 12,  // Norwegian Krone
	KRW = 13,  // South Korean Won
	TRY = 14,  // Turkish Lira
	RUB = 15,  // Russian Ruble
	INR = 16,  // Indian Rupee
	BRL = 17,  // Brazilian Real
	ZAR = 18,  // South African Rand
	CNH = 19,  // Offshore Chinese Yuan
};

/* TradeHandler Enums and Structs */
enum Exchanges
{
	// SHENZHEN STOCK EXCHANGE
	SZSE = '0',
	// SHANGHAI STOCK EXCHANGE
	SSE = '1',
	HKEX = '2', // FIXME make this consistent with trader
	// SHANGHAI FUTURES EXCHANGE
	SHFE = 'A',//XSGE = 'A',
	// CHINA FINANCIAL FUTURES EXCHANGE
	CFFEX = 'G',//CCFX = 'G',
	// DALIAN COMMODITY EXCHANGE
	DCE = 'B', //XDCE = 'B',
	// ZHENGZHOU COMMODITY EXCHANGE
	CZCE = 'C', //XZCE = 'C',
	FUT_EXCH = 'X',
	SGX = 'S', // FIXME make this consistent with trader
	//shanghai gold exchange
	SGE = 'D', //**make this consistent with trader, shanghai gold exchange
	//eSunny Foreign	//**make this consistent with trader, eSunny Foreign
	CBOT = 'F',
	CME = 'M',
	LME = 'L',
	COMEX = 'O',
	NYMEX = 'N',  //TODO: Change to NYMX
	BLANK_EXCH = '\0',
	UNDEFINED_EXCH = 'u'
};

//Order related
enum ORDER_DIR_FLAG {
	SIG_ORDER_CANCEL = 0,
	SIG_ORDER_SEND = 1
};
enum ORDER_DIRECTION{
	SIG_DIRECTION_BUY = 0,
	SIG_DIRECTION_SELL = 1,
	SIG_DIRECTION_CANCEL = 3,
	UNDEFINED_SIDE = 4
};
enum OPEN_CLOSE_FLAG{
	SIG_ACTION_OPEN = 0,
	SIG_ACTION_CLOSE = 1,
	SIG_ACTION_ROLLED = 2,     // Close today
	SIG_ACTION_CLOSE_YES = 3,  // For SHFE, we have to tell tunnel close today or yes.
	UNDEFINED_OPEN_CLOSE_FLAG = 4
};
enum ORDER_TYPES
{
	/* limit order */
	ORDER_TYPE_LIMIT = 0,
	/* market order */
	ORDER_TYPE_MARKET = 1,
	ORDER_TYPE_STOP = 2,
};
enum TIME_IN_FORCE
{
	/* Nomal */
	TIF_DAY = 0,
	/* Fill And Kill */
	TIF_FAK = 1,
	/* Immediate Or Cancel */
	TIF_IOC = 1,
	/* Fill Or Kill */
	TIF_FOK = 2,
	/* Good Till Day */
	TIF_GTD = 3,
	/* Good Till Cancel */
	TIF_GTC = 4,
};
enum INVESTOR_TYPES
{
	INVESTOR_TYPE_SPECULATOR = 0,
	INVESTOR_TYPE_HEDGER = 1,
	INVESTOR_TYPE_ARBITRAGEURS = 2,
};

//st_order_info.status
enum ORDER_STATUS{
	SIG_STATUS_INIT = -1,			 //new order
	SIG_STATUS_SUCCEED, 	 	     
	SIG_STATUS_ENTRUSTED, 	 	     /* Acknowledge by exchange */
	SIG_STATUS_PARTED, 	 	 	     /* Partial Fill  */
	SIG_STATUS_CANCELED,
	SIG_STATUS_REJECTED, 	 	     /* Rejected by exchange or broker */
	SIG_STATUS_CANCEL_REJECTED,
	SIG_STATUS_INTRREJECTED,         /* Rejected by trading system */
	UNDEFINED_STATUS
};

enum SecurityType
{
	FUTURE = 0,
	STOCK = 1,
	OPTION = 2,
	FX = 3,
	ETF = 4,
	SPOT = 5
};

enum PRODUCT_RANK{
	UNDEFINED_RANK = 0,
	main_product = 1,
	second_product = 2,
	ALL_RANK = 100
};

enum POSITION_FLAG{
	LONG_OPEN = 0,
	LONG_CLOSE = 1,
	SHORT_OPEN = 2,
	SHORT_CLOSE = 3
};

enum REQUEST_TYPES
{
	REQUEST_TYPE_PLACE_ORDER = 0,
	REQUEST_TYPE_CANCEL_ORDER = 1,
	REQUEST_TYPE_REQ_QUOTE = 2,
	REQUEST_TYPE_QUOTE = 3
};

enum RANK_TYPE{
	UNDEFINED_RANKTYPE = -1,
	RANK_BY_VOLUME = 0,
	RANK_BY_TIME = 1
};

/* Stock specific definitions */
enum STOCK_INDEX_ENUM {
	STOCK_INDEX_HS300,
	STOCK_INDEX_ZZ500,
	STOCK_INDEX_SH50,
	STOCK_INDEX_A50,
	STOCK_INDEX_COUNT
};

/* Stock pool definitions */
enum STOCK_POOL_ENUM {
	STOCK_POOL_A50,
	STOCK_POOL_SH50,
	STOCK_POOL_HS300,
	STOCK_POOL_ZZ500,
	STOCK_POOL_ZZ800,
	STOCK_POOL_ZZ1000,
	STOCK_POOL_ASSETS_PROFIT_RATE_INCREASE3,
	STOCK_POOL_CASHFLOW_NETINCOME,
	STOCK_POOL_DUPONT_ROA,
	STOCK_POOL_INTDEBT_TO_TOTAL_CAP,
	STOCK_POOL_LONG_DEBT_ODEBT,
	STOCK_POOL_NET_PROFIT_RATE_INCREASE3,
	STOCK_POOL_OCF_TO_DEBT,
	STOCK_POOL_OCF_TO_OR,
	STOCK_POOL_OPER_REVINCREASE3,
	STOCK_POOL_ROE_AVG,
	STOCK_POOL_SHRHLDR_PROFIT_RATE_INCREASE3,
	STOCK_POOL_YOYBPS,
	STOCK_POOL_GROSS_PROFIT_RATE_INCREASE3,
	STOCK_POOL_COUNT
};

enum  FEED_STATUS{
	DEBUT = '0',             /*��������*/
	REISSUE = '1',           /*�����¹�*/
	ONLINE_SET_PRICE = '2',  /*2 �������۷���*/
	ONLINE_BID_PRICE = '3',  /*3 �������۷���*/
	CLOSE = 'A',             /*A ���׽�����*/
	HALT_FOR_TODAY = 'B',    /*B ����ͣ��*/
	END_OF_DAY = 'C',        /*C ȫ������*/
	PAUSE_TRADE = 'D',       /*D ��ͣ����*/
	START = 'E',             /*�������*/
	PRE_ENTER = 'F',         /*��ǰ����*/
	HOLIDAY = 'H',           /*�ż�*/
	OPEN_AUCTION = 'I',      /*���м��Ͼ���*/
	IN_MARKET_AUCTION = 'J', /*���м��Ͼ���*/
	OPEN_PRE_ORDER_BOOK = 'K', /*���ж�����ƽ��ǰ��*/
	IN_MARKET_PRE_ORDER_BOOK = 'L', /*���ж�����ƽ��ǰ��*/
	OPEN_ORDER_BOOK = 'M',          /*���ж�����ƽ��*/
	IN_MARKET_ORDER_BOOK = 'N',     /*���ж�����ƽ��*/
	TRADE = 'O',             /*�������*/
	BREAK = 'P',             /*����*/
	VOL_BREAK = 'Q',         /*�������ж�*/
	BETWEEN_TRADE = 'R',     /*���׼�*/
	NO_TRADE_SUPPORT = 'S',  /*�ǽ��׷���֧��*/
	FIX_PRICE_AUCTION = 'T', /*�̶��۸񼯺Ͼ���*/
	POST_TRADE = 'U',        /*�̺���*/
	END_TRADE = 'V',         /*��������*/
	PAUSE = 'W',             /*��ͣ*/
	SUSPEND = 'X',           /*ͣ��*/
	ADD = 'Y',               /*������Ʒ*/
	DEL = 'Z',               /*��ɾ���Ĳ�Ʒ*/

	//eSunny foreign
	FRN_UNKNOWN = -1,
	FRN_MARKET_OPEN = 0,
	FRN_NO_DIV = 1,
	FRN_RACE_PRICE = 2,
	FRN_ON_HOLD = 3,
	FRN_MARKET_CLOSE = 4,
	FRN_PRE_MARKET_OPEN = 5,
	FRN_PRE_MARKET_CLOSE = 6,
	FRN_FAST_MARKET = 7
};

/*
��ö�ٶ��彻��ͨ����صĴ�����Ϣ
*/
enum channel_errors
{
	// ִ�й���ʧ��
	RESULT_FAIL = -1,
	// 	ִ�й��ܳɹ�
	RESULT_SUCCESS = 0,
	// �޴˹��ܺ�
	UNSUPPORTED_FUNCTION_NO = 1,
	// ����ͨ��δ�����ڻ���
	NO_VALID_CONNECT_AVAILABLE = 2,
	// ���������ָ��
	ERROR_REQUEST = 3,
	// ��ָ�ڻ��ۼƿ��ֳ����������
	CFFEX_EXCEED_LIMIT = 4,
	// ��֧�ֵĹ���
	UNSUPPORTED_FUNCTION = 100,
	// �޴�Ȩ��
	NO_PRIVILEGE = 101,
	// û�б�������Ȩ��
	NO_TRADING_RIGHT = 102,
	// �ý���ϯλδ���ӵ�������
	NO_VALID_TRADER_AVAILABLE = 103,
	// ��ϯλĿǰû�д��ڿ���״̬
	MARKET_NOT_OPEN = 104,
	// ������δ�������󳬹������
	CFFEX_OVER_REQUEST = 105,
	// ������ÿ�뷢�����������������
	CFFEX_OVER_REQUEST_PER_SECOND = 106,
	// ������δȷ��
	SETTLEMENT_INFO_NOT_CONFIRMED = 107,
	// �Ҳ�����Լ
	INSTRUMENT_NOT_FOUND = 200,
	// ��Լ���ܽ���
	INSTRUMENT_NOT_TRADING = 201,
	// �����ֶ�����
	BAD_FIELD = 202,
	// ����ı��������ֶ�
	BAD_ORDER_ACTION_FIELD = 203,
	// �������ظ�����
	DUPLICATE_ORDER_REF = 204,
	// �������ظ�����
	DUPLICATE_ORDER_ACTION_REF = 205,
	// �����Ҳ�����Ӧ����
	ORDER_NOT_FOUND = 206,
	// ��ǰ����״̬��������
	UNSUITABLE_ORDER_STATUS = 207,
	// ֻ��ƽ��
	CLOSE_ONLY = 208,
	// ƽ���������ֲ���
	OVER_CLOSE_POSITION = 209,
	// �ʽ���
	INSUFFICIENT_MONEY = 210,
	// �ֻ����ײ������
	SHORT_SELL = 211,
	// ƽ���λ����
	OVER_CLOSETODAY_POSITION = 212,
	// ƽ���λ����
	OVER_CLOSEYESTERDAY_POSITION = 213,
	// ί�м۸񳬳��ǵ�������
	PRICE_OVER_LIMIT = 214,
};

enum EXCH_ORDER_STATUS
{
	// δ��(n:�ȴ�����; s:�����걨)
	pending_new = '0',
	// �Ѿ�����
	new_state = 'a',
	// ���ֳɽ�
	partial_filled = 'p',
	// ȫ���ɽ�
	filled = 'c',
	// �ȴ�����
	pending_cancel = 'f',
	// ���ھܾ�(e:����ί��)
	//rejected = 'q',
	rejected = 'e',
	// �Ѿ�����(b:���ɲ���)
	cancelled = 'd',
	cancel_rejected = 'z', /* Add by Wen, need to make sure it's consist with live trading*/
	UNDEFINED_STATE = 'u'
};

/**
* Turing/Shannon status code for individual strategy
*/
enum ST_STATUS_ENUM {
	ST_STATUS_NORMAL = 0,
	ST_STATUS_INIT_FAILED,
	ST_STATUS_ORDER_REJECTED,
	ST_STATUS_ORDER_TOO_FREQUENT,
	ST_STATUS_INVALID_QUOTE,
	ST_STATUS_EXIT_MODE,
	ST_STATUS_COUNT
};

/* TradeHandler Enums and Structs */
/* local simulation include win && librss */
enum TRADER_ENV {
	TRADER_ENV_LIVE_TRADING,
	TRADER_ENV_LOCAL_SIMULATION
};

/* Trader type - Normal, Turing, etc. */
enum TRADER_TYPE {
	TRADER_TYPE_LIBRSS,
	TRADER_TYPE_SOFTWARE_TURING,
	TRADER_TYPE_SHANNON
};
/************************************************************************
 Structs
*************************************************************************/

#pragma pack (8)
struct hash_bucket{
	struct list_head head;
};
#pragma pack ()

struct symbol_pos_t {
	unsigned char exchg_code;
	int           long_volume;
	double        long_price;
	int           short_volume;
	double        short_price;
};

struct contract_info {
	char               symbol[SYMBOL_LEN];    /* ��Լ���� */
	int                max_pos;                    /* ���ֲ�(��Ӫ���ã���ʾ����ÿ���µ�������������뿪��������޹�) */
	int                exch;
	int                single_side_max_pos;  /* ��󵥱߲�λ���ޣ����Զ�ο�ƽ�֣�һ�������λ�������� max_pos���� */
	int                max_accum_open_vol;   /* ���˫���ۼƿ������ޣ��������涨��һ���ﵽ�������޷��ٿ��� */
	int                expiration_date;      /* ��Լ�������ڣ����磺20161125  */
	symbol_pos_t       yesterday_pos;      /* ���ճֲ� */
	symbol_pos_t       today_pos;          /* ����ֲ� */
	/* account info */
	char               account[ACCOUNT_LEN];
	/* For future use */
	int                future_int_param1;
	int                future_int_param2;
	int                future_int_param3;
	int                future_int_param4;
	int                future_int_param5;
	uint64_t           future_uint64_param1;
	uint64_t           future_uint64_param2;

	double             future_double_param1;
	double             future_double_param2;
	double             future_double_param3;
	double             future_double_param4;
	double             future_double_param5;
	char               future_char64_param1[64];
	char               future_char64_param2[64];
	char               future_char64_param3[64];
	char               future_char64_param4[64];
	char               future_char64_param5[64];
};

struct account_info {
	char   account[ACCOUNT_LEN];
	int    currency;
	double cash;
};

struct St_Config
{
	contract_info  contracts[SYMBOL_MAX];
	int     contracts_num;
	int64_t strat_id;           /* Strategy ID */
	int64_t v_strat_id;
	char    strat_name[PATH_LEN]; /* Used in Turing strategy setup */
	int     sim_date;
	int     day_night_flag;
	char    strat_param_file[EV_PATH_LEN];     /* Strat Param file path, including folder path */
	char    file_path[EV_PATH_LEN];           /* File path for user to open/read/write files */
	char    log_name[PATH_LEN];               /* model log file path,not including file name */
	account_info  accounts[ACCOUNT_MAX];
	int     accounts_num;
};

/* New internal_book */
//stock
struct Stock_Internal_Book
{
	char wind_code[32];         //600001.SH
	char ticker[32];             //ԭʼCode
	int action_day;             //ҵ������(��Ȼ��)
	int trading_day;            //������
	int exch_time;				//ʱ��(HHMMSSmmm)
	int status;				    //��Ʊ״̬
	float pre_close_px;				//ǰ���̼� 
	float open_px;					//���̼� 
	float high_px;					//��߼� 
	float low_px;					//��ͼ� 
	float last_px;				//���¼� 
	float ap_array[10];			//����� 
	uint32_t av_array[10];			//������
	float bp_array[10];			//����� 
	uint32_t bv_array[10];			//������
	uint32_t num_of_trades;			//�ɽ�����
	int64_t total_vol;				//�ɽ�����
	int64_t total_notional;		    //�ɽ��ܶ�׼ȷֵ,Turnover
	int64_t total_bid_vol;			//ί����������
	int64_t total_ask_vol;			//ί���������
	float weighted_avg_bp;	//��Ȩƽ��ί��۸� 
	float weighted_avg_ap;  //��Ȩƽ��ί��۸�
	float IOPV;					//IOPV��ֵ��ֵ  ������
	float yield_to_maturity;		//����������	��ծȯ��
	float upper_limit_px;			//��ͣ�� 
	float lower_limit_px;			//��ͣ�� 
	char prefix[4];			//֤ȯ��Ϣǰ׺
	int PE1;					//��ӯ��1	δʹ�ã���ǰֵΪ0��
	int PE2;					//��ӯ��2	δʹ�ã���ǰֵΪ0��
	int change;					//����2���Ա���һ�ʣ�	δʹ�ã���ǰֵΪ0��
};

//futures spot
struct Futures_Internal_Book
{
	int  feed_type; /* type of the data source */
	char symbol[64];
	int16_t exchange;
	int  int_time;  /* 090059000, HourMintueSecondMilliSeconds*/
	float pre_close_px;
	float pre_settle_px;
	double pre_open_interest;
	double open_interest;
	float open_px;
	float high_px;
	float low_px;
	float avg_px;
	float last_px;
	float bp_array[5];
	float ap_array[5];
	int  bv_array[5];
	int  av_array[5];
	uint64_t total_vol;
	double total_notional;  /* Trade Notional, Turnover */
	float upper_limit_px;
	float lower_limit_px;
	float close_px;	/* Today's close price */
	float settle_px;
	/* DCE */
	int implied_bid_size[5];  /* Implied Bid/Ask Size */
	int implied_ask_size[5];
	/* Statistics Info, DCE have these from another feed */
	int total_buy_ordsize;		/* Total Open Buy Order Size */
	int total_sell_ordsize;     /* Total Open Sell Order Size */
	float weighted_buy_px;   /* Weighted Buy Order Price */
	float weighted_sell_px;  /* Weighted Sell Order Price */
};

struct Internal_Bar {
	char symbol[SYMBOL_LEN];
	int  int_time;
	double open;
	double close;
	double high;
	double low;
	uint64_t volume;   // Total shares/vol
	double turnover;   // Total Traded Notional
	double upper_limit;
	double lower_limit;
	double open_interest;
	int bar_index;
};

/* Signal responce received from exchange */
struct Order_Update
{
	char           exchg_timestamp[13];
	char           local_timestamp[13];
	uint64_t       order_id;
	char           symbol[SYMBOL_LEN];
	unsigned short direction;
	unsigned short openclose;
	double         exe_price;
	int            exe_volume;
	int            status; // order_status
	int            error_no;
	char           error_info[512];
};

#pragma pack (8)

struct Position{
	int              qty;
	double           notional;   /* A cost of current position, including fees in points */
};

struct Contract{
	int           contract_id;	  /* Contract Id */
	Exchanges	  exch;
	PRODUCT_RANK  rank;
	RANK_TYPE     rank_type;
	SecurityType  security_type;
	char          product[SYMBOL_LEN];
	char          symbol[SYMBOL_LEN];
	int           expiration_date;        /* 20161020 */
	double        tick_size;
	double        multiplier;

	Position      pre_long_pos;
	Position      pre_short_pos;
	int           init_today_long_qty;   /* Remaining Today's long position, including yesterday's pos */
	int           init_today_short_qty;  /* Remaining Today's short position, including yesterday's pos */
	int           pre_long_qty_remain;   /* Remaining pre long position */
	int           pre_short_qty_remain;  /* Remaining pre short position */
	Position      positions[4];          // POSITION_FLAG, 1-4 LONG_OPEN = 1,LONG_CLOSE = 2,SHORT_OPEN = 3,SHORT_CLOSE = 4

	int           type_num;
	int           feed_types[MAX_FEED_TYPE_SIZE];
	int           group_num;
	int           group_ids[MAX_GROUP_SIZE];

	/* Risk Related */
	int           max_pos;
	int           source;
	int           max_accum_open_vol;     /* �������涨�����˫���ۼƿ������ޣ��������� rb 7700, Ŀǰͼ��ᴫ�룬��SDP �����. Read from rss config in simulation */
	int           single_side_max_pos;    /* ��󵥱߲�λ���ޣ��������ֹ��� */
	int           single_side_max_pos_limit; /* ����λ�����������ޣ��Ǿ��Եĺ��ߣ��������ֹ��� */
	int           order_max_vol;          /* max vol per order */
	int           order_lmt_per_second;
	int           order_cancel_lmt_per_day;
	int           order_cum_open_vol;     /* Used in compliance check in sim_mytrader. Should be lower than max_accum_open_vol*/

	struct        list_head hs_link;
	struct        list_head link;

	/* Market Data Info */
	int           bv1;
	int           av1;
	double        bp1;
	double        ap1;
	double        last_px;
	double        pre_close_px;
	double        upper_limit;
	double        lower_limit;

	bool          fee_by_lot;	          // True - Fee by Lot, False - Fee by Notional
	double        exchange_fee;
	double        yes_exchange_fee;
	double        broker_fee;
	double        stamp_tax;
	double        acc_transfer_fee;      // Account Transfer Fee - only for SSE
	int           pre_data_time;
	uint64_t  pre_total_vol;

	/* Order Info*/
	int m_pending_buy_open_size;
	int m_pending_sell_open_size;
	int m_pending_buy_close_size;
	int m_pending_sell_close_size;

	/*Handle no entrusted response*/
	int m_order_no_entrusted_num;
	int account_id;    // Related Account, used to calculate cash.

	/* Stock Info */
	long long     total_out_shrs[STOCK_INDEX_COUNT];         /* Total Shares issued */
	long long     free_float_out_shrs[STOCK_INDEX_COUNT];    /* ��ͨ�ɱ�, different index company might have different number */
	double        inclusion_factor[STOCK_INDEX_COUNT];       /* 1-100, a percentage, how much is included in index */
	double        stock_pool_factor[STOCK_POOL_COUNT];       /* 1-100, a percentage, how much is included in index */
	double        cap_factor[STOCK_INDEX_COUNT];             /* CapFactor */
	double        adj_open_px[STOCK_INDEX_COUNT];            /* Open px after dividend, shares issue/split adjustment */
	double        stock_pre_close_px[STOCK_INDEX_COUNT];     /* stock pre close price */
	long long     total_mkt_cap[STOCK_INDEX_COUNT];          /* TotalMarketCapitalization */
	long long     free_float_mkt_cap[STOCK_INDEX_COUNT];     /* free_float_out_shrs * px */
	double        index_divisor[STOCK_INDEX_COUNT];          /* NewDivisor */
	int           industry_CICS[4];       /* CICS industry categorization */
	int           industry_CSRC[4];       /* CSRC industry categorization */
	bool          is_index_constituent[STOCK_INDEX_COUNT]; /* Is index constituent */
	bool          is_stock_pool[STOCK_POOL_COUNT];/* Is stock pool*/
	bool          is_traded;              /* Is the stock traded (not suspended) */
	bool          is_ex_date;             /* Is ex-rights / ex-dividend date */

};

struct Order{
	uint64_t signal_id;
	Exchanges exchange;
	Contract* contr;
	double price;
	int volume;
	ORDER_DIRECTION side;
	OPEN_CLOSE_FLAG sig_openclose;
	char symbol[SYMBOL_LEN];

	REQUEST_TYPES request_type;
	ORDER_STATUS  status;
	EXCH_ORDER_STATUS state;

	OPEN_CLOSE_FLAG position_effect;
	TIME_IN_FORCE ord_type;
	double last_px;

	int last_qty;
	int cum_qty;
	double cum_amount;

	channel_errors error_no;
	ORDER_TYPES price_type;
	long orig_cl_ord_id;
	long orig_ord_id;
	INVESTOR_TYPES invest_type;

	bool pending_cancel;
	bool close_yes_pos_flag;
	struct list_head hs_link; // hash link
	struct list_head pd_link; // pending link
	struct list_head fr_link; // free unused link

	struct list_head search_link; // used to place in the search list

	long exch_ord_id;
	long strat_id;
};

#pragma pack ()

#endif
