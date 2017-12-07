#ifndef ORDER_TYPES_DEF_H
#define ORDER_TYPES_DEF_H

#include <stdint.h>
#include "macro_def.h"

#pragma pack(push)
#pragma pack(8)

#define MAX_NAME_SIZE  64
#define MAX_PATH_SIZE  256

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



enum MatchEngineMode{
	NORMAL_MATCH = 1,
	IDEAL_MATCH = 2,
	LINEAR_IMPACT_MATCH = 3,
	DOUBLE_GAUSSIAN_MATCH = 4,
	SPLIT_TWO_PRICE_MATCH = 5,
	BAR_CLOSE_MATCH = 6
};

enum OrderStatus{
	ME_ORDER_INIT = -1,			 //new order
	ME_ORDER_SUCCESS, 	 	     /* ����ȫ���ɽ� */
	ME_ORDER_ENTRUSTED, 	 	 /* ����ί�гɹ� */
	ME_ORDER_PARTED, 	 	 	 /* �������ֳɽ� */
	ME_ORDER_CANCEL, 	 	 	 /* ���������� */
	ME_ORDER_REJECTED, 	 	     /* �������ܾ� */
	ME_ORDER_CANCEL_REJECTED,    /* �������ܾ� */
	ME_ORDER_INTRREJECTED,       /* �ڲ��ܵ� */
	ME_ORDER_UNDEFINED
};


// place & cancel order struct
struct order_request
{
	uint64_t         order_id;
	uint64_t         org_order_id;  //only for cancel order
	int              strat_id;

	char 	            symbol[MAX_NAME_SIZE];
	ORDER_DIRECTION 	direction;
	OPEN_CLOSE_FLAG 	open_close;
	double 	            limit_price;
	long 	            order_size;

	Exchanges	    exchange;
	TIME_IN_FORCE   time_in_force; //order_kind
	ORDER_TYPES	    order_type;
	INVESTOR_TYPES  investor_type; //speculator
};

struct settle_position {
	char symbol[SHANNON_SYMBOL_LEN];
	char account[SHANNON_ACCOUNT_LEN];
	char exchange[SHANNON_SYMBOL_LEN];
	char daynight[8];
	double yest_long_avg_price;
	double yest_short_avg_price;
	double today_long_avg_price;
	double today_short_avg_price;
	int yest_long_pos;
	int yest_short_pos;
	int today_long_pos;
	int today_short_pos;
	double today_settle_price;
	double symbol_net_pnl;
};

struct settle_account {
	char   account[SHANNON_ACCOUNT_LEN];
	double available_cash;
	double cash;
	double account_accumulated_pnl;
};

// settle resp
struct settle_resp {
	settle_position settle_pos[SHANNON_SYMBOL_MAX];
	settle_account settle_acc[SHANNON_ACCOUNT_MAX];
	double accumulated_pnl;
	double available_cash;
	double cash;
};

// order & trade return struct
struct signal_response {
	char           exchg_time[13];         /* �ر�ʱ�� */
	char           local_time[13];         /* ����ʱ�� */

	uint64_t       serial_no;         /* ����ί�к� */
	char           symbol[MAX_NAME_SIZE];  /* ��Լ�� */
	unsigned short direction;         /* DCE ��-'0'/��-'1' */  /* SHFE ��-0/��-1 */
	unsigned short open_close;        /* DCE ��ƽ��ʶ��'0'��ʾ����'1'��ʾƽ */  /* SHFE ��-0/ƽ-1 */

	double         exe_price;        /* ��ǰ�ɽ��۸� */
	int            exe_volume;       /* ��ǰ�ɽ��� */
	OrderStatus    status;            /* �ɽ�״̬ */
	int            error_no;          /* ����� */
	char           error_info[512];   /*������Ϣ*/

	int            strat_id;          /*new add for sim */
	int            entrust_no;
};


/* information of a product */
struct product_info   //TODO: Replace this with FeeField
{
	char    name[MAX_NAME_SIZE]; /* IF, dla, dli, ag, cu, sr */
	char    prefix[MAX_NAME_SIZE];	/* IF, a, i, ag, cu, SR */

	int     fee_fixed;		/* 0, 1 */
	double  rate;		/* 0.00002625 for IF */
	double  unit;		/* min-change, 0.2 for IF */
	int     multiple;		/* 300 for IF, 15 for ag */
	char    xchg_code;		/* STAT_NEW, 'B', STAT_FULL, 'G' */

	double  acc_transfer_fee; //add for stock
	double  stamp_tax;        //add for stock
};

// Duplicate struct with task_define.h, match_param_t
struct mode_config{
	int      mode;
	char     exch;
	char     product[MAX_NAME_SIZE];

	int      param_num;
	double   trade_ratio;
	double   params[MAX_NAME_SIZE];
};

class QueryContract;

typedef struct {
	int  curr_days;   //��ǰִ������
	int  param_index; //���������ֵ
	int  task_type;   //��������
	int  task_date;   //��ǰ����
	int  all_date;    //�ܵ�����
	int  day_night;   //��ҹ�̱�־
	int  cycle_num;   //�����ѭ������
	int  strat_num;   //���Եĸ���
	bool use_settle;  //�Ƿ�ʹ������

	QueryContract *query; //����ӷѣ���ѯ����
	void          *strat_info; //���ݲ��Եĳ�ʼ����Ϣ��
	void          *currency_rate; //����
} task_conf_t;

struct match_engine_config {
	char           contract[MAX_NAME_SIZE];
	task_conf_t    task;
	product_info   pi;
	mode_config    mode_cfg;
};

#pragma pack(pop)

#endif
