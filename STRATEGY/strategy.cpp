#include <string.h>
#include <stdio.h>

#include "strategy_interface.h"
#include "quote_format_define.h"


int
my_st_init(int type, int length, void *cfg)
{
	printf("strategy my_st_init\n");
	return 0;
}

int
my_on_book(int type, int length, void *book)
{
	printf("strategy my_on_book\n");

	switch (type) {
	case DEFAULT_STOCK_QUOTE:
		printf("pass on_book stock\n");
		break;
	case DEFAULT_FUTURE_QUOTE:
		printf("pass on_book future\n");
		break;
	}
	return 0;
}

int
my_on_response(int type, int length, void *resp)
{
	printf("strategy my_on_response\n");
	return 0;
}

int
my_on_timer(int type, int length, void *resp)
{
	printf("strategy my_on_timer\n");
	return 0;
}

void
my_destroy()
{
	printf("strategy my_destroy\n");
}
