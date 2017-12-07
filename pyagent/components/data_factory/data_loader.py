# -*- coding: utf-8 -*-

import os
import shutil
import sys
from enum import Enum
from urllib.error import HTTPError
from urllib.parse import urljoin
from urllib.request import urlretrieve

from components.data.config import NqpPathParser, BASE_PATH, DATA_URL, TEMP_PATH
from components.data.meta_api import get_new_trading_date_range, get_mc, get_previous_date
from components.data.mi_type import MI_TYPE, MI_NEW_BOOK_BASE
from components.data.tradelist_api import get_con_stock
from components.data_factory.business import remove_letters

try:
    from .business import contains_future
except ImportError:
    from business import contains_future


class NpqMethod(Enum):
    LOCAL_COPY = 0
    REMOTE_WGETS = 1


class NpqDownloader:
    def __init__(self):
        self.npq_ldl = NqpPathParser('')

    def _dir_ensure(self, dir):
        if not os.path.isdir(dir):
            os.makedirs(dir)

    def _file_dir_ensure(self, file):
        dir = os.path.dirname(file)
        self._dir_ensure(dir)

    @property
    def npq_path_parser(self):
        return self.npq_ldl

    def local_copy(self, path, tag_name):
        # easy realization: copy from nfs mount disk to local directory
        try:
            local_file = os.path.join(BASE_PATH, path)
            if os.path.exists(local_file):
                return True
            # copy file from nfs
            source = os.path.join('x:\\', path)
            if not os.path.isfile(source):
                print("%s mount file not exist, please contact the dev team" % source)
                sys.exit(-1)
            self._file_dir_ensure(local_file)
            shutil.copy(source, local_file)
            print("download %s success" % tag_name)
            return True
        except Exception:
            print("download %s failed, try it later!" % tag_name)
            return False

    def remote_gets(self, path, tag_name):
        try:
            local_file = os.path.join(BASE_PATH, path)
            if os.path.exists(local_file):
                return True
            # wgets file from aliyun
            temp_file = os.path.join(TEMP_PATH, os.path.basename(path))
            urlpath = path.replace('\\', '/')
            remote_url = urljoin(DATA_URL, urlpath)
            # print("the remote file is", remote_url)
            urlretrieve(remote_url, temp_file)
            self._file_dir_ensure(local_file)
            shutil.move(temp_file, local_file)
            print("download %s success" % tag_name)
            return True
        except HTTPError as e:
            if e.code == 404:
                print("remote file %s not exist" % path)
            else:
                print("down %s failed, try it later!" % tag_name)
            return False
        except Exception:
            print("down %s failed, try it later!" % tag_name)
            return False

    def download(self, path, tag_name, method=NpqMethod.REMOTE_WGETS):
        if method == NpqMethod.REMOTE_WGETS:
            return self.remote_gets(path, tag_name)
        else:
            return self.local_copy(path, tag_name)

    def bulk_extra_download(self, date):
        try:
            self.download(self.npq_ldl.path_currency(date), "currency")
            return True
        except Exception:
            return False

    def bulk_contract_download(self, date):
        # download oneday contract corresponding information
        # fee & expire date
        try:
            self.download(self.npq_ldl.path_fee(date), "fee")
            self.download(self.npq_ldl.path_fee_v2(date), "fee_v2")
            self.download(self.npq_ldl.path_expire_date(), "expire_date")
            return True
        except Exception:
            return False

    def bulk_quote_download(self, date, day_night, quote_def):
        # download oneday task quote dependence
        try:
            for q_item in quote_def:
                q_path = self.npq_ldl.path_quote(date, q_item['mi_type'], q_item['code'], day_night, q_item['source'])
                self.download(q_path, "quote")
            return True
        except Exception:
            return False


npq_ldr = NpqDownloader()


def download_query_main_code(date, day_night, product, rank):
    # auto download files if not exist
    pre_date = get_previous_date(date)
    path = npq_ldr.npq_path_parser.path_main_contract(pre_date)
    print(path)
    ret = npq_ldr.download(path, "initial main code %s" % date)
    if ret:
        mc = get_mc(date, day_night, product, rank)
        if isinstance(mc, bytes):
            return mc.decode()
        return mc
    raise RuntimeError("download initial main code failed %s" % date)


def download_query_constituent_stock(date, index_name):
    # auto download files if not exist
    path = npq_ldr.npq_path_parser.path_constituent(date)
    ret = npq_ldr.download(path, "initial contituent stock %s" % date)
    if ret:
        return get_con_stock(date, index_name)
    raise RuntimeError("download initial contituent stock failed %s" % date)


def download_query_trade_date_range(date_start, date_end):
    # auto download files if not exist
    path = npq_ldr.npq_path_parser.path_trade_date()
    ret = npq_ldr.download(path, "initial date range %s,%s" % (date_start, date_end))
    if ret:
        return get_new_trading_date_range(date_start, date_end)
    raise RuntimeError("download initial trade date range failed %s,%s" % (date_start, date_end))


class TaskAnalysis:
    def __init__(self):
        pass

    @staticmethod
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

    @staticmethod
    def parse_by_length(contr_list):
        if len(contr_list) != 3:
            exch = contr_list[2]
            # convert to inner data mi_type the return value is a list
            mi_type = TaskAnalysis.mi_type_convert(contr_list[3])
        else:
            exch = None
            mi_type = [MI_TYPE.MI_MY_STOCK_LV2.new_book]
        # the configuration contains a quote source selection
        if len(contr_list) == 7:
            source = contr_list[5]
        else:
            source = 0
        return exch, mi_type, source

    @staticmethod
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

    @staticmethod
    def gather_quote_contract(symbols, exches, mi_types, org_products, sources, accounts):
        all_market = False
        parsed_contracts = []
        query_quotes = []
        key_set = set()
        for idx in range(len(symbols)):
            symbol = symbols[idx]
            exch = exches[idx]
            mi_type = mi_types[idx]
            oproduct = org_products[idx]
            source = sources[idx]
            account = accounts[idx]
            new_item = {'code': symbol, 'exch': exch, 'mi_type': mi_type, 'source': source, 'account': account}
            str_key = str(new_item)
            if str_key not in key_set:
                parsed_contracts.append(new_item)
                if oproduct == 'HSA':
                    if not all_market:
                        query_quotes.append({'code': 'all', 'exch': None, 'mi_type': mi_type, 'source': source})
                        all_market = True
                else:
                    query_quotes.append(new_item)
                key_set.add(str_key)
        return parsed_contracts, query_quotes

    @staticmethod
    def parse_contracts(date, day_night, contracts):
        """
        :param date: trade date
        :param day_night:  day night flag
        :param contracts:  the contract dictionary in task
        :return:
        """
        org_products = []
        symbols = []
        exches = []
        mi_types = []
        sources = []
        accounts = []

        for contr in contracts:
            contr_list = contr.split('|')
            exch, mi_type, source = TaskAnalysis.parse_by_length(contr_list)
            account = contr_list[-1]

            product = contr_list[0]
            rank_field = contr_list[1].strip()

            if rank_field == 'I':
                # it is stock index
                stock_list = download_query_constituent_stock(date, product)
                # all stock_t type data store with mi_type = 209
                for mt in mi_type:
                    for code in stock_list:
                        if code in symbols:
                            continue
                        tsym, texch = TaskAnalysis.winds_code_to_code_exch(code)
                        symbols.append(tsym)
                        exches.append(texch)
                        mi_types.append(mt)
                        org_products.append(product)
                        sources.append(source)
                        accounts.append(account)
            elif ',' in rank_field:
                # it has more than one rank symbol
                ranks = rank_field.split(',')
                for mt in mi_type:
                    for rank_str in ranks:
                        rank = int(remove_letters(rank_str))
                        symbol = get_mc(date, day_night, product, rank)
                        symbols.append(symbol)
                        exches.append(exch)
                        mi_types.append(mt)
                        org_products.append(product)
                        sources.append(source)
                        accounts.append(account)
            elif rank_field.strip() == '':
                # it specified the trading symbol
                for mt in mi_type:
                    symbols.append(product)
                    exches.append(exch)
                    mi_types.append(mt)
                    org_products.append(product)
                    sources.append(source)
                    accounts.append(account)
            else:
                # it only contains one rank symbol
                rank = int(remove_letters(rank_field))
                symbol = download_query_main_code(date, day_night, product, rank)
                if symbol is None:
                    err_msg = 'not valid mc: %s,%s,%s,%s' % (date, day_night, product, rank)
                    raise ValueError(err_msg)
                for mt in mi_type:
                    symbols.append(symbol)
                    exches.append(exch)
                    mi_types.append(mt)
                    org_products.append(product)
                    sources.append(source)
                    accounts.append(account)
        return TaskAnalysis.gather_quote_contract(symbols, exches, mi_types, org_products, sources, accounts)

    @staticmethod
    def disassemble_task(task):
        """
        :return: {'date': 20171010, 'day_night': 0, 'quote_def': [], 'contracts': []} iterator
        """
        # get the trade date range
        start_date = task['start_date']
        end_date = task['end_date']
        date_range = download_query_trade_date_range(start_date, end_date)
        day_night_enum = [1, 0] if task['day_night_flag'] is 2 else [task['day_night_flag']]
        date_dn_atom = []
        for tuple_dd in date_range:
            date = int(tuple_dd[0].replace('-', ''))
            has_night = tuple_dd[1]
            for dn in day_night_enum:
                # this night is not trade time
                if dn is 1 and has_night is 0:
                    continue
                else:
                    date_dn_atom.append((date, dn))
        # query each day's trade symbol and quote inner definition
        for (date, dn) in date_dn_atom:
            order_symbols, quote_symbols = TaskAnalysis.parse_contracts(date, dn, task['strat_item']['contracts'])
            trade_symbols_iter = []
            for tdata in order_symbols:
                trade_symbols_iter.append((
                    tdata['code'],
                    tdata['account'],
                    tdata['exch']
                ))
            # trade_symbols_account = list(set([(tdata['code'], tdata['account']) for tdata in order_symbols]))
            yield {
                'date': date,
                'day_night': dn,
                'quote_def': quote_symbols,
                'contracts': trade_symbols_iter,
            }


def data_download(task_jons):
    """
    :param task_jons: normal task item
    :return: print download progress return True if success else return False
    """
    try:
        d_hdl = NpqDownloader()
        for t_item in TaskAnalysis.disassemble_task(task_jons):
            date = t_item['date']
            day_night = t_item['day_night']
            print("execute date:", date, "day_night", day_night)
            quote_def = t_item['quote_def']
            contracts = t_item['contracts']
            if contains_future(contracts):
                d_hdl.bulk_contract_download(date)
            d_hdl.bulk_quote_download(date, day_night, quote_def)
        print("download task all files success")
    except Exception:
        print("download task files failed, please try later")


if __name__ == "__main__":
    import json

    with open('..\\..\\test\\config\\easy_task.json', "r") as f:
        data = f.read()
        task = json.loads(data)
        m = data_download(task)
