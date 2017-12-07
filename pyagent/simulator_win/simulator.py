# encoding: utf-8

import os
import sys
import traceback

import numpy as np
from components.data.my_types import ibook_futures_dtype
from components.data_factory.business import format_position
from components.data_factory.data_provider import data_produce
from components.st_wrapper import base as st_base
from config import config_json
from match_engine import MatchEngine
from simu_defines import OpenClose
from st_loader import StrategyWrapper


class Simulator(object):
    def __init__(self):
        self.task = None
        self.data = None
        self.orders = []
        self.ord_resp = []
        self.ord_request = []
        self.position = {}
        self.remained_pos = {}
        self.first_init_strat_flag = True

    def update_task_info(self, task):
        self.task = task
        self.prepare_all()

    def update_data(self, data_iter):
        self.data = data_iter
        self.me = MatchEngine()

    def set_time(self, cur_day, day_night):
        self.cur_day = cur_day
        self.day_night = day_night

    def prepare_all(self):
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
        self.init_pos_info = format_position(self.task['strat_item'])

    def init_st(self):
        st_cfg_item = self.task['strat_item']
        account = np.zeros(64, dtype=st_base.st_account)
        idx = 0
        for acc_item in self.accounts_cfg:
            currency_type = acc_item[3]
            account['currency'][idx] = self.data['currency'][currency_type][0]
            account['exch_rate'][idx] = self.data['currency'][currency_type][1]
            account['account'][idx] = acc_item[0]
            if self.first_init_strat_flag:  # 第一次初始化的时候使用配置中的信息
                account['cash_available'][idx] = float(acc_item[1])
                account['cash_asset'][idx] = float(acc_item[1])
            idx += 1
        contract = np.zeros(4096, dtype=st_base.st_contract)

        max_cancel_limit = st_cfg_item.get('max_cancel_limit', 500)
        max_accum_open_vol = st_cfg_item.get('max_accum_open_vol', 10000)
        file_path = st_cfg_item.get('output_file_path', './output')
        idx = 0
        for symbol in self.data['contracts']:
            symbol_item = self.data['contracts'][symbol]
            contract['symbol'][idx] = symbol
            contract['exch'][idx] = symbol_item['exch']
            contract['tick_size'][idx] = symbol_item['tick_size']
            contract['multiple'][idx] = symbol_item['lot_size']
            contract['max_accum_open_vol'][idx] = max_accum_open_vol
            contract['max_cancel_limit'][idx] = max_cancel_limit
            contract['account'][idx] = symbol_item['account']
            contract['expiration_date'][idx] = symbol_item['expire_date']
            contract['fee'][idx]['fee_by_lot'] = symbol_item['fee_type'].value
            contract['fee'][idx]['exchange_fee'] = symbol_item['fee']
            contract['fee'][idx]['yes_exchange_fee'] = symbol_item['yes_fee']
            # now the following three arguments writen as constant value
            contract['fee'][idx]['broker_fee'] = self.task.get('broker_fee', 0.0002)
            contract['fee'][idx]['stamp_tax'] = self.task.get('stamp_tax', 0.001)
            contract['fee'][idx]['acc_transfer_fee'] = self.task.get('acc_transfer_fee', 0.00002)
            if self.first_init_strat_flag:  # 第一次初始化的时候使用配置中的信息
                if symbol in self.init_pos_info['yes_pos']:
                    contract['yesterday_pos'][idx] = self.init_pos_info['yes_pos'][symbol]
                if symbol in self.init_pos_info['tod_pos']:
                    contract['today_pos'][idx] = self.init_pos_info['tod_pos'][symbol]
            else:  # 如果清算中存在对应symbol的仓位信息，则用清算的，清算没有的，清零仓位
                if symbol in self.remained_pos:
                    symbol_data = self.remained_pos[symbol]
                    # if trading day changed then the settlement result should be adjusted then feed into strategy
                    if self.cur_day != self.data['pre_date']:
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
                # today long position
                self.position[0]['pos'] = contract['today_pos'][idx]['long_volume']
                self.position[0]['notional'] = self.position[0]['pos'] * contract['today_pos'][idx]['long_price']
                # today short position
                self.position[2]['pos'] = contract['today_pos'][idx]['short_volume']
                self.position[2]['notional'] = self.position[2]['pos'] * contract['today_pos'][idx]['short_price']
                # yesterday long position
                self.position[0]['yes_pos'] = contract['yesterday_pos'][idx]['long_volume']
                self.position[0]['yes_notional'] = self.position[0]['yes_pos'] * \
                                                   contract['yesterday_pos'][idx]['long_price']
                # yesterday short position
                self.position[1]['yes_pos'] = contract['yesterday_pos'][idx]['short_volume']
                self.position[1]['yes_notional'] = self.position[1]['yes_pos'] * \
                                                   contract['yesterday_pos'][idx]['short_price']

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
            self.strategy = StrategyWrapper(st_file)
        self.confs = np.zeros(len(self.task['so_path']), dtype=st_base.st_config_dtype)
        for idx in range(len(self.task['so_path'])):
            self.confs[idx] = conf
        self.strategy.my_on_init(self.confs)

    def init_me(self):
        pass

    def enter_idle_mode(self, exchange_time):
        time_diff = exchange_time - self.last_idle_time
        if time_diff > 0 and time_diff > self.idle_interval:
            st_idle_cnt = time_diff // self.idle_interval
        else:
            st_idle_cnt = 0
        for i in range(st_idle_cnt):
            self.ord_request = self.strategy.my_on_timer()
            self.process_send_order_request()
            self.last_idle_time += self.idle_interval
            # Avoid bad exchange time
            if st_idle_cnt > 10000:
                self.last_idle_time += (st_idle_cnt - 1) * self.idle_interval
                break

    def process_send_order_request(self):
        while len(self.ord_request) > 0:
            # Pack to list of Order
            self.ord_resp = self.me.feed_order(self.ord_request)
            self.ord_request = []
            self.process_order_response()

    def switch_side(self, direction):
        return 0 if direction == 1 else 1

    def process_order_response(self):
        while len(self.ord_resp) > 0:
            # calculate pos
            for resp in self.ord_resp:
                container = self.position.setdefault(resp.symbol, [{}, {}, {}, {}])
                if resp.open_close == OpenClose.CLOSE_YES.value:
                    container[self.switch_side(resp.direction)]['yes_pos'] -= resp.exe_volume
                    container[self.switch_side(resp.direction)]['yes_notional'] -= resp.exe_volume * resp.exe_price

                # CLOSE_TOD && CLOSE_YES convert to CLOSE
                open_close = resp.open_close if resp.open_close <= OpenClose.CLOSE.value else OpenClose.CLOSE.value
                # index: buy_open = 0, buy_close = 1, sell_open = 2, sell_close = 3
                container_index = resp.direction * 2 + open_close
                container[container_index].setdefault('pos', 0)
                container[container_index].setdefault('notional', 0)
                container[container_index]['pos'] += resp.exe_volume
                container[container_index]['notional'] += resp.exe_volume * resp.exe_price
                self.ord_request.extend(self.strategy.my_on_response(resp))
            self.ord_resp = []
            self.process_send_order_request()


    def process_historical_data(self):
        self.last_idle_time = self.data['quote'][0]['exchange_time']
        
        for tick in self.data['quote']:
            self.exchange_time = tick['exchange_time']

            # feed quote to match_engine
            self.ord_resp = self.me.feed_quote(tick)
            self.process_order_response()

            quote = np.array(tick['quote'], dtype=ibook_futures_dtype)

            # feed quote to strategy
            self.ord_request = self.strategy.my_on_book(quote)

            self.process_send_order_request()

            if self.idle_interval > 0:
                self.enter_idle_mode(self.exchange_time)

    def process_results(self):
        for symbol in self.position:
            container = self.position.setdefault(symbol, [{},{},{},{}])
            container[0].setdefault('yes_pos', 0)
            container[0].setdefault('yes_pos', 0)
            container[1].setdefault('pos', 0)
            container[1].setdefault('yes_pos', 0)
            container[2].setdefault('pos', 0)
            container[1].setdefault('notional', 0)
            container[2].setdefault('notional', 0)
            # long_pos = long_open - short_close
            long_pos = container[0]['pos'] - container[3]['pos']
            self.remained_pos.setdefault(symbol, {})
            self.remained_pos[symbol]['today_long_pos'] = long_pos
            if long_pos != 0:
                self.remained_pos[symbol]['today_long_avg_price'] = (container[0]['notional'] - container[3]['notional']) / long_pos
            else:
                self.remained_pos[symbol]['today_long_avg_price'] = 0

            short_pos = container[2]['pos'] - container[1]['pos']
            self.remained_pos[symbol]['today_short_pos'] = short_pos
            if short_pos != 0:
                self.remained_pos[symbol]['today_short_avg_price'] = (container[2]['notional'] - container[1]['notional']) / short_pos
            else:
                self.remained_pos[symbol]['today_short_avg_price'] = 0
            
            yes_long_pos = container[0]['yes_pos']
            self.remained_pos[symbol]['yest_long_pos'] = yes_long_pos
            if yes_long_pos != 0:
                self.remained_pos[symbol]['yest_long_avg_price'] = container[0]['yes_notional'] / yes_long_pos
            else:
                self.remained_pos[symbol]['yest_long_avg_price'] = 0
            yes_short_pos = container[1]['yes_pos']
            self.remained_pos[symbol]['yest_short_pos'] = yes_short_pos
            if yes_long_pos != 0:
                self.remained_pos[symbol]['yest_short_avg_price'] = container[1]['yes_notional'] / yes_long_pos
            else:
                self.remained_pos[symbol]['yest_short_avg_price'] = 0
                #clear current pos
        self.position = {}
        
    def start_trading_session(self, cur_day, day_night):
        try:
            self.st_init_success_flag = False
            self.set_time(cur_day, day_night)
            if self.first_init_strat_flag:
                self.init_st()
                self.first_init_strat_flag = False
            else:
                self.init_st()
            self.st_init_success_flag = True

            if self.data['quote'] is not None:
                print('Processing: %d %s' % (cur_day, day_night))
                self.init_me()
                self.process_historical_data()
                self.process_results()
            else:
                err_msg = 'Processing: %d %s quote error' % (cur_day, day_night)
                raise ValueError(err_msg)
        except Exception:
            traceback.print_exc()
        finally:
            if self.st_init_success_flag:
                self.strategy.my_destroy()


def execute_task(task):
    simu_hdl = Simulator()
    data_all = data_produce(task)
    simu_hdl.update_task_info(task)
    try:
        for data_iterator in data_all:
            simu_hdl.update_data(data_iterator)
            simu_hdl.start_trading_session(data_iterator['cur_date'], data_iterator['day_night'])
    except Exception:
        traceback.print_exc()
    finally:
        sys.exit(0)
