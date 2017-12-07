#ifndef MI_TYPE_H
#define MI_TYPE_H

#include "c_quote_cfex.h"
#include "c_quote_czce_my.h"
#include "c_quote_dce_my.h"
#include "c_quote_dce_level1.h"
#include "c_quote_ib.h"
#include "c_quote_my.h"
#include "c_quote_my_each_datasource.h"
#include "c_quote_my_level1.h"
#include "c_quote_my_stock.h"
#include "c_quote_shfe_my.h"
#include "c_quote_sungard.h"
#include "c_quote_tap_my.h"
#include "c_quote_sge_deep.h"
#include "c_quote_esunny_foreign.h"


enum MI_TYPE
{
	/* Internal Book (No conversion needed) */
	MI_INTERNAL_BOOK = -1,

	/* CFEX, 'G' */
	MI_CFEX_DEEP = 0,			// 0  cfex_deep_quote

	/* DCE, 'B' */
	MI_DCE_BEST_DEEP,			// 1  dce_my_best_deep_quote
	MI_DCE_MARCH_PRICE,			// 2  dce_my_march_pri_quote
	MI_DCE_ORDER_STATISTIC,		// 3  dce_my_ord_stat_quote
	MI_DCE_REALTIME_PRICE,		// 4  dce_my_rt_pri_quote
	MI_DCE_TEN_ENTRUST,			// 5  dce_my_ten_entr_quote

	/* SHFE, 'A' */
	MI_SHFE_MY,					// 6  shfe_my_quote

	/* CZCE, 'C' */
	MI_CZCE_DEEP_MY,			// 7  czce_my_deep_quote
	MI_CZCE_SNAPSHOT_MY,		// 8  czce_my_snapshot_quote

	/* MY Stock data, from wind */
	MI_MY_STOCK_LV2,			// 9  my_stock_lv2_quote
	MI_MY_STOCK_INDEX,			// 10 my_stock_index_quote

	/* MY quote, so called 'inner quote' */
	MI_MY_MARKET_DATA,			// 11 my_marketdata

	/* MY Level1 quote */
	MI_MY_LEVEL1,				// 12 my_level1_quote

	/* sungard option quote */
	MI_SGD_OPTION,				// 13 sgd_option_quote
	MI_SGD_ORDER_QUEUE,			// 14 sgd_order_queue
	MI_SGD_PER_ENTRUST,			// 15 sgd_per_entrust
	MI_SGD_PER_BARGAIN,			// 16 sgd_per_bargain

	/* MY quote, for stock & IF, another 'inner quote' */
	MI_MY_EACH_DATASRC,			// 17 my_each_datasource

	/* quote structure for A50 */
	MI_TAP_MY,					// 18 tap_my_quote

	/* Combined Book Structure for InteractiveBroker */
	MI_IB_BOOK,					// 19 InteractiveBroker's Book Structure, combined from each level's data.
	MI_IB_QUOTE,			    // 20 InteractiveBroker's Quote Structure

	MI_RAW_INTERNAL_DATA,		// 21 raw internal data
	
	/* shanghai gold exchange */
	MI_SGE_DEEP,				// 22 sge_deep_quote

	/* factor quote */
	MI_MY_STATIC_FACTOR,		// 23 static factor
	MI_MY_DYNAMIC_FACTOR,		// 24 dynamic factor

	/* esunny foreign */
	MI_MY_ESUNNY_FOREIGN,       // 25 c_quote_esunny_foreign.h  my_esunny_frn_quote
	MI_SHFE_MY_LVL3,			// 26 shfe_my_quote, level 3 shfe feed
	MI_DCE_DEEP_ORDS,			// 27 dce_deep_ords
	MI_DCE_LV1,                 // 28 dce HT Level 1
	MI_MY_BAR,                  // 29 Bar Data, only for converting convience. To request data, please use corresponding mi_type.
	MI_SIZE,					/* TODO: Need to change to MI_UNDEFINED_TYPE = -1 along with MyTrader */
};

#endif
