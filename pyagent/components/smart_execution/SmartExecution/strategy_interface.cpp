#include <stdio.h>
#include "strategy_interface.h"
#include "core/sdp_handler.h"
#include "core/smart_execution.h"
#include "utils/log.h"

static SDPHandler* sdp_handler = NULL;
static SmartExecution* smart_execution = NULL;

int my_st_init(int type, int length, void * cfg)
{
	if (sdp_handler == NULL && smart_execution == NULL) {
		PRINT_INFO("Welcome to VWAP Module!");
		sdp_handler = new SDPHandler(type, length, cfg);
		smart_execution = new SmartExecution(sdp_handler, type, length, cfg);
		return 0;
	}
	return -1;
}

int my_on_book(int type, int length, void * book)
{
	if (sdp_handler != NULL && smart_execution != NULL) {
		sdp_handler->on_book(type, length, book);
		smart_execution->on_book(type, length, book);
		return 0;
	}
	return -1;
}

int my_on_response(int type, int length, void * resp)
{
	if (sdp_handler != NULL && smart_execution != NULL) {
		sdp_handler->on_response(type, length, resp);
		smart_execution->on_response(type, length, resp);

		st_response_t* l_resp = (st_response_t*)((st_data_t*)resp)->info;
		Order* l_order = sdp_handler->m_orders->query_order(l_resp->order_id);
		sdp_handler->m_orders->update_order_list(l_order);
		return 0;
	}
	return -1;
}

int my_on_timer(int type, int length, void * info)
{
	return 0;
}

void my_destroy()
{
	if (sdp_handler != NULL) {
		delete sdp_handler;
		sdp_handler = NULL;
	}
	if (smart_execution != NULL) {
		delete smart_execution;
		smart_execution = NULL;
	}
}
