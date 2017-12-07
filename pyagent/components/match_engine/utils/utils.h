/*
* utils.h
*
* Provide utils functions
*
* Copyright(C) by MY Capital Inc. 2007-2999
*/
#pragma once
#ifndef __UTILS_H__
#define __UTILS_H__

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <ctime>
#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Placeholders like [DATE] goes here */
#define CFG_KEY_PLACEHOLDER_DATE         "[DATE]"
#define CFG_KEY_PLACEHOLDER_PRODUCT      "[PRODUCT]"
#define CFG_KEY_PLACEHOLDER_DAYNIGHT     "[DAYNIGHT]"


#define MAX_SYMBOL_SIZE        64

/************************************************************************/
/* LIVE TRADING headers
/* This part should be consistent with Makefile
/************************************************************************/
#if defined(_HI5) || defined(_HI5A) || defined(_HI50) || defined(_HI50A) || defined(_HI51) || defined(_HI51A) \
	|| defined(_HI52) || defined(_HI52A) || defined(_HI53) || defined(_HI55) || defined(_HI55A) || defined(_HI70) \
	|| defined(_HI71) || defined(_HI72) || defined(_HI73) || defined(_HI74) || defined(_HI75) || defined(_HI76) \
	|| defined(_HI80) || defined(_HI81) || defined(_HI82) || defined(_HI83) || defined(_HI84) || defined(_HI85) \
	|| defined(_HI86) || defined(_HI91) || defined(_HI92) || defined(_HI93)
#define __HI5789
#endif
#if defined(_IP11) || defined(_IP91) || defined(_IP92) || defined(_IP93) || defined(_IP94) || defined(_IP95) || defined(_IP96) || defined(_IP97)
#define __IP
#endif

#ifdef _SDP
#define __HI5789
//#define _TS1 
//#define _IQ
//#define __IP
//#define _H0
//#define _ARB_STRATEGY
#define _SAMPLE
#endif // _SDP

static const std::map<std::string, int> currency_int { { "CNY", 0 },{ "USD", 1 },{ "HKD", 2 },{ "EUR", 3 },
	{ "JPY", 4 },{ "GBP", 5 },{ "AUD", 6 },{ "CAD", 7 },{ "SEK", 8 },{ "NZD", 9 },{ "MXN", 10 },{ "SGD", 11 },
	{ "NOK", 12 }, { "KRW", 13 }, { "TRY", 14 }, { "RUB", 15 }, { "INR", 16 }, { "BRL", 17 }, { "ZAR", 18 }, { "CNH", 19 } };

//------------------Data type related---------------------

struct Parameter
{
	double min;
	double max;
	double increment;
	int    increment_times;
};


using SplitVec = std::vector<std::string>;

//------------------System related---------------------
#define TOKEN_BUFFER_SIZE 4096
#define PRECISION         0.0001
#define MAX(a,b)  ((a)>(b)?(a):(b))
#define MIN(a,b)  ((a)>(b)?(b):(a))

void  create_dir(const char *path);
void  my_sleep(int ms);
void** create_2d_array(int row, int col, int size);
#define free_mem(data_ptr) do{\
	if(data_ptr != nullptr) { free(data_ptr); data_ptr = nullptr; }\
}while(0)
#define delete_mem(cls_obj) do{\
	if(cls_obj != nullptr) { delete cls_obj; cls_obj = nullptr; }\
}while(0)

#ifdef _WIN32
	#ifndef snprintf
		#define snprintf sprintf_s
	#endif

	#define BACKTRACE()
#else
inline void
__localtime_s_(struct tm *result, const time_t *time){
	*result = *localtime(time);
}

inline void
__fopen_s_(FILE **pFile, const char *filename, const char *mode){
	*pFile = fopen(filename, mode);
}

#define strcpy_s(dest, size, src)       strlcpy(dest, src, size)
#define localtime_s(result, time)       __localtime_s_(result, time)
#define fopen_s(pFile, filename, mode)  __fopen_s_(pFile, filename, mode)
#define strtok_s(str, delim, saveptr)   strtok_r(str, delim, saveptr)

/* High Precision Time Compare */
#define HP_TIMING_NOW(Var) \
 ({ uint64_t _hi, _lo; \
  asm volatile ("rdtsc" : "=a" (_lo), "=d" (_hi)); \
  (Var) = _hi << 32 | _lo; })

/*  Example: Compare clock time
unsigned long start, end;
HP_TIMING_NOW(start);
Do something...
HP_TIMING_NOW(end);

printf("%ld\n", end - start);
*/

timespec diff(timespec start, timespec end);

/*  Example: Compare clock time
timespec ts0,ts1;
clock_gettime(CLOCK_MONOTONIC_RAW, &ts0);
Do something...
clock_gettime(CLOCK_MONOTONIC_RAW, &ts1);
cout<<"============TimeDiff: "<<diff(ts0,ts1).tv_sec<<":"<<diff(ts0,ts1).tv_nsec<<endl;
*/

#include <execinfo.h>
#include <unistd.h>
#define BACKTRACE() do{\
		int nptrs;\
		const int size = 100;\
		void *buffer[size];\
		char **strings;\
		nptrs = backtrace(buffer, size);\
		printf("backtrace() returned %d addresses \r\n", nptrs);\
		backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO);\
	}while (0)

#endif   /* WIN32 */

//------------------String related---------------------
/*
* Copy src to string dst of size siz. 
* At most siz-1 characters will be copied. 
* Always NUL terminates (unless siz == 0).
* Returns strlen(src); 
* If retval >= siz, truncation occurred.
*/
size_t strlcpy(char *dst, const char *src, size_t siz);

/*
* Copy src to string dst of size siz with high efficiency.
* At most siz-1 characters will be copied.
* Always NUL terminates.
*/
void my_strncpy(char *dst, const char *src, size_t siz);

/*
* Copy src to string dst of size siz with lower case.
* Always NUL terminates.
*/
void my_strcpy_lower_case(char *dst, const char *src);

/*
* Appends src to string dst of size siz (unlike strncat, siz is the full size of dst, not space left).
* At most siz-1 characters will be copied.
* Always NUL terminates (unless siz <= strlen(dst)).
* Returns strlen(src) + MIN(siz, strlen(initial dst)).
* If retval >= siz, truncation occurred.
*/
size_t strlcat(char *dst, const char *src, size_t siz);

/*
* Compare two strings with high efficiency.
* Returns 0 for equal, else for not equal.
*/
int my_strcmp(const char *s1, const char *s2);

bool case_compare(const char *one, const char *two);
// strcmp is similar to case_compare, but return value is different
int strcmp_case(const char *one, const char *two);


//------------------Conversion related---------------------
void convert_to_rss_symbol(const char* a_symbol, char* a_out_rss_symbol);
int  convert_stratname_to_id(const char* a_name);
int  convert_currency_number(std::string cur_name);

void get_product_by_symbol(const char *symbol, const char *product);
void get_foreign_product_by_symbol(const char *symbol, const char *product);
void get_symbol_by_windcode(const char *windcode, const char *symbol);

char get_exch_by_name(const char *name);
std::string get_exch_name_by_enum(const char name);
int get_strat_id_by_subid(int a_subid);
void string_trim(const char* a_str, const char * a_out_str);

uint64_t my_hash_value(const char *str_key);

void split_string(const char *line, const char *delims, SplitVec &elems);
bool split_string(std::string tmpStr, const char *delims, std::string & tmpName, double & tmpNum);
void split_string(const std::string &line, const char *delims, SplitVec &elems);

bool is_endswith(char *large, const char *sub);

std::string& my_trim(std::string &ss);
int str2int(char *str);
double str2double(char * str);

int calc_increment_count(Parameter *a_parm, double epsilon = PRECISION);

template<class TP>
std::string num2str(TP num)
{
	std::stringstream ss;
	ss << num;
	return ss.str();
}

template<class TP>
TP str2num(std::string str, TP &ret)
{
	TP num;
	std::stringstream ss(str);
	ss >> num;
	ret = num;
	return num;
}

/* Compare two doubles, if less return -1, if more return 1, or return 0 when equal */
int compare(double a, double b, double epsilon = PRECISION);

//------------------ Time & date related---------------------
int get_now_time();
int get_now_date();

int get_time_diff(int time1, int time2);
int add_time(int int_time, int seconds);

//"2016-03-14" --> int(20160314)
int str_date_to_int(const char *str_date);

int convert_itime_2_sec(int itime);

int get_int_time(char *str_time);

void get_char_time(int int_time, char *des_time);

int get_timestamp(char *src, int millisec);

void merge_timestamp(char *update, int millisec, char *result);

int get_timestamp(char *timestamp, bool is_all);

int get_timestamp_all_number(char *src, int millisec);

int64_t convert_exchange_time(int64_t exch_time);

int get_date(const char *str_date);

int get_days_between_int_dates(int start_date, int end_date);

//------------------ Algorithm  related---------------------
template<typename TP> double
sum(TP data[], int len)
{
	if (len <= 0) return 0.0;

	double std_sum = 0.0;
	for (int i = 0; i < len; i++){
		std_sum += data[i];
	}
	return std_sum;
}

template<typename TP> double
mean(TP data[], int len)
{
	if (len <= 0) return 0.0;

	double std_sum = 0.0;
	for (int i = 0; i < len; i++){
		std_sum += data[i];
	}
	return std_sum / len;
}

template<typename TP> double
stdev(TP data[], int len)
{
	if (len <= 0) return 0.0;

	double pow_sum = 0.0;
	double l_mean = mean(data, len);
	for (int i = 0; i < len; i++){
		double diff = data[i] - l_mean;
		pow_sum += std::pow(diff, 2);
	}
	return std::sqrt(pow_sum / len);
}

template<typename TP> double
stdev_deltas(TP data[], int len)
{
	if (len <= 0) return 0.0;

	for (int i = len-1; i > 0; i--){
		data[i] -= data[i-1];
	}

	return stdev(data, len);
}


/* Convert cumulative values to deltas (replacing in-place) */
/* Example: {0, 5, 8, 15, 9} -> {0, 5, 3, 7, -6} */
template<typename T> inline std::vector<T>&
vector_to_deltas(std::vector<T>& vec)
{
	if (vec.size() > 1) {
		/* iterate backward, and subtract the previous one */
		std::transform(std::next(vec.rbegin()), vec.rend(), vec.rbegin(), vec.rbegin(), [](T a, T b){ return b - a; });
	}
	return vec;
}

template<typename T> inline double
vector_mean(std::vector<T> const& vec)
{
	if (vec.size() > 0) {
		double res = std::accumulate(vec.begin(), vec.end(), (T)0);
		return res / vec.size();
	}
	else {
		return 0.0;
	}
}

template<typename T> inline double
vector_sum(std::vector<T> const& vec)
{
	if (vec.size() > 0) {
		double res = std::accumulate(vec.begin(), vec.end(), (T)0);
		return res;
	}
	else {
		return 0.0;
	}
}

template<typename T> inline double
vector_stdev(std::vector<T> const& v)
{
	if (v.size() > 1) {
		double mean = vector_mean(v);
		std::vector<double> diff(v.size());
		std::transform(v.begin(), v.end(), diff.begin(), [mean](double x) { return x - mean; });
		double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
		return std::sqrt(sq_sum / (v.size() - 1));
	}
	else if (v.size() == 1){
		return 0.0;
	}
	else {
		return std::numeric_limits<double>::quiet_NaN();
	}
}

template<typename T> inline bool
sort_cmp(const T &a, const T &b)
{
	return a > b;
}

template<typename T> void
sort_array(T arr[], int size, bool ascending = true)
{
	if (ascending)
		std::sort(arr, arr + size);
	else
		std::sort(arr, arr + size, sort_cmp<T>);
}

template<typename T> int
search_array(T arr[], int size, T target)
{
	for (int i = 0; i < size; i++){
		if (arr[i] == target)
			return i;
	}
	return -1;
}

int search_double_array(double arr[], int size, double target);

template<typename T>  void
search_all_array(T arr[], int size, T target, std::vector<int>& result)
{
	result.clear();
	for (int i = 0; i < size; i++){
		if (arr[i] == target)
			result.push_back(i);
	}
}

//---------------------- strategy status --------------------------
#define STRAT_STATUS_BIT_SIZE   4      /* each strategy has this number of bits in m_strat_status */
#define STRAT_STATUS_BIT_BLOCK  15ULL  /* 0000...00001111 (15) if STRAT_STATUS_BIT_SIZE == 4 */

/* Get strategy's status by index */
uint64_t inline
get_strat_status(uint64_t a_all_status, unsigned int a_idx)
{
	return (a_all_status >> (a_idx * STRAT_STATUS_BIT_SIZE)) & STRAT_STATUS_BIT_BLOCK;
}

/* Set strategy's status by index */
uint64_t inline
set_strat_status(uint64_t *a_all_status, unsigned int a_idx, int a_status)
{
	unsigned int l_offset = a_idx * STRAT_STATUS_BIT_SIZE;
	(*a_all_status) &= ~(STRAT_STATUS_BIT_BLOCK << l_offset); /* clear status */
	return (*a_all_status) |= (((uint64_t)a_status) & STRAT_STATUS_BIT_BLOCK) << l_offset; /* set status */
}

// 100000 is 1 seconds  static const int g_scale = 100000; //1seconds
void get_hours_min(int time_val, int &hours, int &min);
int add_interval(int &hours, int &min);
int convert_unixtime_with_nanoseconds(int64_t market_time);
int64_t normalize_inttime_to_milliseconds(int64_t a_int_time);

double calc_curr_avgp(double a_notional, int a_volume, double a_pre_avg_px, double a_multiple);
#endif