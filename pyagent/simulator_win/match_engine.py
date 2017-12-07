# encoding: utf-8

from simu_defines import Order, OrderResp, ORDER_STATUS

# for ideal match, no need for init, destory, feed_quote
class MatchEngine(object):
    def __init__(self):
        pass
    
    def destroy(self):
        pass
    
    def feed_quote(self, quote):
        return []

    def feed_order(self, order_request):
        OrderResp = []
        for ord in order_request:
            if ord.org_ord_id == 0:
                OrderResp.append(self.add_entrust_status(ord))
                OrderResp.append(self.process_send_order(ord))
            else:
                OrderResp.append(self.process_cancel_order(ord))
        return OrderResp
    
    def add_entrust_status(self, order):
        resp = OrderResp()
        resp.order_id = order.order_id
        resp.symbol = order.symbol
        resp.direction = order.direction
        resp.open_close = order.open_close
        resp.exe_price = order.price
        resp.exe_volume = order.volume
        resp.status = ORDER_STATUS.ENTRUSTED.value
        resp.error_no = 0
        return resp
    
    def process_send_order(self, order):
        resp = OrderResp()
        resp.order_id = order.order_id
        resp.symbol = order.symbol
        resp.direction = order.direction
        resp.open_close = order.open_close
        resp.exe_price = order.price
        resp.exe_volume = order.volume
        resp.status = ORDER_STATUS.SUCCEED.value
        resp.error_no = 0
        return resp
    
    def process_cancel_order(self, order):
        resp = OrderResp()
        resp.order_id = order.org_ord_id
        resp.symbol = order.symbol
        resp.direction = order.direction
        resp.open_close = order.open_close
        resp.exe_price = order.price
        resp.exe_volume = order.volume
        resp.status = ORDER_STATUS.CANCELED.value
        resp.error_no = 0
        return resp