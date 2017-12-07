# -*- coding: utf-8 -*-

import numpy as np
from enum import Enum
from heapq import heappush, heappop
from components.data import futures


class QuoteSortMethod(Enum):
    BY_EXCHG_TIME = 0
    BY_LOCAL_TIME = 1


def merge_two_quote(q1, q2, sort_method=QuoteSortMethod.BY_EXCHG_TIME):
    """
    q1 is preference to sorted front
    @param q1, q2: numpy.array type of quote
    """
    if q1 is None and q2 is not None:
        return q2
    if q1 is not None and q2 is None:
        return q1
    if q1 is None and q2 is None:
        return None
    idx, idy = 0, 0
    cnt1, cnt2 = len(q1), len(q2)
    sort_key = 'exchange_time'
    if sort_method == QuoteSortMethod.BY_LOCAL_TIME:
        sort_key = 'local_time'
    quote_items = []
    while idx <= cnt1 and idy <= cnt2:
        if idx == cnt1:
            if idy == cnt2:
                break
            quote_items.append(q2[idy])
            idy += 1
        elif idy == cnt2:
            if idx == cnt1:
                break
            quote_items.append(q1[idx])
            idx += 1
        else:
            time1 = q1[idx][sort_key]
            time2 = q2[idy][sort_key]
            if time1 > time2:
                quote_items.append(q2[idy])
                idy += 1
            else:
                quote_items.append(q1[idx])
                idx += 1
    return quote_items


def merge_quotes(quote_list, sort_method=QuoteSortMethod.BY_EXCHG_TIME):
    """
    @param a list of predefined numpy quote
    """
    if len(quote_list) == 1:
        return quote_list[0]
    quote_arr_cnt = len(quote_list)
    pair_cnt = quote_arr_cnt // 2
    # print(pair_cnt)
    n_quote_list = []
    for idx in range(0, quote_arr_cnt, 2):
        left_q = quote_list[idx]
        right_q = quote_list[idx + 1]
        new_q = merge_two_quote(left_q, right_q, sort_method)
        n_quote_list.append(new_q)
    if pair_cnt * 2 != quote_arr_cnt:
        n_quote_list.append(quote_list[-1])
    return merge_quotes(n_quote_list, sort_method)


def convert_to_numpy(quote_list):
    return np.array(quote_list)


class TickNode:
    def __init__(self, value, bucket, tick, cmp_key):
        self.value = value
        self.bucket = bucket
        self.tick = tick
        self.cmp_key = cmp_key

    @property
    def tick_idx(self):
        return self.tick

    @property
    def bucket_idx(self):
        return self.bucket

    def __repr__(self):
        return "%d-%d-%s" % (self.bucket, self.tick, self.cmp_key)

    def __lt__(self, other):
        return self.value[self.cmp_key] < other.value[self.cmp_key]

    def __le__(self, other):
        return self.value[self.cmp_key] <= other.value[self.cmp_key]

    def __gt__(self, other):
        return self.value[self.cmp_key] > other.value[self.cmp_key]

    def __ge__(self, other):
        return self.value[self.cmp_key] >= other.value[self.cmp_key]


def tree_merge_quotes(quote_list, sort_method=QuoteSortMethod.BY_EXCHG_TIME):
    """
    @param a list of predefined numpy quote by loser tree merge sort method
    """
    # init the tournament tree(loser tree)
    cmp_key = 'exchange_time'
    if sort_method == QuoteSortMethod.BY_LOCAL_TIME:
        cmp_key = 'local_time'
    heap_tree = []
    bucket_size = []
    total_len = 0
    for bucket_idx, ql in enumerate(quote_list):
        node = TickNode(ql[0], bucket_idx, 0, cmp_key)
        heappush(heap_tree, node)
        bucket_size.append(len(ql) - 1)
        total_len += len(ql)
    g_tick_idx = 0
    arr_result = []
    while heap_tree:
        top_item = heappop(heap_tree)
        top_item.value['serial'] = g_tick_idx
        arr_result.append(top_item.value)
        g_tick_idx += 1
        if top_item.tick_idx == bucket_size[top_item.bucket_idx]:
            continue
        tick_idx = top_item.tick_idx + 1
        tick_data = quote_list[top_item.bucket_idx][tick_idx]
        new_node = TickNode(tick_data, top_item.bucket_idx, tick_idx, cmp_key)
        heappush(heap_tree, new_node)
    return arr_result


def get_combined_quote(date, day_night, quote_def, sort_method=QuoteSortMethod.BY_EXCHG_TIME):
    """
    @param date: 20171020
    @param day_night: 1
    @param quote_def: [{'source': 0, 'code': ‘rb1701’, 'mi_type': 101}]
    @sort_method: 0
    """
    if len(quote_def) == 0:
        raise ValueError("quote_def in get_combined_quote can't be empty")
    multichannel_quote = []
    _load_from_all = True
    for priv_key in quote_def:
        if priv_key['mi_type'] == 250 and _load_from_all:
            channel_quote = futures.data(date, priv_key['mi_type'], "all", day_night, priv_key['source'])
            if channel_quote is None:
                raise RuntimeError("quote_date:%s, day_night:%s, code:all, mi_type:250 not found" % (date, day_night))
            _subscribed = [i['code'].encode() for i in quote_def if i['mi_type'] == 250]
            _condition = np.isin(channel_quote['quote']['symbol'], _subscribed)
            channel_quote = np.extract(_condition, channel_quote)
            _load_from_all = False
        else:
            channel_quote = futures.data(date, priv_key['mi_type'], priv_key['code'], day_night, priv_key['source'])
        #channel_quote = futures.data(date, priv_key['mi_type'], priv_key['code'], day_night, priv_key['source'])
        # print(len(channel_quote))
        multichannel_quote.append(channel_quote)
    multichannel_quote = [qt for qt in multichannel_quote if qt is not None]
    if len(multichannel_quote) == 0:
        msg = "quote date:%s,day_night:%s,def:%s not found" % (date, day_night, quote_def)
        raise RuntimeError(msg)
    if len(multichannel_quote) == 1:
        return multichannel_quote[0]
    if len(multichannel_quote) <= 16:
        arr_quote = merge_quotes(multichannel_quote, sort_method)
    else:
        # efficiency is not as good as we imagined
        arr_quote = tree_merge_quotes(multichannel_quote, sort_method)
    return np.array(arr_quote, order='c')


if __name__ == "__main__":
    import time
    t0 = time.time()
    res = get_combined_quote(20171205, 0, [
        {'source': 0, 'code': 'SF801', 'mi_type': 250},
        {'source': 0, 'code': 'SR801', 'mi_type': 250},
        {'source': 0, 'code': 'SM801', 'mi_type': 250},
        {'source': 0, 'code': 'fb1807', 'mi_type': 250},
    ])

    #res = get_combined_quote(20171205, 0, [
    #    {'source': 0, 'code': 'MA801', 'mi_type': 107},
    #    {'source': 0, 'code': 'FG801', 'mi_type': 107},
    #    {'source': 0, 'code': 'SR801', 'mi_type': 107},],
    #                         QuoteSortMethod.BY_EXCHG_TIME)
    print(time.time() - t0)
    print(res[:5])

    #res = get_combined_quote(20170912, 0, [
    #    {'source': 0, 'code': 'all', 'mi_type': }],
    #                         QuoteSortMethod.BY_EXCHG_TIME)
    #print(len(res))
