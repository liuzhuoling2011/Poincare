#ifndef _BASE_DEFINE_H_
#define _BASE_DEFINE_H_

#include <stdint.h>
#include "utils/list.h"

typedef struct {
	int         fee_by_lot;                             /* True - Fee by Lot, False - Fee by Notional */
	double      open_fee;
	double      close_fee;
	double      yes_close_fee;
	int         multiple;                               /* A constant set by an exchange, which when multiplied by the futures price gives the dollar value of a stock index futures contract */
} fee_t;

typedef struct {
	int         long_volume;
	double      long_price;
	int         short_volume;
	double      short_price;
} position_t;

typedef struct {
	char        symbol[64];                             /* 合约名称 */
	char        exch[9];                                /* exchange */
	position_t  yesterday_pos;                          /* 昨日持仓 */
	position_t  today_pos;                              /* 今日持仓 */
} contract_t;

#endif
