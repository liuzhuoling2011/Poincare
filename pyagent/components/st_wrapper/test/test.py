#!/bin/python3.5

import os
import sys
import numpy as np
import lf_strategy as strategy
from base import lf_strategy_config_dtype, numpy_book_dtype, lf_contract, lf_account


def sample_st_create():
    # initial account
    account = np.zeros(64, dtype=lf_account)
    conf = np.zeros(2, dtype=lf_strategy_config_dtype)
    strategy.load("./so/sample_easy.so")
    strategy.load("./so/sample_pass.so")
    strategy.init(conf)


def test1():
    """ test sample strategy only run new api
    """
    sample_st_create()

    quotes = np.fromfile('/data/20170912/0/107/0/MA801.npq', dtype = numpy_book_dtype)
    response = [{"symbol": b"MA801", "direction": 0, "open_close": 0, "entrust_status": 97, "entrust_no": 111123}]
    for tick in quotes:
        a = np.array(tick, dtype = numpy_book_dtype)
        signal = strategy.on_book(0, 0, a)
        if signal:
            print(signal)
            if isinstance(signal, list) and len(signal) != 0:
                response[0]['entrust_no'] += 1
                response[0]['serial_no'] = signal[0]['order_id']
                print(response)
                strategy.on_response(0, 0, response)
    strategy.destroy()


if __name__ == "__main__":
    test1()
