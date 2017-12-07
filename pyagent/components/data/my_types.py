# -*- coding: utf-8 -*-

import numpy as np


class mi_type_t(dict):
    def __init__(self, _data):
        self.data = _data

    def __getattr__(self, attr):
        return self.data[attr]

    def types(self):
        return tuple(self.data.keys())


mi_type = mi_type_t({
    'MI_DCE_DEEP_IB': 101,
    'MI_DCE_ORDER_IB': 103,
    'MI_SHFE_DEEP_IB': 106,
    'MI_CZCE_DEEP_IB': 107,
    'MI_EQUITY_DEEP_IB': 109,
    'MI_EQUITY_INDEX_IB': 110,
    'MI_SHFE_LV1_IB': 112,
    'MI_SHFE_LV3_IB': 126,
    'MI_DCE_DEEP_ORD_IB': 127,
})

fee_dtype = np.dtype([
    ('date', '<i4'),
    ('Exch', 'S8'),
    ('ExchCode', 'S4'),
    ('ProductChi', 'S12'),
    ('ProductPinyin', 'S12'),
    ('Product', 'S8'),
    ('MyProduct', 'S8'),
    ('TickSize', '<f8'),
    ('QuoteUnit', 'S16'),
    ('TradeUnit', '<i4'),
    ('Night', '<i4'),
    ('FeeMode', 'S8'),
    ('CloseFeeMode', 'S12'),
    ('Side', '<i4'),
    ('ExchFee', '<f8'),
    ('ExchDiscount', '<f8'),
    ('ExchFeeAfterDiscount', '<f8'),
    ('BrokerFee', '<f8'),
    ('SideChg', '<i4'),
    ('TradeUnitChg', '<i4'),
    ('SimuFee', '<f8'),
    ('Category', '<i4'),
    ('InternalDate', '<i4')
], align=False)

trading_date_dtype = np.dtype([
    ('TRADE_DT', '<i4'),
    ('EXCHANGE', 'S5'),
    ('NIGHTTDATE', '<i4'),
    ('HAS_NIGHT', '<i8'),
    ('NIGHT_INTERNAL_DATE', '<i4')
], align=False)

main_con_dtype = np.dtype([
    ('date', '<i4'),
    ('NEXTDATE', '<i4'),
    ('MARKET', 'S1'),
    ('EXCHANGE', 'S5'),
    ('SYMBOL', 'S6'),
    ('PRODUCT', 'S2'),
    ('IDENTIFIER', 'S2'),
    ('MAINFLAG', 'S4'),
    ('MAINLEVEL', '<i8'),
    ('UPDATETIME', '<f8')
], align=False)

expire_date_dtype = np.dtype([
    ('S_INFO_WINDCODE', 'S11'),
    ('S_INFO_CODE', 'S7'),
    ('FS_INFO_SCCODE', 'S3'),
    ('FS_INFO_TYPE', '<i4'),
    ('FS_INFO_CCTYPE', '<i4'),
    ('S_INFO_EXCHMARKET', 'S5'),
    ('S_INFO_LISTDATE', '<i4'),
    ('S_INFO_DELISTDATE', '<i4'),
    ('FS_INFO_DLMONTH', '<i4'),
    ('FS_INFO_LPRICE', '<f8'),
    ('FS_INFO_LTDLDATE', '<i4'),
    ('OPDATE', '<f8'),
    ('OPMODE', '<i4'),
    ('date', '<i4')
], align=False)

Ashare_dtype = np.dtype([
    ('SYMBOL', 'S9'),
    ('TRADE_DT', '<i4'),
    ('OBJECT_ID', 'S38'),
    ('CRNCY_CODE', 'S3'),
    ('S_DQ_PRECLOSE', '<f8'),
    ('S_DQ_OPEN', '<f8'),
    ('S_DQ_HIGH', '<f8'),
    ('S_DQ_LOW', '<f8'),
    ('S_DQ_CLOSE', '<f8'),
    ('S_DQ_CHANGE', '<f8'),
    ('S_DQ_PCTCHANGE', '<f8'),
    ('S_DQ_VOLUME', '<f8'),
    ('S_DQ_AMOUNT', '<f8'),
    ('S_DQ_ADJFACTOR', '<f8'),
    ('S_DQ_AVGPRICE', '<f8'),
    ('OPDATE', '<f8'),
    ('OPMODE', 'S1'),
    ('S_DQ_TRADESTATUS', 'S9'),
    ('UPDATETIME', '<f8'),
    ('date', '<i4')
], align=False)

CFutures_dtype = np.dtype([
    ('SYMBOL', 'S6'),
    ('TRADE_DT', '<i4'),
    ('PRESETTLEPRICE', '<f8'),
    ('OPENPRICE', '<f8'),
    ('HIGHPRICE', '<f8'),
    ('LOWPRICE', '<f8'),
    ('CLOSEPRICE', '<f8'),
    ('SETTLEPRICE', '<f8'),
    ('VOLUME', '<f8'),
    ('AMOUNT', '<f8'),
    ('OPENINTEREST', '<f8'),
    ('EXCHANGE', 'S5'),
    ('SYMBOL1', 'S6'),
    ('PRODUCT', 'S2'),
    ('CHG', '<f8'),
    ('date', '<i4'),
    ('PRELASTCLOSE', '<f8')
], align=False)

CGoldSpotEODPrices_dtype = np.dtype([
    ('S_INFO_WINDCODE', 'S12'),
    ('TRADE_DT', '<i4'),
    ('S_DQ_OPEN', '<f8'),
    ('S_DQ_HIGH', '<f8'),
    ('S_DQ_LOW', '<f8'),
    ('S_DQ_CLOSE', '<f8'),
    ('S_DQ_AVGPRICE', '<f8'),
    ('S_DQ_VOLUME', '<f8'),
    ('S_DQ_AMOUNT', '<f8'),
    ('S_DQ_OI', '<f8'),
    ('DEL_AMT', '<f8'),
    ('S_DQ_SETTLE', '<f8'),
    ('DELAY_PAY_TYPECODE', '<f8'),
    ('OPDATE', '<f8'),
    ('OPMODE', 'S1'),
    ('CONSTITUENTCODE', 'S8'),
    ('SYMBOL', 'S8'),
    ('date', '<i4')
], align=False)

EONPrices_dtype = np.dtype([
    ('date', '<i4'),
    ('Symbol', 'S12'),
    ('OpenPrice', '<f8'),
    ('HighestPrice', '<f8'),
    ('LowestPrice', '<f8'),
    ('ClosePrice', '<f8'),
    ('Volume', '<i8'),
    ('Turnover', '<f8'),
    ('OpenInterest', '<f8'),
    ('Exchange', 'S4'),
    ('DataSource', 'S11')
], align=False)

edepth_dtype = np.dtype([
    ('szWindCode', 'S32'),
    ('szCode', 'S32'),
    ('nActionDay', np.int32),
    ('nTradingDay', np.int32),
    ('nTime', np.int32),
    ('nStatus', np.int32),
    ('nPreClose', np.int32),
    ('nOpen', np.int32),
    ('nHigh', np.int32),
    ('nLow', np.int32),
    ('nMatch', np.int32),
    # ('nAskPrice', (np.int32,10)),
    ('nAskPrice1', np.int32),
    ('nAskPrice2', np.int32),
    ('nAskPrice3', np.int32),
    ('nAskPrice4', np.int32),
    ('nAskPrice5', np.int32),
    ('nAskPrice6', np.int32),
    ('nAskPrice7', np.int32),
    ('nAskPrice8', np.int32),
    ('nAskPrice9', np.int32),
    ('nAskPrice10', np.int32),
    # ('nAskVol', (np.int32,10)),
    ('nAskVol1', np.int32),
    ('nAskVol2', np.int32),
    ('nAskVol3', np.int32),
    ('nAskVol4', np.int32),
    ('nAskVol5', np.int32),
    ('nAskVol6', np.int32),
    ('nAskVol7', np.int32),
    ('nAskVol8', np.int32),
    ('nAskVol9', np.int32),
    ('nAskVol10', np.int32),
    # ('nBidPrice', (np.int32,10)),
    ('nBidPrice1', np.int32),
    ('nBidPrice2', np.int32),
    ('nBidPrice3', np.int32),
    ('nBidPrice4', np.int32),
    ('nBidPrice5', np.int32),
    ('nBidPrice6', np.int32),
    ('nBidPrice7', np.int32),
    ('nBidPrice8', np.int32),
    ('nBidPrice9', np.int32),
    ('nBidPrice10', np.int32),
    # ('nBidVol', (np.int32,10)),
    ('nBidVol1', np.int32),
    ('nBidVol2', np.int32),
    ('nBidVol3', np.int32),
    ('nBidVol4', np.int32),
    ('nBidVol5', np.int32),
    ('nBidVol6', np.int32),
    ('nBidVol7', np.int32),
    ('nBidVol8', np.int32),
    ('nBidVol9', np.int32),
    ('nBidVol10', np.int32),
    ('nNumTrades', np.int32),
    ('iVolume', np.int64),
    ('iTurnover', np.int64),
    ('nTotalBidVol', np.int64),
    ('nTotalAskVol', np.int64),
    ('nWeightedAvgBidPrice', np.uint32),
    ('nWeightedAvgAskPrice', np.uint32),
    ('nIOPV', np.int32),
    ('nYieldToMaturity', np.int32),
    ('nHighLimited', np.uint32),
    ('nLowLimited', np.uint32),
    ('chPrefix', 'S4'),
    ('nSyl1', np.int32),
    ('nSyl2', np.int32),
    ('nSD2', np.int32)
], align=True)

ibook_dtype = np.dtype([
    ('contract_id', 'i4'),
    ('symbol', 'S64'),
    ('Wind_equity_code', 'S32'),
    ('exchange', 'i4'),
    ('char_time', 'S10'),
    ('int_time', 'i4'),
    ('feed_status', 'i4'),
    ('pre_settle_px', 'f8'),
    ('last_px', 'f8'),
    ('last_trade_size', 'i4'),
    ('open_interest', 'f8'),
    ('total_vol', 'u8'),
    ('total_notional', 'f8'),
    ('upper_limit_px', 'f8'),
    ('lower_limit_px', 'f8'),
    ('open_px', 'f8'),
    ('high_px', 'f8'),
    ('low_px', 'f8'),
    ('pre_close_px', 'f8'),
    ('close_px', 'f8'),
    ('pre_open_interest', 'f8'),
    ('avg_px', 'f8'),
    ('settle_px', 'f8'),
    ('bp1', 'f8'),
    ('ap1', 'f8'),
    ('bv1', 'i4'),
    ('av1', 'i4'),

    ('bp_array', ('f8', 30)),
    ('ap_array', ('f8', 30)),
    ('bv_array', ('i4', 30)),
    ('av_array', ('i4', 30)),

    ('interval_trade_vol', 'i4'),
    ('interval_notional', 'f8'),
    ('interval_avg_px', 'f8'),
    ('interval_oi_change', 'f8'),
    # SHFE
    ('shfe_data_flag', 'i4'),
    # DCE
    ('implied_bid_size', ('i4', 30)),
    ('implied_ask_size', ('i4', 30)),

    # DCE Statistics Info
    ('total_buy_ordsize', 'i4'),
    ('total_sell_ordsize', 'i4'),
    ('weighted_buy_px', 'f8'),
    ('weighted_sell_px', 'f8'),
    ('life_high', 'f8'),
    ('life_low', 'f8'),
    # Stock Lv2 */
    ('num_trades', 'u4'),
    ('total_bid_vol', 'i8'),
    ('total_ask_vol', 'i8'),
    # mi_type type of the data source - mainly used to identify level 1 quote */),
    ('book_type', 'i4'),
], align=True)

numpy_book_dtype = np.dtype([
    ('serial', 'i4'),
    ('mi_type', 'i4'),
    ('local_time', 'i8'),
    ('exchange_time', 'i8'),
    ('quote', ibook_dtype),
], align=True)

ibook_stock_dtype = np.dtype([
    ('wind_code', 'S32'),
    ('ticker', 'S32'),
    ('action_day', 'i4'),
    ('trading_day', 'i4'),
    ('exch_time', 'i4'),
    ('status', 'i4'),
    ('pre_close_px', 'f4'),
    ('open_px', 'f4'),
    ('high_px', 'f4'),
    ('low_px', 'f4'),
    ('last_px', 'f4'),
    ('ap_array', ('f4', 10)),
    ('av_array', ('u4', 10)),
    ('bp_array', ('f4', 10)),
    ('bv_array', ('u4', 10)),
    ('num_of_trades', 'u4'),
    ('total_vol', 'u8'),
    ('total_notional', 'u8'),
    ('total_bid_vol', 'u8'),
    ('total_ask_vol', 'u8'),
    ('weighted_avg_bp', 'f4'),
    ('weighted_avg_ap', 'f4'),
    ('IOPV', 'f4'),
    ('yield_to_maturity', 'f4'),
    ('upper_limit_px', 'f4'),
    ('lower_limit_px', 'f4'),
    ('prefix', "S4"),
    ('PE1', 'i4'),
    ('PE2', 'i4'),
    ('change', 'i4')
], align=True)

ibook_futures_dtype = np.dtype([
    ('book_type', 'i4'),
    ('symbol', 'S64'),
    ('exchange', 'i2'),
    ('int_time', 'i4'),
    ('pre_close_px', 'f4'),
    ('pre_settle_px', 'f4'),
    ('pre_open_interest', 'f8'),
    ('open_interest', 'f8'),
    ('open_px', 'f4'),
    ('high_px', 'f4'),
    ('low_px', 'f4'),
    ('avg_px', 'f4'),
    ('last_px', 'f4'),
    ('bp_array', ('f4', 5)),
    ('ap_array', ('f4', 5)),
    ('bv_array', ('i4', 5)),
    ('av_array', ('i4', 5)),
    ('total_vol', 'u8'),
    ('total_notional', 'f8'),
    ('upper_limit_px', 'f4'),
    ('lower_limit_px', 'f4'),
    ('close_px', 'f4'),
    ('settle_px', 'f4'),
    ('implied_bid_size', ('i4', 5)),
    ('implied_ask_size', ('i4', 5)),
    ('total_buy_ordsize', 'i4'),
    ('total_sell_ordsize', 'i4'),
    ('weighted_buy_px', 'f4'),
    ('weighted_sell_px', 'f4'),
], align=True)

manual_order_dtype = np.dtype([
    ('order_type', 'i4'),
    ('serial_no', 'i8'),
    ('symbol', 'S32'),
    ('direction', 'i4'),
    ('volume', 'i4'),
    ('price', 'f8'),
    ('price_type', 'i4'),
    ('c_p', 'i4'),
    ('expiry', 'i4'),
    ('strike', 'i4'),
    ('sigma', 'f8'),
    ('mat_next', 'f8'),
    ('mat_by_trade', 'f8'),
    ('unit_price', 'f8'),
    ('delta_coff', 'f8'),
    ('theta', 'f8'),
    ('date', 'i4'),
    ('tdx_date', 'i4'),
    ('exchg', 'S32'),
    ('open_close', 'i4'),
    ('speculator', 'i4'),
], align=True)

hedge_order_dtype = np.dtype([
    ('order_count', 'i4'),
    ('msg_size', 'i4'),
    ('seq_no', 'i8'),
    ('buf', manual_order_dtype),
], align=True)

numpy_book_all_dtype = np.dtype([
    ('serial', 'i4'),
    ('mi_type', 'i4'),
    ('local_time', 'i8'),
    ('exchange_time', 'i8'),
    ('quote', ibook_dtype)
], align=True)

numpy_book_futures_dtype = np.dtype([
    ('serial', 'i4'),
    ('mi_type', 'i4'),
    ('local_time', 'i8'),
    ('exchange_time', 'i8'),
    ('quote', ibook_futures_dtype),
], align=True)

numpy_book_stock_dtype = np.dtype([
    ('serial', 'i4'),
    ('mi_type', 'i4'),
    ('local_time', 'i8'),
    ('exchange_time', 'i8'),
    ('quote', ibook_stock_dtype),
], align=True)

numpy_book_order_dtype = np.dtype([
    ('serial', 'i4'),
    ('mi_type', 'i4'),
    ('local_time', 'i8'),
    ('exchange_time', 'i8'),
    ('quote', hedge_order_dtype),
], align=True)

numpy_send_order_dtype = np.dtype([
    ('exch', 'S1'),
    ('symbol', 'S64'),
    ('volume', 'i4'),
    ('price', 'f8'),
    ('direction', 'i4'),
    ('open_close', 'i4'),
    ('investor_type', 'i4'),
    ('order_type', 'i4'),
    ('time_in_force', 'i4'),
    ('st_id', 'i8'),
    ('order_id', 'i8'),
    ('org_ord_id', 'i8')
])

numpy_response_dtype = np.dtype([
    ('order_id', 'i8'),
    ('symbol', 'S64'),
    ('direction', 'i4'),
    ('open_close', 'i4'),
    ('exe_price', 'f8'),
    ('exe_volume', 'i4'),
    ('status', 'i4'),
    ('error_no', 'i4'),
    ('error_info', 'S512'),
    ('reserved_data', 'i8')
])

SpotWind_dtype = np.dtype([
    ('ITEM', 'S8'),
    ('TRADE_DT', '<i4'),
    ('ITEM_DATA', '<f8'),
    ('date', '<i4')
], align=False)

FeeRateV2_dtype = np.dtype([
    ('Product', 'S7'),
    ('MyProduct', 'S7'),
    ('ExchCode', 'S1'),
    ('FeeMode', '<i4'),
    ('TradeUnit', '<i4'),
    ('MyIntraSimuFee', '<f8'),
    ('TickSize', '<f4'),
    ('MyInterSimuFee', '<f8'),
    ('ExchIntraOpenFee', '<f8'),
    ('ExchIntraCloseFee', '<f8'),
    ('ExchInterCloseFee', '<f8'),
    ('MyBrokerOpenFee', '<f8')
], align=False)

DataFromWindAPI_dtype = np.dtype([
    ('ITEM', 'S8'),
    ('TRADE_DT', '<i4'),
    ('ITEM_DATA', '<f8'),
    ('date', '<i4')
], align=False)

ConStock_dtype = np.dtype([
    ('SYMBOL', 'S16'),
    ('TRADE_DT', '<i4'),
    ('TradeStatus', 'S8'),
    ('INDEXCODE', 'S8'),
    ('INDUSTRYCODE', 'S8'),
    ('Constituent', '<i8'),
    ('ReferenceOpenPrice', '<f4'),
    ('OpenWeight', '<f4'),
    ('SharesInIndex', '<i8'),
    ('closePrice', '<f4'),
    ('CloseWeight', '<f4'),
    ('Divisor', '<f8')
], align=False)
