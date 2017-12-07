# -*-coding: utf-8 -*-

import json
from collections import OrderedDict
from enum import Enum

import numpy as np

try:
    from .config import NqpPathParser
except ImportError:
    from config import NqpPathParser

npq_hdl = NqpPathParser()


class CURRENCY(Enum):
    CNY = 0  # Chinese Yuan
    USD = 1  # US Dollar
    HKD = 2  # HongKong Dollar
    EUR = 3  # Euro
    JPY = 4  # Japanese Yen
    GBP = 5  # Puond Sterling
    AUD = 6  # Australian Dollar
    CAD = 7  # Canadian Dollar
    SEK = 8  # Swedish Krona
    NZD = 9  # New Zealand Dollar
    MXN = 10  # Mexican Peso
    SGD = 11  # Singapore Dollar
    NOK = 12  # Norwegian Krone
    KRW = 13  # South Korean Won
    TRY = 14  # Turkish Lira
    RUB = 15  # Russian Ruble
    INR = 16  # Indian Rupee
    BRL = 17  # Brazilian Real
    ZAR = 18  # South African Rand
    CNH = 19  # Offshore Chinese Yuan


currency_dic = {
    "M0000185": CURRENCY.USD.value,
}

Ashare_dtype = np.dtype([
    ('SYMBOL', 'S9'),
    ('TRADE_DT', '<i4'),
    ('OBJECT_ID', 'S38'),
    ('CRNCY_CODE', 'S3'),
    ('S_DQ_PRECLOSE', '<f8'),
    ('S_DQ_OPEN', '<f8'),
    ('S_DQ_HIGH', '<f8'),
    ('S_DQ_LOW', '<f8'),
    ('S_DQ_CLOSE', '<f8'),
    ('S_DQ_CHANGE', '<f8'),
    ('S_DQ_PCTCHANGE', '<f8'),
    ('S_DQ_VOLUME', '<f8'),
    ('S_DQ_AMOUNT', '<f8'),
    ('S_DQ_ADJFACTOR', '<f8'),
    ('S_DQ_AVGPRICE', '<f8'),
    ('OPDATE', '<f8'),
    ('OPMODE', 'S1'),
    ('S_DQ_TRADESTATUS', 'S9'),
    ('UPDATETIME', '<f8'),
    ('date', '<i4')], align=False)

CFutures_dtype = np.dtype([
    ('SYMBOL', 'S16'),
    ('TRADE_DT', '<i4'),
    ('PRESETTLEPRICE', '<f8'),
    ('OPENPRICE', '<f8'),
    ('HIGHPRICE', '<f8'),
    ('LOWPRICE', '<f8'),
    ('CLOSEPRICE', '<f8'),
    ('SETTLEPRICE', '<f8'),
    ('VOLUME', '<f8'),
    ('AMOUNT', '<f8'),
    ('OPENINTEREST', '<f8'),
    ('EXCHANGE', 'S8'),
    ('SYMBOL1', 'S16'),
    ('PRODUCT', 'S8'),
    ('CHG', '<f8'),
    ('date', '<i4'),
    ('PRELASTCLOSE', '<f8')], align=False)

CGoldSpotEODPrices_dtype = np.dtype([
    ('S_INFO_WINDCODE', 'S12'),
    ('TRADE_DT', '<i4'),
    ('S_DQ_OPEN', '<f8'),
    ('S_DQ_HIGH', '<f8'),
    ('S_DQ_LOW', '<f8'),
    ('S_DQ_CLOSE', '<f8'),
    ('S_DQ_AVGPRICE', '<f8'),
    ('S_DQ_VOLUME', '<f8'),
    ('S_DQ_AMOUNT', '<f8'),
    ('S_DQ_OI', '<f8'),
    ('DEL_AMT', '<f8'),
    ('S_DQ_SETTLE', '<f8'),
    ('DELAY_PAY_TYPECODE', '<f8'),
    ('OPDATE', '<f8'),
    ('OPMODE', 'S1'),
    ('CONSTITUENTCODE', 'S8'),
    ('SYMBOL', 'S8'),
    ('date', '<i4')], align=False)

EONPrices_dtype = np.dtype([
    ('date', '<i4'),
    ('Symbol', 'S12'),
    ('OpenPrice', '<f8'),
    ('HighestPrice', '<f8'),
    ('LowestPrice', '<f8'),
    ('ClosePrice', '<f8'),
    ('Volume', '<i8'),
    ('Turnover', '<f8'),
    ('OpenInterest', '<f8'),
    ('Exchange', 'S4'),
    ('DataSource', 'S11')], align=False)

SpotWind_dtype = np.dtype([
    ('ITEM', 'S8'),
    ('TRADE_DT', '<i4'),
    ('ITEM_DATA', '<f8'),
    ('date', '<i4')], align=False)

FeeRateV2_dtype = np.dtype([
    ('Product', 'S7'),
    ('MyProduct', 'S7'),
    ('ExchCode', 'S1'),
    ('FeeMode', '<i4'),
    ('TradeUnit', '<i4'),
    ('MyIntraSimuFee', '<f8'),
    ('TickSize', '<f4'),
    ('MyInterSimuFee', '<f8'),
    ('ExchIntraOpenFee', '<f8'),
    ('ExchIntraCloseFee', '<f8'),
    ('ExchInterCloseFee', '<f8'),
    ('MyBrokerOpenFee', '<f8'),
    ('ExchInterOpenFee', '<f8')], align=False)

DataFromWindAPI_dtype = np.dtype([
    ('ITEM', 'S8'),
    ('TRADE_DT', '<i4'),
    ('ITEM_DATA', '<f8'),
    ('date', '<i4')], align=False)

ConStock_dtype = np.dtype([
    ('SYMBOL', 'S16'),
    ('TRADE_DT', '<i4'),
    ('TradeStatus', 'S8'),
    ('INDEXCODE', 'S8'),
    ('INDUSTRYCODE', 'S8'),
    ('Constituent', '<i8'),
    ('ReferenceOpenPrice', '<f4'),
    ('OpenWeight', '<f4'),
    ('SharesInIndex', '<i8'),
    ('closePrice', '<f4'),
    ('CloseWeight', '<f4'),
    ('Divisor', '<f8')], align=False)


def get_con_stock(date, symbol):
    """
    for chaolin the first time:
    get stock cons,

    param1: date
    param2: sym: Index list :eg. ["HS300","ZZ500"]
    enum:A50, SH50, HS300, ZZ500, ZZ800, ZZ1000,
    Returns:a numpy array list with dtype S16
    usage:
            data =get_con_stock('2017-01-03',['ZZ500'])
    """
    try:
        if isinstance(date, int):
            date = str(date)
        date = int(date.replace('-', ''))
        data = get_tables(date, "ConsituentStock", ConStock_dtype)
        res = []
        for x in data:
            if x[3].decode('utf-8') in symbol:
                res.append(x[0].decode('utf-8'))
        return np.array(res, dtype='S16')
    except Exception as s:
        return np.array([], dtype='S16')


def get_tables(date, tbl, dtype):
    """
    util function. not open as api
    get data from disk path(date+table) and use dtype to parse
    Returns: np.array data
    """
    npq_file = npq_hdl.path_table(date, tbl)
    try:
        data = np.fromfile(npq_file, dtype=dtype)
        return data
    except Exception as s:
        return []


def get_tables_daynight(date, day_night, tbl, dtype):
    """
    util function. not open as api
    get data from disk path(date+table+daynight) and use dtype to parse
    Returns: np.array data
    """
    npq_file = npq_hdl.path_table_day_night(date, day_night, tbl)
    try:
        data = np.fromfile(npq_file, dtype=dtype)
        return data
    except Exception as s:
        return []


def fee2json(data):
    """
    util function for feev2,get some columns for feeratev2
    and wrap it into dict to ready fo json
    """
    # dic = OrderedDict()
    dic = {}
    dic['FeeMode'] = bool(data[3])
    dic['MyIntraSimuFee'] = float(data[5])  # floatkk
    dic['TickSize'] = float(data[6])  # float
    dic['MyInterSimuFee'] = float(data[7])  # float
    dic['ExchCode'] = data[2]  # str
    dic['TradeUnit'] = int(data[4])  # int
    dic['ExchIntraOpenFee'] = float(data[8])  # float
    dic['ExchIntraCloseFee'] = float(data[9])  # float
    dic['ExchInterCloseFee'] = float(data[10])  # float
    dic['MyBrokerOpenFee'] = float(data[11])  # float
    dic['ExchInterOpenFee'] = float(data[12])  # float
    return dic


def utf(x):
    """
    compatibility for python3:
    if python3 read data from npq,if the data is str type,it witll to bytes
    so decode it to utf-8
    """
    res = []
    for xx in x:
        if type(xx) == np.bytes_:
            xx = xx.decode("utf-8")
        res.append(xx)
    return res


def get_feev2(date, category):
    """
    for wangzheng the first time:
    get feeratev2 api:
    param1:date,
    param2:day_night
    Returns:
            Json Data

    usage:
       get_feev2('2017-09-01',0)

    """
    date = int(date.replace('-', ''))
    try:
        data = get_tables_daynight(date, '0', "feev2", FeeRateV2_dtype)
        dic = {}
        for x in data:
            # utf(x)
            x = utf(x)
            dic[x[0]] = fee2json(x)
        return dic
    except Exception as s:
        return {}


def get_fee_direction(date, sym=('S0182164', 'S0182163')):
    """
    for wangzheng the first time:
    get fee_direction,

    param1: date
    param2: sym: defalut is ag and au.

    Returns:a list of data	for the param2 passed symbol
    usage:
            get_fee_direction('2017-09-01')
    """
    date = int(date.replace('-', ''))
    try:
        data = get_tables(date, "SpotWind", SpotWind_dtype)
        res = []
        for x in data:
            if x[0].decode('utf-8') in sym:
                res.append(x[2])
        return res
    except Exception as s:
        return []


def get_currency(date):
    """
    for colin the first time:
    get currency
    params: date
    Returns: the whole currency in CURRENCY enum and datatype is np array
    usage:
            get_currency('20170901')
            get_currency('2017-09-01')
"""
    date = int(date.replace('-', ''))
    try:
        data = get_tables(date, "SpotWind", SpotWind_dtype)
        res = []
        for x in data:
            key_ = x[0].decode('utf-8')
            if key_ in currency_dic.keys():
                res.append((currency_dic.get(key_), x[2]))
        return np.array(res, dtype=([('Currency', 'i4'), ('Ratio', 'f8')]))
    except Exception as s:
        return []


def get_defer_fee_rate(date):
    """
    for wangzheng the first time:
    get defer fee rate just for au ag
    param1: date
    Returns: the tuple data
    usage:
            get_defer_fee_rate('2017-09-01')
    """
    try:
        date = int(date.replace('-', ''))
        data = get_tables(date, "DataFromWindAPI", DataFromWindAPI_dtype)
        au = None
        ag = None
        for x in data:
            if x[0].decode("utf-8") == "MY010002":
                ag = x[2]
            if x[0].decode("utf-8") == "MY010001":
                au = x[2]
        return ag, au
    except:
        return None, None


def get_stoke_fee_rate(date):
    """
    for wangzheng the first time:
    get stoke fee rate
    exchange_fee_rate , broker_fee_rate,transfer_fee_rate,stamp_fee_rate
    param1: date
    Returns: json Data
    usage:
            get_stoke_fee_rate('2017-09-04'))
    """

    dic = OrderedDict()
    dic['exchange_fee_rate'] = None
    dic['broker_fee_rate'] = None
    dic['transfer_fee_rate'] = None
    dic['stamp_fee_rate'] = None
    try:
        date = int(date.replace('-', ''))
        data = get_tables(date, "DataFromWindAPI", DataFromWindAPI_dtype)
        exchange_fee_rate = None
        broker_fee_rate = None
        transfer_fee_rate = None
        stamp_fee_rate = None
        for x in data:
            first_col = x[0].decode('utf-8')
            if first_col == "MY020001":
                exchange_fee_rate = x[2]
            elif first_col == "MY020002":
                broker_fee_rate = x[2]
            elif first_col == "MY020003":
                transfer_fee_rate = x[2]
            elif first_col == "MY020004":
                stamp_fee_rate = x[2]
        dic = OrderedDict()
        dic['exchange_fee_rate'] = exchange_fee_rate
        dic['broker_fee_rate'] = broker_fee_rate
        dic['transfer_fee_rate'] = transfer_fee_rate
        dic['stamp_fee_rate'] = stamp_fee_rate
        return json.dumps(dic, sort_keys=False)
    except:
        return json.umps(dic, sort_keys=False)


def get_end_price(symbol, sym_type, date, daynight):
    """
    for wangzheng the firs time:
    get end price for different symbol: stoke,future,spot
    usage:
      get_end_price("000001","stock",'2017-09-01',0)
      get_end_price("l1709","future",'2017-05-11',0)
      get_end_price("c1605","future","2016-01-08",0)
      get_end_price("au(t+d)","spot",'2017-09-01',0)
    Returns:
            Only One Value

    """
    date = int(date.replace('-', ''))
    if sym_type == "stock":
        data = get_tables(date, "AShareEODPrices", Ashare_dtype)
        if len(data) > 0:
            for d in data:
                if d[0].decode('utf-8')[0:-3] == symbol:
                    return d[8]
    if sym_type == "spot":
        if daynight == 0:
            data = get_tables(date, "CGoldSpotEODPrices",
                              CGoldSpotEODPrices_dtype)
            if len(data) > 0:
                for d in data:
                    if d[-2].decode('utf-8').lower() == symbol:
                        return d[11]
        if daynight == 1:
            data = get_tables(date, "EONPrices", EONPrices_dtype)
            if len(data) > 0:
                for d in data:
                    if d[1].decode('utf-8').lower() == symbol:
                        return d[5]
    if sym_type == "future":
        if daynight == 0:
            data = get_tables(date, "CFuturesEODPrices", CFutures_dtype)
            if len(data) > 0:
                for d in data:
                    if d[0].decode('utf-8').lower() == symbol:
                        return d[7]
        if daynight == 1:
            data = get_tables(date, "EONPrices", EONPrices_dtype)
            if len(data) > 0:
                for d in data:
                    if d[1].decode('utf-8').lower() == symbol:
                        return d[5]

    return None


if __name__ == "__main__":
    # print(get_end_price("000001", "stock", '2017-09-01', 0))
    # print(get_end_price("l1709", "future", '2017-05-11', 0))
    # print(get_end_price("c1605", "future", "2016-01-08", 0))
    # print(get_end_price("au(t+d)", "spot", '2017-09-01', 0))
    # print(get_fee_direction('2017-09-01'))
    # print(get_defer_fee_rate('2017-09-01'))
    # print(get_stoke_fee_rate('2017-09-04'))

    print(get_feev2('2017-09-01', 0))

    # print("----------")
    # print(get_con_stock('2017-01-03', ['HS300']))
    # print(get_con_stock('2017-01-03', ['A50']))
    # print(get_con_stock('2017-01-03', ['SH50']))
    # print(get_con_stock('2017-01-03', ['ZZ800']))
    # print(get_con_stock('2017-01-03', ['ZZ1000']))
    # print(get_con_stock('2017-01-03', ['ZZ500']))
    # print("----------")
    # print(get_currency('20170901'))
