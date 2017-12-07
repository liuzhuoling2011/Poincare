#include "sdp_handler.h"
#include "utils/log.h"

#define SINGLE_ORDER_FUTURE_MAX_VAL 290
#define SINGLE_ORDER_STOCK_MAX_VAL  1000000
#define PAY_UP_TICKS 3
#define DEBUG_MODE -1

extern send_data send_log;
extern int int_time;
extern bool debug_mode;
const static char STATUS[][64] = { "SUCCEED", "ENTRUSTED", "PARTED", "CANCELED", "REJECTED", "CANCEL_REJECTED", "INTRREJECTED", "UNDEFINED_STATUS" };
const static char OPEN_CLOSE_STR[][16] = { "OPEN", "CLOSE", "CLOSE_TOD", "CLOSE_YES" };
const static char BUY_SELL_STR[][8] = { "BUY", "SELL" };

static st_data_t g_st_data_t = { 0 };
static order_t g_send_order = { 0 };

SDPHandler::SDPHandler(int type, int length, void * cfg)
{
	if (length == DEBUG_MODE)
		debug_mode = true;

	m_orders = new OrderHash();
	m_contracts = new MyHash<Contract>(1024);

	m_config = (st_config_t*)cfg;
	m_send_resp_func = m_config->pass_rsp_hdl;
	m_send_order_func = m_config->proc_order_hdl;
	m_send_info_func = m_config->send_info_hdl;
	send_log = m_config->send_info_hdl;

	g_st_data_t.info = (void*)&g_send_order;

	m_strat_id = m_config->vst_id;

	for (int i = 0; i < ACCOUNT_MAX; i++) {
		if (m_config->accounts[i].account[0] == '\0') break;
		account_t& l_account = m_accounts.get_next_free_node();
		strlcpy(l_account.account, m_config->accounts[i].account, ACCOUNT_LEN);
		l_account.cash_available = m_config->accounts[i].cash_available;
		l_account.cash_asset = m_config->accounts[i].cash_asset;
		l_account.exch_rate = m_config->accounts[i].exch_rate;
		l_account.currency = m_config->accounts[i].currency;
		m_accounts.insert_current_node(l_account.account);

		PRINT_INFO("account: %s, cash_available: %f, cash_asset: %f, exch_rate: %f, currency: %d",
				l_account.account, l_account.cash_available, l_account.cash_asset, l_account.exch_rate, l_account.currency);
	}

	for (int i = 0; i < SYMBOL_MAX; i++) {
		if (m_config->contracts[i].symbol[0] == '\0') break;
		Contract& l_instr = m_contracts->get_next_free_node();
		contract_t& l_config_instr = m_config->contracts[i];
		strlcpy(l_instr.symbol, l_config_instr.symbol, SYMBOL_LEN);
		strlcpy(l_instr.account, l_config_instr.account, SYMBOL_LEN);
		l_instr.max_pos = 1000;
		l_instr.exch = (EXCHANGE)l_config_instr.exch;
		l_instr.max_accum_open_vol = l_config_instr.max_accum_open_vol;
		l_instr.max_cancel_limit = l_config_instr.max_cancel_limit;
		l_instr.order_cum_open_vol = 0;
		l_instr.single_side_max_pos = 0;
		if (l_instr.single_side_max_pos > 0)
			m_use_lock_pos = true;
		l_instr.expiration_date = l_config_instr.expiration_date;
		
		l_instr.pre_long_qty_remain = l_config_instr.yesterday_pos.long_volume;
		l_instr.pre_short_qty_remain = l_config_instr.yesterday_pos.short_volume;
		l_instr.positions[LONG_OPEN].qty = l_config_instr.today_pos.long_volume;
		l_instr.positions[LONG_OPEN].notional = l_config_instr.today_pos.long_price * l_config_instr.today_pos.long_volume;
		l_instr.positions[SHORT_OPEN].qty = l_config_instr.today_pos.short_volume;
		l_instr.positions[SHORT_OPEN].notional = l_config_instr.today_pos.short_price * l_config_instr.today_pos.short_volume;

		l_instr.fee_by_lot = l_config_instr.fee.fee_by_lot;
		l_instr.exchange_fee = l_config_instr.fee.exchange_fee;
		l_instr.yes_exchange_fee = l_config_instr.fee.yes_exchange_fee;
		l_instr.broker_fee = l_config_instr.fee.broker_fee;
		l_instr.stamp_tax = l_config_instr.fee.stamp_tax;
		l_instr.acc_transfer_fee = l_config_instr.fee.acc_transfer_fee;
		l_instr.tick_size = l_config_instr.tick_size;
		l_instr.multiplier = l_config_instr.multiple;
		m_contracts->insert_current_node(l_instr.symbol);

		PRINT_INFO("symbol: %s, account: %s, exch: %c, max_accum_open_vol: %d, max_cancel_limit: %d, order_cum_open_vol: %d, expiration_date: %d, "
			"pre_long_qty_remain: %d, pre_short_qty_remain: %d, today_long_pos: %d, today_long_price: %f, today_short_pos: %d, today_short_price: %f, "
			"fee_by_lot: %d, exchange_fee : %f, yes_exchange_fee : %f, broker_fee : %f, stamp_tax : %f, acc_transfer_fee : %f, tick_size : %f, multiplier : %f",
			l_instr.symbol, l_instr.account, l_instr.exch, l_instr.max_accum_open_vol, l_instr.max_cancel_limit, l_instr.order_cum_open_vol, l_instr.expiration_date,
			l_instr.pre_long_qty_remain, l_instr.pre_short_qty_remain, l_config_instr.today_pos.long_volume, l_config_instr.today_pos.long_price, l_config_instr.today_pos.short_volume, l_config_instr.today_pos.short_price,
			l_instr.fee_by_lot, l_instr.exchange_fee, l_instr.yes_exchange_fee, l_instr.broker_fee, l_instr.stamp_tax, l_instr.acc_transfer_fee, l_instr.tick_size, l_instr.multiplier);
	}
}

SDPHandler::~SDPHandler()
{
	PRINT_INFO("It is over: %s", __FUNCTION__);
	delete_mem(m_orders);
	delete_mem(m_contracts);
}

int SDPHandler::on_book(int type, int length, void * book)
{
	int ret = 0;
	switch (type) {
		case DEFAULT_STOCK_QUOTE: {
			Stock_Internal_Book *s_book = (Stock_Internal_Book *)((st_data_t*)book)->info;
			int_time = s_book->exch_time;
			update_contracts(s_book);
			break;
		}
		case DEFAULT_FUTURE_QUOTE: {
			Futures_Internal_Book *f_book = (Futures_Internal_Book *)((st_data_t*)book)->info;
			int_time = f_book->int_time;
			update_contracts(f_book);
			break;
		}
		case S_PLACE_ORDER_DEFAULT: {
			order_t *order = (order_t *)((st_data_t*)book)->info;
			Contract *instr = find_contract(order->symbol);
			bool close_yes = order->open_close == ORDER_CLOSE_YES ? true : false;
			bool flag_syn_cancel = false;
			ret = send_single_order(ALGORITHM_NORMAL, instr, (EXCHANGE)order->exch, order->price, order->volume, (DIRECTION)order->direction, (OPEN_CLOSE)order->open_close, flag_syn_cancel,
				close_yes, (INVESTOR_TYPE)order->investor_type, (ORDER_TYPE)order->order_type, (TIME_IN_FORCE)order->time_in_force);
			break;
		}
		case S_CANCEL_ORDER_DEFAULT: {
			order_t *order = (order_t *)((st_data_t*)book)->info;
			Order* l_order = m_orders->query_order(order->org_ord_id);
			ret = cancel_single_order(l_order);
			break;
		}
	}
	return ret;
}

int SDPHandler::on_response(int type, int length, void * resp)
{
	st_response_t* l_resp = (st_response_t*)((st_data_t*)resp)->info;
	
	PRINT_INFO("Order Resp: %lld %s %s %s %f %d %s %d %s", l_resp->order_id, l_resp->symbol,
		BUY_SELL_STR[l_resp->direction], OPEN_CLOSE_STR[l_resp->open_close], l_resp->exe_price, l_resp->exe_volume,
		STATUS[l_resp->status], l_resp->error_no, l_resp->error_info);
	
	process_order_resp(l_resp);

	Contract *instr = find_contract(l_resp->symbol);
	PRINT_INFO("Contract: %s pre_long_remain: %d pre_short_remain: %d long_pos: %d short_pos: %d long_open: %d long_close: %d short_open: %d short_close: %d\n"
		"pending_buy_open_size: %d pending_sell_open_size: %d pending_buy_close_size: %d pending_sell_close_size: %d\n"
		"Account: %s cash_available: %f",
		instr->symbol, instr->pre_long_qty_remain, instr->pre_short_qty_remain, long_position(instr), short_position(instr), instr->positions[LONG_OPEN].qty, instr->positions[LONG_CLOSE].qty, instr->positions[SHORT_OPEN].qty, instr->positions[SHORT_CLOSE].qty,
		instr->pending_buy_open_size, instr->pending_sell_open_size, instr->pending_buy_close_size, instr->pending_sell_close_size,
		instr->account, m_accounts.at(instr->account).cash_available);
	return 0;
}

int get_pos_by_side(Contract *a_instr, DIRECTION side)
{
	if (side == ORDER_BUY)
		return long_position(a_instr);
	else if (side == ORDER_SELL)
		return short_position(a_instr);

	return 0;
}

int SDPHandler::send_single_order(ORDER_ALGORITHM algorithm, Contract * instr, EXCHANGE exch, double price, int size, DIRECTION side, OPEN_CLOSE sig_openclose, bool flag_syn_cancel, bool flag_close_yesterday_pos, INVESTOR_TYPE investor_type, ORDER_TYPE order_type, TIME_IN_FORCE time_in_force)
{
	if (size <= 0) {
		if (size < 0)
			PRINT_WARN("OrderSize is invalid, OrderSize < 0:%d", size);
		return -1;
	}

	if (price <= 0) {
		PRINT_WARN("Price is invalid, price <= 0: %.2f!", price);
		return -1;
	}

	if (flag_syn_cancel == true && m_orders->get_pending_cancel_order_size_by_instr(instr) > 0) {
		PRINT_WARN("Don't send orders before receiving order cancelled.");
		return -1;
	}

	if (m_orders->full() == true) {
		PRINT_WARN("You have too much pending orders, exit!");
		return -1;
	}

	uint64_t l_order_id;
	int l_size, l_order_count = 0;
	int l_single_order_max_vol = SINGLE_ORDER_FUTURE_MAX_VAL;
	if (exch == SSE || exch == SZSE)
		l_single_order_max_vol = SINGLE_ORDER_STOCK_MAX_VAL;

	for (int l_remaining_size = size; l_remaining_size > 0; l_remaining_size -= l_single_order_max_vol, l_order_count++) {
		l_order_id = 0;
		l_size = (l_remaining_size > l_single_order_max_vol) ? l_single_order_max_vol : l_remaining_size; // take min of the two

		OPEN_CLOSE l_sig_openclose = sig_openclose;
		if (sig_openclose == ORDER_CLOSE && flag_close_yesterday_pos) {
			l_sig_openclose = ORDER_CLOSE_YES;
		}

		int l_compliance = 0;
		g_send_order.exch = exch;
		strcpy(g_send_order.symbol, (char *)instr->symbol);
		g_send_order.volume = l_size;
		g_send_order.price = price;
		g_send_order.direction = side;
		g_send_order.open_close = l_sig_openclose;
		g_send_order.investor_type = investor_type;
		g_send_order.order_type = order_type;
		g_send_order.time_in_force = time_in_force;
		g_send_order.st_id = m_strat_id;
		g_send_order.org_ord_id = 0;
		l_compliance = m_send_order_func(S_PLACE_ORDER_DEFAULT, sizeof(g_st_data_t), (void *)&g_st_data_t);
		l_order_id = g_send_order.order_id;
		switch(l_compliance) {
			case ERROR_REARCH_MAX_ACCUMULATE_OPEN_VAL: {
				is_exit_mode = true;
				PRINT_WARN("%s reached max vol %d allowed to open, Strategy %d send order failed, entering exit mode", instr->symbol, instr->max_accum_open_vol, m_strat_id);
			}
			case ERROR_OPEN_POS_IS_NOT_ENOUGH:
			case ERROR_SELF_MATCHING:
			case ERROR_CASH_IS_NOT_ENOUGH:
			case ERROR_PRE_POS_IS_NOT_ENOUGH_TO_CLOSE: {
				PRINT_WARN("%d %s Order %d@%f %s %s compliance check failed: %d",
					m_strat_id, instr->symbol, l_size, price,
					(side == ORDER_BUY ? "Buy" : "Sell"),
					(sig_openclose == ORDER_OPEN ? "Open" : "Close"), l_compliance);
				l_order_id = 0;
				break;
			}
			case 0: break;
		}

		if (l_order_id != 0) {
			/* If successfully sent an order, make a record of it */
			if (l_order_count >= MAX_ORDER_COUNT - 1) {
				PRINT_WARN("Too large order size, send failed!");
				return -3;
			}
			m_cur_ord_id_arr[l_order_count] = l_order_id;
			m_cur_ord_size_arr[l_order_count] = l_size;
			m_total_order_size += l_size;
		}
		else {
			PRINT_WARN("%s OrderID is 0. The order was sent unsuccessfully", instr->symbol);
			break;
		}
	}

	if (l_order_count > 0) {
		for (int i = 0; i < l_order_count; i++) {
			PRINT_INFO("%d OrderID: %lld Sent single order, %s %s %s %d@%f", m_strat_id, m_cur_ord_id_arr[i], instr->symbol,
				BUY_SELL_STR[side], OPEN_CLOSE_STR[sig_openclose],
				m_cur_ord_size_arr[i], price);
			int index = reverse_index(m_cur_ord_id_arr[i]);
			Order *l_order = m_orders->update_order(index, instr, price, m_cur_ord_size_arr[i],
				side, m_cur_ord_id_arr[i], sig_openclose);
			l_order->algorithm = algorithm;
			instr->m_order_no_entrusted_num++;
			if (flag_close_yesterday_pos) {
				update_yes_pos(instr, side, m_cur_ord_size_arr[i]);
			}
			update_account_cash(l_order);
		}

		m_new_order_times += l_order_count;
		return 0;
	}
	else {
		return -1;
	}
}

int SDPHandler::cancel_single_order(Order * a_order)
{
	if (!is_cancelable(a_order)) {
		return -1;
	}

	bool is_cancel_compliant = true;

	int l_compliance = 0;
	
	g_send_order.exch = a_order->exch;
	strcpy(g_send_order.symbol, (char *)a_order->symbol);
	g_send_order.volume = leaves_qty(a_order);
	g_send_order.price = a_order->price;
	g_send_order.direction = a_order->side;
	g_send_order.open_close = a_order->openclose;
	g_send_order.investor_type = a_order->invest_type;
	g_send_order.order_type = a_order->ord_type;
	g_send_order.time_in_force = a_order->time_in_force;
	g_send_order.st_id = m_strat_id;
	g_send_order.org_ord_id = a_order->signal_id;
	l_compliance = m_send_order_func(S_CANCEL_ORDER_DEFAULT, sizeof(g_st_data_t), (void *)&g_st_data_t);

	if (0 != l_compliance) {
		PRINT_WARN("%d OrderID: %lld Cancel order compliance check failed: %d",
			m_strat_id, a_order->signal_id, l_compliance);
		/* Not Compliant */
		is_cancel_compliant = false;

		if (ERROR_CANCEL_ORDER_REARCH_LIMIT == l_compliance) {
			is_exit_mode = true;
			PRINT_INFO("%s orderID: %lld reached max cancel allowed, Strategy %d cancel order failed, entering exit mode", a_order->symbol, a_order->signal_id, m_strat_id);
		}
	}

	// If we are cancelling a close order, mark it flag to true

	if (is_cancel_compliant) {
		m_cancelled_times++;
		m_total_cancel_size += leaves_qty(a_order);
		a_order->pending_cancel = true;
		m_orders->minus_pending_size(a_order, leaves_qty(a_order));
		PRINT_INFO("%d OrderID: %lld Cancel single order", m_strat_id, a_order->signal_id);
		return 0;
	} else {
		return -1;
	}
}

int SDPHandler::cancel_orders_by_side(Contract * instr, DIRECTION side, OPEN_CLOSE flag)
{
	struct list_head *pos, *n;
	Order *l_ord;

	list_head *ord_list = m_orders->get_order_by_side(side);
	list_for_each_safe(pos, n, ord_list) {
		l_ord = list_entry(pos, Order, pd_link);
		if (l_ord->contr == instr && l_ord->openclose == flag) {
			cancel_single_order(l_ord);
		}
	}

	return 0;
}

int SDPHandler::cancel_orders_with_dif_px(Contract * instr, DIRECTION side, double price, int & pending_open_size, int & pending_close_size, int & pending_close_yes_size)
{
	list_head *cancel_list = m_orders->get_order_by_side(side);
	struct list_head *pos, *n;
	Order *l_ord;

	list_for_each_safe(pos, n, cancel_list) {
		l_ord = list_entry(pos, Order, pd_link);
		// The Contract Address is the same, Save time do string compare
		if (l_ord->contr == instr) {
			if (double_compare(l_ord->price, price) == 0 && is_cancelable(l_ord) && l_ord->openclose == ORDER_OPEN) {
				pending_open_size += leaves_qty(l_ord);
			}
			else if (double_compare(l_ord->price, price) == 0 && is_cancelable(l_ord) && l_ord->openclose == ORDER_CLOSE) {
				pending_close_size += leaves_qty(l_ord);
				if (l_ord->openclose == ORDER_CLOSE_YES)
					pending_close_yes_size += leaves_qty(l_ord);
			}
			else {
				cancel_single_order(l_ord);
			}
		}
	}

	return pending_open_size + pending_close_size;
}

int SDPHandler::cancel_orders_by_side(Contract * instr, DIRECTION side)
{
	struct list_head *pos, *n;
	Order *l_ord;

	list_head *ord_list = m_orders->get_order_by_side(side);
	list_for_each_safe(pos, n, ord_list) {
		l_ord = list_entry(pos, Order, pd_link);
		// The Contract Address is the same£¬ Save time do string compare
		if (l_ord->contr == instr) {
			cancel_single_order(l_ord);
		}
	}

	return 0;
}

int SDPHandler::cancel_all_orders()
{
	list_t *pos, *n;
	Order *l_ord;

	list_head *buy_list = m_orders->get_order_by_side(ORDER_BUY);
	list_for_each_safe(pos, n, buy_list) {
		l_ord = list_entry(pos, Order, pd_link);
		cancel_single_order(l_ord);
	}

	list_head *sell_list = m_orders->get_order_by_side(ORDER_SELL);
	list_for_each_safe(pos, n, sell_list) {
		l_ord = list_entry(pos, Order, pd_link);
		cancel_single_order(l_ord);
	}

	return 0;
}

int SDPHandler::cancel_all_orders(Contract * instr)
{
	list_t *pos, *n;
	Order *l_ord;

	list_head *buy_list = m_orders->get_order_by_side(ORDER_BUY);
	list_for_each_safe(pos, n, buy_list) {
		l_ord = list_entry(pos, Order, pd_link);
		// The Contract Address is the same, Save time do string compare
		if (l_ord->contr == instr)
			cancel_single_order(l_ord);
	}

	list_head *sell_list = m_orders->get_order_by_side(ORDER_SELL);
	list_for_each_safe(pos, n, sell_list) {
		l_ord = list_entry(pos, Order, pd_link);
		// The Contract Address is the same, Save time do string compare
		if (l_ord->contr == instr)
			cancel_single_order(l_ord);
	}

	return 0;
}

void SDPHandler::process_exit_mode()
{
	if (exit_mode_orders_sent == true) {
		PRINT_WARN("In exit mode, exit!");
		return;
	}

	cancel_all_orders();

	for (auto iter = m_contracts->begin(); iter != m_contracts->end(); iter++) {
		Contract* l_instr = &iter->second;
		int l_pos = position(l_instr);
		double l_ord_px = 0.0;
		if (l_pos > 0) {
			l_ord_px = l_instr->bp1 - PAY_UP_TICKS * l_instr->tick_size;
			if (l_ord_px < l_instr->lower_limit || l_instr->bp1 == 0)
				l_ord_px = l_instr->lower_limit;

			// If direction is the opposite side, close all position.
			send_single_order(ALGORITHM_DMA, l_instr, l_instr->exch, l_ord_px, l_pos, ORDER_SELL, ORDER_CLOSE);
		}
		else if (l_pos < 0) {
			int l_abs_pos = -1 * l_pos;
			l_ord_px = l_instr->ap1 + PAY_UP_TICKS * l_instr->tick_size;
			if (l_ord_px > l_instr->upper_limit || l_instr->ap1 == 0)
				l_ord_px = l_instr->upper_limit;

			send_single_order(ALGORITHM_DMA, l_instr, l_instr->exch, l_ord_px, l_abs_pos, ORDER_BUY, ORDER_CLOSE);
		}
	}

	exit_mode_orders_sent = true;
}

Contract * SDPHandler::find_contract(char * symbol)
{
	return &m_contracts->at(symbol);
}

void SDPHandler::update_yes_pos(Contract * instr, DIRECTION side, int size)
{
	if (side == ORDER_BUY) {
		instr->pre_short_qty_remain -= size;
	}
	else if (side == ORDER_SELL) {
		instr->pre_long_qty_remain -= size;
	}
}

bool SDPHandler::process_order_resp(st_response_t * ord_resp)
{
	// In live trading, there's an extra callback with vol=0, ignore it.
	if ((ord_resp->status == SIG_STATUS_PARTED || ord_resp->status == SIG_STATUS_SUCCEED)
		&& ord_resp->exe_volume == 0)
		return false;

	Order* l_order = m_orders->query_order(ord_resp->order_id);
	if (l_order == NULL)
		return false;

	Contract& l_instr = *l_order->contr;

	if (ord_resp->open_close == ORDER_CLOSE_YES) {
		if (ord_resp->status == SIG_STATUS_CANCELED
			|| (ord_resp->status == SIG_STATUS_REJECTED && !l_order->pending_cancel)
			|| ord_resp->status == SIG_STATUS_INTRREJECTED) {
			// Get Contract, and update this position.
			if (ord_resp->direction == ORDER_BUY) {
				l_instr.pre_short_qty_remain += leaves_qty(l_order);
			}
			if (ord_resp->direction == ORDER_SELL) {
				l_instr.pre_long_qty_remain += leaves_qty(l_order);
			}
		}
	}

	if (ord_resp->status == SIG_STATUS_PARTED || ord_resp->status == SIG_STATUS_SUCCEED) {
		l_order->cum_amount += ord_resp->exe_volume * ord_resp->exe_price;
		l_order->cum_qty += ord_resp->exe_volume;
		l_order->last_px = ord_resp->exe_price;
		l_order->last_qty = ord_resp->exe_volume;
		if (l_order->cum_qty < l_order->volume) {
			l_order->status = SIG_STATUS_PARTED;
		}
		else if (l_order->cum_qty == l_order->volume) {
			l_order->status = SIG_STATUS_SUCCEED;
		}

		double l_notional_change = ord_resp->exe_price * ord_resp->exe_volume;

		// If fee by lot, calculated fee based lot size, otherwise use notional change.
		double l_fees = 0.0;
		double l_close_fees = 0.0;
		if (m_is_use_fee) {
			l_fees = get_transaction_fee(&l_instr, ord_resp->exe_volume, ord_resp->exe_price);
			if (ord_resp->open_close == ORDER_CLOSE_YES) {
				// If it's close yesterday, revert back using today's fee.
				l_close_fees -= l_fees;
				// Add two yesterday fee back.
				l_close_fees += (2 * get_transaction_fee(&l_instr, ord_resp->exe_volume, ord_resp->exe_price, true));
			}
			else {
				l_close_fees = l_fees;
			}
		}

		/* fees are added to the notional, like a cost */
		if (ord_resp->direction == ORDER_BUY) {
			if (ord_resp->open_close == ORDER_OPEN) {
				l_instr.positions[LONG_OPEN].notional += (l_notional_change + l_fees);
				l_instr.positions[LONG_OPEN].qty += ord_resp->exe_volume;
			}
			else if (ord_resp->open_close == ORDER_CLOSE || ord_resp->open_close == ORDER_CLOSE_YES) {
				l_instr.positions[LONG_CLOSE].notional += (l_notional_change + l_close_fees);
				l_instr.positions[LONG_CLOSE].qty += ord_resp->exe_volume;
			}
		}
		else if (ord_resp->direction == ORDER_SELL) {
			l_fees += l_notional_change * l_instr.stamp_tax; /* For stocks, there's stamp tax when selling */

			if (ord_resp->open_close == ORDER_OPEN) {
				l_instr.positions[SHORT_OPEN].notional += (l_notional_change - l_fees);
				l_instr.positions[SHORT_OPEN].qty += ord_resp->exe_volume;
			}
			else if (ord_resp->open_close == ORDER_CLOSE || ord_resp->open_close == ORDER_CLOSE_YES) {
				l_instr.positions[SHORT_CLOSE].notional += (l_notional_change - l_close_fees);
				l_instr.positions[SHORT_CLOSE].qty += ord_resp->exe_volume;
			}
		}
	}

	if (l_order->status != SIG_STATUS_INIT && ord_resp->status == SIG_STATUS_ENTRUSTED) {
		/* If already entrusted, no need to call update_order_list() */
	}
	else {
		switch (ord_resp->status) {
			case SIG_STATUS_CANCELED:
			case SIG_STATUS_REJECTED:
			case SIG_STATUS_INTRREJECTED:
				if (l_order->status != SIG_STATUS_INIT) break;
			case SIG_STATUS_ENTRUSTED:
				l_instr.m_order_no_entrusted_num--; break;
			case SIG_STATUS_PARTED:
			case SIG_STATUS_SUCCEED: {
				if (l_order->status == SIG_STATUS_INIT)
					l_instr.m_order_no_entrusted_num--; break;
			}
			default: break;
		}
		l_order->status = (ORDER_STATUS)ord_resp->status;
		update_account_cash(l_order);
	}

	update_pending_status(l_order);
	return true;
}

void SDPHandler::update_account_cash(Order* a_order)
{
	if (a_order == NULL) {
		PRINT_INFO("update_account_cash: order_t is NULL");
		return;
	}
    
	Contract& l_instr = *a_order->contr;
	account_t& l_account = m_accounts.at(l_instr.account);

	switch (a_order->status) {
	case SIG_STATUS_INIT: {/* New Order */
		if (a_order->exch == SZSE || a_order->exch == SSE) {  // Stocks, multiplier is 1
			if (a_order->side == ORDER_BUY) {
				l_account.cash_available -= a_order->price * a_order->volume;
			}
			else { /* Stock has stamp tax*/
				l_account.cash_available -= a_order->price * a_order->volume * l_instr.stamp_tax;
			}
			l_account.cash_available -= get_transaction_fee(&l_instr, a_order->volume, a_order->price);
		}
		else {  // Futures
			if (a_order->openclose == ORDER_OPEN) {  // Add back the notional if canceled or rejected. Minus the notional if it's Init.
				l_account.cash_available -= a_order->price * a_order->volume * l_instr.multiplier;
			}
			l_account.cash_available -= get_transaction_fee(&l_instr, a_order->volume, a_order->price) * l_instr.multiplier;
		}
	} break;
	case SIG_STATUS_CANCELED:
	case SIG_STATUS_REJECTED: {   /* Cancel or rejected, resume the cash */
		if (a_order->exch == SZSE || a_order->exch == SSE) {  // Stocks, multiplier is 1
			if (a_order->side == ORDER_BUY) {
				l_account.cash_available += a_order->price * leaves_qty(a_order);
			} else { /* Stock has stamp tax*/
				l_account.cash_available += a_order->price * leaves_qty(a_order) * l_instr.stamp_tax;
			}
			l_account.cash_available += get_transaction_fee(&l_instr, leaves_qty(a_order), a_order->price);
		} else {  // Futures
			if (a_order->openclose == ORDER_OPEN) {  // Add back the notional if canceled or rejected. Minus the notional if it's Init.
				l_account.cash_available += a_order->price * leaves_qty(a_order) * l_instr.multiplier;
			}
			l_account.cash_available += get_transaction_fee(&l_instr, leaves_qty(a_order), a_order->price) * l_instr.multiplier;
		}
	} break;
	case SIG_STATUS_SUCCEED:
	case SIG_STATUS_PARTED: {
		/* Add cash upon part/success sell response */
		if (a_order->exch == SZSE || a_order->exch == SSE) {  // Stocks, multiplier is 1, so we don't need to use "* l_instr->multiplier" here.
			if (a_order->side == ORDER_SELL) {
				l_account.cash_available += a_order->last_px * a_order->last_qty;
			}
		}
		else if (a_order->openclose == ORDER_CLOSE || a_order->openclose == ORDER_CLOSE_YES) {
			// When close a position, we reset the cash.
			// When a position is closed, it will release some cash for openning that account
			if (a_order->side == ORDER_BUY) {
				double l_sell_open_avg_px = l_instr.positions[SHORT_OPEN].notional / l_instr.positions[SHORT_OPEN].qty;
				l_account.cash_available += (l_sell_open_avg_px + (l_sell_open_avg_px - a_order->last_px)) * a_order->last_qty * l_instr.multiplier;
			}
			else if (a_order->side == ORDER_SELL) {
				l_account.cash_available += a_order->last_px * a_order->last_qty * l_instr.multiplier;
				//double l_buy_open_avg_px = l_instr->positions[LONG_OPEN].notional / l_instr->positions[LONG_OPEN].qty;
				//l_account.cash_available += (l_buy_open_avg_px + (a_order->last_px - l_buy_open_avg_px)) * a_order->last_qty * l_instr->multiplier;
			}
		}
		else if (a_order->openclose == ORDER_OPEN) {
			double price_gap = a_order->price - a_order->last_px; // If we send a price higher than last_px, we need to re-adjust the gap to cash.
			l_account.cash_available += price_gap * a_order->last_qty * l_instr.multiplier;
		}
	} break;  
	case SIG_STATUS_ENTRUSTED:
		break;
	case SIG_STATUS_CANCEL_REJECTED:
		break;
	default:
		break;
	}
}

void SDPHandler::update_pending_status(Order * a_order)
{
	/*Update pending order size*/
	switch (a_order->status)
	{
		case SIG_STATUS_ENTRUSTED: {
			break;
		}
		case SIG_STATUS_SUCCEED: {
			if (a_order->pending_cancel == false)
				m_orders->minus_pending_size(a_order, a_order->last_qty);
			else
				a_order->pending_cancel = false;
			break;
		}
		case SIG_STATUS_PARTED: {
			if (a_order->pending_cancel == false)
				m_orders->minus_pending_size(a_order, a_order->last_qty);
			break;
		}
		case SIG_STATUS_CANCELED: {
			if (a_order->pending_cancel == false)     //Sometimes in SHFE we will receive cancel response after we send a order. Here We recognize the cancel as rejected.
				m_orders->minus_pending_size(a_order, leaves_qty(a_order));
			else
				a_order->pending_cancel = false;
			break;
		}
		case SIG_STATUS_INTRREJECTED:
		case SIG_STATUS_REJECTED: {
			if (a_order->pending_cancel == true) { //Cancel reject
				m_orders->add_pending_size(a_order, leaves_qty(a_order));
				a_order->pending_cancel = false;
				m_cancelled_times--;
				m_total_cancel_size -= leaves_qty(a_order);
			}
			else {
				m_orders->minus_pending_size(a_order, leaves_qty(a_order));
			}
			break;
		}
		case SIG_STATUS_CANCEL_REJECTED: {
			m_orders->add_pending_size(a_order, leaves_qty(a_order));
			a_order->pending_cancel = false;
			m_cancelled_times--;
			m_total_cancel_size -= leaves_qty(a_order);
			break;
		}
	}
}

bool SDPHandler::update_contracts(Stock_Internal_Book * a_book)
{
	Contract& instr = m_contracts->at(a_book->ticker);
	instr.bp1 = a_book->bp_array[0];
	instr.ap1 = a_book->ap_array[0];
	instr.bv1 = a_book->bv_array[0];
	instr.av1 = a_book->av_array[0];
	instr.lower_limit = a_book->lower_limit_px;
	instr.upper_limit = a_book->upper_limit_px;
	instr.pre_data_time = a_book->exch_time;
	return true;
}

bool SDPHandler::update_contracts(Futures_Internal_Book * a_book)
{
	Contract& instr = m_contracts->at(a_book->symbol);
	instr.bp1 = a_book->bp_array[0];
	instr.ap1 = a_book->ap_array[0];
	instr.bv1 = a_book->bv_array[0];
	instr.av1 = a_book->av_array[0];
	instr.lower_limit = a_book->lower_limit_px;
	instr.upper_limit = a_book->upper_limit_px;
	instr.pre_data_time = a_book->int_time;
	return true;
}

bool SDPHandler::update_contracts(Internal_Bar * a_bar)
{
	Contract& instr = m_contracts->at(a_bar->symbol);
	instr.pre_data_time = a_bar->int_time;
	instr.upper_limit = a_bar->upper_limit;
	instr.lower_limit = a_bar->lower_limit;
	return true;
}

bool SDPHandler::check_process_strat_singal_inputs(Contract *a_contract, DIRECTION side, int desired_size)
{
	if (side != ORDER_BUY && side != ORDER_SELL)
		return false;

	// Check whether the desired size is wrong, and our current position exceed the max position.
	if (desired_size > a_contract->max_pos) {
		PRINT_WARN("Error in process_strat_singal, desired_size: %d > max_pos: %d", desired_size, (a_contract)->max_pos);
		return false;
	}
	/*if (position(a_contract) > a_contract->max_pos && side == ORDER_BUY){
	PRINT_INFO("%s's position %d exceed max vol %d allowed to buy in process_strat_signal", a_contract->symbol, position(a_contract), a_contract->max_pos);
	return false;
	}
	if (position(a_contract) < -a_contract->max_pos && side == ORDER_SELL){
	PRINT_INFO("%s's position %d exceed max vol %d allowed to sell in process_strat_signal", a_contract->symbol, position(a_contract), a_contract->max_pos);
	return false;
	}*/
	if (desired_size < 0) { // if desired_size error, just return
		PRINT_WARN("Strategy called process_strat_signal wrong, desired_size %d is less than 0", desired_size);
		return false;
	}
	if (a_contract->m_order_no_entrusted_num != 0) {
		PRINT_WARN("No entrusted response received.");
		return false;
	}
	//if (desired_size == 0){
	//	if (position(a_contract) > 0 && side != ORDER_SELL)
	//		return false;
	//	else if (position(a_contract) < 0 && side != ORDER_BUY)
	//		return false;
	//}

	return true;
}

bool
SDPHandler::process_strat_signal(Contract * l_instr, DIRECTION a_side, int desired_size,
	double a_price, bool flag_syn_cancel)
{
	// Check instrument is not NULL
	if (l_instr == NULL) {
		PRINT_WARN("Strategy::process_strat_signal - l_instr is NULL");
		return false;
	}
	/*LOG_INFO("%d %s Enter process_strat_signal, desire %s %d@%f", m_strat_id, l_instr->symbol,
	(a_side == ORDER_BUY ? "Buy" : "Sell"), desired_size, a_price);*/

	// 1. Check variable values, and initial variables. 
	if (check_process_strat_singal_inputs(l_instr, a_side, desired_size) == false)
		return false;

	if (desired_size == 0) {
		if (position(l_instr) > 0 && a_side != ORDER_SELL)
			a_side = ORDER_SELL;
		else if (position(l_instr) < 0 && a_side != ORDER_BUY)
			a_side = ORDER_BUY;
	}

	if (is_exit_mode == true) {
		process_exit_mode();
	}
	else {
		if (m_use_lock_pos) {
			process_strat_signal_with_lock_pos(l_instr, a_side, desired_size, a_price, flag_syn_cancel);
		}
		else {
			process_strat_signal_without_lock_pos(l_instr, a_side, desired_size, a_price, flag_syn_cancel);
		}
	}
	return true;
}

void
SDPHandler::process_strat_signal_with_lock_pos(Contract * l_instr, DIRECTION a_side, int desired_size,
	double a_price, bool flag_syn_cancel)
{
	int *opposite_remained_pos_ptr = NULL;
	int pending_open_size = 0;
	int pending_close_size = 0;
	int open_size = 0;
	int l_pending_close_yes_size = 0;
	int total_pending_buy_close_size = l_instr->pending_buy_close_size;
	int total_pending_sell_close_size = l_instr->pending_sell_close_size;

	cancel_orders_with_dif_px(l_instr, a_side, a_price, pending_open_size, pending_close_size, l_pending_close_yes_size);
	if (a_side == ORDER_BUY) {
		opposite_remained_pos_ptr = &(l_instr->pre_short_qty_remain);
	}
	else if (a_side == ORDER_SELL) {
		opposite_remained_pos_ptr = &(l_instr->pre_long_qty_remain);
	}
	else
		return;

	// Consider different position senarios
	int same_side_pos = get_pos_by_side(l_instr, a_side);
	int opposite_side_pos = get_pos_by_side(l_instr, switch_side(a_side));

	/*if (pending_close_size > opposite_side_pos){
	// PRINT('N', "Close size is larger than current other a_side position");
	cancel_orders_by_side(l_instr, a_side, ORDER_CLOSE);
	open_size -= pending_close_size;
	pending_close_size = 0;
	}*/

	// 3 big scenarios based on pos_gap
	int pos_gap = desired_size - (same_side_pos - opposite_side_pos);

	if (pos_gap < 0) {  // We have too much position, need to reduce some.
						/*
						cancel_orders_with_same_px(a_side, a_price);
						cancel_orders_by_side(switch_side(a_side), ORDER_OPEN);
						process_reduce_position(l_instr, a_side, a_price, same_side_pos, abs(pos_gap));
						*/
		cancel_orders_by_side(l_instr, a_side);
		send_orders_with_lockpos(l_instr, switch_side(a_side), abs(pos_gap), a_price, pending_open_size, pending_close_size, l_pending_close_yes_size,
			total_pending_buy_close_size, total_pending_sell_close_size, flag_syn_cancel);

		return;
	}
	else if (pos_gap == 0) { // If current pos is equal with desired size, we will do nothing.
		cancel_all_orders(l_instr);
		return;
	}
	cancel_orders_by_side(l_instr, switch_side(a_side));
	send_orders_with_lockpos(l_instr, a_side, pos_gap, a_price, pending_open_size, pending_close_size, l_pending_close_yes_size,
		total_pending_buy_close_size, total_pending_sell_close_size, flag_syn_cancel);

}

void
SDPHandler::process_strat_signal_without_lock_pos(Contract * l_instr, DIRECTION a_side, int desired_size,
	double a_price, bool flag_syn_cancel)
{
	int pending_open_size = 0;
	int pending_close_size = 0;
	int pending_close_yes_size = 0;
	int total_pending_buy_close_size = l_instr->pending_buy_close_size;
	int total_pending_sell_close_size = l_instr->pending_sell_close_size;

	//cancel_orders_with_dif_px(l_instr, a_side, a_price, pending_open_size, pending_close_size, pending_close_yes_size);

	// Consider different position senarios
	int same_side_pos = get_pos_by_side(l_instr, a_side);
	int opposite_side_pos = get_pos_by_side(l_instr, switch_side(a_side));

	// 3 big scenarios based on pos_gap
	int pos_gap = desired_size - (same_side_pos - opposite_side_pos);

	// Cancel all orders at the other a_side.
	if (pos_gap < 0) {  // We have too much positon, need to reduce some.
						/*
						cancel_orders_with_same_px(a_side, a_price);
						cancel_orders_by_side(switch_side(a_side), ORDER_OPEN);
						process_reduce_position(l_instr, a_side, a_price, same_side_pos, abs(pos_gap));
						*/
		cancel_orders_with_dif_px(l_instr, switch_side(a_side), a_price, pending_open_size, pending_close_size, pending_close_yes_size);
		cancel_orders_by_side(l_instr, a_side);
		send_orders_without_lockpos(l_instr, switch_side(a_side), abs(pos_gap), a_price, pending_open_size, pending_close_size,
			total_pending_buy_close_size, total_pending_sell_close_size, flag_syn_cancel);
		return;
	}
	else if (pos_gap == 0) { // If current pos is equal with desired size, we will do nothing.
		cancel_all_orders(l_instr);
		return;
	}

	cancel_orders_with_dif_px(l_instr, a_side, a_price, pending_open_size, pending_close_size, pending_close_yes_size);
	cancel_orders_by_side(l_instr, switch_side(a_side));
	// else if pos_gap > 0 or desired_size > (same_side_pos - opposite_side_pos)
	send_orders_without_lockpos(l_instr, a_side, pos_gap, a_price, pending_open_size, pending_close_size,
		total_pending_buy_close_size, total_pending_sell_close_size, flag_syn_cancel);
}


void SDPHandler::send_open_orders_consider_pending_open(Contract * a_instr, DIRECTION a_side,
	double a_price, int a_need_to_open, int a_pending_open_size, bool flag_syn_cancel)
{
	if (a_pending_open_size <= a_need_to_open) {
		send_single_order(ALGORITHM_DMA, a_instr, a_instr->exch, a_price, a_need_to_open - a_pending_open_size,
			a_side, ORDER_OPEN, flag_syn_cancel);
	}
	else {
		cancel_orders_by_side(a_instr, a_side, ORDER_OPEN);
		send_single_order(ALGORITHM_DMA, a_instr, a_instr->exch, a_price, a_need_to_open,
			a_side, ORDER_OPEN, flag_syn_cancel);
	}
}

void
SDPHandler::send_available_close_orders(Contract * a_instr, DIRECTION a_side, double a_price, int a_need_to_open,
	int a_need_to_close, int &a_available_for_close, int &a_pending_open_size, int a_remained_opposite_yes_pos,
	bool a_flag_too_much_pending_close, bool flag_syn_cancel)
{
	if (a_need_to_open < a_pending_open_size) {
		cancel_orders_by_side(a_instr, a_side, ORDER_OPEN);
		a_pending_open_size = 0;
	}
	if (a_flag_too_much_pending_close) {
		cancel_orders_by_side(a_instr, a_side, ORDER_CLOSE);
	}
	if (a_remained_opposite_yes_pos > 0) {
		if (a_need_to_close > a_remained_opposite_yes_pos) {
			send_single_order(ALGORITHM_DMA, a_instr, a_instr->exch, a_price, a_remained_opposite_yes_pos,
				a_side, ORDER_CLOSE, flag_syn_cancel, true);
			send_single_order(ALGORITHM_DMA, a_instr, a_instr->exch, a_price, a_need_to_close - a_remained_opposite_yes_pos,
				a_side, ORDER_CLOSE, flag_syn_cancel, false);
		}
		else {
			send_single_order(ALGORITHM_DMA, a_instr, a_instr->exch, a_price, a_need_to_close,
				a_side, ORDER_CLOSE, flag_syn_cancel, true);
		}
	}
	else {
		send_single_order(ALGORITHM_DMA, a_instr, a_instr->exch, a_price, a_need_to_close,
			a_side, ORDER_CLOSE, flag_syn_cancel, false);
	}
	a_available_for_close -= a_need_to_close;
}

void
SDPHandler::asynch_cancel_closing_order(Contract * a_instr, DIRECTION a_side, double a_price, int a_need_to_open,
	int a_need_to_close, int &a_available_for_close, int &a_pending_open_size, int a_remained_opposite_yes_pos,
	bool a_flag_too_much_pending_close, bool flag_syn_cancel) {
	if (a_need_to_close > a_available_for_close) {     // Asynchronously cancel closing order
		send_open_orders_instead_of_close(a_instr, a_side, a_price, a_need_to_open, a_need_to_close,
			a_available_for_close, a_pending_open_size, a_remained_opposite_yes_pos, a_flag_too_much_pending_close, flag_syn_cancel);
	}
	else {
		send_available_close_orders(a_instr, a_side, a_price, a_need_to_open, a_need_to_close,
			a_available_for_close, a_pending_open_size, a_remained_opposite_yes_pos, a_flag_too_much_pending_close, flag_syn_cancel);
	}
}


void
SDPHandler::send_orders_with_lockpos(Contract * a_instr, DIRECTION a_side, int a_desired_size,
	double a_price, int pending_open_size, int pending_close_size, int pending_close_yes_size, int total_pending_buy_close_size,
	int total_pending_sell_close_size, bool flag_syn_cancel)
{
	int l_max_holding = a_instr->single_side_max_pos;
	int l_remained_opposite_pos;
	int available_for_close_yes = 0;
	int cur_opposite_pos;
	int l_canceling_close_order_size = 0;
	if (a_side == ORDER_BUY) {
		l_remained_opposite_pos = a_instr->pre_short_qty_remain;
		available_for_close_yes = l_remained_opposite_pos;     //  <0
		cur_opposite_pos = short_position(a_instr);
	}
	else if (a_side == ORDER_SELL) {
		l_remained_opposite_pos = a_instr->pre_long_qty_remain;
		available_for_close_yes = l_remained_opposite_pos;     //  <0
		cur_opposite_pos = long_position(a_instr);
	}
	/*Available_for_close should not less than zero. We might send a big open order in asynch when it is less than zero.*/
	if (available_for_close_yes < 0) {
		PRINT_WARN("Something is wrong, CHECK available_for_close < 0. %s total_pending_buy_close: %d, total_pending_sell_close: %d",
			(a_side == ORDER_BUY ? "Buy" : "Sell"), total_pending_buy_close_size, total_pending_sell_close_size);
		cancel_orders_by_side(a_instr, a_side, ORDER_CLOSE);
		return;
	}
	int cur_yes_opp_pos = l_remained_opposite_pos + pending_close_yes_size;
	if (a_desired_size <= cur_yes_opp_pos) { //Closing (a_desired_size) yesterday's opposite position is enough. 
		if (pending_close_yes_size < a_desired_size) {
			cancel_orders_by_side(a_instr, a_side, ORDER_OPEN);

			int need_to_close = a_desired_size - pending_close_yes_size;
			send_single_order(ALGORITHM_DMA, a_instr, a_instr->exch, a_price, need_to_close, a_side, ORDER_CLOSE, flag_syn_cancel, true);
			//asynch_cancel_closing_order(a_instr, a_side, a_price, 0, need_to_close, available_for_close_yes,
			//pending_open_size, l_remained_opposite_pos, false);
		}
		else if (pending_close_yes_size > a_desired_size) {
			asynch_cancel_closing_order(a_instr, a_side, a_price, 0, a_desired_size, available_for_close_yes,
				pending_open_size, l_remained_opposite_pos, true, flag_syn_cancel);
		}
		else {
			cancel_orders_by_side(a_instr, a_side, ORDER_OPEN);
		}
	}
	else {  //a_desired_size >= cur_yes_opp_pos. Closing yesterday's opposite position is not enough. Open today / Open today and close today.
		int need_to_open = a_desired_size - l_remained_opposite_pos - pending_close_yes_size; // need_to_open is definitely greater than zero
		int max_qty_allowed_to_open = 0;
		/*close yesterday if necessary*/
		if (l_remained_opposite_pos > 0) {
			send_single_order(ALGORITHM_DMA, a_instr, a_instr->exch, a_price, l_remained_opposite_pos, a_side, ORDER_CLOSE, flag_syn_cancel, true);
			available_for_close_yes -= l_remained_opposite_pos;
		}
		if (need_to_open <= max_qty_allowed_to_open) {
			//cancel_close_orders_by_side_and_posflag(a_side, false, a_session);  //Cancel all today's close orders.
			/*open today*/
			send_open_orders_consider_pending_open(a_instr, a_side, a_price, need_to_open, pending_open_size, flag_syn_cancel);
		}
		else { //need_to_open > max_qty_allowed_to_open.
			send_open_orders_consider_pending_open(a_instr, a_side, a_price, max_qty_allowed_to_open, pending_open_size, flag_syn_cancel);
		}
	}
}

void
SDPHandler::send_orders_without_lockpos(Contract * a_instr, DIRECTION a_side, int a_desired_size,
	double a_price, int pending_open_size, int pending_close_size, int total_pending_buy_close_size,
	int total_pending_sell_close_size, bool flag_syn_cancel)
{
	int l_remained_opposite_yes_pos;
	int available_for_close = 0;
	int cur_opposite_pos;
	int l_canceling_close_order_size = 0;
	bool flag_yesterday_remained = false;
	if (a_side == ORDER_BUY) {
		l_remained_opposite_yes_pos = a_instr->pre_short_qty_remain;
		available_for_close = short_position(a_instr) - total_pending_buy_close_size;     //  <0
		cur_opposite_pos = short_position(a_instr);
	}
	else if (a_side == ORDER_SELL) {
		l_remained_opposite_yes_pos = a_instr->pre_long_qty_remain;
		available_for_close = long_position(a_instr) - total_pending_sell_close_size;     //  <0
		cur_opposite_pos = long_position(a_instr);
	}
	/*Available_for_close should not less than zero. We might send a big open order in asynch when it is less than zero.*/
	if (available_for_close < 0) {
		PRINT_WARN("Something is wrong, CHECK available_for_close < 0. %s total_pending_buy_close: %d, total_pending_sell_close: %d",
			(a_side == ORDER_BUY ? "Buy" : "Sell"), total_pending_buy_close_size, total_pending_sell_close_size);
		cancel_orders_by_side(a_instr, a_side, ORDER_CLOSE);
		return;
	}

	if (a_desired_size <= cur_opposite_pos) {  // We just close a_desired_size
		if (pending_close_size < a_desired_size) {   // Not enough pending close, need to close more
			int need_to_close = a_desired_size - pending_close_size;
			asynch_cancel_closing_order(a_instr, a_side, a_price, 0, need_to_close, available_for_close,
				pending_open_size, l_remained_opposite_yes_pos, false, flag_syn_cancel);
		}
		else if (pending_close_size > a_desired_size) {   // Too much pending close
			asynch_cancel_closing_order(a_instr, a_side, a_price, 0, a_desired_size, available_for_close,
				pending_open_size, l_remained_opposite_yes_pos, true, flag_syn_cancel);
		}
		else {  // pending_close_size = a_desired_size
			cancel_orders_by_side(a_instr, a_side, ORDER_OPEN);
		}

	}
	else {   // We close cur_opposite_pos and also need to open
		int need_to_open = a_desired_size - cur_opposite_pos;        // *****Important: Here is different from the front part.*****
		if (pending_close_size < cur_opposite_pos) {    // Not enough pending close, need to close more
			int need_to_close = cur_opposite_pos - pending_close_size;
			asynch_cancel_closing_order(a_instr, a_side, a_price, need_to_open, need_to_close, available_for_close,
				pending_open_size, l_remained_opposite_yes_pos, false, flag_syn_cancel);
		}
		else if (pending_close_size > cur_opposite_pos) {
			asynch_cancel_closing_order(a_instr, a_side, a_price, need_to_open, cur_opposite_pos, available_for_close,
				pending_open_size, l_remained_opposite_yes_pos, true, flag_syn_cancel);
		}
		else {  // pending_close_size = cur_opposite_pos
			if (need_to_open < pending_open_size) {
				cancel_orders_by_side(a_instr, a_side, ORDER_OPEN);
				pending_open_size = 0;
			}
		}
		// need_to_open = a_desired_size - cur_opposite_pos
		send_single_order(ALGORITHM_DMA, a_instr, a_instr->exch, a_price, need_to_open - pending_open_size,
			a_side, ORDER_OPEN, flag_syn_cancel);
	}
}

void
SDPHandler::send_open_orders_instead_of_close(Contract * a_instr, DIRECTION a_side, double a_price, int a_need_to_open,
	int a_need_to_close, int &a_available_for_close, int &a_pending_open_size, int a_remained_opposite_yes_pos,
	bool a_flag_too_much_pending_close, bool flag_syn_cancel)
{
	int need_to_open_asynch = a_need_to_close - a_available_for_close;
	if (a_need_to_open + need_to_open_asynch >= a_pending_open_size) {               // Complicated part.
		if (need_to_open_asynch >= a_pending_open_size) {
			need_to_open_asynch = need_to_open_asynch - a_pending_open_size;
			a_pending_open_size = 0;
		}
		else {
			a_pending_open_size -= need_to_open_asynch;
			need_to_open_asynch = 0;
		}
		if (a_flag_too_much_pending_close) {
			cancel_orders_by_side(a_instr, a_side, ORDER_CLOSE);   // Still need to cancel pending close
		}
	}
	else {
		cancel_orders_by_side(a_instr, a_side, ORDER_OPEN);
		if (a_flag_too_much_pending_close) {
			cancel_orders_by_side(a_instr, a_side, ORDER_CLOSE);   // Still need to cancel pending close
		}
		a_pending_open_size = 0;
	}
	/*send order*/
	if (a_remained_opposite_yes_pos > 0) {
		if (a_available_for_close > a_remained_opposite_yes_pos) {
			send_single_order(ALGORITHM_DMA, a_instr, a_instr->exch, a_price, a_remained_opposite_yes_pos,
				a_side, ORDER_CLOSE, flag_syn_cancel, true);
			send_single_order(ALGORITHM_DMA, a_instr, a_instr->exch, a_price, a_available_for_close - a_remained_opposite_yes_pos,
				a_side, ORDER_CLOSE, flag_syn_cancel, false);
		}
		else {
			send_single_order(ALGORITHM_DMA, a_instr, a_instr->exch, a_price, a_available_for_close,
				a_side, ORDER_CLOSE, flag_syn_cancel, true);
		}
	}
	else {
		send_single_order(ALGORITHM_DMA, a_instr, a_instr->exch, a_price, a_available_for_close,
			a_side, ORDER_CLOSE, flag_syn_cancel, false);
	}
	//a_flag_yesterday_remained = false;
	send_single_order(ALGORITHM_DMA, a_instr, a_instr->exch, a_price, need_to_open_asynch,
		a_side, ORDER_OPEN, flag_syn_cancel);
	a_available_for_close = 0;
}
