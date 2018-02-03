#include "bar.h"
#include "utils/MyHash.h"

MyHash<Bar_Struct> bar_cont_map;
static int bar_interval = 0;
strategy_on_bar st_on_bar;

int int_time_to_min(int int_time) {
	int hour2min = int_time / 10000000 * 60;
	int minutes = int_time % 10000000 / 100000;
	return hour2min + minutes;
}

void activate_bar(int bar_interval_, strategy_on_bar st_on_bar_)
{
	bar_interval = bar_interval_;
	st_on_bar = st_on_bar_;
}

void process_bar_data(Futures_Internal_Book * a_book)
{
	//conversion from int_time to minutes
	if (a_book->feed_type == 3) return;	//3 is DCE MI_DCE_ORDER_STATISTIC

	if (!bar_cont_map.exist(a_book->symbol)) {
		Bar_Struct &bar_item = bar_cont_map.get_next_free_node();
		//first tick new bar
		my_strncpy(bar_item.cur_bar.symbol, a_book->symbol, sizeof(SYMBOL_LEN));
		bar_item.cur_bar.int_time = a_book->int_time / 100000 * 100000;
		bar_item.cur_bar.open = a_book->last_px;
		bar_item.cur_bar.high = a_book->last_px;
		bar_item.cur_bar.low = a_book->last_px;
		bar_item.cur_bar.upper_limit = a_book->upper_limit_px;
		bar_item.cur_bar.lower_limit = a_book->lower_limit_px;

		//store bar open information
		bar_item.bar_index = 0;
		bar_item.last_bar_time = int_time_to_min(a_book->int_time);
		bar_item.open_vol = a_book->total_vol;
		bar_item.open_notional = a_book->total_notional;

		bar_cont_map.insert_current_node(a_book->symbol);
	}
	else {
		Bar_Struct &bar_item = bar_cont_map.at(a_book->symbol);
		Internal_Bar *l_bar = &bar_item.cur_bar;

		if (l_bar->int_time == 0) {
			//new bar
			my_strncpy(l_bar->symbol, a_book->symbol, sizeof(SYMBOL_LEN));
			l_bar->open = a_book->last_px;
			l_bar->high = a_book->last_px;
			l_bar->low = a_book->last_px;
			l_bar->upper_limit = a_book->upper_limit_px;
			l_bar->lower_limit = a_book->lower_limit_px;

			//store bar open information
			bar_item.open_vol = a_book->total_vol;
			bar_item.open_notional = a_book->total_notional;
		}
		else {
			//update high and low prices
			if (a_book->last_px > l_bar->high) l_bar->high = a_book->last_px;
			if (a_book->last_px < l_bar->low) l_bar->low = a_book->last_px;
		}
		//update int_time each tick
		l_bar->int_time = a_book->int_time / 100000 * 100000;

		int cur_time = int_time_to_min(a_book->int_time);
		if (cur_time - bar_item.last_bar_time >= bar_interval) {
			l_bar->close = a_book->last_px;
			l_bar->open_interest = a_book->open_interest;
			l_bar->turnover = a_book->total_notional - bar_item.open_notional;
			l_bar->volume = a_book->total_vol - bar_item.open_vol;
			l_bar->bar_index = bar_item.bar_index;
			st_on_bar(l_bar);
			memset(l_bar, 0, sizeof(Internal_Bar));
			bar_item.last_bar_time = cur_time;
			bar_item.bar_index++;
		}
	}
}

void destory_bar()
{
	bar_cont_map.clear();
	bar_interval = 0;
}
