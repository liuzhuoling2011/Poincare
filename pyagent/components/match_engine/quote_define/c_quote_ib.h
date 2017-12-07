#ifndef C_QUOTE_IB_H
#define C_QUOTE_IB_H

#pragma pack(push)
#pragma pack(8)

struct ib_deep_quote
{
    char Symbol[64];
    char Exchange[8];
    char Currency[8];
    long long Timestamp;
    double OpenPrice;
    double HighPrice;
    double LowPrice;
    double LastClosePrice;
    double LastPrice;
    int LastSize;
    int TotalVolume;
    double AskPrice[10];
    double BidPrice[10];
    int AskVol[10];
    int BidVol[10];
};

struct ib_tick_quote
{
    char Symbol[64];
    int Position;
    int Operation;
    int Side;
    double Price;
    int Size;
};

#pragma pack(pop)

#endif

