# -*- coding: utf-8 -*-

import os
import platform
from .mi_type import MI_TYPE

###################### Flags #######################
RELEASE = False

##################### File Path ####################
file_path = os.path.abspath(__file__)
curr_path = os.path.dirname(file_path)
comp_path = os.path.dirname(curr_path)
proj_path = os.path.dirname(comp_path)
####################################################

TEMP_PATH = os.path.join(proj_path, "test")

if platform.system() == 'Windows':
    BASE_PATH = 'c:\\data'
    # BASE_PATH = 'x:\\'
else:
    BASE_PATH = '/data'

if RELEASE:
    DATA_URL = 'http://'
else:
    DATA_URL = 'http://192.168.1.14/media/npqfiles/'


class NqpPathParser:
    def __init__(self, base_path=BASE_PATH):
        self.base_path = base_path

    def path_table(self, date, tbl):
        return os.path.join(self.base_path, str(date), '0', tbl + '.npq')

    def path_table_day_night(self, date, day_night, tbl):
        return os.path.join(self.base_path, str(date), str(day_night), tbl + '.npq')

    def path_all_depth_future(self, date, day_night, mi_type, source):
        return os.path.join(self.base_path, str(date), str(day_night), str(mi_type), str(source), 'all.npq')

    def path_quote(self, date, mi_type, code, day_night, source):
        if mi_type == MI_TYPE.MI_ALL_FUTURE.value:
            return self.path_all_depth_future(date, day_night, mi_type, source)
        return os.path.join(self.base_path, str(date), str(day_night), str(mi_type), str(source), code + '.npq')

    def path_constituent(self, date):
        return self.path_table(date, "ConsituentStock")

    def path_currency(self, date):
        return self.path_table(date, "SpotWind")

    def path_fee(self, date):
        return os.path.join(self.base_path, str(date), '0', "fee.npq")

    def path_fee_v2(self, date):
        return os.path.join(self.base_path, str(date), '0', 'feev2.npq')

    def path_expire_date(self):
        return os.path.join(self.base_path, 'basic', 'expire_date.npq')

    def path_trade_date(self):
        return os.path.join(self.base_path, 'basic', 'trading_date.npq')

    def path_trade_vol(self, date, day_night, symbol, mi_type):
        return os.path.join(self.base_path, str(date), str(day_night), str(mi_type), '0', symbol)

    def path_main_contract(self, date, method=0):
        return os.path.join(self.base_path, str(date), '0', 'mc', str(method) + '.npq')

    def path_Ashare(self, date):
        return os.path.join(self.base_path, str(date), str(0), "AShareEODPrices.npq")

    def path_CFutures(self, date):
        return os.path.join(self.base_path, str(date), str(0), "CFuturesEODPrices.npq")

    def path_EONPrices(self, date):
        return os.path.join(self.base_path, str(date), str(0), "EONPrices.npq")

    def path_CGoldSpotEODPrices(self, date):
        return os.path.join(self.base_path, str(date), str(0), "CGoldSpotEODPrices.npq")
