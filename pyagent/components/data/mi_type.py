# -*- coding: utf-8 -*-


from enum import Enum

MI_NEW_BOOK_BASE = 200
MI_INTERNAL_BOOK_BASE = 100

class MI_TYPE(Enum):
    MI_INTERNAL_BOOK = -1           # -1 Internal Book (No conversion needed now)
    MI_CFEX_DEEP = 0                # 0
    MI_DCE_BEST_DEEP = 1            # 1  original dce_my_best_deep_quote
    MI_DCE_MARCH_PRICE = 2          # 2  original data dce_my_march_pri_quote
    MI_DCE_ORDER_STATISTIC = 3      # 3  original data dce_my_ord_stat_quote
    MI_DCE_REALTIME_PRICE = 4       # 4  original data dce_my_rt_pri_quote
    MI_DCE_TEN_ENTRUST = 5          # 5  original data dce_my_ten_entr_quote
    MI_SHFE_MY = 6                  # 6  original data shfe_my_quote
    MI_CZCE_DEEP_MY = 7             # 7  original data czce_my_deep_quote /* CZCE, 'C' */
    MI_CZCE_SNAPSHOT_MY = 8         # 8  original data czce_my_snapshot_quote
    MI_MY_STOCK_LV2 = 9             # 9  original data my_stock_lv2_quote
    MI_MY_STOCK_INDEX = 10          # 10 original data my_stock_index_quote
    MI_MY_MARKET_DATA = 11          # 11 original data my_marketdata /* MY quote, so called 'inner quote' */
    MI_MY_LEVEL1 = 12               # 12 original data my_level1_quote /* MY Level1 quote */
    MI_SGD_OPTION = 13              # 13 original data sgd_option_quote /* sungard option quote */
    MI_SGD_ORDER_QUEUE = 14         # 14 original data sgd_order_queue
    MI_SGD_PER_ENTRUST = 15         # 15 original data sgd_per_entrust
    MI_SGD_PER_BARGAIN = 16         # 16 original data sgd_per_bargain
    MI_MY_EACH_DATASRC = 17         # 17 original data my_each_datasource
    MI_TAP_MY = 18                  # 18 original data tap_my_quote
    MI_IB_BOOK = 19                 # 19 original data InteractiveBroker's Book Structure, combined from each level's data.
    MI_IB_QUOTE = 20                # 20 original data InteractiveBroker's Quote Structure
    MI_INNER_STOCK_QUOTE = 21       # 21 original data Strategy generate inner stock quote
    MI_SGE_DEEP = 22                # 22 original data sge_deep_quote, shanghai gold exchange
    MI_MY_STATIC_FACTOR = 23        # 23 original data static factor, factor quote
    MI_MY_DYNAMIC_FACTOR = 24       # 24 original data dynamic factor
    MI_MY_ESUNNY_FOREIGN = 25       # 25 original data c_quote_esunny_foreign.h  my_esunny_frn_quote esunny foreign
    MI_SHFE_MY_LV3 = 26             # 26 original data shfe_my_quote, level 3 shfe feed
    MI_DCE_DEEP_ORDS = 27           # 27 original data union data Dce quote & Order status

    MI_ALL_FUTURE = 50              # 50 all future depth data

    def __init__(self, mi):
        self.mi = mi

    @property
    def new_book(self):
        return self.mi + MI_NEW_BOOK_BASE

    @property
    def internal_book(self):
        return self.mi + MI_INTERNAL_BOOK_BASE
