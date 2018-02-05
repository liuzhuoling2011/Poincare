#ifndef	_UTILS_H_
#define	_UTILS_H_

#include <string.h>
#include "core/base_define.h"

#define MAX(a,b)  ((a)>(b)?(a):(b))
#define MIN(a,b)  ((a)>(b)?(b):(a))
#define DOUBLE_PRECISION 0.0000000001
#define FLOAT_PRECISION 0.00001

#define delete_mem(cls_obj) do{\
	if(cls_obj != nullptr) { delete cls_obj; cls_obj = nullptr; }\
}while(0)

#ifndef _WIN32
	#define strtok_s(str, delim, saveptr)   strtok_r(str, delim, saveptr)
#endif

/* Compare two doubles, if less return -1, if more return 1, or return 0 when equal */
int double_compare(double a, double b, double epsilon = DOUBLE_PRECISION);

/* Compare two doubles, if less return -1, if more return 1, or return 0 when equal */
int float_compare(float a, float b, float epsilon = FLOAT_PRECISION);

/*
* Copy src to string dst of size siz with high efficiency.
* At most siz-1 characters will be copied.
* Always NUL terminates.
*/
void my_strncpy(char *dst, const char *src, size_t siz);

size_t strlcpy(char * dst, const char * src, size_t siz);

/*
* Compare two strings with high efficiency.
* Returns 0 for equal, else for not equal.
*/
int my_strcmp(const char *s1, const char *s2);

// Replace strcmp without case sensitivity. But if the two strings'
// length equal, we only return -1 when the strings are not equal,
// that's a bit different from strcmp
int strcmp_case(const char *one, const char *two);

uint64_t my_hash_value(const char *str_key);

void duplicate_contract(Contract *src, Contract *dest);

double avg_px(Position &pos);

int position(Contract *cont);

int position(DIRECTION side, Contract *cont);

int long_position(Contract *cont);

int short_position(Contract *cont);

double long_notional(Contract *cont);

double short_notional(Contract *cont);

double avg_buy_price(Contract *cont);

double avg_sell_price(Contract *cont);

double get_transaction_fee(Contract *cont, int size, double price, bool flag_close_yes = false);

int process_debug_info(int type, int length, void *data);

int get_time_diff(int time1, int time2);

int add_time(int int_time, int seconds);

void reset_order_count_for_test();

int get_seconds_from_int_time(int int_time);

int get_seconds_from_char_time(char* char_time);

void get_time_record(char *time_str);

double Round(double price);

bool read_json_config(ST_BASE_CONFIG& base_config, char* ev_path);

#endif