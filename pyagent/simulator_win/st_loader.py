# -*- coding: utf-8 -*-

import os
import sys

import numpy as np
from components.data.my_types import ibook_futures_dtype, ibook_stock_dtype, numpy_response_dtype
from components.st_wrapper.base import st_config_dtype

try:
    from .router import simu_router
    from .sdptypes import npy_to_obj, TYPE_CONFIG, TYPE_FUTURES_QUOTE, TYPE_RESPONSE
except ImportError:
    from router import simu_router
    from sdptypes import npy_to_obj, TYPE_CONFIG, TYPE_FUTURES_QUOTE, TYPE_RESPONSE


class StrategyWrapper:
    class NoneObject:
        pass

    def __init__(self, st_path):
        module_path = os.path.dirname(st_path)
        sys.path.append(module_path)
        st_file_name = os.path.basename(st_path)
        self.module_name = self._to_str(st_file_name.split('.')[0])
        self.context_object = self.NoneObject()
        self.type_map = {
            st_config_dtype: TYPE_CONFIG,
            ibook_futures_dtype: TYPE_FUTURES_QUOTE,
            ibook_stock_dtype: TYPE_RESPONSE,
            dict: TYPE_RESPONSE,
        }
        self._import_st_api()

    def _to_str(self, f_str):
        if isinstance(f_str, bytes):
            return f_str.decode()
        return f_str

    def _import_st_api(self):
        self.st_mod = __import__(self.module_name)
        self.on_init = self.st_mod.on_init
        self.on_book = self.st_mod.on_book
        self.on_response = self.st_mod.on_response
        self.on_timer = self.st_mod.on_timer
        self.on_destroy = self.st_mod.on_day_finish

    def _response_converter(self, resp):
        np_data = np.zeros(shape=(1,), dtype=numpy_response_dtype)
        np_data['order_id'] = resp.order_id
        np_data['symbol'] = resp.symbol
        np_data['direction'] = resp.direction
        np_data['open_close'] = resp.open_close
        np_data['exe_price'] = resp.exe_price
        np_data['exe_volume'] = resp.exe_volume
        np_data['status'] = resp.status
        return np_data

    def my_on_init(self, config):
        data_type = self.type_map[config.dtype]
        inner_data = npy_to_obj(config, data_type)
        self.on_init(self.context_object, data_type, inner_data)
        ret = simu_router.gather_order()
        simu_router.clear_order()
        return ret

    def my_on_book(self, quote):
        data_type = self.type_map[quote.dtype]
        inner_data = npy_to_obj(quote, data_type)
        self.on_book(self.context_object, data_type, inner_data)
        ret = simu_router.gather_order()
        simu_router.clear_order()
        return ret

    def my_on_response(self, response):
        data_type = TYPE_RESPONSE
        np_data = self._response_converter(response)
        inner_data = npy_to_obj(np_data, data_type)
        self.on_response(self.context_object, data_type, inner_data)
        ret = simu_router.gather_order()
        simu_router.clear_order()
        return ret

    def my_on_timer(self):
        self.on_timer(self.context_object, 0, 0)
        ret = simu_router.gather_order()
        simu_router.clear_order()
        return ret

    def my_destroy(self):
        self.on_destroy(self.context_object)


if __name__ == "__main__":
    st = StrategyWrapper(r"C:\cygwin64\home\colin\projects\mp-com\pysdp\core\st.py")
    # init
    ret = st.my_on_init(np.zeros(shape=(1,), dtype=st_config_dtype))
    print("on_init", ret)
    # on book
    ret = st.my_on_book(np.zeros(shape=(1,), dtype=ibook_futures_dtype))
    print("on_book", ret)
    # on_response
    ret = st.my_on_response({
        'serial_no': 123,
        'symbol': b'rb1801',
        'direction': 0,
        'open_close': 0,
        'price': 12.8,
        'volume': 100,
        'entrust_status': 0,
    })
    print("on_response", ret)
    # on_timer
    ret = st.my_on_timer()
    print("on_timer", ret)
    # on_destroy
    st.my_destroy()