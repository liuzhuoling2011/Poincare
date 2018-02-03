#ifndef _BAR_H_
#define _BAR_H_

#include "quote_format_define.h"

struct Bar_Struct {
	int bar_index;
	Internal_Bar cur_bar;
	int last_bar_time;
	uint64_t open_vol;
	double open_notional;
};

typedef int (*strategy_on_bar)(Internal_Bar* bar_quote);

void activate_bar(int bar_interval, strategy_on_bar st_on_bar);

void process_bar_data(Futures_Internal_Book * a_book);

void destory_bar();


#endif