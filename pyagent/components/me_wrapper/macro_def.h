#ifndef MACRO_DEF_H
#define MACRO_DEF_H

/*Limits*/
#define MAX_SIGNAL_SIZE      1000
#define MAX_CAN_TRADE_SIZE   64
#define MAX_ORDER_COUNT      256
#define MAX_COUNT_NUM        64
#define MAX_CURRENCY_NUM     64
#define MAX_NAME_SIZE        64
/*Contract related*/
#define MAX_FEED_TYPE_SIZE   8
//for Hash
#define HASH_TABLE_SIZE	     (8192)
#define MAX_ITEM_SIZE	     (4096)
#define MAX_DIR_PATH_SIZE    (2048)
#define SHANNON_SYMBOL_LEN   (64)
#define SHANNON_ACCOUNT_LEN  (64)
#define SHANNON_SYMBOL_MAX   (4096)
#define SHANNON_ACCOUNT_MAX  (64)
#define SHANNON_STRAT_NAME_LEN (64)

#define PRINT_ERROR   printf
#define PRINT_WARN    printf

#define PRICE_FACTOR_STOCK (10000)     /* Prices in stock Lv2 are 10000 times the real value */
#define NOTIONAL_FACTOR_INDEX 100      /* Notional in stock index are 100 times the real value */
#define DEFAULT_FUTURES_ORDER_MAX_VOL 290      /* Default maximum volume per order for futures */
#define PAY_UP_TICKS 3
#define MAX_SYMBOL_SIZE        64

#define BROKER_FEE 0.0002
#define STAMP_TAX 0.001
#define ACC_TRANSFER_FEE 0.00002

#endif // MACRO_DEF_H