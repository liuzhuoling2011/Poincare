#ifndef C_QUOTE_DCE_LEVEL1_H
#define C_QUOTE_DCE_LEVEL1_H

#pragma pack(push)
#pragma pack(8)

////////////////////////////////////////////////
///MDBestAndDeep：一档深度行情
////////////////////////////////////////////////
struct dce_my_ctp_deep_quote
{
	unsigned int Time;                           //预留字段
	char Contract[80];                   //合约代码
	double LastClearPrice;                 //昨结算价
	double ClearPrice;                     //今结算价
	unsigned int LastOpenInterest;               //昨持仓量
	double LastPrice;                      //最新价
	double HighPrice;                      //最高价
	double LowPrice;                       //最低价
	unsigned int LastMatchQty;                   //最新成交量
	unsigned int MatchTotQty;                    //成交数量
	double Turnover;                       //成交金额
	double LifeLow;                        //历史最低价
	double LifeHigh;                       //历史最高价
	double RiseLimit;                      //最高报价
	double FallLimit;                      //最低报价
	double LastClose;                      //昨收盘
	double BuyPriceOne;                     //买入价格1
	unsigned int BuyQtyOne;                //买入数量1
	unsigned int BuyImplyQtyOne;           //买1推导量
	double SellPriceOne;             //卖出价格1
	unsigned int SellQtyOne;               //买出数量1
	unsigned int SellImplyQtyOne;          //卖1推导量
	double AvgPrice;                       //成交均价
	double OpenPrice;                      //今开盘
	double Close;                          //今收盘
	double Delta;                          //delta
	double Gamma;                          //gama
	double Rho;                            //rho
	double Theta;                          //theta
	double Vega;                           //vega
	unsigned int OpenInterest;                   //持仓量
	int InterestChg;                    //持仓量变化

	char TradeDate[9];                   //行情日期
};

#pragma pack(pop)

#endif
