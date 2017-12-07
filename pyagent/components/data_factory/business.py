# -*- coding: utf-8 -*-

import datetime
from string import digits, ascii_letters

import numpy as np
from components.data import meta_api
from components.data import tradelist_api
from components.data.mi_type import MI_TYPE, MI_NEW_BOOK_BASE
from components.st_wrapper import base as st_base


def remove_digits(tstr):
    rds = str.maketrans('', '', digits)
    return tstr.translate(rds)


def remove_letters(tstr):
    rls = str.maketrans('', '', ascii_letters)
    return tstr.translate(rls)


def winds_code_to_code_exch(code):
    if isinstance(code, bytes):
        code = code.decode()
    if isinstance(code, str):
        if code.endswith('SH'):
            return code[:-3], 'SSE'
        if code.endswith('SZ'):
            return code[:-3], 'SZSE'
        else:
            raise ValueError('it is not valid wind code')
    else:
        raise ValueError("code not valid: %s" % code)


def convert_symbol_to_product(symbol, exch):
    """
    internal product name should be lower case
    """
    if not isinstance(exch, bytes):
        exch = exch.encode()
    if isinstance(symbol, bytes):
        symbol = symbol.decode()
    if exch.upper() in [b'SHFE', b'DCE', b'CZCE', b'CFFEX']:
        # TODO: cu3m
        symbol = remove_digits(symbol)
    return symbol.lower()


def contains_future(contracts):
    """ check if a list of contract contains future data
    :param contracts:
    :return:
    """
    return any([True for (_,_,exch) in contracts if exch in ["SHFE", "DCE", "CZCE", "CFFEX"]])


def to_int_date(date):
    if isinstance(date, int):
        return date
    if isinstance(date, str):
        if ',' in date:
            date = date.replace(',', '')
        elif '-' in date:
            date = date.replace('-', '')
        return int(date)
    raise TypeError("original date error %s" % date)


def int_date_to_kdb_date(int_date):
    d = datetime.datetime.strptime(str(int_date), '%Y%m%d')
    return d.strftime('%Y.%m.%d')


def convert_to_my_product(product, exch):
    if product.lower() == "au(t+d)":
        return "Au(T+D)"
    prefix_map = {
        'SHFE': 'sh',
        'DCE': 'dl',
        'CZCE': 'zz',
        'A': 'sh',
        'B': 'dl',
        'C': 'zz'
    }
    upper_exch = exch.upper()
    if upper_exch in prefix_map:
        return '%s%s' % (prefix_map[upper_exch], product)
    return product


def reverse_to_product(product):
    if len(product) > 2:
        fake_prefix = product[:2]
        if fake_prefix in ['dl', 'sh', 'zz']:
            return product[2:]
    return product


def mi_type_convert(mi_type_str):
    if ',' in str(mi_type_str):
        shfe_normal = '%d,%d' % (MI_TYPE.MI_MY_LEVEL1.value, MI_TYPE.MI_SHFE_MY.value)
        # 12,6 -> 26
        if mi_type_str == shfe_normal:
            return [MI_TYPE.MI_SHFE_MY_LV3.value + MI_NEW_BOOK_BASE]
        dce_normal = '%d,%d' % (MI_TYPE.MI_DCE_BEST_DEEP.value, MI_TYPE.MI_DCE_ORDER_STATISTIC.value)
        # 1,3 -> 27
        if mi_type_str == dce_normal:
            return [MI_TYPE.MI_DCE_DEEP_ORDS.value + MI_NEW_BOOK_BASE]
        else:
            return [int(item) + MI_NEW_BOOK_BASE for item in mi_type_str.split(',')]
    return [int(mi_type_str) + MI_NEW_BOOK_BASE]


def convert_exchange_to_prefix(exch):
    prefix_map = {
        'SHFE': 'A',
        'DCE': 'B',
        'CZCE': 'C',
        'CFFEX': 'G',
        'SSE': '1',
        'SZSE': '0',
        'HKEX': '2',
        'FUT_EXCH': 'X',
        'SGX': 'S',
        'SGE': 'D',
        'CBOT': 'F',
        'CME': 'M',
        'LME': 'L',
        'COMEX': 'O',
        'NYMEX': 'N'
    }
    upper_exch = exch.upper()
    if upper_exch in prefix_map:
        return prefix_map[upper_exch]
    else:
        raise ValueError('%s not found in exchange prefix map' % exch)

def get_currency(date):
    currency = tradelist_api.get_currency(str(date))
    reversed_currency = {}
    for cur in currency:
        cur_type, rate = cur
        enum_cur = tradelist_api.CURRENCY(cur_type)
        reversed_currency[enum_cur.name] = (cur_type, rate)
    reversed_currency.setdefault('CNY', (0, 1))
    # print(reversed_currency)
    return reversed_currency


def format_book_contract(date, day_night, constracts):
    """ query the contract information and convert it to uniform format

    "contracts": [
        "pp|R1|DCE|1,3|1|0|Account1"     # normal kind
        "SH50|I|TestAccount",            # stock index item
        "600820||SSE|9|100|TestAccount", # specified symbol directly
        "pp|R1|DCE|1,3|1|TestAccount",   # product and rank
        "rb1701||SHFE|6|5|TestAccount",  # with source
    ],
    """
    # convert it to same format of
    # "symbol|exchange|mi_type|max_pos|source|account|expire_date"

    parsed_contracts = []
    query_quotes = []
    key_set = set()
    is_stock = False
    org_products = []
    symbols = []
    exches = []
    mi_types = []

    for contr in constracts:
        contr_list = contr.split('|')
        # stock index
        if len(contr_list) != 3:
            exch = contr_list[2]
            # convert to inner data mi_type the return value is a list
            mi_type = mi_type_convert(contr_list[3])
            max_pos = contr_list[4]
            if MI_TYPE.MI_MY_STOCK_LV2.new_book in mi_type:
                is_stock = True
        else:
            exch = None
            mi_type = [MI_TYPE.MI_MY_STOCK_LV2.new_book]
            max_pos = 0
            is_stock = True
        # the configuration contains a quote source selection
        if len(contr_list) == 7:
            source = contr_list[5]
        else:
            source = 0
        product = contr_list[0]
        rank_field = contr_list[1].strip()
        account = contr_list[-1]
        # records each symbol's information
        if rank_field == 'I':
            stock_list = tradelist_api.get_con_stock(str(date), product)
            # all stock_t type data store with mi_type = 209
            for mt in mi_type:
                for code in stock_list:
                    if code in symbols:
                        continue
                    tsym, texch = winds_code_to_code_exch(code)
                    symbols.append(tsym)
                    exches.append(texch)
                    mi_types.append(mt)
                    org_products.append(product)
        elif ',' in rank_field:
            # more than one rank symbol
            ranks = rank_field.split(',')
            for mt in mi_type:
                for rank_str in ranks:
                    rank = int(remove_letters(rank_str))
                    symbol = meta_api.get_mc(date, day_night, product, rank)
                    symbols.append(symbol)
                    exches.append(exch)
                    mi_types.append(mt)
                    org_products.append(product)
        elif rank_field.strip() == '':
            # it specified the trading symbol
            for mt in mi_type:
                symbols.append(product)
                exches.append(exch)
                mi_types.append(mt)
                org_products.append(product)
        else:
            # only one rank symbol
            rank = int(remove_letters(rank_field))
            symbol = meta_api.get_mc(date, day_night, product, rank)
            if symbol is None:
                err_msg = 'not valid mc: %s,%s,%s,%s' % (date, day_night, product, rank)
                raise ValueError(err_msg)
            for mt in mi_type:
                symbols.append(symbol)
                exches.append(exch)
                mi_types.append(mt)
                org_products.append(product)
    all_market = False
    for idx in range(len(symbols)):
        symbol = symbols[idx]
        exch = exches[idx]
        mi_type = mi_types[idx]
        oproduct = org_products[idx]
        if is_stock:
            expire_date = 0
        else:
            expire_date = meta_api.get_expire_date(symbol, date)
        new_item = [symbol, exch, mi_type, max_pos, source, account, expire_date]
        str_key = str(new_item)
        if str_key not in key_set:
            parsed_contracts.append(new_item)
            if oproduct == 'HSA':
                if not all_market:
                    query_quotes.append(['all', None, mi_type, max_pos, source, account, expire_date])
                    all_market = True
            else:
                query_quotes.append(new_item)
            key_set.add(str_key)
    return parsed_contracts, is_stock, query_quotes


def format_position(strategy_item_cfg):
    """ format the initial position in task config

    "yesterday_pos": [
        "rb1706|long_volume|long_price|short_volume|short_price"
    ]
    """
    format_pos = {'yes_pos': {}, 'tod_pos': {}}
    if 'yesterday_pos' in strategy_item_cfg:
        for pos_cfg in strategy_item_cfg['yesterday_pos']:
            pos_l = pos_cfg.split('|')
            symbol = pos_l[0]
            pos = np.zeros(1, st_base.st_position)
            pos['long_volume'] = pos_l[1]
            pos['long_price'] = pos_l[2]
            pos['short_volume'] = pos_l[3]
            pos['short_price'] = pos_l[4]
            format_pos['yes_pos'][symbol] = pos
    if 'today_pos' in strategy_item_cfg:
        for pos_cfg in strategy_item_cfg['today_pos']:
            pos_l = pos_cfg.split('|')
            symbol = pos_l[0]
            pos = np.zeros(1, st_base.st_position)
            pos['long_volume'] = pos_l[1]
            pos['long_price'] = pos_l[2]
            pos['short_volume'] = pos_l[3]
            pos['short_price'] = pos_l[4]
            format_pos['tod_pos'][symbol] = pos
    return format_pos


def format_future_info(date, day_night, contracts):
    """ query fee information and init data
    """
    product_info = {}
    for contr in contracts:
        symbol = contr[0]
        if isinstance(symbol, bytes):
            symbol = symbol.decode()
        exch = contr[1]
        product = convert_symbol_to_product(symbol, exch)
        if product not in product_info:
            fee_item = meta_api.get_latest_fee(date, day_night, product)
            product_info[symbol] = fee_item
    return product_info


def byte_to_str(data):
    if isinstance(data, bytes):
        return data.decode()
    return data


def format_product_exch(symbol_cfg):
    product_exch_map = {}
    for item in symbol_cfg:
        symbol = byte_to_str(item[0])
        exch_str = byte_to_str(item[1])
        product = convert_symbol_to_product(symbol, exch_str)
        product_exch_map[product] = convert_exchange_to_prefix(exch_str)
    return product_exch_map


def query_exch_by_symbol(symbol):
    product = remove_digits(symbol)
    if isinstance(product, str):
        product = product.lower()
    exch_map = {
        'SHFE': ['cu', 'ni', 'rb', 'fuefp', 'ag', 'snefp', 'pb', 'au', 'ru', 'alefp', 'hc', 'hcefp', 'auefp', 'al',
                 'pbefp', 'cuefp', 'sn', 'wrefp', 'wr', 'buefp', 'znefp', 'fu', 'ruefp', 'niefp', 'zn', 'rbefp',
                 'agefp', 'bu'],
        'DCE': ['cs', 'c', 'jd', 'l', 'p', 'a', 'pp', 'm', 'j', 'y', 'bb', 'fb', 'b', 'jm', 'v', 'i'],
        'CZCE': ['cy', 'zc', 'sm', 'fg', 'rs', 'jr', 'wh', 'sr', 'src', 'ta', 'lr', 'pm', 'sf', 'srp', 'ma', 'ri', 'oi',
                 'cf', 'rm'],
        'CFFEX': ['tf', 'ih', 'ic', 't', 'if'],
    }
    for key, value in exch_map.items():
        if product in value:
            return key
    raise RuntimeError("symbol %s not found exchange" % symbol)


def get_exchange(datadict, symbol):
    if symbol in datadict:
        return datadict[symbol]
    return query_exch_by_symbol(symbol)


def exch_str_to_int(exch):
    if isinstance(exch, int):
        return exch
    if isinstance(exch, bytes):
        exch = exch.decode()
    if isinstance(exch, str):
        exch_int_map = {
            'SZSE': ord('0'),
            'SSE': ord('1'),
            'HKEX': ord('2'),
            'SHFE': ord('A'),
            'DCE': ord('B'),
            'CZCE': ord('C'),
            'CFFEX': ord('G'),
            'FUT_EXCH': ord('X'),
            'SGX': ord('S'),
            'SGE': ord('D'),
            'CBOT': ord('F'),
            'CME': ord('M'),
            'LME': ord('L'),
            'COMEX': ord('O'),
            'NYMEX': ord('N')
        }
        return exch_int_map[exch]
    raise ValueError("not found the key %s in exch_int map" % exch)


def format_symbol_exch(symbol_cfg):
    symbol_exch_map = {}
    for item in symbol_cfg:
        symbol = byte_to_str(item[0])
        exch_str = byte_to_str(item[1])
        symbol_exch_map[symbol] = exch_str
    return symbol_exch_map


def to_day_night_str(day_night):
    day_night_map = {
        0: 'DAY',
        1: 'NIGHT',
    }
    if day_night in day_night_map:
        return day_night_map[day_night]
    raise RuntimeError("day_night flag not valid %s" % day_night)


def to_settlement_date(int_date):
    c_date = datetime.datetime.strptime(str(int_date), "%Y%m%d")
    return c_date.strftime('%Y-%m-%d')


def dict_set_default_data(dict_data, def_key_val):
    for key, val in def_key_val.items():
        if key not in dict_data:
            dict_data[key] = val
    return dict_data


def construct_settlement_position(st_id, date, day_night, account_info, pos_info, symbol_map):
    """
    {
        '920': {
            'cash' : 2650,              # 策略现金
            'accumulated_pnl': -65.56,  # 策略累计pnl
            'daynight':'DAY',           # 日夜盘信息
            'accounts' : {              # 账号信息
                '41900045601' : {
                    'account_accumulated_pnl':-129.65,  # 账号累计pnl
                    'cash' : 2650,                      # 账号现金
                    'daynight':'DAY'                    # 日夜盘信息
                },
                ...
            },
            'data':{                     # 持仓信息
                '000776': {              # symbol信息
                    'today_long_avg_price' : 17.96 ,   # 今日多仓平均成本价
                    'today_short_avg_price' : 0 ,      # 今日空仓平均成本价
                    'yest_long_avg_price' : 16.69 ,    # 昨日多仓平均成本价
                    'yest_short_avg_price' : 0 ,       #  昨日空仓平均成本价
                    'today_long_pos': 100,             # 今日多仓仓位，股票单位：股，期货单位：手
                    'today_short_pos':0,               # 今日空仓仓位，期货单位：手
                    'yest_long_pos':100,               # 昨日多仓仓位，股票单位：股，期货单位：手
                    'yest_short_pos':0,                # 昨日空仓仓位，期货单位：手
                    'account':'41900045602',           # symbol对应账号
                    'today_settle_price' : 17.21,      # 今日结算价
                    'exchange': 'SZSE',                # 交易所
                    'daynight':'DAY',                  # 日夜盘信息
                    'symbol_net_pnl':345.67            # symbol 当前交易日、当前日夜盘的pnl
                },
                ...
            }
        }
    }
    """
    ret_data = {}
    strategy_data = ret_data.setdefault(str(st_id), {})
    cur_cash = float(account_info[0][1])
    strategy_data['cash'] = cur_cash
    strategy_data['available_cash'] = cur_cash
    strategy_data['asset_cash'] = cur_cash
    strategy_data['accumulated_pnl'] = 0
    strategy_data['day_night'] = day_night
    strategy_data['accounts'] = {
        account_info[0][0]: {
            'account_accumulated_pnl': 0,
            'cash': cur_cash,
            'day_night': day_night,
            'available_cash': cur_cash,
            'asset_cash': cur_cash
        }
    }

    position = {}
    yes_pos = pos_info['yes_pos']
    tod_pos = pos_info['tod_pos']

    for symbol, pos_np_item in yes_pos.items():
        pos_item = position.setdefault(symbol, {})
        pos_item['yest_long_avg_price'] = float(pos_np_item['long_price'])
        pos_item['yest_long_pos'] = int(pos_np_item['long_volume'])
        pos_item['yest_short_avg_price'] = float(pos_np_item['short_price'])
        pos_item['yest_short_pos'] = int(pos_np_item['short_volume'])
    for symbol, pos_np_item in tod_pos.items():
        pos_item = position.setdefault(symbol, {})
        pos_item['today_long_avg_price'] = float(pos_np_item['long_price'])
        pos_item['today_long_pos'] = int(pos_np_item['long_volume'])
        pos_item['today_short_avg_price'] = float(pos_np_item['short_price'])
        pos_item['today_short_pos'] = int(pos_np_item['short_volume'])

    default_dict = {
        'yest_long_avg_price': 0,
        'yest_long_pos': 0,
        'yest_short_avg_price': 0,
        'yest_short_pos': 0,
        'today_long_avg_price': 0,
        'today_long_pos': 0,
        'today_short_avg_price': 0,
        'today_short_pos': 0,
    }
    new_position = strategy_data.setdefault('data', {})
    for symbol, pos_detail in position.items():
        # tmp_dict = copy.deepcopy(default_dict)
        new_pos_item = dict_set_default_data(pos_detail, default_dict)
        new_pos_item['account'] = account_info[0][0]
        # it is not necessary to send a initial today_settle_price
        # new_pos_item['today_settle_price'] = 0
        new_pos_item['symbol_net_pnl'] = 0
        new_pos_item['exchagne'] = get_exchange(symbol_map, symbol)
        new_pos_item['daynight'] = day_night
        new_position[symbol] = new_pos_item
    return ret_data


def format_stock_info(trade_day, day_night, symbol_info):
    product_info = {}
    for symbol_line in symbol_info:
        product = symbol_line[0]
        if product not in product_info:
            fee_item = {
                'fee_type': 0,
                'fee': 0,
                'yes_fee': 0,
                'tick_size': 0.01,
                'lot_size': 1,
            }
            product_info[product] = fee_item
    return product_info
