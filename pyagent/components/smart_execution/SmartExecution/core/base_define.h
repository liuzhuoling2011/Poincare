#ifndef _BASE_DEFINE_H_
#define _BASE_DEFINE_H_

#include <stdint.h>
#include "utils/list.h"
#include "strategy_interface.h"

enum ORDER_ALGORITHM
{
	ALGORITHM_NORMAL,
	ALGORITHM_DMA,
	ALGORITHM_TVWAP
};

struct Position {
	int     qty;
	double  notional;   /* A cost of current position, including fees in points */
};

enum POSITION_FLAG {
	LONG_OPEN = 0,
	LONG_CLOSE = 1,
	SHORT_OPEN = 2,
	SHORT_CLOSE = 3
};

struct Contract {
	char      account[ACCOUNT_LEN];    // Related Account, used to calculate cash.
	char      symbol[SYMBOL_LEN];
	int       expiration_date;        /* 20161020 */
	EXCHANGE  exch;

	int       init_today_long_qty;   /* Remaining Today's long position, including yesterday's pos */
	int       init_today_short_qty;	 /* Remaining Today's short position, including yesterday's pos */
	int       pre_long_qty_remain;   /* Remaining pre long position */
	int       pre_short_qty_remain;  /* Remaining pre short position */
	Position  positions[4];          // POSITION_FLAG, 1-4 LONG_OPEN = 1,LONG_CLOSE = 2,SHORT_OPEN = 3,SHORT_CLOSE = 4

	/* Risk Related */
	int       max_pos;
	int       max_accum_open_vol;     /* �������涨�����˫���ۼƿ������ޣ��������� rb 7700, Ŀǰͼ��ᴫ�룬��SDP �����. Read from rss config in simulation */
	int       max_cancel_limit;
	int       single_side_max_pos;    /* ��󵥱߲�λ���ޣ��������ֹ��� */
	int       order_cum_open_vol;

	bool      fee_by_lot;	          // True - Fee by Lot, False - Fee by Notional
	double    exchange_fee;
	double    yes_exchange_fee;
	double    broker_fee;
	double    stamp_tax;
	double    acc_transfer_fee;      // Account Transfer Fee - only for SSE
	double    tick_size;
	double    multiplier;

	int       bv1;
	int       av1;
	double    bp1;
	double    ap1;
	double    upper_limit;
	double    lower_limit;
	int       pending_buy_open_size;
	int       pending_sell_open_size;
	int       pending_buy_close_size;
	int       pending_sell_close_size;
	int       order_no_entrusted_num;
	int       pre_data_time;
	int       m_order_no_entrusted_num;
};

struct Order {
	uint64_t signal_id;
	EXCHANGE exch;
	Contract *contr;
	double price;
	int volume;
	DIRECTION side;
	OPEN_CLOSE openclose;
	INVESTOR_TYPE invest_type;
	ORDER_TYPE ord_type;
	TIME_IN_FORCE time_in_force;
	char symbol[SYMBOL_LEN];

	ORDER_STATUS status;

	double last_px;
	int last_qty;
	int cum_qty;
	double cum_amount;

	long orig_cl_ord_id;
	long orig_ord_id;

	bool pending_cancel;
	ORDER_ALGORITHM algorithm;

	list_t hs_link; // hash link
	list_t pd_link; // pending link
	list_t fr_link; // free unused link
	list_t search_link; // used to place in the search list
};

#endif
