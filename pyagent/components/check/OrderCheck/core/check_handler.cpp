#include "quote_format_define.h"
#include "check_handler.h"
#include "utils/log.h"

extern send_data send_log;
extern int int_time;
const static char STATUS[][64] = { "SUCCEED", "ENTRUSTED", "PARTED", "CANCELED", "REJECTED", "CANCEL_REJECTED", "INTRREJECTED", "UNDEFINED_STATUS" };
const static char OPEN_CLOSE_STR[][16] = { "OPEN", "CLOSE", "CLOSE_TOD", "CLOSE_YES" };
const static char BUY_SELL_STR[][8] = { "BUY", "SELL" };

extern bool debug_mode;

#define DEBUG_MODE -1

CheckHandler::CheckHandler(int type, int length, void * cfg)
{
	if (length == DEBUG_MODE)
		debug_mode = true;

	m_orders = new OrderHash();
	m_contracts = new MyHash<Contract>(1024);

	m_config = *((st_config_t*)cfg);
	m_send_resp_func = m_config.pass_rsp_hdl;
	m_send_order_func = m_config.proc_order_hdl;
	m_send_info_func = m_config.send_info_hdl;
	send_log = m_config.send_info_hdl;

	for (int i = 0; i < ACCOUNT_MAX; i++) {
		if (m_config.accounts[i].account[0] == '\0') break;
		account_t& l_account = m_accounts.get_next_free_node();
		strlcpy(l_account.account, m_config.accounts[i].account, ACCOUNT_LEN);
		l_account.cash_available = m_config.accounts[i].cash_available;
		l_account.cash_asset = m_config.accounts[i].cash_asset;
		l_account.exch_rate = m_config.accounts[i].exch_rate;
		l_account.currency = m_config.accounts[i].currency;
		m_accounts.insert_current_node(l_account.account);

		PRINT_INFO("account: %s, cash_available: %f, cash_asset: %f, exch_rate: %f, currency: %d",
			l_account.account, l_account.cash_available, l_account.cash_asset, l_account.exch_rate, l_account.currency);
	}

	for (int i = 0; i < SYMBOL_MAX; i++) {
		if (m_config.contracts[i].symbol[0] == '\0') break;
		Contract& l_instr = m_contracts->get_next_free_node();
		contract_t& l_config_instr = m_config.contracts[i];
		strlcpy(l_instr.symbol, l_config_instr.symbol, SYMBOL_LEN);
		strlcpy(l_instr.account, l_config_instr.account, SYMBOL_LEN);
		l_instr.exch = (EXCHANGE)l_config_instr.exch;
		l_instr.max_accum_open_vol = l_config_instr.max_accum_open_vol;
		l_instr.max_cancel_limit = l_config_instr.max_cancel_limit;
		l_instr.order_cum_open_vol = 0;
		l_instr.expiration_date = l_config_instr.expiration_date;
		
		l_instr.pre_long_qty_remain = l_instr.pre_long_position = l_config_instr.yesterday_pos.long_volume;
		l_instr.pre_short_qty_remain = l_instr.pre_short_position = l_config_instr.yesterday_pos.short_volume;
		l_instr.positions[LONG_OPEN].qty = l_config_instr.today_pos.long_volume;
		l_instr.positions[LONG_OPEN].notional = l_config_instr.today_pos.long_price * l_config_instr.today_pos.long_volume;
		l_instr.positions[SHORT_OPEN].qty = l_config_instr.today_pos.short_volume;
		l_instr.positions[SHORT_OPEN].notional = l_config_instr.today_pos.short_price * l_config_instr.today_pos.short_volume;
		l_instr.positions[LONG_CLOSE].qty = 0;
		l_instr.positions[LONG_CLOSE].notional = 0;
		l_instr.positions[SHORT_CLOSE].qty = 0;
		l_instr.positions[SHORT_CLOSE].notional = 0;

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
			"pre_long_pos: %d, pre_short_pos: %d, pre_long_qty_remain: %d, pre_short_qty_remain: %d, today_long_pos: %d, today_long_price: %f, today_short_pos: %d, today_short_price: %f, "
			"fee_by_lot: %d, exchange_fee : %f, yes_exchange_fee : %f, broker_fee : %f, stamp_tax : %f, acc_transfer_fee : %f, tick_size : %f, multiplier : %f",
			l_instr.symbol, l_instr.account, l_instr.exch, l_instr.max_accum_open_vol, l_instr.max_cancel_limit, l_instr.order_cum_open_vol, l_instr.expiration_date,
			l_instr.pre_long_position, l_instr.pre_short_position, l_instr.pre_long_qty_remain, l_instr.pre_short_qty_remain, l_config_instr.today_pos.long_volume, l_config_instr.today_pos.long_price, l_config_instr.today_pos.short_volume, l_config_instr.today_pos.short_price,
			l_instr.fee_by_lot, l_instr.exchange_fee, l_instr.yes_exchange_fee, l_instr.broker_fee, l_instr.stamp_tax, l_instr.acc_transfer_fee, l_instr.tick_size, l_instr.multiplier);
	}
}

CheckHandler::~CheckHandler()
{
	PRINT_INFO("It is over: %s", __FUNCTION__);
	delete_mem(m_orders);
	delete_mem(m_contracts);
}

int CheckHandler::on_book(int type, int length, void * book)
{
	int ret = 0;
	switch (type) {
		case S_PLACE_ORDER_DEFAULT: {
			ret = process_send_order((st_data_t*)book);
			break;
		}
		case S_CANCEL_ORDER_DEFAULT: {
			ret = process_cancel_order((st_data_t*)book);
			break;
		}
		case DEFAULT_FUTURE_QUOTE: {
			Futures_Internal_Book *f_book = (Futures_Internal_Book *)((st_data_t*)book)->info;
			int_time = f_book->int_time;
			break;
		}
		case DEFAULT_STOCK_QUOTE: {
			Stock_Internal_Book *s_book = (Stock_Internal_Book *)((st_data_t*)book)->info;
			int_time = s_book->exch_time;
			break;
		}
		default: {
			m_send_info_func(type, length, book);
		}
	}
	return ret;
}

int CheckHandler::on_response(int type, int length, void * resp)
{
	st_response_t* l_resp = (st_response_t*)((st_data_t*)resp)->info;

	PRINT_INFO("Order Resp: %lld %s %s %s %f %d %s %d %s", l_resp->order_id, l_resp->symbol,
		BUY_SELL_STR[l_resp->direction], OPEN_CLOSE_STR[l_resp->open_close], l_resp->exe_price, l_resp->exe_volume,
		STATUS[l_resp->status], l_resp->error_no, l_resp->error_info);
		
	process_order_resp(l_resp);

	Contract *instr = &m_contracts->at(l_resp->symbol);
	PRINT_INFO("Contract: %s pre_long_remain: %d pre_short_remain: %d long_pos: %d short_pos: %d long_open: %d long_close: %d short_open: %d short_close: %d\n"
		"Account: %s cash_available: %f",
		instr->symbol, instr->pre_long_qty_remain, instr->pre_short_qty_remain, long_position(instr), short_position(instr), instr->positions[LONG_OPEN].qty, instr->positions[LONG_CLOSE].qty, instr->positions[SHORT_OPEN].qty, instr->positions[SHORT_CLOSE].qty,
		instr->account, m_accounts.at(instr->account).cash_available);
		
	m_send_resp_func(type, length, resp);
	
	return 0;
}

int CheckHandler::process_send_order(st_data_t * book)
{
	order_t *ord = (order_t*)book->info;

	PRINT_INFO("Send Order: %c %s %d %f %s %s %d %d %d %lld %lld %lld", ord->exch, ord->symbol,
		ord->volume, ord->price, BUY_SELL_STR[ord->direction], OPEN_CLOSE_STR[ord->open_close],
		ord->investor_type, ord->order_type, ord->time_in_force,
		ord->st_id, ord->order_id, ord->org_ord_id);

	/*compliance check before adding m_shannon_ord_count inside shannon_get_sig_id*/
	int l_compliance = compliance_check(ord->exch, ord->symbol, ord->volume, ord->price, ord->direction, ord->open_close);
	if (l_compliance != 0) {
		print_failed_order_info(l_compliance);
		return l_compliance;
	}

	Contract& l_instr = m_contracts->at(ord->symbol);
	if (ord->open_close == ORDER_OPEN) {
		l_instr.order_cum_open_vol += ord->volume;
	}

	/*if pass check, send order to match engine*/
	m_send_order_func(S_PLACE_ORDER_DEFAULT, sizeof(st_data_t), (void*)book);

	Order* l_order = m_orders->update_order(reverse_index(ord->order_id), &l_instr, ord->price, ord->volume,
		(DIRECTION)ord->direction, ord->order_id, (OPEN_CLOSE)ord->open_close);

	if(ord->open_close == ORDER_CLOSE_YES)
		update_yes_pos(&l_instr, (DIRECTION)ord->direction, ord->volume);

	update_account_cash(l_order);

	return 0;
}

int CheckHandler::process_cancel_order(st_data_t * book)
{
	order_t *ord = (order_t*)book->info;

	PRINT_INFO("Cancel Order: %c %s %d %f %s %s %d %d %d %lld %lld %lld", ord->exch, ord->symbol,
		ord->volume, ord->price, BUY_SELL_STR[ord->direction], OPEN_CLOSE_STR[ord->open_close],
		ord->investor_type, ord->order_type, ord->time_in_force,
		ord->st_id, ord->order_id, ord->org_ord_id);

	int ret = 0;
	Order* l_order = m_orders->query_order(ord->org_ord_id);
	if(l_order != NULL && is_cancelable(l_order)) {
		if(l_order->contr->max_cancel_limit > 0) {
			l_order->pending_cancel = true;
			m_send_order_func(S_CANCEL_ORDER_DEFAULT, sizeof(st_data_t), book);
		}
		else {
			ord->order_id = ord->org_ord_id;
			print_failed_order_info(ERROR_CANCEL_ORDER_REARCH_LIMIT);
			ret = ERROR_CANCEL_ORDER_REARCH_LIMIT;
		}
	} else {
		ord->order_id = ord->org_ord_id;
		print_failed_order_info(ERROR_CANCEL_ORDER_FAIL);
		ret = ERROR_CANCEL_ORDER_FAIL;
	}
	
	return ret;
}

bool CheckHandler::process_order_resp(st_response_t * ord_resp)
{
	Order* l_order = m_orders->query_order(ord_resp->order_id);
	if (l_order == NULL)
		return false;

	if (ord_resp->open_close == ORDER_CLOSE_YES) {
		if (ord_resp->status == SIG_STATUS_CANCELED
			|| (ord_resp->status == SIG_STATUS_REJECTED && !l_order->pending_cancel)
			|| ord_resp->status == SIG_STATUS_INTRREJECTED) {
			// Get Contract, and update this position.
			Contract& l_instr = *l_order->contr;
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

		// Get Contract, and update this position.
		Contract& l_instr = *l_order->contr;
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
			else if (ord_resp->open_close == ORDER_CLOSE) {
				l_instr.positions[LONG_CLOSE].notional += (l_notional_change + l_close_fees);
				l_instr.positions[LONG_CLOSE].qty += ord_resp->exe_volume;
			}
			else if (ord_resp->open_close == ORDER_CLOSE_YES) {
				l_instr.positions[LONG_CLOSE].notional += (l_notional_change + l_close_fees);
				l_instr.positions[LONG_CLOSE].qty += ord_resp->exe_volume;
				l_instr.pre_short_position -= ord_resp->exe_volume;
			}
		}
		else if (ord_resp->direction == ORDER_SELL) {
			l_fees += l_notional_change * l_instr.stamp_tax; /* For stocks, there's stamp tax when selling */

			if (ord_resp->open_close == ORDER_OPEN) {
				l_instr.positions[SHORT_OPEN].notional += (l_notional_change - l_fees);
				l_instr.positions[SHORT_OPEN].qty += ord_resp->exe_volume;
			}
			else if (ord_resp->open_close == ORDER_CLOSE) {
				l_instr.positions[SHORT_CLOSE].notional += (l_notional_change - l_close_fees);
				l_instr.positions[SHORT_CLOSE].qty += ord_resp->exe_volume;
			}
			else if (ord_resp->open_close == ORDER_CLOSE_YES) {
				l_instr.positions[SHORT_CLOSE].notional += (l_notional_change - l_close_fees);
				l_instr.positions[SHORT_CLOSE].qty += ord_resp->exe_volume;
				l_instr.pre_long_position -= ord_resp->exe_volume;
			}
		}
	}

	if (l_order->status != SIG_STATUS_INIT && ord_resp->status == SIG_STATUS_ENTRUSTED) {
		/* If already entrusted, no need to call update_order_list() */
	}
	else {
		l_order->status = (ORDER_STATUS)ord_resp->status;
		update_account_cash(l_order);
	}

	update_pending_cancel_status(l_order);
	m_orders->update_order_list(l_order);
	return true;
}

void CheckHandler::update_account_cash(Order* a_order)
{
	if (a_order == NULL) {
		LOG_LN("update_account_cash: order_t is NULL");
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

void CheckHandler::update_pending_cancel_status(Order * a_order)
{
	/*Update pending order size*/
	switch (a_order->status)
	{
		case SIG_STATUS_ENTRUSTED: {
			break;
		}
		case SIG_STATUS_SUCCEED: {
			if (a_order->pending_cancel == true)
				a_order->pending_cancel = false;
			break;
		}
		case SIG_STATUS_PARTED: {
			break;
		}
		case SIG_STATUS_CANCELED: {
			if (a_order->pending_cancel == true) {
				//Sometimes in SHFE we will receive cancel response after we send a order. Here We recognize the cancel as rejected.
				a_order->pending_cancel = false;
				a_order->contr->max_cancel_limit--;
			}
			break;
		}
		case SIG_STATUS_INTRREJECTED:
		case SIG_STATUS_REJECTED: {
			if (a_order->pending_cancel == true) { //Cancel reject
				a_order->pending_cancel = false;
			}
			break;
		}
		case SIG_STATUS_CANCEL_REJECTED: {
			a_order->pending_cancel = false;
			break;
		}
	}
}

void CheckHandler::print_failed_order_info(int check_code)
{
	switch (check_code) {
		case ERROR_OPEN_POS_IS_NOT_ENOUGH: PRINT_WARN("Not enouph position to close!");  break;
		case ERROR_SELF_MATCHING: PRINT_WARN("Self matching check fail!");  break;
		case ERROR_REARCH_MAX_ACCUMULATE_OPEN_VAL: PRINT_WARN("Max accumulate vol check fail!"); break;
		case ERROR_CANCEL_ORDER_FAIL: PRINT_WARN("Cancel order fail!"); break;
		case ERROR_CANCEL_ORDER_REARCH_LIMIT: PRINT_WARN("Cancel order fail, reach max limit!"); break;
		case ERROR_CASH_IS_NOT_ENOUGH: PRINT_WARN("Current cash is not enough to buy/sell!"); break;
		case ERROR_PRE_POS_IS_NOT_ENOUGH_TO_CLOSE: PRINT_WARN("Remaining pre position is not enough to close!"); break;
	}
}

int CheckHandler::compliance_check(char exch, char * symbol, int volume, double price, int direct, int open_close_flag)
{
	Contract& l_instr = m_contracts->at(symbol);

	/* Check remaining pre long position for stocks - cannot sell more than yesterday's position */
	if (exch == SZSE || exch == SSE) {
		// Check whether we have enough cash
		if (direct == ORDER_BUY) {
			account_t& l_account = m_accounts.at(l_instr.account);
			if (double_compare(l_account.cash_available, price * volume * l_instr.multiplier) < 0)
				return ERROR_CASH_IS_NOT_ENOUGH;
		}
		else if (direct == ORDER_SELL) {  	// Check whether we have enough stocks to sell
			if (l_instr.pre_long_qty_remain < volume)
				return ERROR_PRE_POS_IS_NOT_ENOUGH_TO_CLOSE;
		}
	}
	else if (open_close_flag == ORDER_OPEN) {  // For other exchanges, if we send order to open positions, we need to check if we have enough cash to be buy/sell.
		account_t& l_account = m_accounts.at(l_instr.account);
		if (double_compare(l_account.cash_available, price * volume * l_instr.multiplier) < 0)
			return ERROR_CASH_IS_NOT_ENOUGH;
	}
	
	/*Open position is not enough for closing*/
	if(open_close_flag == ORDER_CLOSE || open_close_flag == ORDER_CLOSE_YES) {
		int open_val = position((DIRECTION)TURNOVER_DIRECTION(direct), &l_instr);
		if (open_val < volume) {
			return ERROR_OPEN_POS_IS_NOT_ENOUGH;
        }
	}

	if(open_close_flag == ORDER_CLOSE_YES) {
		if (direct == ORDER_BUY) {
			if (l_instr.pre_short_position < volume) {
				return ERROR_PRE_POS_IS_NOT_ENOUGH_TO_CLOSE;
			}
		}
		else if(direct == ORDER_SELL) {
			if (l_instr.pre_long_position < volume) {
				return ERROR_PRE_POS_IS_NOT_ENOUGH_TO_CLOSE;
			}
		}
	}
	
	/*Self matching check*/
	list_t ret_list;
	m_orders->search_order_by_side_instr((DIRECTION)TURNOVER_DIRECTION(direct), symbol, &ret_list);   //convert from turing direction 0/1 to normal 1/2
	list_t *pos, *n;
	Order *l_ord;

	list_for_each_prev_safe(pos, n, &ret_list) {
		l_ord = list_entry(pos, Order, search_link);
		if (my_strcmp(symbol, l_ord->symbol) == 0) {
			if ((direct == ORDER_BUY) && (double_compare(price, l_ord->price) >= 0)) {//BUY
				return ERROR_SELF_MATCHING;
			}
			if ((direct == ORDER_SELL) && (double_compare(price, l_ord->price) <= 0)) {//SELL
				return ERROR_SELF_MATCHING;
			}
		}
	}
	/*Max accumulate vol allowed to open check*/
	if (open_close_flag == ORDER_OPEN) {  //OPEN
		if (l_instr.order_cum_open_vol + volume >= l_instr.max_accum_open_vol) {
			return ERROR_REARCH_MAX_ACCUMULATE_OPEN_VAL;
		}
	}
	return 0;
}

void CheckHandler::update_yes_pos(Contract * instr, DIRECTION side, int size)
{
	if (side == ORDER_BUY) {
		instr->pre_short_qty_remain -= size;
	}
	else if (side == ORDER_SELL) {
		instr->pre_long_qty_remain -= size;
	}
}