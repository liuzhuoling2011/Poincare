#include "strategy_module.h"

#define  NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "numpy/arrayobject.h"
#include "strategy_interface.h"
#include "quote_format_define.h"

#define MAX_ST_LEVEL       (5)
#define MAX_ST_SIZE        (20)
#define MAX_ORDER_PER_TICK (1024)
#define MAX_INFO_PER_TICK  (1024)
#define MAX_RESP_PER_TICK  (2048)
#define MAX_INFO_MSG_SIZE  (1024)
#define SERIAL_NO_MULTI    (10000000000)
#define MAX_ORDER_NO       (1000000000000000)

typedef enum {
    COREDUMP_MSG = 0,
    SCREEN_MSG = 1,
} callback_msg_t;

typedef int (*st_data_func_t)(int type, int length, void *data);
typedef int (*st_none_func_t)();

typedef struct {
	void             *hdl;
    st_data_func_t    st_init;
    st_data_func_t    on_book;
    st_data_func_t    on_response;
    st_data_func_t    on_timer;
    st_none_func_t    st_exit;
} st_api_t;

typedef struct {
	/* store all loaded strategy api */
	st_api_t api[MAX_ST_SIZE];
	int cnt;
} st_grp_t;

typedef struct {
	unsigned long    order_no;
	order_t          ord[MAX_ORDER_PER_TICK];
	int              ord_cnt;
} st_signal_t;

typedef struct {
    char data[MAX_INFO_MSG_SIZE];
    int length;
    int type;
} st_info_t;

typedef struct {
    st_info_t        msg[MAX_INFO_PER_TICK];
    int              msg_cnt;
} st_notify_t;

typedef struct{
	int                     serial;
	int                     mi_type; /*MI_TYPE*/
	int64_t                 local_time;
	int64_t                 exchange_time;
	Stock_Internal_Book     quote;
} numpy_stock_t;

typedef struct {
    int                     serial;
	int                     mi_type;
    int64_t                 local_time;
    int64_t                 exchange_time;
    Futures_Internal_Book   quote;
} numpy_future_t;

typedef struct {
    int                     serial;
    int                     mi_type; 
    int64_t                 local_time;
    int64_t                 exchange_time;
    manual_order_info_t     quote;
} numpy_manual_order_t;
