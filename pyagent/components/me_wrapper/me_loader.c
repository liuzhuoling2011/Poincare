#include "me_module.h"
#include "me_loader.h"
#include "me.h"
#include "record_time.h"
#include <ctype.h>

#define NORMAL_MATCH  1
#define IDEAL_MATCH 2
#define LINEAR_IMPACT_MATCH 3
#define DOUBLE_GAUSSIAN_MATCH 4
#define CLASSIC_MATCH 5  //2-price dividen

#define RETURN_QT (100)

enum MARKET_TYPE{ KDB_QUOTE = 1, NUMPY_QUOTE = 2 };

static me_handle_t*
me_create(me_config_t* config)
{
    me_handle_t *handle=(me_handle_t*)malloc(sizeof(me_handle_t));
    if(!handle){
        PyErr_Format(PyExc_TypeError,"Match engine create failed:%s.", strerror(errno));
        return NULL;
    }
    /* init match engine. */
    void *me = trader_init(config->default_mt);
    if(!me){
        PyErr_Format(PyExc_TypeError, "Match engine init failed:type=%d", config->default_mt);
        return NULL;
    }
    handle->me = me;
    // load simu product info
    trader_load_product(me, config->pi_cfg, config->pro_cnt);
    // load match param config
    trader_load_config(me, config->mp_cfg, config->par_cnt);
    return handle;
}

static PyObject *
py_me_create(PyObject *self, PyObject *args)
{
    PyArrayObject *arg1;
    if (!PyArg_ParseTuple(args, "O!:create", &PyArray_Type, &arg1)){
        PyErr_Format(PyExc_TypeError,"Match engine create failed: parameter is not a numpy ndarary.");
        return NULL;
    }
    Py_INCREF(arg1);

    me_handle_t *handle = me_create((me_config_t*)PyArray_DATA(arg1));
    if(!handle){
        return NULL;
    }
    handle->conf_arg = arg1;
    PyObject *ret = Py_BuildValue("K", handle);
    return ret;
}


static PyObject *
py_me_feed_quote(PyObject *self, PyObject *args)
{
    int is_bar;
    unsigned long me_ptr, qindex;
    PyArrayObject *arg2;
    if (!PyArg_ParseTuple(args, "KOKI:feed_quote", &me_ptr, &arg2, &qindex, &is_bar)){
        PyErr_Format(PyExc_TypeError,"Match engine feed quote failed: parameter is not a numpy ndarary.");
        return NULL;
    }
    me_handle_t *h = (me_handle_t*)me_ptr;
    //numpy_book_data *book= (numpy_book_data*)PyArray_DATA(arg2);
    numpy_book_data *book= (numpy_book_data *)PyArray_GETPTR1(arg2, qindex);
    //DEBUG
    //printf("[ME] feed quote:%s\n", book->quote.symbol);
    /* Return trade response. */
#define MI_DCE_ORDER_STATISTIC  3
    if(book->mi_type == MI_DCE_ORDER_STATISTIC) {
        return PyList_New(0);
    }

    struct signal_response *trade_return;
    int trade_return_num=0;
#ifdef PROFILE
    uint64_t start, end;
    start = rdtsc();
#endif
    // only use the numpy quote now
    trader_feed_quote(h->me, &book->quote, book->mi_type, &trade_return, &trade_return_num, NUMPY_QUOTE, (bool)is_bar);
#ifdef PROFILE
    end = rdtsc();
    add_time_record("feed_quote", end - start);
#endif
    PyObject *root = PyList_New(trade_return_num);
    if (!root) {
        PyErr_Format(PyExc_ValueError, "Trade responses return failed.");
        return NULL;
    }
    int i;
    PyObject *val;
    for(i=0; i< trade_return_num; ++i) {
        PyObject *item= PyDict_New();
        struct signal_response *r = &trade_return[i];
        val = PyLong_FromLong((long)r->strat_id);
        PyDict_SetItemString(item, "strat_id", val);
        Py_DECREF(val);
                
        val = PyLong_FromUnsignedLong(r->serial_no);
        PyDict_SetItemString(item, "serial_no", val);
        Py_DECREF(val);
                
        val = PyBytes_FromString(r->symbol);
        PyDict_SetItemString(item, "symbol", val);
        Py_DECREF(val);
                
        val = PyLong_FromLong((long)r->direction);
        PyDict_SetItemString(item, "direction", val);
        Py_DECREF(val);
                
        val = PyLong_FromLong(r->open_close);
        PyDict_SetItemString(item, "open_close", val);
        Py_DECREF(val);
                
        val = PyLong_FromLong(r->entrust_no);
        PyDict_SetItemString(item, "entrust_no", val);
        Py_DECREF(val);
                
        val = PyFloat_FromDouble(r->exe_price);
        PyDict_SetItemString(item, "price", val);
        Py_DECREF(val);
                
        val = PyLong_FromLong(r->exe_volume);
        PyDict_SetItemString(item, "volume", val);
        Py_DECREF(val); 

        val = PyLong_FromLong((long)r->status);
        PyDict_SetItemString(item, "entrust_status", val);
        Py_DECREF(val); 
                
        PyList_SetItem(root, i, item);
        //Py_DECREF(item);  pylist_setitem -> borrow reference
    }
    return root;
}


static PyObject *
py_me_feed_order(PyObject *self, PyObject *args)
{
    unsigned long me_ptr;
    PyObject *arg2;
    if (!PyArg_ParseTuple(args, "KO:feed_order",&me_ptr,&arg2)){
        PyErr_Format(PyExc_TypeError,"Match engine feed order failed:"
                " parameter is not a numpy ndarary.");
        return NULL;
    }
    /* List of dicts */
    if(!PyList_Check(arg2)){
        PyErr_Format(PyExc_TypeError,"Match engine feed order parameters mismatch:"
                " parameter 2 should be a list.");
        return NULL;
    }

    me_handle_t *h = (me_handle_t*) me_ptr;
    struct signal_response ors[RETURN_QT];
    int ors_index=0;

    int i, order_len = PyList_Size(arg2);
    PyObject *item;
    for(i=0; i<order_len; ++i){
        item = PyList_GetItem(arg2, i);
        if( !item || !PyDict_Check(item)){
            PyErr_Format(PyExc_TypeError,"Match engine feed order parameters mismatch:"
                    "List item [%d] should be a dictionary.",i);
            return NULL;
        }
        // TODO: remove the type
        PyObject *v = PyDict_GetItemString(item, "type");
        if(!v || !PyLong_Check(v)){
            PyErr_Format(PyExc_TypeError,"Match engine feed order parameters mismatch:"
                    "Item [%d] key missed or value type mismatch (should be C long): type", i);
            return NULL;
        }
        int type = PyLong_AsLong(v);
        if(type==0){
            /* For cancel ordrs. */
            struct order_request co;
            memset(&co, 0, sizeof(co));
            v = PyDict_GetItemString(item, "symbol");
            /* TODO: Should deal with both unicode and bytes. */
            if(!PyBytes_Check(v)){
                PyErr_Format(PyExc_TypeError,"symbol in cancel order is not python bytes.");
                return NULL;
            }
            strncpy(co.symbol, PyBytes_AsString(v), sizeof(co.symbol));
            v = PyDict_GetItemString(item, "order_id");
            co.order_id = PyLong_AsUnsignedLong(v);
            v = PyDict_GetItemString(item, "org_order_id");
            co.org_order_id= PyLong_AsUnsignedLong(v);
            v = PyDict_GetItemString(item, "strategy_id");
            co.strat_id = (int)PyLong_AsLong(v);
            //char    direction;
            //char    open_close;
            //char    speculator;
            //char    xchg_code;
            //printf("C cancel:%s,%lu,%lu\n",co.symbol, co.order_id, co.org_order_id);

#ifdef PROFILE
            uint64_t start, end;
            start = rdtsc();
#endif
            trader_cancel_order(h->me, &co, &ors[ors_index++]);
#ifdef PROFILE
            end = rdtsc();
            add_time_record("cancel_order", end - start);
#endif
        } else {
            /* For place orders  */
            struct order_request po;
            v = PyDict_GetItemString(item, "symbol");
            if(!PyBytes_Check(v)){
                PyErr_Format(PyExc_TypeError,"symbol in place order is not python bytes.");
                return NULL;
            }
            strncpy(po.symbol, PyBytes_AsString(v), sizeof(po.symbol));
            //printf("the inner symbol is %s\n", po.symbol);
            v = PyDict_GetItemString(item, "volume");
            po.order_size = PyLong_AsLong(v);
            v = PyDict_GetItemString(item, "direction");
            po.direction = (ORDER_DIRECTION)PyLong_AsLong(v);
            v = PyDict_GetItemString(item, "open_close");
            po.open_close = (OPEN_CLOSE_FLAG)PyLong_AsLong(v);
            v = PyDict_GetItemString(item, "price");
            po.limit_price = PyFloat_AsDouble(v);
            v = PyDict_GetItemString(item, "order_id");
            po.order_id = PyLong_AsUnsignedLong(v);
            v = PyDict_GetItemString(item, "time_in_force");
            po.time_in_force = (TIME_IN_FORCE)PyLong_AsLong(v);
            v = PyDict_GetItemString(item, "type");
            po.order_type = (ORDER_TYPES)PyLong_AsLong(v);
            //v = PyDict_GetItemString(item, "investor_type");
            //po.investor_type = (INVESTOR_TYPES)PyLong_AsLong(v);
            po.investor_type = (INVESTOR_TYPES)0;
            /* close_dir (0: close yesterday, 1: close today)  is useless here. */
            //    char    xchg_code;
            //    int     max_pos;
            v = PyDict_GetItemString(item, "exchange");
            po.exchange = (Exchanges)PyLong_AsLong(v);
            v = PyDict_GetItemString(item, "strategy_id");
            po.strat_id = (int) PyLong_AsLong(v);
#ifdef PROFILE
            uint64_t start, end;
            start = rdtsc();
#endif
            trader_place_order(h->me, &po, &ors[ors_index++]);
#ifdef PROFILE
            end = rdtsc();
            add_time_record("place_order", end - start);
#endif
            if(ors_index >= RETURN_QT){
                PyErr_Format(PyExc_ValueError,"Too much order response.");
                return NULL;
            }
        }
    }

    /* Return order response. */
    PyObject *val;
    PyObject *root=PyList_New(ors_index);
    //PyObject *root=PyList_New(0);
    if(!root){
        PyErr_Format(PyExc_ValueError,"Order responses return failed.");
        return NULL;
    }
    for(i=0;i<ors_index;++i){
        PyObject *item= PyDict_New();
        struct signal_response *r= &ors[i];
        val = PyLong_FromUnsignedLong(r->serial_no);
        PyDict_SetItemString(item, "serial_no", val);
        Py_DECREF(val);
                
        val = PyBytes_FromString(r->symbol);
        PyDict_SetItemString(item, "symbol", val);
        Py_DECREF(val);
                
        val = PyLong_FromLong((long)r->direction);
        PyDict_SetItemString(item, "direction", val);
        Py_DECREF(val);
                
        val = PyLong_FromLong((long)r->open_close);
        PyDict_SetItemString(item, "open_close", val);
        Py_DECREF(val);
                
        val = PyLong_FromLong((long)r->entrust_no);
        PyDict_SetItemString(item, "entrust_no", val);
        Py_DECREF(val);
                
        val = PyFloat_FromDouble(r->exe_price);
        PyDict_SetItemString(item, "price", val);
        Py_DECREF(val);

        val = PyLong_FromLong((long)r->exe_volume);
        PyDict_SetItemString(item, "volume", val);
        Py_DECREF(val);
                
        val = PyLong_FromLong((long)r->status);
        PyDict_SetItemString(item, "entrust_status", val);
        Py_DECREF(val);

        val = PyLong_FromLong((long)r->strat_id);
        PyDict_SetItemString(item, "strat_id", val);
        Py_DECREF(val);
                
        PyList_SetItem(root, i, item);
        //PyList_Append(root, item);
        //Py_DECREF(item); // pylist_setitem -> borrow reference
    }
    return root;
}

static PyObject *
py_me_destroy(PyObject *self, PyObject *args)
{
    unsigned long me_ptr;
    if (!PyArg_ParseTuple(args, "K:destroy",&me_ptr)){
        PyErr_Format(PyExc_TypeError,"Match engine destroy failed.");
        return NULL;
    }

    me_handle_t *h = (me_handle_t*)me_ptr;
    trader_destroy(h->me);
    Py_XDECREF(h->conf_arg);
    free(h);
#ifdef PROFILE
    stat_time_record("cprofile_me.log");
#endif
    Py_RETURN_NONE;
}

extern char me_doc_create[];
extern char me_doc_feed_quote[];
extern char me_doc_feed_order[];
extern char me_doc_destroy[];

static PyMethodDef me_methods[] = {
        {"create", py_me_create,  METH_VARARGS, me_doc_create},
        {"feed_quote", py_me_feed_quote,  METH_VARARGS, me_doc_feed_quote},
        {"feed_order", py_me_feed_order,  METH_VARARGS, me_doc_feed_order},
        {"destroy", py_me_destroy,  METH_VARARGS, me_doc_destroy},
        {0, 0,0,0},     /* sentinel */
};

PyDoc_STRVAR(me_doc_module,
    "Match engine module.\n"
    "\n"
    "bss@mycapital.net "
    "MY CAPITAL All Rights Reserved. (C) 2017"
    );

static struct PyModuleDef me_module = {
    PyModuleDef_HEAD_INIT,
    "me",
    me_doc_module,
    -1,
    me_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_me(void)
{
    PyObject *m;
    import_array();

    m = PyModule_Create(&me_module);
    if (m == NULL){
        return NULL;
    }

    Py_INCREF(PyExc_OSError);
    PyModule_AddObject(m, "error", PyExc_OSError);

    /* Add Macros */
    PyModule_AddIntMacro(m, IDEAL_MATCH);
    PyModule_AddIntMacro(m, DOUBLE_GAUSSIAN_MATCH);
    PyModule_AddIntMacro(m, CLASSIC_MATCH);

    return m;
}



