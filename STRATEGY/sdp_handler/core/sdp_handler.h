#ifndef _CHECK_HANDLER_H_
#define _CHECK_HANDLER_H_

#include "base_define.h"
#include "utils/order_hash.h"
#include "utils/utils.h"
#include "utils/MyHash.h"
#include "strategy_interface.h"
#include "quote_format_define.h"

#define MAX_ORDER_COUNT      256

class SDPHandler
{
public:
	SDPHandler(int type, int length, void * cfg); // Should be called in my_st_init
	~SDPHandler(); // Should be called in my_destroy
	int on_book(int type, int length, void * book);	// Should be called in my_on_book
	int on_response(int type, int length, void * resp);	// Should be called in my_on_response
	int on_timer(int type, int length, void * info){ return 0; } // Should be called in my_on_timer

	// Strategy can use these function below

	/**
	* Send order info to trader.
	* When your order size is too large, we will split it into some small orders
	*
	* @param instr			Contract you want to trade
	* @param exch			Enums of exchange's name
	* @param price			Price of order
	* @param size			Size of order
	* @param side			Side of order, eg. ORDER_BUY, ORDER_SELL
	* @param sig_openclose				eg. ORDER_OPEN, ORDER_CLOSE
	* @param flag_syn_cancel			Flag of whether cancel a order with synchronization
	* @param flag_close_yesterday_pos	Flag of close yesterday orders
	* @param investor_type		eg. ORDER_SPECULATOR, ORDER_HEDGER, if you don't know its meaning, just keep default value
	* @param order_type			eg. ORDER_TYPE_LIMIT, ORDER_TYPE_MARKET, if you don't know its meaning, just keep default value
	* @param time_in_force		eg. ORDER_TIF_DAY, ORDER_TIF_FAK, if you don't know its meaning, just keep default value
	* return			0		-- Success.
	*                   else	-- Failed.
	*/
	int send_single_order(Contract *instr, EXCHANGE exch,
		double price, int size, DIRECTION side,
		OPEN_CLOSE sig_openclose = ORDER_OPEN, bool flag_close_yesterday_pos = false, bool flag_syn_cancel = false,
		INVESTOR_TYPE investor_type = ORDER_SPECULATOR, ORDER_TYPE order_type = ORDER_TYPE_LIMIT, TIME_IN_FORCE time_in_force = ORDER_TIF_DAY);
	
	/**
	* Smart send/cancel orders to get your target position.
	* This function will consider the pending positions and yesterday positions
	*
	* @param instr				Contract you want to trade
	* @param side				Side of order, eg. SIG_DIRECTION_BUY, SIG_DIRECTION_SELL
	* @param size				Size of order
	* @param price				Price of order
	* @param flag_syn_cancel	Flag of whether use synchronization
	* return			0		-- Success.
	*                   else	-- Failed.
	*/
	int send_dma_order(Contract *instr, DIRECTION side,
		int size, double price, bool flag_syn_cancel = true);

	/**
	* Using Twap/Vwap algorithm to sent order with target size at certain times.
	*
	* @param instr				Contract you want to trade
	* @param side				Side of order, eg. SIG_DIRECTION_BUY, SIG_DIRECTION_SELL
	* @param size				Size of order
	* @param price				Price of order
	* @param start_time			Start time to trade
	* @param end_time			End time to trade
	* return			0		-- Success.
	*                   else	-- Failed.
	*/
	int send_twap_order(Contract *instr, int size, DIRECTION side, double price,
		int start_time, int end_time);
	int send_vwap_order(Contract *instr, int size, DIRECTION side, double price,
		int start_time, int end_time);

	/**
	* Cancel pending order functions.
	* These functions will cancel order with different situation
	*
	* @param order			A pointer to Order
	* @param instr			Contract you want to trade
	* @param side			Side of order, eg. SIG_DIRECTION_BUY, SIG_DIRECTION_SELL
	* @param sig_openclose	eg. SIG_ACTION_OPEN, SIG_ACTION_CLOSE
	* @param price			Price of order
	* return				0		-- Success.
	*						else	-- Failed.
	*/
	int cancel_single_order(Order* order);
	int cancel_orders_by_side(Contract *instr, DIRECTION side);
	int cancel_orders_by_side(Contract *instr, DIRECTION side, OPEN_CLOSE flag);
	int cancel_orders_with_dif_px(Contract *instr, DIRECTION side, double price);
	int cancel_orders_with_dif_px(Contract *instr, DIRECTION side, double price, int tolerance, int &pending_open_size, int &pending_close_size, int& pending_close_yes_size);
	int cancel_all_orders();
	int cancel_all_orders(Contract *instr);

	void cancel_old_order(int tick_time);

	//平掉所有的仓位
	int close_all_position();
	//智能操作达到目标仓位
	void long_short(Contract * instr, int desired_pos, double ap, double bp, int tolerance);

	Contract * find_contract(char* symbol);
	int get_pos_by_side(Contract *a_instr, DIRECTION side);
	double get_account_cash(char* account);

	double get_realized_pnl(Contract *cont);
	double get_unrealized_pnl(Contract *cont); 
	double get_contract_pnl(Contract *cont);
	double get_contract_pnl_cash(Contract *cont);
	double get_strategy_pnl_cash();

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
