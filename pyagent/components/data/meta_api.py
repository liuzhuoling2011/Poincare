# -*- coding: utf-8 -*-

import datetime

import numpy as np

try:
    from .config import NqpPathParser
    from .my_types import fee_dtype, trading_date_dtype, main_con_dtype, expire_date_dtype
    from .tradelist_api import get_feev2
except ImportError:
    from config import NqpPathParser
    from my_types import fee_dtype, trading_date_dtype, main_con_dtype, expire_date_dtype
    from tradelist_api import get_feev2

npq_hdl = NqpPathParser()


def get_previous_date(date):
    try:
        end_fdate = datetime.datetime.strptime(str(date), "%Y%m%d")
        start_fdate = end_fdate - datetime.timedelta(days=10)
        sdate = int(datetime.datetime.strftime(start_fdate, "%Y%m%d"))
        dr = get_trading_date_range(sdate, date, "SHFE")
        return int(dr[-2].replace('-', ''))
    except Exception as e:
        raise ValueError("previous date not found") from e


def kdb_days_to_date(days):
    """Change KDB dates stamp to normal date
    Examples
    -------
    >>> kdb_days_to_date(6971)
    """
    origin_date = datetime.date(2000, 1, 1)
    return str(origin_date + datetime.timedelta(days=days))


def days_to_kdb(d):
    """Change nor dates to kdb stamp
    Examples
    -------
    >>> days_to_kdb(20170917)
    """
    years = d // 10000
    months = d // 100 % 100
    days = d % 100
    origin_date = datetime.date(2000, 1, 1)
    return (datetime.date(years, months, days) - origin_date).days


def int_date2_str(date):
    """Change int date to str date
    Examples
    -------
    >>> int_date2_str(20170807)
    """
    date = str(date)
    return "%s-%s-%s" % (date[0:4], date[4:6], date[6:])


def bytes_format_product(product):
    if isinstance(product, str):
        product = product.encode()
    if len(product) > 2 and product[:2] in [b'dl', b'sh', b'zz']:
        product = product[2:]
    return product


def str_format_product(product):
    if isinstance(product, bytes):
        product = product.decode()
    if len(product) > 2 and product[:2] in ['dl', 'sh', 'zz']:
        product = product[2:]
    return product


def get_fee(date, day_night, product):
    """Load Future and Equity depth quote.
    Examples
    -------
    >>> get_fee(20170807, 0, "ag")
    """
    try:
        npq_file = npq_hdl.path_fee(date)
        product = bytes_format_product(product)
        data = np.fromfile(npq_file, dtype=fee_dtype)
        for line in data:
            if line[5] == product:
                d = np.array(line, dtype=fee_dtype)
                return {
                    'exch': d['ExchCode'][0],
                    'fee_type': bool(d['FeeMode'] == 'ByVol'),
                    'fee': float(d['SimuFee']),
                    'tick_size': float(d['TickSize']),
                    'lot_size': float(d['TradeUnit'])
                }
        raise ValueError("fee not found %s in %s" % (product, npq_file))
    except Exception as s:
        raise ValueError("read fee failed") from s


def get_latest_fee(date, day_night, product):
    """Load lastest fee data
    Examples
    --------
    >>> get_latest_fee(20170801, 0, "ag")
    """
    try:
        product = str_format_product(product)
        data = get_feev2(str(date), day_night)
        if not data:
            # feev2 not store in kdb before 20170201
            if int(date) < 20170201:
                # none data from data feev2 then read from fee and set yest_fee = 0
                data = get_fee(date, day_night, product)
                data['yes_fee'] = 0.0
                return data
            else:
                raise ValueError("feev2 not found %s, %s, %s" % (product, date, day_night))
        for prod, val in data.items():
            if product == prod:
                return {
                    'exch': val['ExchCode'],
                    'fee_type': val['FeeMode'],
                    'fee': val['MyIntraSimuFee'],
                    'yes_fee': val['MyInterSimuFee'],
                    'tick_size': val['TickSize'],
                    'lot_size': val['TradeUnit']
                }
        raise ValueError("fee not found %s, %s, %s" % (product, date, day_night))
    except Exception as e:
        raise ValueError("read feev2 failed") from e


def upper_bytes(temp_str):
    temp_str = temp_str.upper()
    if not isinstance(temp_str, bytes):
        temp_str = temp_str.encode('utf-8')
    return temp_str


def get_mc(date, day_night, product, rank):
    """get main contract
    Examples
    -------
    >>> get_mc(20170807,0, "ag", 1)
    """
    try:
        product = bytes_format_product(product)
        date = get_previous_date(date)
        npq_file = npq_hdl.path_main_contract(date)
        data_list = np.fromfile(npq_file, dtype=main_con_dtype)
        for data in data_list:
            if data['MAINLEVEL'] == rank and data['PRODUCT'] == product:
                return data['SYMBOL']
        raise ValueError("main code level %s not found in %s" % (rank, npq_file))
    except Exception as s:
        raise ValueError("read main code error") from s


def get_expire_date(code, date):
    """get expire date for one contract code
    Examples
    -------
    >>> get_expire_date('zn1710', 20170913)
    """
    # S_INFO_DELISTDATE
    try:
        code = upper_bytes(code)
        npq_file = npq_hdl.path_expire_date()
        datalist = np.fromfile(npq_file, dtype=expire_date_dtype)
        for data in datalist:
            if data['S_INFO_CODE'] == code and days_to_kdb(date) < data[-4]:
                return kdb_days_to_date(int(data['S_INFO_DELISTDATE']))
        raise ValueError("expire_date not found %s" % code)
    except Exception as s:
        raise ValueError("read expire date error") from s


def get_trading_date_range(start_date, end_date, exch):
    """get trading date in a range
    Examples
    -------
    >>> data(20170101, 20170710, "SHFE")
    """
    try:
        start_date = days_to_kdb(start_date)
        end_date = days_to_kdb(end_date)
        npq_file = npq_hdl.path_trade_date()
        exch = upper_bytes(exch)
        data = np.fromfile(npq_file, dtype=trading_date_dtype)
        res = [kdb_days_to_date(int(d[0])) for d in data if d[1] == exch and d[0] >= start_date and d[0] <= end_date]
        return res
    except Exception as s:
        raise RuntimeError("read trading date error") from s


def get_new_trading_date_range(start_date, end_date, exch="SHFE"):
    """get trading date in a range
    Examples
    -------
    >>> data(20170101, 20170710, "SHFE")
    """
    try:
        start_date = days_to_kdb(start_date)
        end_date = days_to_kdb(end_date)
        npq_file = npq_hdl.path_trade_date()
        exch = upper_bytes(exch)
        data = np.fromfile(npq_file, dtype=trading_date_dtype)
        res = []
        for d in data:
            if d['EXCHANGE'] == exch and start_date <= d['TRADE_DT'] <= end_date:
                res.append((kdb_days_to_date(int(d['TRADE_DT'])), d['HAS_NIGHT']))
        return res
    except Exception as s:
        raise RuntimeError("read trading date error") from s


def internal2trading(date, daynight, exch="SHFE"):
    """calender date to trading date
    Examples
    -------
    >>> data(20170101, 0, "SHFE")
    """
    try:
        if daynight == 0:
            return int_date2_str(date)
        d1 = datetime.datetime.strptime(str(date), '%Y%m%d')
        delta = datetime.timedelta(days=10)
        day_start = (int)((d1 - delta).strftime('%Y%m%d'))
        day_end = (int)((d1 + delta).strftime('%Y%m%d'))
        datelist = get_trading_date_range(day_start, day_end, exch)
        for i in range(len(datelist)):
            if int_date2_str(date) == datelist[i]:
                return datelist[i + 1]
        raise ValueError("trading date not exist %s" % date)
    except Exception as s:
        raise RuntimeError("trading date convert failed") from s


if __name__ == "__main__":
    print(get_expire_date("tf1312", 20131201))
    print(get_trading_date_range(20170901, 20171101, "SHFE"))
    print(get_new_trading_date_range(20170901, 20171101, "SHFE"))
    print(internal2trading(20170103, 1))
    print(internal2trading(20160105, 1))
    print(get_latest_fee(20170801, 0, "pp"))
    print(get_fee(20160808, 0, "dlpp"))
    print(get_mc(20171101, 0, 'rb', 1))
    print(get_mc(20171101, 1, 'rb', 1))
    print(get_mc(20171102, 1, 'rb', 1))
    print(get_mc(20171102, 0, 'rb', 1))
