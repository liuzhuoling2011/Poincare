import numpy as np


match_para_dtype=np.dtype([
   ('mode', 'i4'),
   ('exch', 'B'),
   ('product', 'S64'),
   ('param_num', 'i4'),
   ('trade_ratio', 'f8'),
   ('params', ('f8', 64)),
], align=True)

product_info_dtype=np.dtype([
   ('product', 'S64'),
   ('my_product', 'S64'),
   ('exch', 'B'),
   ('fixed', 'i4'),
   ('rate', 'f8'),               # simu fee
   ('yes_rate', 'f8'),           # yesterday simu fee
   ('unit', 'f8'),               # small unit of change
   ('type', 'i4'),
   ('multiple', 'i4'),           # tradeing of the samll number
   ('order_max_vol', 'i4'),      # max volume in a single order
   ('order_cum_open_lmt', 'i4'), # max volume allowed to open in single day
], align=True)

me_config_dtype=np.dtype([
    ('default_mt', 'i4'),
    ('pro_cnt', 'i4'),
    ('pro_cfg', (product_info_dtype, 2048)),
    ('par_cnt', 'i4'),
    ('par_cfg', (match_para_dtype, 2048)),
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

