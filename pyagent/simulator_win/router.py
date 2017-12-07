# -*- coding: utf-8 -*-

from components.data.my_types import numpy_send_order_dtype, numpy_response_dtype
from simu_defines import Order
from tools import Singleton

S_PLACE_ORDER_DEFAULT = 0
S_CANCEL_ORDER_DEFAULT = 1
S_STRATEGY_DEBUG_LOG = 2


@Singleton
class SimuRouter:
    def __init__(self):
        self.order_list = []
        self.info_list = []
        self._type = {
            'order': numpy_send_order_dtype,
            'response': numpy_response_dtype
        }
        self.cnt_num = 0

    def get_order_id(self, strategy_id):
        self.cnt_num += 1
        return self.cnt_num * 10000000000 + strategy_id

    def gather_order(self):
        """
        :return: a list of dict which contains the order information
        """
        return self.order_list

    def gether_info(self):
        """
        :return: a list of info message
        """
        return self.info_list

    def clear_order(self):
        self.order_list = []

    def clear_info(self):
        self.info_list = []

    @property
    def types(self):
        return self._type

    def proc_order(self, context, type, order):
        if type == S_PLACE_ORDER_DEFAULT:
            order_id = self.get_order_id(order['st_id'][0])
            ord_obj = Order()
            ord_obj.symbol = order['symbol'][0]
            ord_obj.volume = order['volume'][0]
            ord_obj.exch = order['exch'][0]
            ord_obj.direction = order['direction'][0]
            ord_obj.investor_type = order['order_type'][0]
            ord_obj.open_close = order['open_close'][0]
            ord_obj.volume = order['volume'][0]
            ord_obj.order_id = order_id
            ord_obj.price = order['price'][0]
            ord_obj.time_in_force = order['time_in_force'][0]
            ord_obj.st_id = order['st_id'][0]
            order['order_id'] = order_id
            self.order_list.append(ord_obj)
        elif type == S_CANCEL_ORDER_DEFAULT:
            order_id = self.get_order_id(order['st_id'][0])
            ord_obj = Order()
            ord_obj.symbol = order['symbol'][0]
            ord_obj.exch = order['exch'][0]
            ord_obj.direction = order['direction'][0]
            ord_obj.investor_type = order['order_type'][0]
            ord_obj.volume = order['volume'][0]
            ord_obj.open_close = order['open_close'][0]
            ord_obj.order_id = order_id
            ord_obj.price = order['price'][0]
            ord_obj.time_in_force = order['time_in_force'][0]
            ord_obj.st_id = order['st_id'][0]
            ord_obj.org_ord_id = order['org_order_id'][0]
            order['order_id'] = order_id
            self.order_list.append(ord_obj)

    def send_info(self, context, type, msg):
        if type == S_STRATEGY_DEBUG_LOG:
            # TODO
            print(msg)


simu_router = SimuRouter()


def types():
    return simu_router.types


def proc_order(context, type, order):
    return simu_router.proc_order(context, type, order)


def send_info(context, type, msg):
    return simu_router.send_info(context, type, msg)
