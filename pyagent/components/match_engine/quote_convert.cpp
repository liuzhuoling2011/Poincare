
#include <string.h>
#include "quote_convert.h"
#include "macro_def.h"

#define MY_FUTURES_COUNT 5
#define MY_STOCK_COUNT 5

static int get_divider_volume(Exchanges exch)
{
	int divider = 2;
	switch (exch){
	case CFFEX:
	case CBOT:
	case CME:
	case LME:
	case COMEX:
	case NYMEX:
	case SGX:
	case SZSE:
	case SSE:
		divider = 1;
		break;
	default:
		divider = 2;
		break;
	}
	return divider;
}

void process_stock_book(void *origin, quote_info* info)
{
	Stock_Internal_Book *quote = (Stock_Internal_Book *)origin;
	info->high_limit = quote->upper_limit_px;
	info->low_limit = quote->lower_limit_px;
	info->curr_quote.int_time = quote->exch_time;

	int divider = 1;
	info->curr_quote.amount = quote->total_notional / divider;
	info->curr_quote.total_volume = (int)quote->total_vol / divider;
	info->curr_quote.high_price = quote->high_px;
	info->curr_quote.low_price = quote->low_px;

	info->curr_quote.last_price = quote->last_px;
	info->curr_quote.tot_b_vol = (int)quote->total_bid_vol;
	info->curr_quote.tot_s_vol = (int)quote->total_ask_vol;

	pv_pair* b_pv_ar = info->curr_quote.bs_pv[0];
	pv_pair* s_pv_ar = info->curr_quote.bs_pv[1];
	for (int i = 0; i < QUOTE_LVL_CNT; i++) {
		b_pv_ar[i].price = quote->bp_array[i];
		b_pv_ar[i].vol = quote->bv_array[i];
		s_pv_ar[i].price = quote->ap_array[i];
		s_pv_ar[i].vol = quote->av_array[i];
	}
}

void process_future_book(void *origin, quote_info* info)
{
	Futures_Internal_Book *quote = (Futures_Internal_Book *)origin;
	info->high_limit = quote->upper_limit_px;
	info->low_limit = quote->lower_limit_px;
	info->curr_quote.int_time = quote->int_time;

	int divider = get_divider_volume(Exchanges(quote->exchange));
	info->curr_quote.amount = quote->total_notional / divider;
	info->curr_quote.total_volume = (int)quote->total_vol / divider;
	info->curr_quote.high_price = quote->high_px;
	info->curr_quote.low_price = quote->low_px;

	info->curr_quote.last_price = quote->last_px;
	info->curr_quote.tot_b_vol = (int)quote->total_buy_ordsize;
	info->curr_quote.tot_s_vol = (int)quote->total_sell_ordsize;

	pv_pair* b_pv_ar = info->curr_quote.bs_pv[0];
	pv_pair* s_pv_ar = info->curr_quote.bs_pv[1];
	for (int i = 0; i < QUOTE_LVL_CNT; i++) {
		b_pv_ar[i].price = quote->bp_array[i];
		b_pv_ar[i].vol = quote->bv_array[i];
		s_pv_ar[i].price = quote->ap_array[i];
		s_pv_ar[i].vol = quote->av_array[i];
	}
}

void process_internal_bar(void *origin, int type, quote_info* info)
{
	struct Internal_Bar* quote = (Internal_Bar *)origin;
	info->high_limit = quote->upper_limit;
	info->low_limit = quote->lower_limit;

	info->curr_quote.int_time = quote->int_time;
	info->curr_quote.amount = quote->turnover;
	info->curr_quote.total_volume = (int)quote->volume;
	info->curr_quote.high_price = quote->high;
	info->curr_quote.low_price = quote->low;
	info->curr_quote.last_price = quote->close;
	
}


/************ Convert Data to Internal_Book, Internal_Bar ************/
int
convert_CFFEX_depth_to_futures_book(cffex_deep_quote *a_quote,
	Futures_Internal_Book *a_book_out)
{
	a_book_out->feed_type = MI_CFEX_DEEP;
	a_book_out->last_px = a_quote->dLastPrice;
	a_book_out->pre_settle_px = a_quote->dPreSettlementPrice;
	a_book_out->pre_close_px = a_quote->dPreClosePrice;
	a_book_out->pre_open_interest = a_quote->dPreOpenInterest;
	a_book_out->open_px = a_quote->dOpenPrice;
	a_book_out->high_px = a_quote->dHighestPrice;
	a_book_out->low_px = a_quote->dLowestPrice;
	a_book_out->total_vol = a_quote->nVolume;
	a_book_out->total_notional = a_quote->dTurnover;
	a_book_out->open_interest = a_quote->dOpenInterest;
	a_book_out->close_px = a_quote->dClosePrice;
	a_book_out->settle_px = a_quote->dSettlementPrice;
	a_book_out->upper_limit_px = a_quote->dUpperLimitPrice;
	a_book_out->lower_limit_px = a_quote->dLowerLimitPrice;

	//merge_timestamp(a_quote->szUpdateTime, a_quote->nUpdateMillisec, a_book_out->char_time);
	a_book_out->int_time = get_timestamp(a_quote->szUpdateTime, a_quote->nUpdateMillisec);
	my_strncpy(a_book_out->symbol, a_quote->szInstrumentID, sizeof(a_quote->szInstrumentID));

	
	a_book_out->exchange = CFFEX;
	a_book_out->avg_px = 0;

	a_book_out->bp_array[0] = a_quote->dBidPrice1; a_book_out->bp_array[1] = a_quote->dBidPrice2;
	a_book_out->bp_array[2] = a_quote->dBidPrice3; a_book_out->bp_array[3] = a_quote->dBidPrice4;
	a_book_out->bp_array[4] = a_quote->dBidPrice5;
	a_book_out->ap_array[0] = a_quote->dAskPrice1; a_book_out->ap_array[1] = a_quote->dAskPrice2;
	a_book_out->ap_array[2] = a_quote->dAskPrice3; a_book_out->ap_array[3] = a_quote->dAskPrice4;
	a_book_out->ap_array[4] = a_quote->dAskPrice5;
	a_book_out->bv_array[0] = a_quote->nBidVolume1; a_book_out->bv_array[1] = a_quote->nBidVolume2;
	a_book_out->bv_array[2] = a_quote->nBidVolume3; a_book_out->bv_array[3] = a_quote->nBidVolume4;
	a_book_out->bv_array[4] = a_quote->nBidVolume5;
	a_book_out->av_array[0] = a_quote->nAskVolume1; a_book_out->av_array[1] = a_quote->nAskVolume2;
	a_book_out->av_array[2] = a_quote->nAskVolume3; a_book_out->av_array[3] = a_quote->nAskVolume4;
	a_book_out->av_array[4] = a_quote->nAskVolume5;

	return 0;
}

int
convert_SHFE_depth_to_futures_book(shfe_my_quote *p_stSHQuoteIn,
	Futures_Internal_Book *a_book_out)
{
	a_book_out->feed_type = MI_SHFE_MY_LVL3;
	a_book_out->exchange = SHFE;
	a_book_out->last_px = p_stSHQuoteIn->LastPrice;
	a_book_out->pre_settle_px = p_stSHQuoteIn->PreSettlementPrice;
	a_book_out->pre_close_px = p_stSHQuoteIn->PreClosePrice;
	a_book_out->pre_open_interest = p_stSHQuoteIn->PreOpenInterest;
	a_book_out->open_px = p_stSHQuoteIn->OpenPrice;
	a_book_out->high_px = p_stSHQuoteIn->HighestPrice;
	a_book_out->low_px = p_stSHQuoteIn->LowestPrice;
	a_book_out->total_vol = p_stSHQuoteIn->Volume;
	a_book_out->total_notional = p_stSHQuoteIn->Turnover;
	a_book_out->open_interest = p_stSHQuoteIn->OpenInterest;
	a_book_out->close_px = p_stSHQuoteIn->ClosePrice;
	a_book_out->settle_px = p_stSHQuoteIn->SettlementPrice;
	a_book_out->upper_limit_px = p_stSHQuoteIn->UpperLimitPrice;
	a_book_out->lower_limit_px = p_stSHQuoteIn->LowerLimitPrice;

	//merge_timestamp(p_stSHQuoteIn->UpdateTime, p_stSHQuoteIn->UpdateMillisec, a_book_out->char_time);
	a_book_out->int_time = get_timestamp(p_stSHQuoteIn->UpdateTime, p_stSHQuoteIn->UpdateMillisec);
	//PRINT('N', "Time: %d", a_book_out->int_time);
	my_strncpy(a_book_out->symbol, p_stSHQuoteIn->InstrumentID, sizeof(p_stSHQuoteIn->InstrumentID));
	//if (fabs(a_book_out->bp1 - p_stSHQuoteIn->BidPrice1) > 0.001 ||
	//	fabs(a_book_out->ap1 - p_stSHQuoteIn->AskPrice1) > 0.001)
	//	a_book_out->shfe_data_flag = 2;

	//	}
	//	else{
	//		a_book_out->bp1 = p_stSHQuoteIn->buy_price[0]; //p_stSHQuoteIn->BidPrice1
	//		a_book_out->bv1 = p_stSHQuoteIn->buy_volume[0];
	//		a_book_out->ap1 = p_stSHQuoteIn->sell_price[0];
	//		a_book_out->av1 = p_stSHQuoteIn->sell_volume[0];
	//	}

	for (int i = 0; i < MY_FUTURES_COUNT; i++){
		a_book_out->bp_array[i] = p_stSHQuoteIn->buy_price[i];
		a_book_out->ap_array[i] = p_stSHQuoteIn->sell_price[i];
		a_book_out->bv_array[i] = p_stSHQuoteIn->buy_volume[i];
		a_book_out->av_array[i] = p_stSHQuoteIn->sell_volume[i];
	}

	a_book_out->total_buy_ordsize = p_stSHQuoteIn->buy_total_volume;
	a_book_out->total_sell_ordsize = p_stSHQuoteIn->sell_total_volume;
	a_book_out->weighted_buy_px = p_stSHQuoteIn->buy_weighted_avg_price;
	a_book_out->weighted_sell_px = p_stSHQuoteIn->sell_weighted_avg_price;

	return 0;
}

int
convert_CZCE_depth_to_futures_book(czce_my_deep_quote *p_stcZZQuoteIn,
	Futures_Internal_Book *a_book_out)
{
	a_book_out->feed_type = MI_CZCE_DEEP_MY;
	a_book_out->exchange = CZCE;
	a_book_out->last_px = p_stcZZQuoteIn->LastPrice;
	a_book_out->pre_settle_px = p_stcZZQuoteIn->PreSettle;
	a_book_out->pre_close_px = p_stcZZQuoteIn->PreClose;
	a_book_out->pre_open_interest = p_stcZZQuoteIn->PreOpenInterest;
	a_book_out->open_px = p_stcZZQuoteIn->OpenPrice;
	a_book_out->high_px = p_stcZZQuoteIn->HighPrice;
	a_book_out->low_px = p_stcZZQuoteIn->LowPrice;
	a_book_out->total_vol = p_stcZZQuoteIn->TotalVolume;
	a_book_out->total_notional = 0.0;
	a_book_out->open_interest = p_stcZZQuoteIn->OpenInterest;
	a_book_out->close_px = p_stcZZQuoteIn->ClosePrice;
	a_book_out->settle_px = p_stcZZQuoteIn->SettlePrice;
	a_book_out->upper_limit_px = p_stcZZQuoteIn->HighLimit;
	a_book_out->lower_limit_px = p_stcZZQuoteIn->LowLimit;

	a_book_out->avg_px = p_stcZZQuoteIn->AveragePrice;

	a_book_out->total_buy_ordsize = p_stcZZQuoteIn->TotalBidLot;
	a_book_out->total_sell_ordsize = p_stcZZQuoteIn->TotalAskLot;
	a_book_out->weighted_buy_px = 0.0;
	a_book_out->weighted_sell_px = 0.0;


	a_book_out->int_time = get_timestamp(p_stcZZQuoteIn->TimeStamp, false);
	my_strncpy(a_book_out->symbol, p_stcZZQuoteIn->ContractID, sizeof(p_stcZZQuoteIn->ContractID));

	for (int i = 0; i < MY_FUTURES_COUNT; i++){
		a_book_out->bp_array[i] = p_stcZZQuoteIn->BidPrice[i];
		a_book_out->bv_array[i] = p_stcZZQuoteIn->BidLot[i];
		a_book_out->ap_array[i] = p_stcZZQuoteIn->AskPrice[i];
		a_book_out->av_array[i] = p_stcZZQuoteIn->AskLot[i];
	}

	return 0;
}

int
convert_DCE_depth_to_futures_book(dce_my_best_deep_quote *a_quote,
	Futures_Internal_Book *a_book_out)
{
	a_book_out->feed_type = MI_DCE_BEST_DEEP;
	a_book_out->exchange = DCE;
	my_strncpy(a_book_out->symbol, a_quote->Contract, sizeof(a_book_out->symbol));
	a_book_out->int_time = get_timestamp(a_quote->GenTime, true);

	a_book_out->pre_settle_px = a_quote->LastClearPrice;
	a_book_out->last_px = a_quote->LastPrice;
	a_book_out->open_interest = a_quote->OpenInterest * 1.0;
	a_book_out->total_vol = a_quote->MatchTotQty * 1;
	a_book_out->upper_limit_px = a_quote->RiseLimit;
	a_book_out->lower_limit_px = a_quote->FallLimit;

	a_book_out->bp_array[0] = a_quote->BuyPriceOne;
	a_book_out->bp_array[1] = a_quote->BuyPriceTwo;
	a_book_out->bp_array[2] = a_quote->BuyPriceThree;
	a_book_out->bp_array[3] = a_quote->BuyPriceFour;
	a_book_out->bp_array[4] = a_quote->BuyPriceFive;
	a_book_out->ap_array[0] = a_quote->SellPriceOne;
	a_book_out->ap_array[1] = a_quote->SellPriceTwo;
	a_book_out->ap_array[2] = a_quote->SellPriceThree;
	a_book_out->ap_array[3] = a_quote->SellPriceFour;
	a_book_out->ap_array[4] = a_quote->SellPriceFive;
	a_book_out->bv_array[0] = a_quote->BuyQtyOne;
	a_book_out->bv_array[1] = a_quote->BuyQtyTwo;
	a_book_out->bv_array[2] = a_quote->BuyQtyThree;
	a_book_out->bv_array[3] = a_quote->BuyQtyFour;
	a_book_out->bv_array[4] = a_quote->BuyQtyFive;
	a_book_out->av_array[0] = a_quote->SellQtyOne;
	a_book_out->av_array[1] = a_quote->SellQtyTwo;
	a_book_out->av_array[2] = a_quote->SellQtyThree;
	a_book_out->av_array[3] = a_quote->SellQtyFour;
	a_book_out->av_array[4] = a_quote->SellQtyFive;
	a_book_out->open_px = a_quote->OpenPrice;
	a_book_out->high_px = a_quote->HighPrice;
	a_book_out->low_px = a_quote->LowPrice;
	a_book_out->pre_close_px = a_quote->LastClose;
	a_book_out->pre_open_interest = a_quote->LastOpenInterest * 1.0;
	a_book_out->avg_px = a_quote->AvgPrice;
	a_book_out->total_notional = a_quote->Turnover;

	a_book_out->implied_bid_size[0] = a_quote->BuyImplyQtyOne;
	a_book_out->implied_bid_size[1] = a_quote->BuyImplyQtyTwo;
	a_book_out->implied_bid_size[2] = a_quote->BuyImplyQtyThree;
	a_book_out->implied_bid_size[3] = a_quote->BuyImplyQtyFour;
	a_book_out->implied_bid_size[4] = a_quote->BuyImplyQtyFive;
	a_book_out->implied_ask_size[0] = a_quote->SellImplyQtyOne;
	a_book_out->implied_ask_size[1] = a_quote->SellImplyQtyTwo;
	a_book_out->implied_ask_size[2] = a_quote->SellImplyQtyThree;
	a_book_out->implied_ask_size[3] = a_quote->SellImplyQtyFour;
	a_book_out->implied_ask_size[4] = a_quote->SellImplyQtyFive;

	return 0;
}

int
convert_DCE_level1_to_futures_book(dce_my_ctp_deep_quote *a_quote,
	Futures_Internal_Book *a_book_out)
{
	a_book_out->feed_type = MI_DCE_LV1;
	a_book_out->exchange = DCE;
	my_strncpy(a_book_out->symbol, a_quote->Contract, sizeof(a_book_out->symbol));
	a_book_out->int_time = (int)a_quote->Time;

	a_book_out->pre_settle_px = a_quote->LastClearPrice;
	a_book_out->last_px = a_quote->LastPrice;
	a_book_out->open_interest = a_quote->OpenInterest * 1.0;
	a_book_out->total_vol = a_quote->MatchTotQty * 1;
	a_book_out->upper_limit_px = a_quote->RiseLimit;
	a_book_out->lower_limit_px = a_quote->FallLimit;

	a_book_out->bp_array[0] = a_quote->BuyPriceOne;
	a_book_out->ap_array[0] = a_quote->SellPriceOne;
	a_book_out->bv_array[0] = a_quote->BuyQtyOne;
	a_book_out->av_array[0] = a_quote->SellQtyOne;
	a_book_out->open_px = a_quote->OpenPrice;
	a_book_out->high_px = a_quote->HighPrice;
	a_book_out->low_px = a_quote->LowPrice;
	a_book_out->pre_close_px = a_quote->LastClose;
	a_book_out->pre_open_interest = a_quote->LastOpenInterest * 1.0;
	a_book_out->avg_px = a_quote->AvgPrice;
	a_book_out->total_notional = a_quote->Turnover;

	a_book_out->implied_bid_size[0] = a_quote->BuyImplyQtyOne;
	a_book_out->implied_ask_size[0] = a_quote->SellImplyQtyOne;

	return 0;
}

int
convert_DCE_OrderStat_to_futures_book(dce_my_ord_stat_quote * a_quote,
	Futures_Internal_Book *a_book_out)
{
	a_book_out->feed_type = MI_DCE_ORDER_STATISTIC;
	a_book_out->exchange = DCE;
	my_strncpy(a_book_out->symbol, a_quote->ContractID, sizeof(a_quote->ContractID));
	a_book_out->total_buy_ordsize = a_quote->TotalBuyOrderNum;
	a_book_out->total_sell_ordsize = a_quote->TotalSellOrderNum;
	a_book_out->weighted_buy_px = a_quote->WeightedAverageBuyOrderPrice;
	a_book_out->weighted_sell_px = a_quote->WeightedAverageSellOrderPrice;
	return 0;
}

int
convert_stock_depth_to_stock_book(my_stock_lv2_quote *a_quote,
	Stock_Internal_Book *a_book_out)
{
	my_strncpy(a_book_out->ticker, a_quote->szCode, sizeof(a_quote->szCode));
	my_strncpy(a_book_out->wind_code, a_quote->szWindCode, sizeof(a_quote->szWindCode));

	a_book_out->action_day = a_quote->nActionDay;
	a_book_out->trading_day = a_quote->nTradingDay;
	a_book_out->exch_time = a_quote->nTime;
	a_book_out->status = a_quote->nStatus;

	a_book_out->pre_close_px = (double)a_quote->nPreClose / PRICE_FACTOR_STOCK;
	a_book_out->open_px = (double)a_quote->nOpen / PRICE_FACTOR_STOCK;
	a_book_out->high_px = (double)a_quote->nHigh / PRICE_FACTOR_STOCK;
	a_book_out->low_px = (double)a_quote->nLow / PRICE_FACTOR_STOCK;
	a_book_out->last_px = (double)a_quote->nMatch / PRICE_FACTOR_STOCK;

	for (int i = 0; i < MY_STOCK_COUNT; i++) {
		a_book_out->bp_array[i] = (double)a_quote->nBidPrice[i] / PRICE_FACTOR_STOCK;
		a_book_out->ap_array[i] = (double)a_quote->nAskPrice[i] / PRICE_FACTOR_STOCK;
		a_book_out->bv_array[i] = a_quote->nBidVol[i];
		a_book_out->av_array[i] = a_quote->nAskVol[i];
	}

	a_book_out->num_of_trades = a_quote->nNumTrades;
	a_book_out->total_vol = a_quote->iVolume;
	a_book_out->total_notional = a_quote->iTurnover;
	a_book_out->total_bid_vol = a_quote->nTotalBidVol;
	a_book_out->total_ask_vol = a_quote->nTotalAskVol;
	a_book_out->weighted_avg_bp = a_quote->nWeightedAvgBidPrice;
	a_book_out->weighted_avg_ap = a_quote->nWeightedAvgAskPrice;
	a_book_out->IOPV = a_quote->nIOPV;
	a_book_out->yield_to_maturity = a_quote->nYieldToMaturity;
	a_book_out->upper_limit_px = a_quote->nHighLimited;
	a_book_out->lower_limit_px = a_quote->nLowLimited;
	my_strncpy(a_book_out->prefix, a_quote->chPrefix, sizeof(a_quote->chPrefix));

	a_book_out->PE1 = a_quote->nSyl1;
	a_book_out->PE2 = a_quote->nSyl2;
	a_book_out->change = a_quote->nSD2;
	return 0;
}

/**
* Type 10 - Convert stock index to internal REQUEST_TYPE_QUOTE
*/
int
convert_stock_index_to_stock_book(my_stock_index_quote *a_quote,
	Stock_Internal_Book *a_quote_out)
{
	if (is_endswith(a_quote->szWindCode, ".SH")){
		my_strncpy(a_quote_out->ticker, a_quote->szCode, sizeof(a_quote->szCode));
		strcat(a_quote_out->ticker, ".SH");
	} else if (is_endswith(a_quote->szWindCode, ".SZ")){
		my_strncpy(a_quote_out->ticker, a_quote->szCode, sizeof(a_quote->szCode));
		strcat(a_quote_out->ticker, ".SZ");
	}
	
	my_strncpy(a_quote_out->wind_code, a_quote->szWindCode, sizeof(a_quote->szWindCode));

	a_quote_out->action_day = a_quote->nActionDay;
	a_quote_out->trading_day = a_quote->nTradingDay;
	a_quote_out->exch_time = a_quote->nTime;

	a_quote_out->pre_close_px = (double)a_quote->nPreCloseIndex / PRICE_FACTOR_STOCK;
	a_quote_out->open_px = (double)a_quote->nOpenIndex / PRICE_FACTOR_STOCK;
	a_quote_out->high_px = (double)a_quote->nHighIndex / PRICE_FACTOR_STOCK;
	a_quote_out->low_px = (double)a_quote->nLowIndex / PRICE_FACTOR_STOCK;
	a_quote_out->last_px = (double)a_quote->nLastIndex / PRICE_FACTOR_STOCK;

	a_quote_out->total_vol = a_quote->iTotalVolume * NOTIONAL_FACTOR_INDEX;
	a_quote_out->total_notional = (double)a_quote->iTurnover / NOTIONAL_FACTOR_INDEX;
	
	return 0;
}

/**
* Type 13 - Convert stock option to internal REQUEST_TYPE_QUOTE
*/
int
convert_stock_option_to_stock_book(sgd_option_quote *a_quote,
	Stock_Internal_Book *a_book_out)
{
	my_strncpy(a_book_out->ticker, a_quote->contract_code, sizeof(a_quote->contract_code));
	my_strncpy(a_book_out->wind_code, a_quote->scr_code, sizeof(a_quote->scr_code));

	a_book_out->exch_time = (int)a_quote->time;
	a_book_out->pre_close_px = (double)a_quote->pre_close_price / PRICE_FACTOR_STOCK;
	a_book_out->open_px = (double)a_quote->open_price / PRICE_FACTOR_STOCK;
	a_book_out->high_px = (double)a_quote->high_price / PRICE_FACTOR_STOCK;
	a_book_out->low_px = (double)a_quote->low_price / PRICE_FACTOR_STOCK;
	a_book_out->last_px = (double)a_quote->last_price / PRICE_FACTOR_STOCK;

	for (int i = 0; i < MY_STOCK_COUNT; i++) {
		a_book_out->bp_array[i] = (double)a_quote->bid_price[i] / PRICE_FACTOR_STOCK;
		a_book_out->ap_array[i] = (double)a_quote->sell_price[i] / PRICE_FACTOR_STOCK;
		a_book_out->bv_array[i] = (int)a_quote->bid_volume[i];
		a_book_out->av_array[i] = (int)a_quote->sell_volume[i];
	}

	a_book_out->total_vol = a_quote->bgn_total_vol;
	a_book_out->total_notional = a_quote->bgn_total_amt;
	a_book_out->upper_limit_px = (float)a_quote->high_limit_price / PRICE_FACTOR_STOCK;
	a_book_out->lower_limit_px = (float)a_quote->low_limit_price / PRICE_FACTOR_STOCK;
	return 0;
}

static char
get_exch_by_symbol(const char *a_symbol)
{
	char exch = 'u'; // undefined
	/* convert to lower case */
	char char0 = (a_symbol[0] >= 'A' && a_symbol[0] <= 'Z') ? a_symbol[0] + 32 : a_symbol[0];
	char char1 = (a_symbol[1] >= 'A' && a_symbol[1] <= 'Z') ? a_symbol[1] + 32 : a_symbol[1];

	switch (char0) {
	case 'a':
		switch (char1) {
		case 'g': case 'l': case 'u': // ag al au
			exch = 'A'; break;
		default:  // a
			exch = 'B'; break;
		} break;
	case 'b':
		switch (char1) {
		case 'u': // bu
			exch = 'A'; break;
		default:  // b, bb
			exch = 'B'; break;
		} break;
	case 'c':
		switch (char1) {
		case 'f': // cf
			exch = 'C'; break;
		case 'u': // cu
			exch = 'A'; break;
		default: // c cs
			exch = 'B'; break;
		} break;
	case 'f':
		switch (char1) {
		case 'b': // fb
			exch = 'B'; break;
		case 'g': // fg
			exch = 'C'; break;
		case 'u': // fu
			exch = 'A'; break;
		} break;
	case 'h': // hc
		exch = 'A';
		break;
	case 'i':
		switch (char1) {
		case 'c': case 'f': case 'h': // ic if ih
			exch = 'G'; break;
		default: // i
			exch = 'B'; break;
		} break;
	case 'j':
		switch (char1) {
		case 'r': // jr
			exch = 'C'; break;
		default: // j jd jm
			exch = 'B'; break;
		} break;
	case 'l':
		switch (char1) {
		case 'r':// lr
			exch = 'C'; break;
		default: // l
			exch = 'B'; break;
		} break;
	case 'm':
		switch (char1) {
		case 'a': case 'e': // ma me
			exch = 'C'; break;
		default: // m
			exch = 'B'; break;
		} break;
	case 'n': // ni
		exch = 'A';
		break;
	case 'o': // oi
		exch = 'C';
		break;
	case 'p':
		switch (char1) {
		case 'b': // pb
			exch = 'A'; break;
		case 'm': // pm
			exch = 'C'; break;
		default: // p pp
			exch = 'B'; break;
		} break;
	case 'r':
		switch (char1) {
		case 'b': case 'u': // rb ru
			exch = 'A'; break;
		case 'i': case 'm': case 's': // ri rm rs
			exch = 'C'; break;
		} break;
	case 's': 
		switch (char1)
		{
		case 'n':// sn
			exch = 'A'; break;
		default:// sf sm sr
			exch = 'C'; break;
		}
		break;
	case 't':
		switch (char1) {
		case 'a': case 'c': // ta tc
			exch = 'C'; break;
		default: // t tf
			exch = 'G'; break;
		} break;
	case 'v': // v
		exch = 'B';
		break;
	case 'w':
		switch (char1) {
		case 'h': // wh
			exch = 'C'; break;
		case 'r': // wr
			exch = 'A'; break;
		} break;
	case 'y': // y
		exch = 'B';
		break;
	case 'z':
		switch (char1) {
		case 'c': // zc
			exch = 'C'; break;
		case 'n': // zn
			exch = 'A'; break;
		} break;
	default:
		break;
	}

	return exch;
}
/**
* Type 12
*/
int
convert_my_level1_to_futures_book(my_level1_quote *a_quote,
	Futures_Internal_Book *a_quote_out)
{
	a_quote_out->feed_type = MI_MY_LEVEL1;
	a_quote_out->exchange = (Exchanges)get_exch_by_symbol(a_quote->InstrumentID);
	a_quote_out->last_px = a_quote->LastPrice;
	a_quote_out->pre_settle_px = a_quote->PreSettlementPrice;
	a_quote_out->pre_close_px = a_quote->PreClosePrice;
	a_quote_out->pre_open_interest = a_quote->PreOpenInterest;
	a_quote_out->open_px = a_quote->OpenPrice;
	a_quote_out->high_px = a_quote->HighestPrice;
	a_quote_out->low_px = a_quote->LowestPrice;
	a_quote_out->total_vol = a_quote->Volume;
	a_quote_out->total_notional = a_quote->Turnover;
	a_quote_out->open_interest = a_quote->OpenInterest;
	a_quote_out->close_px = a_quote->ClosePrice;
	a_quote_out->settle_px = a_quote->SettlementPrice;
	a_quote_out->upper_limit_px = a_quote->UpperLimitPrice;
	a_quote_out->lower_limit_px = a_quote->LowerLimitPrice;

	a_quote_out->int_time = get_timestamp(a_quote->UpdateTime, a_quote->UpdateMillisec);
	my_strncpy(a_quote_out->symbol, a_quote->InstrumentID, sizeof(a_quote->InstrumentID));

	a_quote_out->bp_array[0] = a_quote->BidPrice1;
	a_quote_out->bv_array[0] = a_quote->BidVolume1;
	a_quote_out->ap_array[0] = a_quote->AskPrice1;
	a_quote_out->av_array[0] = a_quote->AskVolume1;
	return 0;
}

/**
* Type 19
*/
int
convert_ib_depth_to_futures_book(ib_deep_quote *a_quote,
	Futures_Internal_Book *a_book_out)
{
	a_book_out->feed_type = MI_IB_BOOK;
	my_strncpy(a_book_out->symbol, a_quote->Symbol, sizeof(a_quote->Symbol));

	if (my_strcmp("SGX", a_quote->Exchange) == 0) {
		a_book_out->exchange = SGX;
	}
	else if (my_strcmp("SEHK", a_quote->Exchange) == 0) {
		a_book_out->exchange = HKEX;
	}
	else
		PRINT_ERROR("Unknown Quote Exchange: %s", a_quote->Exchange);

	long long ms = (long long)a_quote->Timestamp / 1000 % 86400000; // milliseconds of the day
	char char_time[10];
	snprintf(char_time, 10, "%02d%02d%02d%03d", // hour minute second millisecond
		(int)ms / 3600000, (int)ms % 3600000 / 60000, (int)ms % 60000 / 1000, (int)ms % 1000);

	a_book_out->int_time = get_int_time(char_time);
	a_book_out->open_px = a_quote->OpenPrice;
	a_book_out->high_px = a_quote->HighPrice;
	a_book_out->low_px = a_quote->LowPrice;
	a_book_out->pre_close_px = a_quote->LastClosePrice;
	a_book_out->last_px = a_quote->LastPrice;
	a_book_out->total_vol = a_quote->TotalVolume;

	for (int i = 0; i < MY_FUTURES_COUNT; i++){
		a_book_out->bp_array[i] = a_quote->BidPrice[i];
		a_book_out->ap_array[i] = a_quote->AskPrice[i];
		a_book_out->bv_array[i] = a_quote->BidVol[i];
		a_book_out->av_array[i] = a_quote->AskVol[i];
	}

	return 0;
}

int
convert_sge_depth_to_futures_book(sge_deep_quote *a_quote,
	Futures_Internal_Book *a_quote_out)
{
	a_quote_out->feed_type = MI_SGE_DEEP;
	a_quote_out->exchange = SGE;
	my_strncpy(a_quote_out->symbol, a_quote->InstrumentID, sizeof(a_quote_out->symbol)); //to make sure how long InstrumentID is
	a_quote_out->last_px = a_quote->LastPrice;
	a_quote_out->pre_settle_px = a_quote->PreSettlementPrice;
	a_quote_out->pre_close_px = a_quote->PreClosePrice;
	a_quote_out->pre_open_interest = a_quote->PreOpenInterest;
	a_quote_out->open_px = a_quote->OpenPrice;
	a_quote_out->high_px = a_quote->HighestPrice;
	a_quote_out->low_px = a_quote->LowestPrice;
	a_quote_out->total_vol = a_quote->Volume;
	a_quote_out->total_notional = a_quote->Turnover;
	a_quote_out->open_interest = a_quote->OpenInterest;
	a_quote_out->close_px = a_quote->ClosePrice;
	a_quote_out->settle_px = a_quote->SettlementPrice;
	a_quote_out->upper_limit_px = a_quote->UpperLimitPrice;
	a_quote_out->lower_limit_px = a_quote->LowerLimitPrice;

	a_quote_out->int_time = get_timestamp(a_quote->UpdateTime, a_quote->UpdateMillisec);	//to be confirmed with trader
	a_quote_out->bp_array[0] = a_quote->BidPrice1;
	a_quote_out->bv_array[0] = a_quote->BidVolume1;
	a_quote_out->ap_array[0] = a_quote->AskPrice1;
	a_quote_out->av_array[0] = a_quote->AskVolume1;
	a_quote_out->bp_array[1] = a_quote->BidPrice2;
	a_quote_out->bv_array[1] = a_quote->BidVolume2;
	a_quote_out->ap_array[1] = a_quote->AskPrice2;
	a_quote_out->av_array[1] = a_quote->AskVolume2;
	a_quote_out->bp_array[2] = a_quote->BidPrice3;
	a_quote_out->bv_array[2] = a_quote->BidVolume3;
	a_quote_out->ap_array[2] = a_quote->AskPrice3;
	a_quote_out->av_array[2] = a_quote->AskVolume3;
	a_quote_out->bp_array[3] = a_quote->BidPrice4;
	a_quote_out->bv_array[3] = a_quote->BidVolume4;
	a_quote_out->ap_array[3] = a_quote->AskPrice4;
	a_quote_out->av_array[3] = a_quote->AskVolume4;
	a_quote_out->bp_array[4] = a_quote->BidPrice5;
	a_quote_out->bv_array[4] = a_quote->BidVolume5;
	a_quote_out->ap_array[4] = a_quote->AskPrice5;
	a_quote_out->av_array[4] = a_quote->AskVolume5;
	a_quote_out->avg_px = a_quote->AveragePrice;

	return 0;
}


int
convert_esunny_frn_to_futures_book(my_esunny_frn_quote *a_quote,
	Futures_Internal_Book *a_quote_out)
{
	a_quote_out->feed_type = MI_MY_ESUNNY_FOREIGN;

	if (strncmp(a_quote->Market, "CBOT", 4) == 0)
		a_quote_out->exchange = CBOT;
	else if (strncmp(a_quote->Market, "CME", 3) == 0)
		a_quote_out->exchange = CME;
	else if (my_strcmp(a_quote->Market, "LME") == 0)
		a_quote_out->exchange = LME;
	else if (my_strcmp(a_quote->Market, "CMX") == 0)
		a_quote_out->exchange = COMEX;
	else if (my_strcmp(a_quote->Market, "NYM") == 0)
		a_quote_out->exchange = NYMEX;
	else if (my_strcmp(a_quote->Market, "SGX4") == 0 || my_strcmp(a_quote->Market, "SGX") == 0)
		a_quote_out->exchange = SGX;
	else
		PRINT_ERROR("a_quote->Market to be implemented!");

	my_strncpy(a_quote_out->symbol, a_quote->Code, sizeof(a_quote_out->symbol));
	a_quote_out->pre_close_px = a_quote->YClose;
	a_quote_out->pre_settle_px = a_quote->YSettle;
	a_quote_out->open_px = a_quote->Open;
	a_quote_out->high_px = a_quote->High;
	a_quote_out->low_px = a_quote->Low;
	a_quote_out->last_px = a_quote->New;
	a_quote_out->settle_px = a_quote->Settle;
	a_quote_out->total_vol = (uint64_t)a_quote->Volume;
	a_quote_out->open_interest = a_quote->Amount;

	for (int i = 0; i < MY_FUTURES_COUNT; i++) {
		a_quote_out->ap_array[i] = a_quote->Ask[i];
		a_quote_out->av_array[i] = (int)a_quote->AskVol[i];
		a_quote_out->bp_array[i] = a_quote->Bid[i];
		a_quote_out->bv_array[i] = (int)a_quote->BidVol[i];
	}

	a_quote_out->avg_px = a_quote->AvgPrice;
	a_quote_out->upper_limit_px = a_quote->LimitUp;
	a_quote_out->lower_limit_px = a_quote->LimitDown;
	a_quote_out->high_px = a_quote->HistoryHigh;
	a_quote_out->low_px = a_quote->HistoryLow;
	a_quote_out->pre_open_interest = a_quote->YOPI;
	a_quote_out->total_notional = a_quote->CJJE;
	a_quote_out->close_px = a_quote->TClose;

	int l_tmp_time = (int)a_quote->updatetime;
	if (l_tmp_time < 30000){
		a_quote_out->int_time = (l_tmp_time + 240000) * 1000;
	} else {
		a_quote_out->int_time = l_tmp_time * 1000;
	}

	return 0;
}

void
normalize_bar(MI_TYPE a_quote_type, void *a_quote)
{
	Internal_Bar *l_tmp_bar = (Internal_Bar *)a_quote;

	if (a_quote_type == MI_MY_STOCK_LV2 || a_quote_type == MI_MY_STOCK_INDEX){
		l_tmp_bar->open = (double)l_tmp_bar->open / PRICE_FACTOR_STOCK;
		l_tmp_bar->high = (double)l_tmp_bar->high / PRICE_FACTOR_STOCK;
		l_tmp_bar->low = (double)l_tmp_bar->low / PRICE_FACTOR_STOCK;
		l_tmp_bar->close = (double)l_tmp_bar->close / PRICE_FACTOR_STOCK;
	}
}

void
normalize_book(MI_TYPE a_quote_type, void *a_quote, Stock_Internal_Book *s_book, Futures_Internal_Book *f_book)
{
	switch (a_quote_type)
	{
	case MI_CFEX_DEEP:
		convert_CFFEX_depth_to_futures_book((cffex_deep_quote *)(a_quote), f_book);
		break;
	case MI_DCE_BEST_DEEP:
		convert_DCE_depth_to_futures_book((dce_my_best_deep_quote *)(a_quote), f_book);
		break;
	case MI_DCE_MARCH_PRICE:
		PRINT_WARN("MI_DCE_MARCH_PRICE to be implemented");
		break;
	case MI_DCE_ORDER_STATISTIC:
		convert_DCE_OrderStat_to_futures_book((dce_my_ord_stat_quote *)a_quote, f_book);
		break;
	case MI_DCE_REALTIME_PRICE:
		PRINT_WARN("MI_DCE_REALTIME_PRICE to be implemented");
		break;
	case MI_DCE_TEN_ENTRUST:
		PRINT_WARN("MI_DCE_TEN_ENTRUST to be implemented");
		break;
	case MI_SHFE_MY:
	case MI_SHFE_MY_LVL3:
		convert_SHFE_depth_to_futures_book((shfe_my_quote *)(a_quote), f_book);
		break;
	case MI_CZCE_DEEP_MY:
		convert_CZCE_depth_to_futures_book((czce_my_deep_quote*)(a_quote), f_book);			   
		break;
	case MI_CZCE_SNAPSHOT_MY:			// 8
		PRINT_WARN("MI_CZCE_SNAPSHOT_MY to be implemented");
		break;
	case MI_MY_STOCK_LV2:				// 9 -  Stock Depth
		convert_stock_depth_to_stock_book((my_stock_lv2_quote*)(a_quote), s_book);
		break;
	case MI_MY_STOCK_INDEX:
		convert_stock_index_to_stock_book((my_stock_index_quote *)a_quote, s_book);
		break;
	case MI_MY_MARKET_DATA:
		PRINT_WARN("MI_MY_MARKET_DATA to be implemented");
		break;
	case MI_MY_LEVEL1:															  
		convert_my_level1_to_futures_book((my_level1_quote *)a_quote, f_book);
		break;
	case MI_SGD_OPTION: 				// 13 sgd_option_quote
		convert_stock_option_to_stock_book((sgd_option_quote *)a_quote, s_book);
		break;
	case MI_SGD_ORDER_QUEUE:			// 14 sgd_order_queue
		PRINT_WARN("MI_SGD_ORDER_QUEUE to be implemented");
		break;
	case MI_SGD_PER_ENTRUST:			// 15 sgd_per_entrust
		PRINT_WARN("MI_SGD_PER_ENTRUST to be implemented");
		break;
	case MI_SGD_PER_BARGAIN:			// 16 sgd_per_bargain
		PRINT_WARN("MI_SGD_PER_BARGAIN to be implemented");
		break;
		/* MY REQUEST_TYPE_QUOTE, for stock & IF, another 'inner REQUEST_TYPE_QUOTE' */
	case MI_MY_EACH_DATASRC:			// 17 my_each_datasource
		PRINT_WARN("It's a Internal Data Update, shouldn't come here");
		break;
	case MI_IB_BOOK:
		convert_ib_depth_to_futures_book((ib_deep_quote *)a_quote, f_book);
		break;
	case MI_TAP_MY:						// 18 tap_my_quote, YiShen A50 Feed, not used.
		PRINT_WARN("MI_TAP_MY to be implemented");
		break;
	case MI_IB_QUOTE:					// 19 ib continuous feed, not used since we use the IB book.
		PRINT_WARN("MI_IB_QUOTE not implemented");
		break;
	case MI_SGE_DEEP:
		convert_sge_depth_to_futures_book((sge_deep_quote *)a_quote, f_book);
		break;
	case MI_MY_ESUNNY_FOREIGN:
		convert_esunny_frn_to_futures_book((my_esunny_frn_quote *)a_quote, f_book);
		break;
	case MI_DCE_LV1:																   
		convert_DCE_level1_to_futures_book((dce_my_ctp_deep_quote*)a_quote, f_book);
		break;
	case MI_MY_BAR:
		break;
	default:
		PRINT_ERROR("QuoteType is not supported!");
		break;
	}
}

