#ifndef ORDER_TYPES_DEF_H
#define ORDER_TYPES_DEF_H

#include <stdint.h>
#include "platform/strategy_base_define.h"
#include "macro_def.h"

#pragma pack(push)
#pragma pack(8)

#define MAX_NAME_SIZE  64
#define MAX_PATH_SIZE  256

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
