# -*- coding: utf-8 -*-

import atexit
import copy
import datetime
import os


class TunnelLogColloctor:
    def __init__(self, st_id, server, account, write_log=False, **kargs):
        self.st_id = st_id
        self.server = server
        self.account = account
        self.default_order = {
            'oder_id': 0,
            'open_close': -1,
            'speculator': -1,
            'price': -1,
            'volume': -1,
            'direction': -1,
            'order_type': -1,
            'remain_vol': -1
        }
        self.status_map = {
            -1: 's',
            0: 'c',
            1: 'a',
            2: 'p',
            3: 'd',
            4: 'i',
            5: 'i'
        }
        self.msg_type_map = {
            -1: '0',
            0: '3',
            1: '1',
            2: '3',
            3: '5',
            4: '1',
            5: '5'
        }
        self.type_map = {
            '0': '[order_request]',
            '1': '[order_response]',
            '2': '[exchange_response]',
            '3': '[trade_rtn]',
            '4': '[cancel_request]',
            '5': '[cancel_response]',
            '6': '[position]',
        }
        self.write_row = []
        self.write_log = write_log
        self._clear()
        atexit.register(self.reset)

    def set_extra_info(self, **kwargs):
        self.date = kwargs.get('date', None)
        self.day_night = kwargs.get('day_night', None)
        self.strategy = kwargs.get('st_id', None)
        self.task_id = kwargs.get('task_id', 0)
        self.write_file_init()

    def get_file_name(self):
        if not self.f_hdl:
            return self.log_file
        return None

    def _format_line(self, msg):
        return 'serial_no: %s, symbol: %s, ord_price: %s, ord_volume: %s, open_close: %s, direction: %s, ' \
               'order_type: %s, investor_type: %s, trd_price: %lf, trd_volume: %s, enstrust_status: %s, msg_type: %s\n' % \
               (msg['serial_no'], msg['symbol'], msg['order_price'], msg['order_vol'], msg['open_close'],
                msg['direction'], msg['order_price_type'], msg['speculator'], msg['trade_price'],
                msg['trade_vol'], msg['entrust_status'], self.type_map[msg['msg_type']])

    def flush_data(self):
        """ rocord contained into trade log """
        if not self.f_hdl:
            return
        write_buf = "Date: %s, day_night: %s, strategy: %s\n\n" % \
                    (self.date, self.day_night, self.strategy)
        for row in self.write_row:
            write_buf += self._format_line(row)
        self.f_hdl.write(write_buf)

    def write_file_init(self):
        if not self.write_log:
            self.f_hdl = None
            return
        log_path = './'
        bas_path = os.path.dirname(log_path)
        par_path = os.path.dirname(bas_path)
        self.log_file = os.path.join(par_path,
                                     "logs/tunnellog/%s_%s_%s.log" % (self.date, self.day_night, self.task_id))
        dir_file = os.path.dirname(self.log_file)
        if not os.path.exists(dir_file):
            os.makedirs(dir_file)
        print("the tunnel log file is %s" % self.log_file)
        self.f_hdl = open(self.log_file, "w+")

    def enum_entrust_status_to_normal(self, entrust_status):
        if entrust_status in self.status_map:
            return self.status_map[entrust_status]
        return 'e'

    def entrust_status_to_tunnel_msg_type(self, entrust_status):
        assert (entrust_status in self.msg_type_map)
        return self.msg_type_map[entrust_status]

    def find_org_order(self, order_id):
        return self.hang_order.get(order_id, self.default_order)

    def record_order(self, orders, exchange_time):
        for order in orders:
            symbol = order['symbol'].decode() if isinstance(order['symbol'], bytes) else order['symbol']
            ntm_data = self.no_trd_msg.setdefault(str(self.st_id), {})
            ntm_list = ntm_data.setdefault(symbol, [])
            if 'org_order_id' in order:
                # cancel order
                msg_type = '4'
                entrust_status = 'f'
                data_item = self.find_org_order(order['org_order_id'])
            else:
                # place order
                msg_type = '0'
                entrust_status = 's'
                data_item = order
                self.hang_order[order['order_id']] = copy.deepcopy(order)
                self.hang_order[order['order_id']]['remain_vol'] = order['volume']
            order_item = {
                'vsid': self.st_id,
                'server': self.server,
                'account': self.account,
                'symbol': symbol,
                'exchange_time': exchange_time,
                'calendar_time': str(datetime.datetime.now()),
                'msg_type': msg_type,
                'entrust_status': entrust_status,
                'order_price': data_item.get('price'),
                'order_vol': data_item.get('volume'),
                'direction': data_item.get('direction'),
                'open_close': data_item.get('open_close'),
                'speculator': data_item.get('investor_type'),
                'order_price_type': data_item.get('order_type'),
                'trade_price': 0,
                'trade_vol': 0,
                'vol_remain': data_item.get('remain_vol', data_item['volume']),
                'serial_no': data_item.get('order_id')
            }
            self.write_row.append(order_item)
            ntm_list.append(order_item)

    def record_response(self, resps, exchange_time):
        for resp in resps:
            symbol = resp['symbol'].decode() if isinstance(resp['symbol'], bytes) else resp['symbol']
            tm_data = self.trd_msg.setdefault(str(self.st_id), {})
            tm_list = tm_data.setdefault(symbol, [])
            ntm_data = self.no_trd_msg.setdefault(str(self.st_id), {})
            ntm_list = ntm_data.setdefault(symbol, [])
            f_order = self.find_org_order(resp['serial_no'])
            fill_price = resp['price']
            fill_volume = resp['volume']
            if resp['entrust_status'] in [0, 3]:
                # complete
                f_order['remain_vol'] = 0
            elif resp['entrust_status'] == 2:
                # partial
                f_order['remain_vol'] -= resp['volume']
            else:
                # nothing
                fill_price = 0
                fill_volume = 0
            resp_item = {
                'vsid': self.st_id,
                'server': self.server,
                'account': self.account,
                'symbol': symbol,
                'exchange_time': exchange_time,
                'calendar_time': str(datetime.datetime.now()),
                'msg_type': self.entrust_status_to_tunnel_msg_type(resp['entrust_status']),
                'entrust_status': self.enum_entrust_status_to_normal(resp['entrust_status']),
                'order_price': f_order.get('price'),
                'order_vol': f_order.get('volume'),
                'direction': f_order.get('direction'),
                'open_close': f_order.get('open_close'),
                'speculator': f_order.get('investor_type'),
                'order_price_type': f_order.get('order_type'),
                'trade_price': fill_price,
                'trade_vol': fill_volume,
                'remain_vol': f_order.get('remain_vol'),
                'serial_no': str(resp['serial_no'])
            }
            self.write_row.append(resp_item)
            if resp['entrust_status'] in [0, 2]:
                # trade rtn
                tm_list.append(resp_item)
            else:
                # no trade rtn
                ntm_list.append(resp_item)
            if resp['entrust_status'] in [0, 3]:
                # complete
                del self.hang_order[resp['serial_no']]

    def get_trade_msg(self):
        return self.trd_msg

    def get_no_trade_msg(self):
        return self.no_trd_msg

    def _clear(self):
        self.trd_msg = {}
        self.no_trd_msg = {}
        self.hang_order = {}
        self.write_row = []
        self.f_hdl = None

    def reset(self):
        if self.f_hdl and self.write_row:
            self.flush_data()
            self.f_hdl.close()
            self.f_hdl = None
        self._clear()
