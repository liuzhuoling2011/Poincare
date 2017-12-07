#!/bin/python3.5
import sys
import me
import base
import numpy as np

# print(dir(me))
# print(help(me))

def me_create():
    conf=np.zeros(1, dtype=base.me_config_dtype)
    conf['default_mt']=me.CLASSIC_MATCH

    # fill the product information
    conf['pro_cnt'] = 1
    pis = np.zeros(2048, dtype=base.product_info_dtype)
    pis['product'][0] = b'ma'
    pis['my_product'][0] = b'zzma'
    pis['exch'][0] = ord('C')
    pis['fixed'][0] = 1
    pis['rate'][0] = 0.0001
    pis['yes_rate'][0] = 0.0001
    pis['unit'][0] = 100
    pis['type'][0] = 1
    pis['multiple'][0] = 10
    pis['order_max_vol'][0] = 10000
    pis['order_cum_open_lmt'] = 100000
    conf['pro_cfg'] = pis

    # fill match parameters
    conf['par_cnt'] = 1
    mps = np.zeros(2048, dtype=base.match_para_dtype)
    mps['mode'][0] = 1
    mps['exch'][0] = ord('C')
    mps['product'][0] = 'ma'
    mps['param_num'][0] = 0
    mps['trade_ratio'][0] = 0.5
    conf['par_cfg'] = mps
    h = me.create(conf)
    return h

def main():
    is_bar = 0
    mh = me_create()
    # Load quote
    quotes = np.fromfile('/data/20170912/0/107/0/MA801.npq', dtype=base.numpy_book_dtype)
    for tick in quotes[:5]:
        print(tick['quote'])
        a=np.array(tick, dtype=base.numpy_book_dtype)
        trade_return = me.feed_quote(mh, a, 0)     #me.feed_quote(mh,tick) #will core
        print('P trade return:', trade_return)
        cancel_order=[{'type':0, 'symbol': b'MA801', 'org_order_id':123, 'order_id':1}]
        order_rsp = me.feed_order(mh, cancel_order)
        print('P cancel return:', order_rsp)
        place_order=[{'symbol': b'MA801', 'order_id': 111000001, 'strategy_id': 111, 'type': 1, 'open_close': 48, 'close_dir': 49,
                     'direction': 49, 'limit_price': 4077.0, 'volume': 10,
                     'time_in_force': 48, 'order_type': 48, 'investor_type': 48}]
        # place_order=[{'type':1, 'symbol': b'ag1709', 'volume':100, 'direction':1, 'open_close':0, 'limit_price':3000.0, 'order_id':2}]
        order_rsp = me.feed_order(mh, place_order)
        print('P place return:', order_rsp)


if __name__ == "__main__":
    main()
