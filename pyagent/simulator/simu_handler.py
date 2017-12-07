# -*- coding: utf -8-*-

import json
import os
import sys
import traceback

import numpy as np
from components.data import meta_api
from components.data import my_types
from components.data.mi_type import MI_TYPE
from components.data_factory import business
from components.data_factory.quote_factory import QuoteSortMethod, get_combined_quote
from components.me_wrapper import base as me_base
from components.me_wrapper import me
from components.st_wrapper import base as st_base
from components.st_wrapper import strategy
from config import client, config_json, kdb_config, mysql_config, PROFILE, DEBUG
from new_settlement import settle_one_session
from simulator.agent_defines import TaskStatus, CallbackMsgType
from simulator.simu_defines import *
from simulator.tools import sw_fn_timer
from simulator.tunnel_log_colloctor import TunnelLogColloctor


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


class AtomTaskProcessor:
    def __init__(self):
        self.orders = []
        self.ord_resp = []
        self.ord_request = []
        self.process_session_count = 0
        self.total_session_num = 0
        self.first_init_strat_flag = True
        self.stop_signal = False
        self.debug_mode_flag = False
        self.cur_day = None
        self.agent_log_file = ''

    def update_task_info(self, task, agent_log_file, rsp_handler, agent_msg_handler):
        self.task = task
        self.agent_log_file = agent_log_file
        self.debug_mode_flag = task.get('flag_debug_mode', 0)
        self.send_back = rsp_handler
        self.agent_msg_handler = agent_msg_handler
        self._prepare_all()

    def write_agent_log(self, *args, **kwargs):
        msg = ' '.join([str(x) for x in args]) + '\n'
        self.agent_msg_handler(CallbackMsgType.PyAgent_Msg.value, msg)

    def flush_agent_log(self):
        self.agent_msg_handler(CallbackMsgType.PyAgent_Flush_Msg.value, '')

    def _prepare_all(self):
        if 'bar_interval' in self.task and self.task['bar_interval'] > 0:
            self.is_bar = 1
        else:
            self.is_bar = 0
        self.idle_interval = self.task.get('time_interval', 0) * 1000 * 1000
        self.accounts_cfg = []
        strategy_cfg = self.task['strat_item']
        if 'accounts' in strategy_cfg:
            for item in strategy_cfg['accounts']:
                item_list = item.split('|')
                self.accounts_cfg.append(item_list)
        self.strat_id = strategy_cfg['strat_id']
        self.trade_account = self.accounts_cfg[0][0]
        self.settle_result = None
        self.server = self.task.get('server', '')
        self.tlog_flag = self.task.get('flag_write_tunnel_log', 0)
        self.tl_colloctor = TunnelLogColloctor(self.strat_id, self.server, self.trade_account, self.tlog_flag)
        self.qt_map = {
            my_types.numpy_book_futures_dtype: QuoteType.FutureQuote.value,
            my_types.numpy_book_all_dtype: QuoteType.IBQuote.value,
            my_types.numpy_book_stock_dtype: QuoteType.StockQuote.value
        }

    def _set_time(self, cur_day, day_night):
        """ set trading date and day_night
        """
        self.pre_day = self.cur_day
        self.cur_day = cur_day
        self.day_night = day_night
        self.tl_colloctor.set_extra_info(date=self.cur_day, day_night=self.day_night,
                                         task_id=self.task['task_id'], st_id=self.strat_id)

    @sw_fn_timer(PROFILE)
    def _init_st(self):
        """ generate strategy configuration and init it

        now only support one strategy per execution
        "accounts": [
                "testaccount1|2000000.0|8888888|CNY|1"
        ],
        """
        settle_results = self.settle_result
        strategy_settle_results = settle_results[str(self.strat_id)]

        st_cfg_item = self.task['strat_item']
        account = np.zeros(64, dtype=st_base.st_account)
        idx = 0
        for acc_item in self.accounts_cfg:
            currency_type = acc_item[3]
            account['currency'][idx] = self.currency[currency_type][0]  # currency type
            account['exch_rate'][idx] = self.currency[currency_type][1]
            account['account'][idx] = acc_item[0]
            if self.first_init_strat_flag:  # 第一次初始化的时候使用配置中的信息
                account['cash_available'][idx] = float(acc_item[1])
                account['cash_asset'][idx] = float(acc_item[1])
            else:
                account_data = strategy_settle_results['accounts'][acc_item[0]]
                account['cash_available'][idx] = account_data['available_cash']
                account['cash_asset'][idx] = account_data['asset_cash']
            idx += 1
        contract = np.zeros(4096, dtype=st_base.st_contract)

        max_cancel_limit = st_cfg_item.get('max_cancel_limit', 500)
        max_accum_open_vol = st_cfg_item.get('max_accum_open_vol', 10000)
        file_path = st_cfg_item.get('output_file_path', './output')
        idx = 0
        self.trade_symbols = []
        for symbol_item in self.symbol_info:
            symbol = symbol_item[0]
            str_symbol = symbol.decode() if isinstance(symbol, bytes) else symbol
            symbol_info = self.product_info[str_symbol]
            self.trade_symbols.append(symbol)
            contract['symbol'][idx] = symbol
            exchg_code = ord(business.convert_exchange_to_prefix(symbol_item[1]))
            contract['exch'][idx] = exchg_code
            contract['tick_size'][idx] = symbol_info['tick_size']
            contract['multiple'][idx] = symbol_info['lot_size']
            contract['max_accum_open_vol'][idx] = max_accum_open_vol
            contract['max_cancel_limit'][idx] = max_cancel_limit
            contract['account'][idx] = symbol_item[5]
            contract['expiration_date'][idx] = business.to_int_date(symbol_item[6])
            contract['fee'][idx]['fee_by_lot'] = symbol_info['fee_type']
            contract['fee'][idx]['exchange_fee'] = symbol_info['fee']
            contract['fee'][idx]['yes_exchange_fee'] = symbol_info['yes_fee']
            # now the following three arguments writen as constant value
            contract['fee'][idx]['broker_fee'] = self.task.get('broker_fee', 0.0002)
            contract['fee'][idx]['stamp_tax'] = self.task.get('stamp_tax', 0.001)
            contract['fee'][idx]['acc_transfer_fee'] = self.task.get('acc_transfer_fee', 0.00002)
            if self.first_init_strat_flag:  # 第一次初始化的时候使用配置中的信息
                if symbol in self.pos_info['yes_pos']:
                    contract['yesterday_pos'][idx] = self.pos_info['yes_pos'][symbol]
                if symbol in self.pos_info['tod_pos']:
                    contract['today_pos'][idx] = self.pos_info['tod_pos'][symbol]
            else:  # 如果清算中存在对应symbol的仓位信息，则用清算的，清算没有的，清零仓位
                if isinstance(symbol, bytes):
                    symbol = symbol.decode()
                if symbol in strategy_settle_results['data']:
                    symbol_data = strategy_settle_results['data'][symbol]
                    # if trading day changed then the settlement result should be ajustedn then feed into strategy
                    if self.cur_day != self.pre_day:
                        contract['yesterday_pos'][idx]['long_volume'] = symbol_data['today_long_pos']
                        contract['yesterday_pos'][idx]['short_volume'] = symbol_data['today_short_pos']
                    else:
                        contract['yesterday_pos'][idx]['long_volume'] = symbol_data['yest_long_pos']
                        contract['yesterday_pos'][idx]['short_volume'] = symbol_data['yest_short_pos']
                    contract['today_pos'][idx]['long_volume'] = symbol_data['today_long_pos']
                    contract['today_pos'][idx]['short_volume'] = symbol_data['today_short_pos']
                    contract['yesterday_pos'][idx]['long_price'] = symbol_data['yest_long_avg_price']
                    contract['yesterday_pos'][idx]['short_price'] = symbol_data['yest_short_avg_price']
                    contract['today_pos'][idx]['long_price'] = symbol_data['today_long_avg_price']
                    contract['today_pos'][idx]['short_price'] = symbol_data['today_short_avg_price']
                else:
                    contract['yesterday_pos'][idx]['long_volume'] = 0
                    contract['yesterday_pos'][idx]['long_price'] = 0
                    contract['yesterday_pos'][idx]['short_volume'] = 0
                    contract['yesterday_pos'][idx]['short_price'] = 0
                    contract['today_pos'][idx]['long_volume'] = 0
                    contract['today_pos'][idx]['long_price'] = 0
                    contract['today_pos'][idx]['short_volume'] = 0
                    contract['today_pos'][idx]['short_price'] = 0
            idx += 1
        conf = np.zeros(1, dtype=st_base.st_config_dtype)
        conf['vst_id'] = st_cfg_item.get('strat_id', -1)
        conf['strat_name'] = st_cfg_item['strat_name']
        conf['trading_date'] = self.cur_day
        conf['day_night'] = self.day_night
        self.st_name = st_cfg_item['strat_name']
        param_file = os.path.join(config_json['BASE_PATH'], st_cfg_item.get('strat_param_file', ''))
        day_night_str = 'day' if self.day_night == 0 else 'night'
        param_file = param_file.replace('[DATE]', str(self.cur_day))
        conf['param_file_path'] = param_file.replace('[DAYNIGHT]', day_night_str)
        conf['output_file_path'] = file_path
        conf['accounts'] = account
        conf['contracts'] = contract
        for file_name in self.task['so_path']:
            st_file = os.path.join(config_json['BASE_PATH'], file_name)
            strategy.load(st_file)
        self.confs = np.zeros(len(self.task['so_path']), dtype=st_base.st_config_dtype)
        for idx in range(len(self.task['so_path'])):
            self.confs[idx] = conf
        strategy.init(self.confs)

    @sw_fn_timer(PROFILE)
    def _init_me(self):
        """ generate match engine configuration and init it

        "match_type": 1,
        //撮合参数，mode, exchange, product(any or other), trade_ratio, params
        "match_param": [
                "1,SHFE,any,0.2",
                "1,DCE,any,0.5",
                "3,SHFE,any,0.2,1000,0.5,80",
                "5,SHFE,any,1"
        ],
        """
        idx = 0
        conf = np.zeros(1, dtype=me_base.me_config_dtype)
        mps = np.zeros(4096, dtype=me_base.match_para_dtype)
        for me_param_item in self.task['match_param']:
            me_param_l = me_param_item.split(',')
            if me_param_l[1] not in ['SHFE', 'DCE', 'CZCE', 'CFFEX', 'SSE', 'SZSE']:
                continue
            mps[idx]['mode'] = me_param_l[0]
            mps[idx]['exch'] = ord(business.convert_exchange_to_prefix(me_param_l[1]))
            mps[idx]['product'] = me_param_l[2]
            mps[idx]['trade_ratio'] = me_param_l[3]
            mps[idx]['param_num'] = 0
            idx += 1
        conf['par_cnt'] = idx
        conf['par_cfg'] = mps
        idx = 0
        pis = np.zeros(4096, dtype=me_base.product_info_dtype)
        for symbol, product_data in self.product_info.items():
            exch = business.get_exchange(self.symbol_exch, symbol)
            product = business.convert_symbol_to_product(symbol, exch)
            pis[idx]['name'] = product
            pis[idx]['prefix'] = business.reverse_to_product(product)
            pis[idx]['fee_fixed'] = product_data['fee_type']
            pis[idx]['rate'] = product_data['fee']
            pis[idx]['unit'] = product_data['tick_size']
            pis[idx]['multiple'] = product_data['lot_size']
            pis[idx]['xchg_code'] = ord(business.convert_exchange_to_prefix(exch))
            idx += 1
        conf['pro_cnt'] = idx
        conf['pro_cfg'] = pis
        default_match_type = 1 if 'match_type' not in self.task else self.task['match_type']
        conf['default_mt'] = default_match_type
        self.me_hdl = me.create(conf)

    @sw_fn_timer(PROFILE)
    def _init_settle_position(self):
        _id = self.strat_id
        _date = business.to_settlement_date(self.cur_day)
        _day_night = business.to_day_night_str(self.day_night)
        _start_point = self._get_pre_day_night(_day_night)
        pos = business.construct_settlement_position(_id, _date, _day_night, self.accounts_cfg,
                                                     self.pos_info, self.symbol_exch)

        trade_msg = self.tl_colloctor.get_trade_msg()
        no_trade_msg = self.tl_colloctor.get_no_trade_msg()
        self.settle_result = settle_one_session(kdb_config, _id, _date, self.day_night, pos, trade_msg,
                                                _start_point, mysql_config=mysql_config, task_id=self.task['task_id'],
                                                tlogs=no_trade_msg)

    def _get_pre_day_night(self, day_night):
        if self.task['day_night_flag'] == DAY_NIGHT_FLAG.DAY_AND_NIGHT.value:
            if day_night == 'DAY':
                return 1
            else:
                return 0
        else:
            if day_night == 'DAY':
                return 0
            else:
                return 1

    @sw_fn_timer(PROFILE)
    def _get_settle_resp(self):
        """ do settle calculation, store the pos and settle result
        """
        _id = self.strat_id
        _date = business.to_settlement_date(self.cur_day)
        _day_night = business.to_day_night_str(self.day_night)
        _start_point = self._get_pre_day_night(_day_night)

        trade_msg = self.tl_colloctor.get_trade_msg()
        no_trade_msg = self.tl_colloctor.get_no_trade_msg()

        if self.debug_mode_flag == True:
            self.write_agent_log('--------------->before settle: \n', self.settle_result)
            self.write_agent_log('--------------->trade msg: \n', trade_msg)

        self.settle_result = settle_one_session(kdb_config, _id, _date, self.day_night, self.settle_result, trade_msg,
                                                _start_point, mysql_config=mysql_config, task_id=self.task['task_id'],
                                                tlogs=no_trade_msg)
        self.tl_colloctor.reset()
        if self.debug_mode_flag == True:
            self.write_agent_log('--------------->after settle: \n', self.settle_result)

    def _enter_idle_mode(self, exchange_time):
        time_diff = exchange_time - self.last_idle_time
        if time_diff > 0 and time_diff > self.idle_interval:
            st_idle_cnt = time_diff // self.idle_interval
        else:
            st_idle_cnt = 0
        for i in range(st_idle_cnt):
            self.ord_request = strategy.on_timer(0, 0, 0)
            if self.ord_request:
                self.tl_colloctor.record_order(self.ord_request, self.exchange_time)
            self._process_send_order_request()
            self.last_idle_time += self.idle_interval
            # Avoid bad exchange time
            if st_idle_cnt > 10000:
                self.last_idle_time += (st_idle_cnt - 1) * self.idle_interval
                break

    @sw_fn_timer(PROFILE)
    def _deal_result(self):
        """ deal with the result information

        1. send back the result data
        2. update the position
        3. update the account cash
        """
        day_pnl_node = {}
        day_pnl_node['task_id'] = self.task['task_id']
        day_pnl_node['status'] = 0
        day_pnl_node['date'] = self.cur_day
        day_pnl_node['day_night'] = self.day_night
        day_pnl_node['progress'] = self.process_session_count * 100 / self.total_session_num
        strategy_settle_results = self.settle_result[str(self.strat_id)]
        day_pnl_node['cash'] = strategy_settle_results['cash']
        day_pnl_node['asset_cash'] = strategy_settle_results['asset_cash']
        if self.first_init_strat_flag and self.debug_mode_flag:
            day_pnl_node['log_name'] = ['nlogs/' + self.strategy_log_name, self.agent_log_file]
        else:
            day_pnl_node['log_name'] = ['nlogs/' + self.strategy_log_name]
        day_pnl_node['pnl_nodes'] = []
        for symbol in strategy_settle_results['data']:
            symbol_pnl = {}
            symbol_data = strategy_settle_results['data'][symbol]
            symbol_pnl['symbol'] = symbol
            exch_str = business.get_exchange(self.symbol_exch, symbol)
            symbol_pnl['exchange'] = business.exch_str_to_int(exch_str)
            symbol_pnl['product'] = business.convert_symbol_to_product(symbol, exch_str)
            symbol_pnl['fee'] = symbol_data['fee']
            symbol_pnl['net_pnl'] = symbol_data['symbol_net_pnl']
            symbol_pnl['gross_pnl'] = symbol_pnl['net_pnl'] + symbol_pnl['fee']
            symbol_pnl['long_price'] = symbol_data['today_long_avg_price']
            symbol_pnl['long_volume'] = symbol_data['today_long_pos']
            symbol_pnl['short_price'] = symbol_data['today_short_avg_price']
            symbol_pnl['short_volume'] = symbol_data['today_short_pos']
            day_pnl_node['pnl_nodes'].append(symbol_pnl)
        result_msg = json.dumps(day_pnl_node)
        self.send_back(result_msg)
        # update message locally for st_init

    def _process_send_order_request(self):
        while len(self.ord_request) > 0:
            if self.debug_mode_flag == True:
                self.write_agent_log(self.ord_request)
            self.ord_resp = me.feed_order(self.me_hdl, self.ord_request)
            if self.ord_resp:
                self.tl_colloctor.record_response(self.ord_resp, self.exchange_time)
            self.ord_request = []
            self._process_order_response()

    def _process_order_response(self):
        while len(self.ord_resp) > 0:
            if self.debug_mode_flag == True:
                self.write_agent_log(self.ord_resp)
            self.ord_request = strategy.on_response(0, 0, self.ord_resp)
            if self.ord_request:
                self.tl_colloctor.record_order(self.ord_request, self.exchange_time)
            self.ord_resp = []
            self._process_send_order_request()

    @sw_fn_timer(PROFILE)
    def _process_historical_data(self):
        # start_time_stamp = convert_epoch_stamp(self.cur_day, self.day_night, 215530000)
        self.last_idle_time = self.quotes[0]['exchange_time']
        # touch the quote feeded to me and strategy
        # _preserve_quote = []
        for qidx, tick_data in enumerate(self.quotes):
            self.exchange_time = tick_data['exchange_time']
            # if tick_data['local_time'] < start_time_stamp:
            #    continue
            # feed information to match_engine
            # quote = np.array(tick_data, dtype=tick_data.dtype)

            self.ord_resp = me.feed_quote(self.me_hdl, self.quotes, qidx, self.is_bar)
            if self.ord_resp:
                self.tl_colloctor.record_response(self.ord_resp, self.exchange_time)
            self._process_order_response()

            # feed quote to strategy
            qtype = self.qt_map[tick_data.dtype]
            self.ord_request = strategy.on_book(qtype, 0, self.quotes, qidx)
            if self.ord_request:
                self.tl_colloctor.record_order(self.ord_request, self.exchange_time)
            self._process_send_order_request()

            if self.idle_interval > 0:
                self._enter_idle_mode(self.exchange_time)
            # _preserve_quote.append(quote)
        self._get_settle_resp()

    def destory(self):
        self.first_init_strat_flag = True
        self.process_session_count = 0

    @sw_fn_timer(PROFILE)
    def _query_basic_data(self):
        self.symbol_info, self.is_stock, self.order_quote = business.format_book_contract(self.cur_day, self.day_night,
                                                                                          self.task['strat_item'][
                                                                                              'contracts'])
        self.product_exch = business.format_product_exch(self.symbol_info)
        self.symbol_exch = business.format_symbol_exch(self.symbol_info)
        if not self.is_stock:
            self.product_info = business.format_future_info(self.cur_day, self.day_night, self.symbol_info)
        else:
            self.product_info = business.format_stock_info(self.cur_day, self.day_night, self.symbol_info)
        self.pos_info = business.format_position(self.task['strat_item'])
        self.currency = business.get_currency(self.cur_day)

    @sw_fn_timer(PROFILE)
    def _query_quote_data(self):
        self.quote_def = []
        mi_types = []
        for sym_info in self.order_quote:
            if isinstance(sym_info[0], bytes):
                sym_info[0] = sym_info[0].decode()
            quote_args = {
                'code': sym_info[0],
                'mi_type': sym_info[2],
                'source': sym_info[4],
            }
            mi_types.append(quote_args['source'])
            self.quote_def.append(quote_args)
        sm = QuoteSortMethod.BY_EXCHG_TIME
        if str(MI_TYPE.MI_DCE_ORDER_STATISTIC.new_book) or str(
                MI_TYPE.MI_DCE_ORDER_STATISTIC.internal_book) in mi_types:
            sm = QuoteSortMethod.BY_LOCAL_TIME
        # print(self.cur_day, self.day_night, quote_def, sm)
        self.quotes = get_combined_quote(self.cur_day, self.day_night, self.quote_def, sm)

    def _init_strategy_extra(self):
        log_name = self.task['strat_item'].get('log_name', 'logs/[DATE]_[PRODUCT]_[DAYNIGHT]_strategy.log')
        product_name_list = list(self.product_info.keys())
        # print(product_name_list)
        if len(product_name_list) <= 2:
            product_name = '_'.join(sorted(product_name_list))
        else:
            product_name = '_'.join(sorted(product_name_list[:2]))
        dn_str = 'day' if self.day_night == 0 else 'night'
        log_name = log_name.replace('[DATE]', str(self.cur_day))
        log_name = log_name.replace('[PRODUCT]', product_name)
        log_name = log_name.replace('[DAYNIGHT]', dn_str)
        self.strategy_log_name = log_name
        log_parent_path = os.path.dirname(log_name)
        if not os.path.exists(log_parent_path):
            os.makedirs(log_parent_path)
        strategy.set_extra(log_name, self.debug_mode_flag, self.agent_msg_handler)

    def start_trading_session(self, cur_day, day_night):
        try:
            self.st_init_success_flag = False
            self.me_init_success_flag = False
            self.process_session_count += 1
            cur_day = business.to_int_date(cur_day)
            self._set_time(cur_day, day_night)
            self._query_basic_data()
            self._init_strategy_extra()

            if self.first_init_strat_flag:
                self._init_settle_position()
                self._init_st()
                self.first_init_strat_flag = False
            else:
                self._init_st()
            self.st_init_success_flag = True

            self._query_quote_data()
            # assert(self.quotes != None)
            qsize = len(self.quotes)
            # logger.debug('Processing: %d %s, quote size: %d' % (cur_day, day_night, qsize))
            if qsize != 0:
                print('Processing: %d %s, quote size: %d' % (cur_day, day_night, qsize))
                self._init_me()
                self.me_init_success_flag = True
                self._process_historical_data()
                self._deal_result()
            else:
                err_msg = 'Processing: %d %s, quote_def %s, quote size: %d' % (
                cur_day, day_night, self.quote_def, qsize)
                raise ValueError(err_msg)
        except Exception:
            eprint(traceback.format_exc())
            self.agent_msg_handler(CallbackMsgType.Screen_Msg.value, traceback.format_exc())
            client.captureException()
        finally:
            if self.st_init_success_flag:
                strategy.destroy()
            if self.me_init_success_flag:
                me.destroy(self.me_hdl)

    def parse_date_range(self):
        return self.task['start_date'], self.task['end_date']


simu_hdl = AtomTaskProcessor()


def task_check(task, rsp_handler):
    """ check if the task valid or not
    """
    return True


def need_stop_now():
    """ check whether taskid in stop set or not
    """
    simu_hdl.stop_signal = True


def execute_task(task, agent_log_file, rsp_handler, agent_msg_handler):
    if not task_check(task, rsp_handler):
        return False
    simu_hdl.update_task_info(task, agent_log_file, rsp_handler, agent_msg_handler)
    date_start, date_end = simu_hdl.parse_date_range()
    # take SHFE as a normal exchange to get trading date
    trading_day = meta_api.get_trading_date_range(date_start, date_end, "SHFE")
    day_multiplier = 2 if task['day_night_flag'] == DAY_NIGHT_FLAG.DAY_AND_NIGHT.value else 1
    simu_hdl.total_session_num = day_multiplier * len(trading_day)

    # for test try only get do simu of lastest trading date
    if 'is_test' in task and task['is_test']:
        trading_day = trading_day[:1] if len(trading_day) >= 1 else trading_day
    try:
        for day in trading_day:
            if task['day_night_flag'] == DAY_NIGHT_FLAG.DAY_AND_NIGHT.value:
                if simu_hdl.stop_signal == False:
                    simu_hdl.start_trading_session(day, DAY_NIGHT_FLAG.NIGHT.value)
                else:
                    break
                if simu_hdl.stop_signal == False:
                    simu_hdl.start_trading_session(day, DAY_NIGHT_FLAG.DAY.value)
                else:
                    break
            else:
                if simu_hdl.stop_signal == False:
                    simu_hdl.start_trading_session(day, task['day_night_flag'])
                else:
                    break
        rsp_handler(
            json.dumps({'task_id': task['task_id'], 'status': TaskStatus.Finished.value, 'msg': 'Task finished!'}))
    except Exception:
        eprint(traceback.format_exc())
        agent_msg_handler(CallbackMsgType.Coredump_Msg.value, traceback.format_exc())
        client.captureException()
        sys.exit(0)
    finally:
        simu_hdl.destory()
        simu_hdl.flush_agent_log()
        # execute one task then exit automatic
        if not DEBUG:
            sys.exit(0)
