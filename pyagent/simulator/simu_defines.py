# -*-coding:utf-8-*-

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
