# -*- coding: utf-8 -*-

import numpy as np

try:
    from .config import NqpPathParser
    from .mi_type import MI_NEW_BOOK_BASE, MI_TYPE
    from .my_types import numpy_book_all_dtype, numpy_book_futures_dtype, numpy_book_stock_dtype
except ImportError:
    from config import NqpPathParser
    from mi_type import MI_NEW_BOOK_BASE, MI_TYPE
    from my_types import numpy_book_all_dtype, numpy_book_futures_dtype, numpy_book_stock_dtype

CFFEX_DEEP = 200
SHFE_DEEP = 206
SHFE_LV3 = 226
DCE_DEEP = 201
DCE_ORDER = 203
CZCE_DEEP = 207
EQUITY_DEEP = 209
EQUITY_INDEX = 210
CTP_LV1 = 212
MERGE_DCE = 227

npq_hdl = NqpPathParser()

def data(date, mi_type, code, day_night, source):
    """ Load Future and Equity depth quote.
    Examples
    -------
    >>> data(20170807,SHFE_DEEP,"ag1709",1,0)
    """
    try:
        npq_file = npq_hdl.path_quote(date, mi_type, code, day_night, source)
        if mi_type < MI_NEW_BOOK_BASE:
            return np.fromfile(npq_file, dtype=numpy_book_all_dtype)
        if mi_type in [MI_TYPE.MI_MY_STOCK_LV2.new_book, MI_TYPE.MI_MY_STOCK_INDEX.new_book]:
            return np.fromfile(npq_file, dtype=numpy_book_stock_dtype)
        else:
            return np.fromfile(npq_file, dtype=numpy_book_futures_dtype)
    except Exception:
        return None


if __name__ == "__main__":
    case_list = [
        (20171017, SHFE_DEEP, "ag1711", 0, 0),
        # (20170901, 209, "all", 0, 0),
        (20170901, DCE_DEEP, "i1709", 0, 0),
        (20170901, DCE_ORDER, "i1710", 0, 0),
        (20170901, CZCE_DEEP, "ZC710", 0, 0),
        (20170901, EQUITY_DEEP, "000001", 0, 0),
        # (20170901, EQUITY_INDEX, "000001", 0, 0),
        (20170901, SHFE_DEEP, "ag1709", 1, 0),
        (20170901, DCE_DEEP, "i1709", 1, 0),
        (20170901, DCE_ORDER, "i1710", 1, 0),
        (20170901, CZCE_DEEP, "ZC710", 1, 0),
        (20170912, CFFEX_DEEP, "T1712", 0, 0)
    ]
    for case in case_list:
        d = len(data(*case))
        print("case:", case, d)