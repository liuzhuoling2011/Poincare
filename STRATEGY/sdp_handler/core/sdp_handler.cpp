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
static special_order_t g_special_order = { 0 };

SDPHandler::SDPHandler(int type, int length, void * cfg)
{
	if (length == DEBUG_MODE)
		debug_mode = true;

	m_orders = new OrderHash();
	m_contracts = new MyHash<Contract>(1024);
	m_config = (st_config_t*)cfg;
    //从shannon 传过来的cfg中解析出这几个函数。
	m_send_resp_func = m_config->pass_rsp_hdl;
    //发单撤单函数
	m_send_order_func = m_config->proc_order_hdl;
	m_send_info_func = m_config->send_info_hdl;
    //日志函数 
	send_log = m_config->send_info_hdl;

	m_strat_id = m_config->vst_id;
	PRINT_INFO("Processing: %d %s", m_config->trading_date, m_config->day_night == 0 ? "Day":"Night");

	for (int i = 0; i < ACCOUNT_MAX; i++) {
		if (m_config->accounts[i].account[0] == '\0') break;
        //填写account信息到 account_t,l_account 
		account_t& l_account = m_accounts.get_next_free_node();
		strlcpy(l_account.account, m_config->accounts[i].account, ACCOUNT_LEN);
		l_account.cash_available = m_config->accounts[i].cash_available;
		l_account.cash_asset = m_config->accounts[i].cash_asset;
		l_account.exch_rate = m_config->accounts[i].exch_rate;
		l_account.currency = m_config->accounts[i].currency;
		m_accounts.insert_current_node(l_account.account);

		PRINT_INFO("account: %s, cash_available: %f, cash_asset: %f, exch_rate: %f, currency: %d",
			l_account.account, l_account.cash_available, l_account.cash_asset, l_account.exch_rate, l_account.currency);
		LOG_LN("account: %s, cash_available: %f, cash_asset: %f, exch_rate: %f, currency: %d",
			l_account.account, l_account.cash_available, l_account.cash_asset, l_account.exch_rate, l_account.currency);
	}

	for (int i = 0; i < SYMBOL_MAX; i++) {
		if (m_config->contracts[i].symbol[0] == '\0') break;
        //完善合约信息 到 Contract&  l_instr 
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
		
		l_instr.pre_long_qty_remain = l_instr.pre_long_position =  l_config_instr.yesterday_pos.long_volume;
		l_instr.pre_short_qty_remain = l_instr.pre_short_position = l_config_instr.yesterday_pos.short_volume;
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
			"pre_long_position: %d, pre_short_position: %d, pre_long_qty_remain: %d, pre_short_qty_remain: %d, today_long_pos: %d, today_long_price: %f, today_short_pos: %d, today_short_price: %f, "
			"fee_by_lot: %d, exchange_fee : %f, yes_exchange_fee : %f, broker_fee : %f, stamp_tax : %f, acc_transfer_fee : %f, tick_size : %f, multiplier : %f",
			l_instr.symbol, l_instr.account, l_instr.exch, l_instr.max_accum_open_vol, l_instr.max_cancel_limit, l_instr.order_cum_open_vol, l_instr.expiration_date,
			l_instr.pre_long_position, l_instr.pre_short_position, l_instr.pre_long_qty_remain, l_instr.pre_short_qty_remain, l_config_instr.today_pos.long_volume, l_config_instr.today_pos.long_price, l_config_instr.today_pos.short_volume, l_config_instr.today_pos.short_price,
			l_instr.fee_by_lot, l_instr.exchange_fee, l_instr.yes_exchange_fee, l_instr.broker_fee, l_instr.stamp_tax, l_instr.acc_transfer_fee, l_instr.tick_size, l_instr.multiplier);
		LOG_LN("symbol: %s, account: %s, exch: %c, max_accum_open_vol: %d, max_cancel_limit: %d, order_cum_open_vol: %d, expiration_date: %d, "
			"pre_long_position: %d, pre_short_position: %d, pre_long_qty_remain: %d, pre_short_qty_remain: %d, today_long_pos: %d, today_long_price: %f, today_short_pos: %d, today_short_price: %f, "
			"fee_by_lot: %d, exchange_fee : %f, yes_exchange_fee : %f, broker_fee : %f, stamp_tax : %f, acc_transfer_fee : %f, tick_size : %f, multiplier : %f",
			l_instr.symbol, l_instr.account, l_instr.exch, l_instr.max_accum_open_vol, l_instr.max_cancel_limit, l_instr.order_cum_open_vol, l_instr.expiration_date,
			l_instr.pre_long_position, l_instr.pre_short_position, l_instr.pre_long_qty_remain, l_instr.pre_short_qty_remain, l_config_instr.today_pos.long_volume, l_config_instr.today_pos.long_price, l_config_instr.today_pos.short_volume, l_config_instr.today_pos.short_price,
			l_instr.fee_by_lot, l_instr.exchange_fee, l_instr.yes_exchange_fee, l_instr.broker_fee, l_instr.stamp_tax, l_instr.acc_transfer_fee, l_instr.tick_size, l_instr.multiplier);
	}
}

SDPHandler::~SDPHandler()
{
	LOG_LN("It is over: %s", __FUNCTION__);
	delete_mem(m_orders);
	delete_mem(m_contracts);
}


// 在策略的my on book中会调用这个函数，简单处理下行情
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
	}
	return ret;
}

int SDPHandler::on_response(int type, int length, void * resp)
{
    //从 shannon 接口 读来的 回报
	st_response_t* l_resp = (st_response_t*)((st_data_t*)resp)->info;
	
	PRINT_INFO("Order Resp: %lld %s %s %s %f %d %s %d %s", l_resp->order_id, l_resp->symbol,
		BUY_SELL_STR[l_resp->direction], OPEN_CLOSE_STR[l_resp->open_close], l_resp->exe_price, l_resp->exe_volume,
		STATUS[l_resp->status], l_resp->error_no, l_resp->error_info);
	LOG_LN("Order Resp: %lld %s %s %s %f %d %s %d %s", l_resp->order_id, l_resp->symbol,
		BUY_SELL_STR[l_resp->direction], OPEN_CLOSE_STR[l_resp->open_close], l_resp->exe_price, l_resp->exe_volume,
		STATUS[l_resp->status], l_resp->error_no, l_resp->error_info);
    //处理回报函数，包括订单仓位管理的逻辑
	process_order_resp(l_resp);

    //找到回报中提到的合约打印出来。
	Contract *instr = find_contract(l_resp->symbol);
	PRINT_INFO("Contract: %s pre_long_pos: %d pre_short_pos: %d pre_long_remain: %d pre_short_remain: %d long_pos: %d short_pos: %d long_open: %d long_close: %d short_open: %d short_close: %d\n"
		"pending_buy_open_size: %d pending_sell_open_size: %d pending_buy_close_size: %d pending_sell_close_size: %d\n"
		"Account: %s cash_available: %f",
		instr->symbol, instr->pre_long_position, instr->pre_short_position, instr->pre_long_qty_remain, instr->pre_short_qty_remain, long_position(instr), short_position(instr), instr->positions[LONG_OPEN].qty, instr->positions[LONG_CLOSE].qty, instr->positions[SHORT_OPEN].qty, instr->positions[SHORT_CLOSE].qty,
		instr->pending_buy_open_size, instr->pending_sell_open_size, instr->pending_buy_close_size, instr->pending_sell_close_size,
		instr->account, get_account_cash(instr->account));
	LOG_LN("Contract: %s pre_long_pos: %d pre_short_pos: %d pre_long_remain: %d pre_short_remain: %d long_pos: %d short_pos: %d long_open: %d long_close: %d short_open: %d short_close: %d\n"
		"pending_buy_open_size: %d pending_sell_open_size: %d pending_buy_close_size: %d pending_sell_close_size: %d\n"
		"Account: %s cash_available: %f",
		instr->symbol, instr->pre_long_position, instr->pre_short_position, instr->pre_long_qty_remain, instr->pre_short_qty_remain, long_position(instr), short_position(instr), instr->positions[LONG_OPEN].qty, instr->positions[LONG_CLOSE].qty, instr->positions[SHORT_OPEN].qty, instr->positions[SHORT_CLOSE].qty,
		instr->pending_buy_open_size, instr->pending_sell_open_size, instr->pending_buy_close_size, instr->pending_sell_close_size,
		instr->account, get_account_cash(instr->account));
	return 0;
}

//取得某一个合约某一个方向的仓位。
int get_pos_by_side(Contract *a_instr, DIRECTION side)
{
	if (side == ORDER_BUY)
		return long_position(a_instr);
	else if (side == ORDER_SELL)
		return short_position(a_instr);

	return 0;
}

//封装shannon传过来的发单函数
int SDPHandler::send_single_order(Contract * instr, EXCHANGE exch, double price, int size, DIRECTION side, OPEN_CLOSE sig_openclose, bool flag_close_yesterday_pos, bool flag_syn_cancel, INVESTOR_TYPE investor_type, ORDER_TYPE order_type, TIME_IN_FORCE time_in_force)
{
	if (size <= 0) {
		if (size < 0)
			LOG_LN("OrderSize is invalid, OrderSize < 0:%d", size);
		return -1;
	}

	if (price <= 0) {
		LOG_LN("Price is invalid, price <= 0: %.2f!", price);
		return -1;
	}

	if (flag_syn_cancel == true && m_orders->get_pending_cancel_order_size_by_instr(instr) > 0) {
		LOG_LN("Don't send orders before receiving order cancelled.");
		return -1;
	}

	if (m_orders->full() == true) {
		LOG_LN("You have too much pending orders, exit!");
		return -1;
	}

	g_st_data_t.info = (void*)&g_send_order;

	uint64_t l_order_id;
	int l_size, l_order_count = 0;
	int l_single_order_max_vol = SINGLE_ORDER_FUTURE_MAX_VAL;
	if (exch == SSE || exch == SZSE)
		l_single_order_max_vol = SINGLE_ORDER_STOCK_MAX_VAL;

	for (int l_remaining_size = size; l_remaining_size > 0; l_remaining_size -= l_single_order_max_vol, l_order_count++) {
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
				LOG_LN("%s reached max vol %d allowed to open, Strategy %d send order failed, entering exit mode", instr->symbol, instr->max_accum_open_vol, m_strat_id);
			}
			case ERROR_OPEN_POS_IS_NOT_ENOUGH: 
			case ERROR_SELF_MATCHING:
			case ERROR_CASH_IS_NOT_ENOUGH:
			case ERROR_PRE_POS_IS_NOT_ENOUGH_TO_CLOSE: {
				LOG_LN("%d %s Order %d@%f %s %s compliance check failed: %d",
					m_strat_id, instr->symbol, l_size, price,
					BUY_SELL_STR[side], OPEN_CLOSE_STR[sig_openclose], l_compliance);
				l_order_id = 0;
				break;
			}
			case 0: break;
		}

		if (l_order_id != 0) {
			/* If successfully sent an order, make a record of it */
			if (l_order_count >= MAX_ORDER_COUNT - 1) {
				LOG_LN("Too large order size, send failed!");
				return -3;
			}
			m_cur_ord_id_arr[l_order_count] = l_order_id;
			m_cur_ord_size_arr[l_order_count] = l_size;
			m_total_order_size += l_size;
		}
		else {
			LOG_LN("%s OrderID is 0. The order was sent unsuccessfully", instr->symbol);
			break;
		}
	}

	if (l_order_count > 0) {
		for (int i = 0; i < l_order_count; i++) {
            PRINT_INFO("%d OrderID: %lld Sent single order, %s %s %s %d@%f", m_strat_id, m_cur_ord_id_arr[i], instr->symbol,
                BUY_SELL_STR[side], OPEN_CLOSE_STR[sig_openclose], m_cur_ord_size_arr[i], price);
			LOG_LN("%d OrderID: %lld Sent single order, %s %s %s %d@%f", m_strat_id, m_cur_ord_id_arr[i], instr->symbol,
				BUY_SELL_STR[side], OPEN_CLOSE_STR[sig_openclose], m_cur_ord_size_arr[i], price); 
			int index = reverse_index(m_cur_ord_id_arr[i]);
			Order *l_order = m_orders->update_order(index, instr, price, m_cur_ord_size_arr[i],
				side, m_cur_ord_id_arr[i], sig_openclose);
			l_order->insert_time = int_time;

			if (sig_openclose == ORDER_CLOSE_YES || flag_close_yesterday_pos) {
				update_yes_pos(instr, side, m_cur_ord_size_arr[i]);
				l_order->close_yes_pos_flag = true;
			}
			update_account_cash(l_order);

			char* all_order_info = m_orders->get_all_active_order_info();
			PRINT_DEBUG("%s", all_order_info);
			LOG_LN("%s", all_order_info);
		}

		m_new_order_times += l_order_count;
		return 0;
	}
	else {
		return -1;
	}
}

int SDPHandler::send_dma_order(Contract * instr, DIRECTION side, int size, double price, bool flag_syn_cancel)
{
	g_st_data_t.info = (void*)&g_special_order;

	g_special_order.exch = instr->exch;
	strlcpy(g_special_order.symbol, (char *)instr->symbol, SYMBOL_LEN);
	g_special_order.volume = size;
	g_special_order.price = price;
	g_special_order.direction = side;
	g_special_order.sync_cancel = flag_syn_cancel;


	m_send_order_func(S_PLACE_ORDER_DMA, sizeof(g_st_data_t), (void *)&g_st_data_t);
	if(g_special_order.status == 0) {
		LOG_LN("Sent DMA Order, %s %s %d@%f %d", instr->symbol, (side == ORDER_BUY ? "Buy" : "Sell"), size, price, flag_syn_cancel);
	}
	return g_special_order.status;
}

int SDPHandler::send_twap_order(Contract * instr, int ttl_odrsize, DIRECTION side, double price, int start_time, int end_time)
{
	g_st_data_t.info = (void*)&g_special_order;

	g_special_order.exch = instr->exch;
	strlcpy(g_special_order.symbol, (char *)instr->symbol, SYMBOL_LEN);
	g_special_order.volume = ttl_odrsize;
	g_special_order.price = price;
	g_special_order.direction = side;
	g_special_order.start_time = start_time;
	g_special_order.end_time = end_time;

	m_send_order_func(S_PLACE_ORDER_TWAP, sizeof(g_st_data_t), (void *)&g_st_data_t);
	if (g_special_order.status == 0) {
		LOG_LN("Sent TWAP Order, %s %s %d@%f %d %d", instr->symbol, (side == ORDER_BUY ? "Buy" : "Sell"), ttl_odrsize, price, start_time, end_time);
	}
	return g_special_order.status;
}

int SDPHandler::send_vwap_order(Contract * instr, int ttl_odrsize, DIRECTION side, double price, int start_time, int end_time)
{
	g_st_data_t.info = (void*)&g_special_order;

	g_special_order.exch = instr->exch;
	strlcpy(g_special_order.symbol, (char *)instr->symbol, SYMBOL_LEN);
	g_special_order.volume = ttl_odrsize;
	g_special_order.price = price;
	g_special_order.direction = side;
	g_special_order.start_time = start_time;
	g_special_order.end_time = end_time;

	m_send_order_func(S_PLACE_ORDER_VWAP, sizeof(g_st_data_t), (void *)&g_st_data_t);
	if (g_special_order.status == 0) {
		LOG_LN("Sent VWAP Order, %s %s %d@%f %d %d", instr->symbol, (side == ORDER_BUY ? "Buy" : "Sell"), ttl_odrsize, price, start_time, end_time);
	}
	return g_special_order.status;
}
//封装shannon传过来的撤单函数
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
		LOG_LN("%d OrderID: %lld Cancel order compliance check failed: %d",
			m_strat_id, a_order->signal_id, l_compliance);
		/* Not Compliant */
		is_cancel_compliant = false;

		if (ERROR_CANCEL_ORDER_REARCH_LIMIT == l_compliance) {
			is_exit_mode = true;
			LOG_LN("%s orderID: %lld reached max cancel allowed, Strategy %d cancel order failed, entering exit mode", a_order->symbol, a_order->signal_id, m_strat_id);
		}
	}

	// If we are cancelling a close order, mark it flag to true

	if (is_cancel_compliant) {
		m_cancelled_times++;
		m_total_cancel_size += leaves_qty(a_order);
		a_order->pending_cancel = true;
		m_orders->minus_pending_size(a_order, leaves_qty(a_order));
        PRINT_INFO("%d OrderID: %lld Cancel single order", m_strat_id, a_order->signal_id);
		LOG_LN("%d OrderID: %lld Cancel single order", m_strat_id, a_order->signal_id);
		return 0;
	} else {
		return -1;
	}
}
int SDPHandler::cancel_orders_with_dif_px(Contract * instr, DIRECTION side, double price, int tolerance, int & pending_open_size, int & pending_close_size, int & pending_close_yes_size)
{
	list_head *cancel_list = m_orders->get_order_by_side(side);
	struct list_head *pos, *n;
	Order *l_ord;

	list_for_each_safe(pos, n, cancel_list) {
		l_ord = list_entry(pos, Order, pd_link);
		// The Contract Address is the same, Save time do string compare
		if (l_ord->contr == instr) {
			double tolerant_px = instr->tick_size * tolerance;
			if (l_ord->price <= (price + tolerant_px) && l_ord->price >= (price - tolerant_px) && is_cancelable(l_ord) && l_ord->openclose == ORDER_OPEN) {
				pending_open_size += leaves_qty(l_ord);
			}
			else if (l_ord->price <= (price + tolerant_px) && l_ord->price >= (price - tolerant_px) && is_cancelable(l_ord) && l_ord->openclose == ORDER_CLOSE) {
				pending_close_size += leaves_qty(l_ord);
				if (l_ord->close_yes_pos_flag)
					pending_close_yes_size += leaves_qty(l_ord);
			}
			else {
				cancel_single_order(l_ord);
			}
		}
	}

	return pending_open_size + pending_close_size;
}

//调用cancel_single_order. 撤掉某一合约某一个方向的单。
int SDPHandler::cancel_orders_by_side(Contract * instr, DIRECTION side, OPEN_CLOSE flag)
{
	struct list_head *pos, *n;
	Order *l_ord;

	list_head *ord_list = m_orders->get_order_by_side(side);
	list_for_each_prev_safe(pos, n, ord_list) {
		l_ord = list_entry(pos, Order, pd_link);
		if (l_ord->contr == instr && l_ord->openclose == flag) {
			cancel_single_order(l_ord);
		}
	}

	return 0;
}
//按照给定的价格，撤掉某一个合约的某一个方向的单
int SDPHandler::cancel_orders_with_dif_px(Contract * instr, DIRECTION side, double price)
{
	list_head *cancel_list = m_orders->get_order_by_side(side);
	struct list_head *pos, *n;
	Order *l_ord;
	int l_size = 0;

	list_for_each_prev_safe(pos, n, cancel_list) {
		l_ord = list_entry(pos, Order, pd_link);
		// The Contract Address is the same, save time do string compare
		if (l_ord->contr == instr) {
			if (double_compare(l_ord->price, price) == 0 && is_cancelable(l_ord)) {
				l_size += leaves_qty(l_ord);
			}
			else {
				cancel_single_order(l_ord);
			}
		}
	}

	return l_size;
}
//撤掉某一个方向 的 某一个合约的所有的单
int SDPHandler::cancel_orders_by_side(Contract * instr, DIRECTION side)
{
	struct list_head *pos, *n;
	Order *l_ord;

	list_head *ord_list = m_orders->get_order_by_side(side);
	list_for_each_prev_safe(pos, n, ord_list) {
		l_ord = list_entry(pos, Order, pd_link);
		// The Contract Address is the same�� Save time do string compare
		if (l_ord->contr == instr) {
			cancel_single_order(l_ord);
		}
	}

	return 0;
}
//撤掉所有的订单
int SDPHandler::cancel_all_orders()
{
	list_t *pos, *n;
	Order *l_ord;

	list_head *buy_list = m_orders->get_order_by_side(ORDER_BUY);
	list_for_each_prev_safe(pos, n, buy_list) {
		l_ord = list_entry(pos, Order, pd_link);
		cancel_single_order(l_ord);
	}

	list_head *sell_list = m_orders->get_order_by_side(ORDER_SELL);
	list_for_each_prev_safe(pos, n, sell_list) {
		l_ord = list_entry(pos, Order, pd_link);
		cancel_single_order(l_ord);
	}

	return 0;
}
//撤掉某一个合约所有的单
int SDPHandler::cancel_all_orders(Contract * instr)
{
	list_t *pos, *n;
	Order *l_ord;

	list_head *buy_list = m_orders->get_order_by_side(ORDER_BUY);
	list_for_each_prev_safe(pos, n, buy_list) {
		l_ord = list_entry(pos, Order, pd_link);
		// The Contract Address is the same, Save time do string compare
		if (l_ord->contr == instr)
			cancel_single_order(l_ord);
	}

	list_head *sell_list = m_orders->get_order_by_side(ORDER_SELL);
	list_for_each_prev_safe(pos, n, sell_list) {
		l_ord = list_entry(pos, Order, pd_link);
		// The Contract Address is the same, Save time do string compare
		if (l_ord->contr == instr)
			cancel_single_order(l_ord);
	}

	return 0;
}

void SDPHandler::cancel_old_order(int tick_time)
{
	list_t *pos, *n;
	Order *l_ord;

	list_t *ord_list = m_orders->get_order_by_side(ORDER_BUY);
	list_for_each_prev_safe(pos, n, ord_list) {
		l_ord = list_entry(pos, Order, pd_link);
		if (tick_time >= get_seconds_from_int_time(l_ord->insert_time) + 1) {
			PRINT_ERROR("Order in OrderList Waiting over 1 Secs; cur time %d, insert time %d, Cancelling...\n", int_time, get_seconds_from_int_time(l_ord->insert_time));
			LOG_LN("Order in OrderList Waiting over 1 Secs; cur time %d, insert time %d, Cancelling...\n", int_time, get_seconds_from_int_time(l_ord->insert_time));
			cancel_single_order(l_ord);
		}
	}

	ord_list = m_orders->get_order_by_side(ORDER_SELL);
	list_for_each_prev_safe(pos, n, ord_list) {
		l_ord = list_entry(pos, Order, pd_link);
		if (tick_time >= get_seconds_from_int_time(l_ord->insert_time) + 1) {
			PRINT_ERROR("Order in OrderList Waiting over 1 Secs; cur time %d, insert time %d, Cancelling...\n", int_time, get_seconds_from_int_time(l_ord->insert_time));
			LOG_LN("Order in OrderList Waiting over 1 Secs; cur time %d, insert time %d, Cancelling...\n", int_time, get_seconds_from_int_time(l_ord->insert_time));
			cancel_single_order(l_ord);
		}
	}
}

int SDPHandler::close_all_position()
{
	cancel_all_orders();
	for(auto iter = m_contracts->begin(); iter != m_contracts->end(); iter++) {
		Contract &instr = iter->second;
		send_single_order(&instr, instr.exch, instr.bp1, instr.pre_long_position, ORDER_SELL, ORDER_CLOSE, true);
		send_single_order(&instr, instr.exch, instr.bp1, long_position(&instr) - instr.pre_long_position, ORDER_SELL, ORDER_CLOSE);
		send_single_order(&instr, instr.exch, instr.ap1, instr.pre_short_position, ORDER_BUY, ORDER_CLOSE, true);
		send_single_order(&instr, instr.exch, instr.ap1, short_position(&instr) - instr.pre_short_position, ORDER_BUY, ORDER_CLOSE);
	}
	return 0;
}

void SDPHandler::long_short(Contract * instr, int desired_pos, double ap, double bp, int tolerance)
{
	LOG_LN("Enter long_short, symbol: %s, desired_pos: %d, ap: %f, bp: %f",
		instr->symbol, desired_pos, ap, bp);

	int current_pos = position(instr);

	if (current_pos == desired_pos) {
		cancel_all_orders();
		return;
	}

	int a_size = abs(desired_pos - current_pos);
	DIRECTION a_side;
	double price;
	int same_side_pos;
	int opposite_side_pos;
	int available_for_close;
	int available_for_close_yes;
	int total_pending_buy_close_size = instr->pending_buy_close_size;
	int total_pending_sell_close_size = instr->pending_sell_close_size;

	if (desired_pos > current_pos) {
		a_side = ORDER_BUY;
		price = ap;
		same_side_pos = long_position(instr);
		opposite_side_pos = short_position(instr);
		available_for_close_yes = instr->pre_short_qty_remain;
		available_for_close = opposite_side_pos - total_pending_buy_close_size;
	}
	else {
		a_side = ORDER_SELL;
		price = bp;
		same_side_pos = short_position(instr);
		opposite_side_pos = long_position(instr);
		available_for_close_yes = instr->pre_long_qty_remain;
		available_for_close = opposite_side_pos - total_pending_sell_close_size;
	}

	LOG_LN("side: %d, price %f, same_side_pos: %d, opposite_side_pos: %d, available_for_close_yes: %d, available_for_close: %d",
		a_side, price, same_side_pos, opposite_side_pos, available_for_close_yes, available_for_close);

	int pending_open_size = 0;
	int pending_close_size = 0;
	int pending_close_yes_size = 0;
	cancel_orders_with_dif_px(instr, a_side, price, tolerance, pending_open_size, pending_close_size, pending_close_yes_size);
	cancel_orders_by_side(instr, switch_side(a_side));

	LOG_LN("pending_open_size: %d, pending_close_size %d, pending_close_yes_size: %d",
		pending_open_size, pending_close_size, pending_close_yes_size);

	if (pending_close_size > opposite_side_pos || available_for_close < 0 || available_for_close < available_for_close_yes)
	{
		LOG_LN("Something is wrong with close_size, close_yes_size, opposite_side_pos.");
		cancel_orders_by_side(instr, a_side, ORDER_CLOSE);
		return;
	}

	if (pending_open_size + pending_close_size + pending_close_yes_size > 0) {
		LOG_LN("Skip current signal since pending_open_size + pending_close_size + pending_close_yes_size > 0 for same side and price.");
		return;
	}

	int need_to_close_yes = MIN(available_for_close_yes, a_size);
	int need_to_close_today = MIN(available_for_close - available_for_close_yes, a_size - need_to_close_yes);
	int need_to_open = a_size - need_to_close_yes - need_to_close_today;

	if (need_to_close_yes > 0)
		send_single_order(instr, instr->exch, price, need_to_close_yes, a_side, ORDER_CLOSE_YES);
	if (need_to_close_today > 0)
		send_single_order(instr, instr->exch, price, need_to_close_today, a_side, ORDER_CLOSE);
	if (need_to_open > 0)
		send_single_order(instr, instr->exch, price, need_to_open, a_side, ORDER_OPEN);
}


//根据symbol找到某一合约结构
Contract * SDPHandler::find_contract(char * symbol)
{
	return &m_contracts->at(symbol);
}

int SDPHandler::get_pos_by_side(Contract * a_instr, DIRECTION side)
{
	if (side == ORDER_BUY)
		return long_position(a_instr);
	else if (side == ORDER_SELL)
		return short_position(a_instr);

	return -1;
}

double SDPHandler::get_account_cash(char * account)
{
	return m_accounts.at(account).cash_available;
}

double SDPHandler::get_realized_pnl(Contract * cont)
{
	double long_side_PNL = 0.0;
	double short_side_PNL = 0.0;

	int long_pos = MIN(cont->positions[LONG_OPEN].qty, cont->positions[SHORT_CLOSE].qty);
	int short_pos = MIN(cont->positions[SHORT_OPEN].qty, cont->positions[LONG_CLOSE].qty);

	if (long_pos > 0)
		long_side_PNL = long_pos * (avg_px(cont->positions[SHORT_CLOSE]) - avg_px(cont->positions[LONG_OPEN]));

	if (short_pos > 0)
		short_side_PNL = short_pos * (avg_px(cont->positions[SHORT_OPEN]) - avg_px(cont->positions[LONG_CLOSE]));

	return long_side_PNL + short_side_PNL;
}

double SDPHandler::get_unrealized_pnl(Contract * cont)
{
	double long_side_unrealized_PNL = 0.0;
	double short_side_unrealized_PNL = 0.0;
	int long_pos = long_position(cont);
	int short_pos = short_position(cont);

	if (double_compare(cont->last_price, 0) == 0) {
		PRINT_WARN("%s Contract last_price is 0", cont->symbol);
		return 0.0;
	}

	if (long_pos >= 0)
		long_side_unrealized_PNL = long_pos * (cont->last_price - avg_px(cont->positions[LONG_OPEN]));
	else
		PRINT_ERROR("Sell Close Qty is higher than Buy Open, something is wrong");

	if (short_pos >= 0)
		short_side_unrealized_PNL = short_pos * (avg_px(cont->positions[SHORT_OPEN]) - cont->last_price);
	else
		PRINT_ERROR("Long Close Qty is higher than Sell Open, something is wrong");

	return long_side_unrealized_PNL + short_side_unrealized_PNL;
}

double SDPHandler::get_contract_pnl(Contract * cont)
{
	return get_realized_pnl(cont) + get_unrealized_pnl(cont);
}

double SDPHandler::get_contract_pnl_cash(Contract * cont)
{
	return get_contract_pnl(cont) * cont->multiplier;
}

double SDPHandler::get_strategy_pnl_cash()
{
	double pnl = 0.0;
	for(auto iter = m_contracts->begin(); iter != m_contracts->end(); iter++) {
		pnl += get_contract_pnl_cash(&iter->second);
	}
	return pnl;
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
//回报处理核心逻辑
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

	update_pending_status(l_order);
	m_orders->update_order_list(l_order);
	return true;
}
//更新资金
void SDPHandler::update_account_cash(Order* a_order)
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
//更新挂单状态
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
//将行情转化为Internalbook
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
	instr.last_price = a_book->last_px;
	return true;
}

//将行情转化为Internalbook
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
	instr.last_price = a_book->last_px;
	return true;
}

//将行情转化为bar
bool SDPHandler::update_contracts(Internal_Bar * a_bar)
{
	Contract& instr = m_contracts->at(a_bar->symbol);
	instr.pre_data_time = a_bar->int_time;
	instr.upper_limit = a_bar->upper_limit;
	instr.lower_limit = a_bar->lower_limit;
	return true;
}

