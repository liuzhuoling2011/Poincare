#include <stdio.h>
#include "strategy_interface.h"
#include "core/check_handler.h"
#include "utils/log.h"

static CheckHandler* check_handler = NULL;

int my_st_init(int type, int length, void * cfg)
{
	if (check_handler == NULL) {
		PRINT_INFO("Welcome to Check Module!");
		check_handler = new CheckHandler(type, length, cfg);
	}
	return 0;
}

int my_on_book(int type, int length, void * book)
{
	if (check_handler != NULL) {
		return check_handler->on_book(type, length, book);
	}
	PRINT_INFO("on_book check_handler not exist!");
	return -1;
}

int my_on_response(int type, int length, void * resp)
{
	if (check_handler != NULL) {
		return check_handler->on_response(type, length, resp);
	}
	PRINT_INFO("on_response check_handler not exist!");
	return -1;
}

int my_on_timer(int type, int length, void * info)
{
	if (check_handler != NULL) {
		return check_handler->on_timer(type, length, info);
	}
	PRINT_INFO("on_timer check_handler not exist!");
	return -1;
}

void my_destroy()
{
	if (check_handler != NULL) {
		delete check_handler;
		check_handler = NULL;
	}
}
