#ifndef _CHECK_HANDLER_H_
#define _CHECK_HANDLER_H_

#include "strategy_interface.h"
#include "utils/order_hash.h"
#include "utils/utils.h"
#include "utils/MyHash.h"

class CheckHandler
{
public:
	CheckHandler(int type, int length, void * cfg);
	~CheckHandler();
	int on_book(int type, int length, void * book);
	int on_response(int type, int length, void * resp);
	int on_timer(int type, int length, void * info){ return 0; }

private:
	int process_send_order(st_data_t* book);
	int process_cancel_order(st_data_t* book);
	bool process_order_resp(st_response_t* resp);
	void update_account_cash(Order* ord_resp);
	void update_pending_cancel_status(Order* a_order);
	void print_failed_order_info(int check_code);
	int compliance_check(char exch, char *symbol, int volume, double price, int direction, int open_close_flag);
	void update_yes_pos(Contract * instr, DIRECTION side, int size);

private:
	OrderHash *m_orders = NULL;
	MyHash<account_t> m_accounts;
	MyHash<Contract> *m_contracts = NULL;
	send_data m_send_resp_func = NULL;
	send_data m_send_order_func = NULL;	
	send_data m_send_info_func = NULL;
	st_config_t m_config;
	bool m_is_use_fee = true;
};

#endif
