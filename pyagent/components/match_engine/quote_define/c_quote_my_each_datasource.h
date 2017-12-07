#ifndef C_QUOTE_MY_EACH_DATASOURCE_H
#define C_QUOTE_MY_EACH_DATASOURCE_H

struct my_each_datasource
{
	long time;
	char symbol[64];
	char item[64];
	double value;
};

#define MAX_INNER_LEN  16384 
/* 16384 byte = 16KB */
#define MAX_INNER_SYM_LEN 4

struct inner_quote_fmt {
	int msg_len;
	char inner_symbol[MAX_INNER_SYM_LEN]; /* Used in MyTrader, not used in Strategy */
	char inner_quote_addr[MAX_INNER_LEN];
};

#endif