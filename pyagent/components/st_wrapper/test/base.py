#!/bin/python3.5


import numpy as np


lf_position = np.dtype([
    ('exchg_code', 'B'),
    ('long_volume', 'i4'),
    ('long_price', 'f8'),
    ('short_volume', 'i4'),
    ('short_price', 'f8'),
], align=True)

lf_contract = np.dtype([
    ('symbol', 'S64'),
    ('max_pos', 'i4'),
    ('exch', 'i4'),
    ('single_side_max_pos', 'i4'),
    ('max_accum_open_vol', 'i4'),
    ('expiration_date', 'i4'),
    ('yesterday_pos', lf_position),
    ('today_pos', lf_position),
    ('account', 'S64'),
    ('future_int_param1', 'i4'),
    ('future_int_param2', 'i4'),
    ('future_int_param3', 'i4'),
    ('future_int_param4', 'i4'),
    ('future_int_param5', 'i4'),
    ('future_uint64_param1', 'i8'),
    ('future_uint64_param2', 'i8'),
    ('future_double_param1', 'f8'),
    ('future_double_param2', 'f8'),
    ('future_double_param3', 'f8'),
    ('future_double_param4', 'f8'),
    ('future_double_param5', 'f8'),
    ('future_char64_param1', 'S64'),
    ('future_char64_param2', 'S64'),
    ('future_char64_param3', 'S64'),
    ('future_char64_param4', 'S64'),
    ('future_char64_param5', 'S64')
], align=True)

lf_account = np.dtype([
    ('account', 'S64'),
    ('currency', 'i4'),
    ('cash', 'f8'),
], align=True)

lf_strategy_config_dtype = np.dtype([
    ('accounts', lf_account, (1, 64)),
    ('contracts', lf_contract, (1, 4096)),
    ('param_file_path', 'S2048'),
    ('file_path', 'S2048'),
    ('st_log', 'i8'),
    ('st_id', 'i8'),
    ('v_st_id', 'i8'),
    ('strat_name', 'S256'),
    ('future_int_param1', 'i4'),
    ('future_int_param2', 'i4'),
    ('future_int_param3', 'i4'),
    ('future_int_param4', 'i4'),
    ('future_int_param5', 'i4'),
    ('future_uint64_param1', 'i8'),
    ('future_uint64_param2', 'i8'),
    ('future_double_param1', 'f8'),
    ('future_double_param2', 'f8'),
    ('future_double_param3', 'f8'),
    ('future_double_param4', 'f8'),
    ('future_double_param5', 'f8'),
    ('future_char64_param1', 'S64'),
    ('future_char64_param2', 'S64'),
    ('future_char64_param3', 'S64'),
    ('future_char64_param4', 'S64'),
    ('future_char64_param5', 'S64'),
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
