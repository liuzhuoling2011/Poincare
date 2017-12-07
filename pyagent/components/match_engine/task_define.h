#ifndef __TASK_DEFINE_H__
#define __TASK_DEFINE_H__

#include "platform/strategy_base_define.h"
#include "macro_def.h"

#define    MIN_NAME_SIZE         8
#define    MIN_BUFF_SIZE         32
#define    MAX_NAME_SIZE         64
#define    MAX_DIR_PATH_SIZE     256 
#define    MAX_EV_PATH_SIZE      2048 
#define    MAX_COUNT_NUM         64
#define    MAX_CONTRACT_SIZE     4096
#define    MAX_OPEN_VAL          10000000

#define    STRAT_LOG_DIR         "./logs"
#define    TUNNEL_DIR            "./tunnel_log"
#define    TUNNEL_LOG_DIR        TUNNEL_DIR"/%s"
#define    TUNNEL_LOG_FILE       TUNNEL_LOG_DIR"/%d_%s_%s.log"
#define    TUNNEL_LOG_FORMAT_STR "%s - [%s] server: 0.0.0.0; account: 000000; model_id: %d; serial_no: %lld; "\
        "cancel_serial_no: %lld; symbol: %s; direction: %d; open_close: %d; order_price: %f; order_vol: %d; "\
        "entrust_no: %d; entrust_status: %d; trade_price: %f; trade_vol: %d; vol_remain: %d; trade_no: %d; "\
        "speculator: %d; order_kind: %d; order_type: %d; error_no: %d; trigger: %s\n"

/**
* @describe: ����ص�����������ÿ��Ľ������
* @param[out]  buff  ���صĽ������
* @return      0 - success; other - fail
*/
typedef int(*day_response_func)(char *buff);
typedef int(*st_log)(char *log_buf, uint64_t log_len);

enum CLIENT_MODE{
	LOCAL = 0,
	AGENT = 1
};

enum BUSINESS_TYPE {
	BUSINESS_TYPE_SIMULATION = 0,
	BUSINESS_TYPE_POSTTRADE = 1,
	BUSINESS_TYPE_UNION_SIM = 2,
	BUSINESS_TYPE_OPTIMIZATION = 3,
	BUSINESS_TYPE_GEN_EV_FILE = 4,
	BUSINESS_TYPE_TEST_STRAT = 99
};

struct basic_order_info {
	ORDER_DIR_FLAG  direct_flag;
	ORDER_DIRECTION direction;
	OPEN_CLOSE_FLAG open_close;
	double exe_price;
	int exe_volume;
	bool yesterday_flag;
};

struct turing_feed_st_info;
struct shannon_feed_st_info;

struct trader_info_t {
	turing_feed_st_info     *turing_info;      /* Turing */
	shannon_feed_st_info 	*shannon_info;
	st_log					log_handler;
	int                     sig_count;
	basic_order_info        sig_out[MAX_SIGNAL_SIZE];
};


struct db_config_t
{
	char        ip[MIN_BUFF_SIZE];
	int         port;
	char        usr_name[MIN_BUFF_SIZE];
	char        password[MIN_BUFF_SIZE];
};

struct param_item_t
{
	 double     start;
	 double     end;
	 double     step;
	 int        increment_times;
};

struct match_param_t
{
	int      mode;
	char     exch;
	char     product[MAX_NAME_SIZE];

	int      param_num;
	double   trade_ratio;
	double   params[MAX_NAME_SIZE];
};

struct contract_info_t
{
	int         group_id;
	char        product[MAX_NAME_SIZE];
	char        exchange[MIN_NAME_SIZE];
	char        rank[MIN_NAME_SIZE];
	char        account[MAX_NAME_SIZE];

	int         max_pos;
	int         source;
	int         feed_type_num;
	int         feed_type[MAX_COUNT_NUM];
};

struct position_info_t {
	char symbol[MAX_NAME_SIZE];
	int  pre_long_pos;
	double pre_long_price;
	int  pre_short_pos;
	double pre_short_price;
};

struct account_info_t {
	char   account[MAX_NAME_SIZE];
	int    currency;
	double cash;
};

struct strategy_item_t{
	char     so_name[MAX_DIR_PATH_SIZE];
	char     strat_name[MAX_COUNT_NUM][SHANNON_STRAT_NAME_LEN];
	int      alpha_mixer_num;
	int64_t  strategy_id[MAX_COUNT_NUM];
	int64_t  v_strategy_id[MAX_COUNT_NUM];

	account_info_t accounts[MAX_COUNT_NUM];
	int      account_num;

	int      contract_num;   //subscribe contract
	contract_info_t  contracts[MAX_CONTRACT_SIZE];
	position_info_t  yesterday_pos[MAX_CONTRACT_SIZE];
	position_info_t  today_pos[MAX_CONTRACT_SIZE];

	char     log_name[MAX_DIR_PATH_SIZE];
	char     strat_param_file[MAX_EV_PATH_SIZE];
	char     file_path[MAX_EV_PATH_SIZE];

	int      max_accum_open_vol;
	int      single_side_max_pos;
};

struct task_item_t
{
	/*==============================
	     General control parameters
	================================*/
	int      client_mode;
	int      business;
	char     task_id[MAX_NAME_SIZE];
	char     user[MAX_NAME_SIZE];
	char     base_path[MAX_DIR_PATH_SIZE];
	int      trader_type;
	int      trader_env;

	/*==============================
	    strategies info
	================================*/
	int      strat_num;
	int      strat_group_num;
	strategy_item_t  strat_item[MAX_COUNT_NUM];

	int      contract_num;
	contract_info_t contracts[MAX_CONTRACT_SIZE];

	/*==============================
	    Quote access module 
	================================*/
	db_config_t  kdb_conf;        // kdb+ server
	int          start_date;      //date & time
	int          end_date;
	int          begin_time;
	int          end_time;

	int          day_night_flag;
	int          quote_delay_type;
	int          quote_delay_value;
	int          bar_interval;

	/*==============================
	     Calculate relevant
	================================*/
	int            match_type;   //match engine
	int            match_param_num;
	match_param_t  match_param[MAX_COUNT_NUM];

	int          time_interval;
	int          param_num;    //multi-parameter
	param_item_t param_item[MAX_COUNT_NUM];

	int          fee_rate_mode;
	bool         use_fee_for_pnl;
	bool         use_lock_pos;
	bool         cross_day_pos;

	/*==============================
	    Log control
	================================*/
	bool         write_strat_log;
	bool         write_tunnel_log;
	int          ev_generation_days;
	char         ev_output_file[MAX_DIR_PATH_SIZE];

	/*==============================
		Trader info
	================================*/
	trader_info_t trader_info;
	bool          stock_flag;
	int           sim_start_date;
	int           sim_end_date;
	int           total_sim_dates;
	int           param_count;
	int           quote_count;

	int           so_type;
	day_response_func  call_back_func;
};


#endif
