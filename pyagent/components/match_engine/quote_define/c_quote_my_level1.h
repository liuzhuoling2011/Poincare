#ifndef C_QUOTE_MY_LEVEL1_H
#define C_QUOTE_MY_LEVEL1_H

#pragma pack(push)
#pragma pack(8)

///深度市场行情（一档行情结构）
struct my_level1_quote
{
    char TradingDay[9];     ///交易日
    char SettlementGroupID[9];   ///结算组代码
    int SettlementID;   ///结算编号
    double LastPrice;   ///最新价
    double PreSettlementPrice;   ///昨结算
    double PreClosePrice;   ///昨收盘
    double PreOpenInterest;   ///昨持仓量
    double OpenPrice;   ///今开盘
    double HighestPrice;   ///最高价
    double LowestPrice;   ///最低价
    int Volume;   ///数量
    double Turnover;   ///成交金额
    double OpenInterest;   ///持仓量
    double ClosePrice;   ///今收盘
    double SettlementPrice;   ///今结算
    double UpperLimitPrice;   ///涨停板价
    double LowerLimitPrice;   ///跌停板价
    double PreDelta;   ///昨虚实度
    double CurrDelta;      ///今虚实度
    char UpdateTime[9];    ///最后修改时间
    int UpdateMillisec;    ///最后修改毫秒
    char InstrumentID[31];   ///合约代码
    double BidPrice1;   ///申买价一
    int BidVolume1;   ///申买量一
    double AskPrice1;   ///申卖价一
    int AskVolume1;   ///申卖量一
    double BidPrice2;   ///申买价二
    int BidVolume2;   ///申买量二
    double AskPrice2;   ///申卖价二
    int AskVolume2;   ///申卖量二
    double BidPrice3;   ///申买价三
    int BidVolume3;   ///申买量三
    double AskPrice3;   ///申卖价三
    int AskVolume3;   ///申卖量三
    double BidPrice4;   ///申买价四
    int BidVolume4;   ///申买量四
    double AskPrice4;   ///申卖价四
    int AskVolume4;   ///申卖量四
    double BidPrice5;   ///申买价五
    int BidVolume5;   ///申买量五
    double AskPrice5;   ///申卖价五
    int AskVolume5;   ///申卖量五
    char ActionDay[9];   ///业务发生日期
};

#pragma pack(pop)

#endif
