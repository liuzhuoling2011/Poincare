void internalize(CThostFtdcDepthMarketDataField * ctp_quote, Futures_Internal_Book * internal_book){
	//correct
	internal_book->feed_type = 0;
	strlcpy(g_config_t.contracts[0].symbol, ctp_quote->InstrumentID, SYMBOL_LEN);
	//correct
	internal_book->exchange = (int)ctp_quote->ExchangeID;
	//correct
	internal_book->int_time = (int)ctp_quote->UpdateTime;
	internal_book->pre_close_px = ctp_quote->PreClosePrice;
	internal_book->pre_settle_px = ctp_quote->PreSettlementPrice;
	internal_book->pre_open_interest = ctp_quote->PreOpenInterest;
	internal_book->open_interest = ctp_quote->OpenInterest;
	internal_book->open_px = ctp_quote->OpenPrice;
	internal_book->high_px = ctp_quote->HighestPrice;
	internal_book->low_px = ctp_quote->LowestPrice;
	internal_book->avg_px = ctp_quote->AveragePrice;
	internal_book->last_px = ctp_quote->LastPrice;
	internal_book->bp_array[0] = ctp_quote->BidPrice1;
	internal_book->bp_array[1] = ctp_quote->BidPrice2;
	internal_book->bp_array[2] = ctp_quote->BidPrice3;
	internal_book->bp_array[3] = ctp_quote->BidPrice4;
	internal_book->bp_array[4] = ctp_quote->BidPrice5;

	internal_book->ap_array[0] = ctp_quote->AskPrice1;
	internal_book->ap_array[1] = ctp_quote->AskPrice2;
	internal_book->ap_array[2] = ctp_quote->AskPrice3;
	internal_book->ap_array[3] = ctp_quote->AskPrice4;
	internal_book->ap_array[4] = ctp_quote->AskPrice5;

	internal_book->bv_array[0] = ctp_quote->AskVolume1;
	internal_book->bv_array[1] = ctp_quote->AskVolume2;
	internal_book->bv_array[2] = ctp_quote->AskVolume3;
	internal_book->bv_array[3] = ctp_quote->AskVolume4;
	internal_book->bv_array[4] = ctp_quote->AskVolume5;

	internal_book->av_array[0] = ctp_quote->AskVolume1;
	internal_book->av_array[1] = ctp_quote->AskVolume2;
	internal_book->av_array[2] = ctp_quote->AskVolume3;
	internal_book->av_array[3] = ctp_quote->AskVolume4;
	internal_book->av_array[4] = ctp_quote->AskVolume5;

	internal_book->upper_limit_px = ctp_quote->UpperLimitPrice;
	internal_book->lower_limit_px = ctp_quote->LowerLimitPrice;
	internal_book->close_px = ctp_quote->ClosePrice;
	internal_book->settle_px = ctp_quote->SettlementPrice;

	//internal_book->total_buy_ordsize =
    //internal_book->total_sell_ordsize=
	//internal_book->total_notional=
	//internal_book->total_vol = ctp_quote->

	//internal_book->weighted_buy_px =
	//internal_book->weighted_sell_px=
	//DCE?
	//internal_book->implied_ask_size[0]=
	//internal_book->implied_bid_size[0]=

}
