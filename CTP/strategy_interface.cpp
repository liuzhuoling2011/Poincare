#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include "strategy_interface.h"

const char strategy_path[] = "strategy.so";

typedef int(*st_data_func_t)(int type, int length, void *data);
typedef int(*st_none_func_t)();

void          *strategy_handler;
st_data_func_t st_init;
st_data_func_t on_book;
st_data_func_t on_response;
st_data_func_t on_timer;
st_none_func_t st_destroy;

void load_strategy() {
	strategy_handler = dlopen(strategy_path, RTLD_LAZY);
	if (strategy_handler == NULL) {
		char err_msg[256];
		snprintf(err_msg, sizeof(err_msg), "Strategy dlopen failed: %s, %s", strategy_path, dlerror());
	}
	st_init = (st_data_func_t)dlsym(strategy_handler, "my_st_init");
	on_book = (st_data_func_t)dlsym(strategy_handler, "my_on_book");
	on_response = (st_data_func_t)dlsym(strategy_handler, "my_on_response");
	on_timer = (st_data_func_t)dlsym(strategy_handler, "my_on_timer");
	st_destroy = (st_none_func_t)dlsym(strategy_handler, "my_destroy");
}

int my_st_init(int type, int length, void *cfg) {
	load_strategy();
	return st_init(type, length, cfg);
}

int my_on_book(int type, int length, void *book) {
	return on_book(type, length, book);
}

int my_on_response(int type, int length, void *resp) {
	return on_response(type, length, resp);
}

int my_on_timer(int type, int length, void *info) {
	
	return on_timer(type, length, info);
}

void my_destroy() {
	st_destroy();
}