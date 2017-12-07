#ifndef _CHECK_HANDLER_H_
#define _CHECK_HANDLER_H_

#include "utils/order_hash.h"
#include "utils/utils.h"
#include "utils/MyHash.h"
#include "strategy_interface.h"
#include "quote_format_define.h"

#define MAX_ORDER_COUNT      256

class SDPHandler
{
public:
	SDPHandler(int type, int length, void * cfg);
	~SDPHandler();
	int on_book(int type, int length, void * book);
	int on_response(int type, int length, void * resp);
	int on_timer(int type, int length, void * info){ return 0; }

	int send_single_order(ORDER_ALGORITHM algorithm, Contract *instr, EXCHANGE exch,
		double price, int size, DIRECTION side,
		OPEN_CLOSE sig_openclose = ORDER_OPEN, bool flag_syn_cancel = false, bool flag_close_yesterday_pos = false,
		INVESTOR_TYPE investor_type = ORDER_SPECULATOR, ORDER_TYPE order_type = ORDER_TYPE_LIMIT, TIME_IN_FORCE time_in_force = ORDER_TIF_DAY);
	int cancel_single_order(Order* l_order);
	int cancel_orders_by_side(Contract *instr, DIRECTION side);
	int cancel_orders_by_side(Contract *instr, DIRECTION side, OPEN_CLOSE flag);
	int cancel_orders_with_dif_px(Contract *instr, DIRECTION side, double price, int &pending_open_size, int &pending_close_size, int& pending_close_yes_size);
	int cancel_all_orders();
	int cancel_all_orders(Contract *instr);
	void process_exit_mode();
	Contract * find_contract(char* symbol);

	/* Process Strategy Singal based, cancel orders on the other side, or the same side with dif px. Send new orders to reach desired size */
	bool check_process_strat_singal_inputs(Contract *a_contract, DIRECTION side, int desired_size);
	bool process_strat_signal(Contract *l_instr, DIRECTION side,
		int size, double price, bool flag_syn_cancel = true);
	void process_strat_signal_with_lock_pos(Contract * l_instr, DIRECTION a_side, int desired_size,
		double a_price, bool flag_syn_cancel);
	void process_strat_signal_without_lock_pos(Contract * l_instr, DIRECTION a_side, int desired_size,
		double a_price, bool flag_syn_cancel);

	/* Process Strategy Singal based, cancel orders on the other side, or the same side with dif px. Send new orders to reach desired size */
	void send_orders_with_lockpos(Contract * a_instr, DIRECTION a_side, int a_desired_size,
		double a_price, int pending_open_size, int pending_close_size, int pending_close_yes_size,
		int total_pending_buy_close_size, int total_pending_sell_close_size, bool flag_syn_cancel);
	void send_orders_without_lockpos(Contract * a_instr, DIRECTION a_side, int a_desired_size,
		double a_price, int pending_open_size, int pending_close_size, int total_pending_buy_close_size,
		int total_pending_sell_close_size, bool flag_syn_cancel);
	void asynch_cancel_closing_order(Contract * a_instr, DIRECTION a_side, double a_price, int a_need_to_open,
		int a_need_to_close, int &a_available_for_close, int &a_pending_open_size, int a_remained_opposite_pos,
		bool a_flag_too_much_pending_close, bool flag_syn_cancel);
	void send_open_orders_consider_pending_open(Contract * a_instr, DIRECTION a_side,
		double a_price, int a_need_to_open, int a_pending_open_size, bool flag_syn_cancel);
	void send_open_orders_instead_of_close(Contract * a_instr, DIRECTION a_side, double a_price, int a_need_to_open,
		int a_need_to_close, int &a_available_for_close, int &a_pending_open_size, int a_remained_opposite_yes_pos,
		bool a_flag_too_much_pending_close, bool flag_syn_cancel);
	void send_available_close_orders(Contract * a_instr, DIRECTION a_side, double a_price, int a_need_to_open,
		int a_need_to_close, int &a_available_for_close, int &a_pending_open_size, int a_remained_opposite_yes_pos,
		bool a_flag_too_much_pending_close, bool flag_syn_cancel);


private:
	void update_yes_pos(Contract *instr, DIRECTION side, int size);

	bool process_order_resp(st_response_t* resp);
	void update_account_cash(Order* ord_resp);
	void update_pending_status(Order* a_order);
	bool update_contracts(Stock_Internal_Book* a_book);
	bool update_contracts(Futures_Internal_Book* a_book);
	bool update_contracts(Internal_Bar* a_bar);

public:
	OrderHash *m_orders = NULL;
	MyHash<account_t> m_accounts;
	MyHash<Contract> *m_contracts = NULL;
	send_data m_send_resp_func = NULL;
	send_data m_send_order_func = NULL;
	send_data m_send_info_func = NULL;

	/* Record orders' ids and sizes in send_single_order */
	uint64_t m_cur_ord_id_arr[MAX_ORDER_COUNT];
	int m_cur_ord_size_arr[MAX_ORDER_COUNT];
private:
	int m_order_count = 0;
	int m_strat_id = 0;
	st_config_t *m_config;
	bool m_is_use_fee = true;

	bool m_use_lock_pos = false;
	bool is_exit_mode = false;
	bool exit_mode_orders_sent = false;
	int m_new_order_times = 0;
	int m_cancelled_times = 0;
	int m_total_order_size = 0;		// Accumulated order size.
	int m_total_cancel_size = 0;	// Accumulated order cancel size.
};

#endif
