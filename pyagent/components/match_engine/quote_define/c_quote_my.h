#ifndef C_QUOTE_MY_H
#define C_QUOTE_MY_H

struct my_marketdata 
{
	long time;
	char contract[64];
	//char direction;
	int curr_volume;		/*当前量*/
	int b_total_volume;		/*买成交量*/
	int s_total_volume;		/*卖成交量*/
	double last_price;      /*最新价*/
	double b_open_interest; /*买持仓量*/
	double s_open_interest; /*卖持仓量*/

	double buy_price[30];
	int buy_vol[30];
	double sell_price[30];
	int sell_vol[30];
};

#endif