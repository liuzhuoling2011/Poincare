#!/bin/python3.5


import numpy as np


st_position = np.dtype([
    ('long_volume', 'i4'),
    ('long_price', 'f8'),
    ('short_volume', 'i4'),
    ('short_price', 'f8'),
], align=True)

st_fee = np.dtype([
    ('fee_by_lot', 'B'),
    ('exchange_fee', 'f8'),
    ('yes_exchange_fee', 'f8'),
    ('broker_fee', 'f8'),
    ('stamp_tax', 'f8'),
    ('acc_transfer_fee', 'f8')
], align=True)

st_contract = np.dtype([
    ('symbol', 'S64'),
    ('exch', 'B'),
    ('max_accum_open_vol', 'i4'),
    ('max_cancel_limit', 'i4'),
    ('expiration_date', 'i4'),
    ('tick_size', 'f8'),
    ('multiple', 'f8'),
    ('account', 'S64'),
    ('yesterday_pos', st_position),
    ('today_pos', st_position),
    ('fee', st_fee),
    ('reserved_data', 'i8'),
], align=True)

st_account = np.dtype([
    ('account', 'S64'),
    ('currency', 'i4'),
    ('exch_rate', 'f8'),
    ('cash_asset', 'f8'),
    ('cash_available', 'f8'),
], align=True)

st_config_dtype = np.dtype([
    ('proc_order_hdl', 'i8'),
    ('send_info_hdl', 'i8'),
    ('pass_rsp_hdl', 'i8'),

    ('trading_date', 'i4'),
    ('day_night', 'i4'),
    ('accounts', (st_account, 64)),
    ('contracts', (st_contract, 4096)),
    ('param_file_path', 'S2048'),
    ('output_file_path', 'S2048'),
    ('vst_id', 'i8'),
    ('strat_name', 'S256'),
    ('reserved_data', 'i8'),
], align=True)

ibook_dtype= np.dtype([
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
    ('upper_limit_px','f8'),
    ('lower_limit_px','f8'),
    ('open_px','f8'),
    ('high_px','f8'),
    ('low_px','f8'),
    ('pre_close_px','f8'),
    ('close_px', 'f8', 'f8'),
    ('pre_open_interest','f8'),
    ('avg_px','f8'),
    ('settle_px','f8'),
    ('bp1', 'f8'),
    ('ap1', 'f8'),
    ('bv1', 'i4'),
    ('av1', 'i4'),

    ('bp_array', ('f8',30)),
    ('ap_array', ('f8',30)),
    ('bv_array', ('i4',30)),
    ('av_array', ('i4',30)),

    ('interval_trade_vol','i4' ),
    ('interval_notional', 'f8'),
    ('interval_avg_px', 'f8'),
    ('interval_oi_change', 'f8'),
    # SHFE
    ('shfe_data_flag','i4'),
    # DCE
    ('implied_bid_size', ('i4',30)),
    ('implied_ask_size', ('i4',30)),

    # DCE Statistics Info
    ('total_buy_ordsize', 'i4'),
    ('total_sell_ordsize', 'i4'),
    ('weighted_buy_px','f8'),
    ('weighted_sell_px','f8'),
    ('life_high','f8'),
    ('life_low','f8'),
    # Stock Lv2 */
    ('num_trades', 'u4'),
    ('total_bid_vol', 'i8'),
    ('total_ask_vol', 'i8'),
    # mi_type type of the data source - mainly used to identify level 1 quote */),
    ('book_type', 'i4'),
],align=True)

numpy_book_dtype= np.dtype([
    ('serial', 'i4'),
    ('mi_type', 'i4'),
    ('local_time', 'i8'),
    ('exchange_time', 'i8'),
    ('quote', ibook_dtype)
], align=True)
