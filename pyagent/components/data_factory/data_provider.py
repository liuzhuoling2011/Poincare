# -*- coding: utf-8 -*-

from enum import Enum

from components.data.meta_api import get_expire_date, get_latest_fee
from components.data.tradelist_api import get_currency, CURRENCY

try:
    from .data_loader import TaskAnalysis, NpqDownloader
    from .quote_factory import QuoteSortMethod, get_combined_quote
    from .business import remove_digits, contains_future, exch_str_to_int
except ImportError:
    from data_loader import TaskAnalysis, NpqDownloader
    from quote_factory import QuoteSortMethod, get_combined_quote
    from business import remove_digits, contains_future, exch_str_to_int


class CommissionType(Enum):
    BY_VOLUME = 0
    BY_MONEY = 1


class DataLoader():
    def __init__(self):
        self.npq_ldl = NpqDownloader()

    def set_trade_date(self, date, day_night):
        self.date = date
        self.day_night = day_night

    def deal_error(self, type, msg):
        print("load %s failed %s, %s, %s" % (type, self.date, self.day_night, msg))
        raise RuntimeError("download file failed")

    def _sort_method(self, quote_def):
        for item in quote_def:
            # DCE statistical quote without exchange_time
            if item['mi_type'] % 100 == 3:
                return QuoteSortMethod.BY_LOCAL_TIME
        return QuoteSortMethod.BY_EXCHG_TIME

    def load_quote(self, quote_def):
        """
        :param quote_def: [{'mi_type': 1, 'source': 0, 'contract': 'rb1801'}] a list of quote define
        :return: merged quote data
        """
        flag = self.npq_ldl.bulk_quote_download(self.date, self.day_night, quote_def)
        if flag:
            cur_sort_method = self._sort_method(quote_def)
            return get_combined_quote(self.date, self.day_night, quote_def, cur_sort_method)
        else:
            self.deal_error("quote", quote_def)

    def load_currency(self, date):
        """
        :param date: trading date
        :return: np.array of dtype=([('Currency', 'i4'), ('Ratio', 'f8')])
        """
        flag = self.npq_ldl.bulk_extra_download(self.date)
        if flag:
            data_array = get_currency(str(date))
            currency_dict = {'CNY': (0, 1.0)}
            for item in data_array:
                ctype = item['Currency']
                ratio = item['Ratio']
                currency_dict[CURRENCY(ctype).name] = (ctype, ratio)
            return currency_dict
        else:
            self.deal_error("currency", date)

    def _symbol_to_product(self, symbol):
        # stock symbol is a string of numbers
        if symbol.isdigit():
            return symbol
        return remove_digits(symbol).lower()

    def load_symbol(self, symbol, account):
        # only considering future now
        product = self._symbol_to_product(symbol)
        # print(self.date, self.day_night, product)
        symbol_info = get_latest_fee(self.date, self.day_night, product)
        c_type = CommissionType.BY_VOLUME if symbol_info['fee_type'] else CommissionType.BY_MONEY
        if product.isdigit():
            expire_date = 0
        else:
            expire_date = get_expire_date(symbol, self.date)
            expire_date = int(expire_date.replace('-', ''))
        return {
            'lot_size': symbol_info['lot_size'],
            'tick_size': symbol_info['tick_size'],
            'expire_date': expire_date,
            'fee_type': c_type,
            'fee': symbol_info['fee'],
            'yes_fee': symbol_info['yes_fee'],
            'exchange': ord(symbol_info['exch']),
            'account': account,
            'product': product,
            'broker_fee': 0.0,
            'stamp_tax': 0.0,
            'acc_transfer_fee': 0.0,
        }

    def load_code(self, code, account, exch):
        return {
            'lot_size': 0,
            'tick_size': 0,
            'expire_date': 0,
            'fee_type': 0,
            'fee': 0,
            'yes_fee': 0,
            'exchange': exch_str_to_int(exch),
            'account': account,
            'product': code,
            'broker_fee': 0.0002,
            'stamp_tax': 0.001,
            'acc_transfer_fee': 0.00002,
        }

    def load_contracts(self, contracts):
        """
        :param contract: a list of contracts
        :return: dictionary of contract information data
        """
        if contains_future(contracts):
            if not self.npq_ldl.bulk_contract_download(self.date):
                self.deal_error("contracts", contracts)
        contract_dict = {}
        for (key_code, account, exch) in contracts:
            if exch.upper() in ["SHFE", "DCE", "CZCE", "CFFEX"]:
                contract_dict[key_code] = self.load_symbol(key_code, account)
            else:
                contract_dict[key_code] = self.load_code(key_code, account, exch)
        return contract_dict


def data_produce(task_json):
    """
    :param task_json: normal task item
    :return: a generator to fetch data like above format
    """
    d_ldl = DataLoader()
    previous_date = None
    for t_item in TaskAnalysis.disassemble_task(task_json):
        date = t_item['date']
        day_night = t_item['day_night']
        quote_def = t_item['quote_def']
        contracts = t_item['contracts']
        d_ldl.set_trade_date(date, day_night)
        yield {
            'cur_date': date,
            'pre_date': previous_date,
            'day_night': day_night,
            'quote': d_ldl.load_quote(quote_def),
            'currency': d_ldl.load_currency(date),
            'contracts': d_ldl.load_contracts(contracts)
        }


def contract_info(date, contracts):
    """
    :param date: then trading date
    :param contract: the contract name
    :return: return the symbol information item in previous normal result format
    """
    d_ldl = DataLoader()
    d_ldl.set_trade_date(date, 0)
    return d_ldl.load_contracts(contracts)


if __name__ == "__main__":
    # case 1
    import json

    data = contract_info(20171101, [
        ('rb1801','123', 'SHFE'),
        ('ru1801','123', 'SHFE'),
        ('zn1801','123', 'SHFE')
    ])
    print(data)

    # case 2
    with open('..\\..\\test\\config\\easy_task.json', "r") as f:
        data = f.read()
        task = json.loads(data)
        m = data_produce(task)
        for t in m:
            print(t)
