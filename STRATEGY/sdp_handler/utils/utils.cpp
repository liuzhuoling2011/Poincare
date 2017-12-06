#include <stdio.h>
#include <ctype.h>
#include "utils.h"
#include "log.h"

#define CHAR_EQUAL_ZERO(a, b, c) do{\
	if (a != b) goto end;\
	if (a == 0) {c = 0; goto end;}\
}while(0)

int my_strcmp(const char * s1, const char * s2)
{
	int ret = -1;
	CHAR_EQUAL_ZERO(s1[0], s2[0], ret);
	CHAR_EQUAL_ZERO(s1[1], s2[1], ret);
	CHAR_EQUAL_ZERO(s1[2], s2[2], ret);
	CHAR_EQUAL_ZERO(s1[3], s2[3], ret);
	CHAR_EQUAL_ZERO(s1[4], s2[4], ret);
	CHAR_EQUAL_ZERO(s1[5], s2[5], ret);
	CHAR_EQUAL_ZERO(s1[6], s2[6], ret);
	CHAR_EQUAL_ZERO(s1[7], s2[7], ret);

	ret = strcmp(s1 + 8, s2 + 8);
end:
	return ret;
}

int strcmp_case(const char * one, const char * two)
{
	char *pOne = (char *)one;
	char *pTwo = (char *)two;
	if (strlen(one) < strlen(two))
		return -1;
	else if (strlen(one) > strlen(two))
		return 1;
	while (*pOne != '\0') {
		if (tolower(*pOne) != tolower(*pTwo))
			return -1;
		pOne++;
		pTwo++;
	}
	return 0;
}

uint64_t my_hash_value(const char *str_key)
{
	char *pt = (char *)str_key;
	unsigned int hash = 1315423911;

	if (pt[0] == 0) goto end;
	hash ^= ((hash << 5) + pt[0] + (hash >> 2));
	if (pt[1] == 0) goto end;
	hash ^= ((hash << 5) + pt[1] + (hash >> 2));
	if (pt[2] == 0) goto end;
	hash ^= ((hash << 5) + pt[2] + (hash >> 2));
	if (pt[3] == 0) goto end;
	hash ^= ((hash << 5) + pt[3] + (hash >> 2));

	if (pt[4] == 0) goto end;
	hash ^= ((hash << 5) + pt[4] + (hash >> 2));
	if (pt[5] == 0) goto end;
	hash ^= ((hash << 5) + pt[5] + (hash >> 2));
	if (pt[6] == 0) goto end;
	hash ^= ((hash << 5) + pt[6] + (hash >> 2));
	if (pt[7] == 0) goto end;
	hash ^= ((hash << 5) + pt[7] + (hash >> 2));

	pt = (char *)(str_key + 8);
	while (*pt != 0) {
		hash ^= ((hash << 5) + *pt + (hash >> 2));
		pt++;
	}

end:
	return hash;
}

int
double_compare(double a, double b, double epsilon)
{
	if (a - b > epsilon)
		return 1;
	else if (a - b < -epsilon)
		return -1;
	else
		return 0;
}

int float_compare(float a, float b, float epsilon)
{
	if (a - b > epsilon)
		return 1;
	else if (a - b < -epsilon)
		return -1;
	else
		return 0;
}

size_t
strlcpy(char * dst, const char * src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';              /* NUL-terminate dst */
		while (*s++);
	}

	return(s - src - 1);    /* count does not include NUL */
}

void
my_strncpy(char *dst, const char *src, size_t siz)
{
	dst[0] = src[0]; if (src[0] == 0) goto end;
	dst[1] = src[1]; if (src[1] == 0) goto end;
	dst[2] = src[2]; if (src[2] == 0) goto end;
	dst[3] = src[3]; if (src[3] == 0) goto end;
	dst[4] = src[4]; if (src[4] == 0) goto end;
	dst[5] = src[5]; if (src[5] == 0) goto end;
	dst[6] = src[6]; if (src[6] == 0) goto end;
	dst[7] = src[7]; if (src[7] == 0) goto end;
	dst[8] = src[8]; if (src[8] == 0) goto end;
	dst[9] = src[9]; if (src[9] == 0) goto end;
	dst[10] = src[10]; if (src[10] == 0) goto end;
	dst[11] = src[11]; if (src[11] == 0) goto end;
	strlcpy(dst + 12, src + 12, siz - 12);
end:
	return;
}

void duplicate_contract(Contract *src, Contract *dest)
{
	dest->exch = src->exch;
	dest->max_pos = src->max_pos;
	dest->max_accum_open_vol = src->max_accum_open_vol;
	dest->single_side_max_pos = src->single_side_max_pos;
	strlcpy(dest->symbol, src->symbol, SYMBOL_LEN);
}

//----------positions related-------
double
avg_px(Position &pos)
{
	if (pos.qty > 0)
		return pos.notional / (double)pos.qty;
	else
		return 0.0;
}

int
position(Contract *cont)
{
	return long_position(cont) - short_position(cont);
}

int 
position(DIRECTION side, Contract * cont)
{
	if (side == ORDER_BUY)
		return long_position(cont);
	else
		return short_position(cont);
}

int
long_position(Contract *cont)
{
	return cont->positions[LONG_OPEN].qty - cont->positions[SHORT_CLOSE].qty;
}

int
short_position(Contract *cont)
{
	return cont->positions[SHORT_OPEN].qty - cont->positions[LONG_CLOSE].qty;
}

double
long_notional(Contract *cont)
{
	return cont->positions[LONG_OPEN].notional + cont->positions[LONG_CLOSE].notional;
}

double
short_notional(Contract *cont)
{
	return cont->positions[SHORT_OPEN].notional + cont->positions[SHORT_CLOSE].notional;
}

double
avg_buy_price(Contract *cont)
{
	return (cont->positions[LONG_OPEN].notional + cont->positions[LONG_CLOSE].notional)
		/ (cont->positions[LONG_OPEN].qty + cont->positions[LONG_CLOSE].qty);
}

double
avg_sell_price(Contract *cont)
{
	return (cont->positions[SHORT_OPEN].notional + cont->positions[SHORT_CLOSE].notional)
		/ (cont->positions[SHORT_OPEN].qty + cont->positions[SHORT_CLOSE].qty);
}

double
get_transaction_fee(Contract *cont, int size, double price, bool flag_close_yes)
{
	double l_fee = 0.0;
	double exchange_fee = 0.0;
	if (flag_close_yes) {
		exchange_fee = cont->yes_exchange_fee;
	}
	else {
		exchange_fee = cont->exchange_fee;
	}
	if (cont->fee_by_lot)
		l_fee = size * (exchange_fee + cont->broker_fee); // Caution, for futures right now, broker fee is 0.0
	else
		l_fee = size * price * (exchange_fee + cont->broker_fee);
	if (cont->exch == SSE)
		l_fee += size * price * cont->acc_transfer_fee;

	return l_fee;
}

double
get_realized_PNL(Contract *cont)
{
	double long_side_PNL = 0.0;
	double short_side_PNL = 0.0;

	int long_pos = MIN(cont->positions[LONG_OPEN].qty, cont->positions[SHORT_CLOSE].qty);
	int short_pos = MIN(cont->positions[SHORT_OPEN].qty, cont->positions[LONG_CLOSE].qty);

	if (long_pos > 0)
		long_side_PNL = long_pos * (avg_px(cont->positions[SHORT_CLOSE]) - avg_px(cont->positions[LONG_OPEN]));

	if (short_pos > 0)
		short_side_PNL = short_pos * (avg_px(cont->positions[SHORT_OPEN]) - avg_px(cont->positions[LONG_CLOSE]));

	return long_side_PNL + short_side_PNL;
}

static uint64_t order_count = 0;

void reset_order_count_for_test() {
	order_count = 0;
}

int process_debug_info(int type, int length, void *data) {
	switch (type) {
	case S_STRATEGY_DEBUG_LOG: {
		PRINT_DEBUG("LOG: %s\n", (char*)data);
		break;
	}
	case S_STRATEGY_PASS_RSP: {
		st_response_t *rsp = (st_response_t *)((st_data_t *)data)->info;
		switch (rsp->status) {
		case SIG_STATUS_PARTED:
			PRINT_INFO("OrderID: %lld Strategy received Partial Filled: %s %s %d@%f\n",
				rsp->order_id,
				rsp->symbol, (rsp->direction == ORDER_BUY ? "Bought" : "Sold"),
				rsp->exe_volume, rsp->exe_price);
			break;
		case SIG_STATUS_SUCCEED:
			PRINT_INFO("OrderID: %lld Strategy received Filled: %s %s %d@%f\n",
				rsp->order_id,
				rsp->symbol, (rsp->direction == ORDER_BUY ? "Bought" : "Sold"),
				rsp->exe_volume, rsp->exe_price);
			break;
		case SIG_STATUS_CANCELED:
			PRINT_WARN("OrderID: %lld Strategy received Canceled\n", rsp->order_id);
			break;
		case SIG_STATUS_CANCEL_REJECTED:
			PRINT_WARN("OrderID: %lld Strategy received CancelRejected\n", rsp->order_id);
			break;
		case SIG_STATUS_REJECTED:
			PRINT_WARN("OrderID: %lld Strategy received Rejected, Error: %s\n",
				rsp->order_id, rsp->error_info);
			break;
		case SIG_STATUS_INTRREJECTED:
			PRINT_WARN("OrderID: %lld Strategy received Rejected, Error: %s\n",
				rsp->order_id, rsp->error_info);
			break;
		case SIG_STATUS_INIT:
			PRINT_DEBUG("OrderID: %lld Strategy received PendingNew\n", rsp->order_id);
			break;
		case SIG_STATUS_ENTRUSTED:
			PRINT_DEBUG("OrderID: %lld Strategy received Entrusted, %s %s\n",
				rsp->order_id, rsp->symbol,
				(rsp->direction == ORDER_BUY ? "Buy" : "Sell"));
			break;
		}
		break;
	}
	}
	return 0;
}

int get_time_diff(int time1, int time2)
{
	int time1_hour = time1 / 10000000;
	int time1_min = (time1 / 100000) % 100;
	int time1_sec = ((time1 / 1000) % 100);
	int time2_hour = time2 / 10000000;
	int time2_min = (time2 / 100000) % 100;
	int time2_sec = ((time2 / 1000) % 100);

	int hour_diff = time2_hour - time1_hour;
	int min_diff = time2_min - time1_min;
	int sec_diff = time2_sec - time1_sec;
	return (hour_diff * 3600 + min_diff * 60 + sec_diff);
}

int add_time(int int_time, int seconds)
{
	int time_hour = int_time / 10000000;
	int time_min = (int_time / 100000) % 100;
	int time_sec = ((int_time / 1000) % 100);

	int current_sec = time_hour * 3600 + time_min * 60 + time_sec;
	int sum_sec = current_sec + seconds;

	int time1_hour = sum_sec / 3600;
	int time1_min = (sum_sec % 3600) / 60;
	int time1_sec = ((sum_sec % 3600) % 60);
	return (time1_hour * 10000 + time1_min * 100 + time1_sec) * 1000;
}

int get_seconds_from_int_time(int int_time)
{
	/*Convert int_time e.g 90000000 to seconds */
	int second = (int_time / 1000) % 100;
	int_time = (int_time / 1000) / 100;
	int minute = int_time % 100;
	int_time = int_time  / 100;
	int hour = int_time % 100;
	return hour * 60 * 60 + minute * 60 + second;
}