#ifndef C_QUOTE_MY_BAR_H
#define C_QUOTE_MY_BAR_H

// 按8字节对齐的转换后数据结构
#pragma pack(push)
#pragma pack(8)

//K线数据
struct my_bar
{
	char symbol[32];            //原始Code
    int nTradingDay;            //交易日 TODO: Remove this.
	int nCalendarDay;           //日历日  TODO: Remove this.
	int int_time;					//时间(HHMMSSmmm)
    
	unsigned int open;			//开盘价 * 10000
	unsigned int high;			//最高价 * 10000
	unsigned int low;			//最低价 * 10000
	unsigned int close;	    //收盘价 * 10000
	
	long long volume;			//成交总量
	long long turnover;		//成交总额准确值
	unsigned int open_interest;	//持仓量
};

#pragma pack(pop)

#endif
