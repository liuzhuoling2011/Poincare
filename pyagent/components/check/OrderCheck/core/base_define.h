#ifndef _BASE_DEFINE_H_
#define _BASE_DEFINE_H_

#include <stdint.h>
#include "utils/list.h"
#include "strategy_interface.h"

#define TURNOVER_DIRECTION(dir) ((dir)^1)

struct hash_bucket {
	list_t head;
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

	int       pre_long_qty_remain;   /* Remaining pre long position */
	int       pre_short_qty_remain;  /* Remaining pre short position */
	int       pre_long_position;
	int       pre_short_position;
	Position  positions[4];          // POSITION_FLAG, 1-4 LONG_OPEN = 1,LONG_CLOSE = 2,SHORT_OPEN = 3,SHORT_CLOSE = 4

	/* Risk Related */
	int       max_pos;
	int       max_accum_open_vol;     /* �������涨�����˫���ۼƿ������ޣ��������� rb 7700, Ŀǰͼ��ᴫ�룬��SDP �����. Read from rss config in simulation */
	int       max_cancel_limit;       /* max cancel order limit number */
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
};

struct Order {
	uint64_t signal_id;
	EXCHANGE exch;
	Contract *contr;
	double price;
	int volume;
	DIRECTION side;
	OPEN_CLOSE openclose;
	char symbol[SYMBOL_LEN];

	ORDER_STATUS status;

	double last_px;
	int last_qty;
	int cum_qty;
	double cum_amount;

	long orig_cl_ord_id;
	long orig_ord_id;

	bool pending_cancel;

	list_t hs_link; // hash link
	list_t pd_link; // pending link
	list_t fr_link; // free unused link
	list_t search_link; // used to place in the search list
};


#endif
