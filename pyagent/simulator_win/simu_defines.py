# -*- coding: utf-8 -*-


from enum import Enum

class DAY_NIGHT_FLAG(Enum):
    DAY = 0
    NIGHT = 1
    DAY_AND_NIGHT = 2


class ORDER_STATUS(Enum):
    INIT = -1,
    SUCCEED = 0
    ENTRUSTED = 1
    PARTED = 2
    CANCELED = 3
    REJECTED = 4
    CANCEL_REJECTED = 5
    INTRREJECTED = 6
    UNDEFINED_STATUS = 7


class QuoteType(Enum):
    FutureQuote = 0
    StockQuote = 1
    ManualOrder = 2
    IBQuote = 3


class OpenClose(Enum):
    OPEN = 0
    CLOSE = 1
    CLOSE_TOD = 2
    CLOSE_YES = 3


class Order(object):
    def __init__(self):
        self.exch = ''
        self.symbol = ''
        self.volume = 0
        self.price = 0
        self.direction = 0
        self.open_close = 0
        self.investor_type = 0
        self.order_type = 0
        self.time_in_force = 0
        self.st_id = 0
        self.order_id = 0
        self.org_ord_id = 0

    
class OrderResp(object):
    def __init__(self):
        self.order_id = 0
        self.symbol = ''
        self.direction = 0
        self.open_close = 0
        self.exe_price = 0
        self.exe_volume = 0
        self.status = 0
        self.error_no = 0
        self.error_info = ''
