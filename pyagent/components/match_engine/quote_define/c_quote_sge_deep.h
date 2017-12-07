#if !defined(_C_QUOTE_SGE_DEEP_H_)
#define _C_QUOTE_SGE_DEEP_H_

#pragma pack(push, 8)

///深度市场行情（一档行情结构）
struct sge_deep_quote
{
    ///交易日
    char TradingDay[9];
    ///合约代码
    char InstrumentID[81];
    ///交易所代码
    char ExchangeID[9];
    ///合约在交易所的代码
    char ExchangeInstID[31];
    ///最新价
    double LastPrice;
    ///昨结算
    double PreSettlementPrice;
    ///昨收盘
    double PreClosePrice;
    ///昨持仓量
    double PreOpenInterest;
    ///今开盘
    double OpenPrice;
    ///最高价
    double HighestPrice;
    ///最低价
    double LowestPrice;
    ///数量
    int Volume;
    ///成交金额
    double Turnover;
    ///持仓量
    double OpenInterest;
    ///今收盘
    double ClosePrice;
    ///今结算
    double SettlementPrice;
    ///涨停板价
    double UpperLimitPrice;
    ///跌停板价
    double LowerLimitPrice;
    ///昨虚实度
    double PreDelta;
    ///今虚实度
    double CurrDelta;
    ///最后修改时间
    char UpdateTime[9];
    ///最后修改毫秒
    int UpdateMillisec;
    ///申买价一
    double BidPrice1;
    ///申买量一
    int BidVolume1;
    ///申卖价一
    double AskPrice1;
    ///申卖量一
    int AskVolume1;
    ///申买价二
    double BidPrice2;
    ///申买量二
    int BidVolume2;
    ///申卖价二
    double AskPrice2;
    ///申卖量二
    int AskVolume2;
    ///申买价三
    double BidPrice3;
    ///申买量三
    int BidVolume3;
    ///申卖价三
    double AskPrice3;
    ///申卖量三
    int AskVolume3;
    ///申买价四
    double BidPrice4;
    ///申买量四
    int BidVolume4;
    ///申卖价四
    double AskPrice4;
    ///申卖量四
    int AskVolume4;
    ///申买价五
    double BidPrice5;
    ///申买量五
    int BidVolume5;
    ///申卖价五
    double AskPrice5;
    ///申卖量五
    int AskVolume5;
    /// 当日均价
    double AveragePrice;
};
#pragma pack(pop)
#endif