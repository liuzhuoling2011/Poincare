#include <stdio.h>

char me_doc_create[]= "	\n\
Match engine creation.  	\n\
	\n\
Parameters 	\n\
========= 	\n\
config: numpy dtype  (base.config_dtype)\n\
\n\
Return 	\n\
====== 	\n\
match engine handle. 	\n\
\n\
Example 	\n\
======= 	\n\
import me 	\n\
import base \n\
conf=np.zeros(1,dtype=base.config_dtype) 	\n\
conf['type']=me.CLASSIC_MATCH	\n\
conf['symbol']='MA801'	\n\
conf['exchange']=ord('C')	\n\
conf['param_len']=1	\n\
conf['params'][:1]=0.8	\n\
conf['fee_mode']=1	\n\
conf['fee_rate']=0.3	\n\
conf['tick_size']=0.2	\n\
conf['lot_size'] = 100	\n\
conf['trade_ratio']=0.2	\n\
\n\
mh = me.create(conf)";


char me_doc_feed_quote[] = "\n\
     Feed quote to match engine. \n\
Parameters\n\
==========\n\
handle (integer)\n\
    match engine handle, returned by me_create()\n\
quote  numpy ndarray (base.numpy_book_data)\n\
    tick quote\n\
Return\n\
======\n\
trade returns : list of dict.\n\
    dictionary keys:\n\
        serial_no: serial number kept by strategy, increase monotonically.\n\
        symbol: trading symbol.\n\
        direction: '0' for bid, '1' for ask.\n\
        open_close: '0' for open, '1' for close.\n\
        entrust_no: entrust number returned by match engine.\n\
        price: trading price.\n\
        volume: volume traded.\n\
";

char me_doc_feed_order[] = "\n\
     Feed orders to match engine. \n\
Parameters\n\
==========\n\
handle (integer)\n\
    match engine handle, returned by me_create()\n\
orders (list of dict)\n\
    orders can be 1)place order or 2)cancel order. \n\
    1)place order keys: \n\
        symbol: symbol \n\
        volume: order volume \n\
        direction: '0' for bid, '1' for ask. \n\
        open_close: '0' for open, '1' for close. \n\
        limit_price: order limit price. \n\
        order_id: order serial number (increase monotonically). \n\
    2)cancel order keys: \n\
        symbol: symbol \n\
        order_id: order serial number \n\
        org_order_id: original order serial number. \n\
Return\n\
======\n\
order return (list of dict): \n\
    dict keys: \n\
        limit_price \n\
        serial_no \n\
        volume \n\
        symbol \n\
        exchange \n\
        entrust_no \n\
        open_close \n\
        volume_remain \n\
        direction \n\
        entrust_status \n\
";

char me_doc_destroy[] = "\n\
     Feed orders to match engine. \n\
Parameters\n\
==========\n\
handle (integer)\n\
    match engine handle, returned by me_create()\n\
Return \n\
====== \n\
None \n\
";




