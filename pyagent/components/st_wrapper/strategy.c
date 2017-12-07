#include <unistd.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "strategy.h"
#include "msp_intf.h"

#ifdef PROFILE
#include "record_time.h"
#endif

#define MIN(a,b) ((a)>(b)?(b):(a))
#define DEFAULT_IB_QUOTE  (4)
#define STRATEGY_DEBUG   (-1)

static st_signal_t            s_st_sig;                       /* temporary strategy signal container */
static st_grp_t               s_st_grp;                       /* strategy api address container */
static st_notify_t            s_st_info;                      /* strategy uploaded data */
static bool                   set_sig_flag;                   /* signal process handler flag */
static bool                   debug_mode;                     /* strategy extra debug flag */

#define MAX_LOG_BUFF_SIZE     8 * 1024 * 1024

static int        stdout_backup;
static char       screen_log_name[1024] = "screen_log_XXXXXX";
static FILE*      screen_log_handle = NULL;
static char       screen_log_buffer[MAX_LOG_BUFF_SIZE + 1];
static char       log_name[1024];
static FILE*      log_handle = NULL;
static char       log_buffer[MAX_LOG_BUFF_SIZE + 1];
static char*      start_point = log_buffer;
static int        log_buffer_len = 0;
static PyObject  *py_sig_hdl = NULL;

const static char STATUS[][64] = { "SUCCEED", "ENTRUSTED", "PARTED", "CANCELED", "REJECTED", "CANCEL_REJECTED", "INTRREJECTED", "UNDEFINED_STATUS" };
const static char OPEN_CLOSE_STR[][16] = { "OPEN", "CLOSE", "CLOSE_TOD", "CLOSE_YES" };
const static char BUY_SELL_STR[][8] = { "BUY", "SELL" };

static void
call_py_sig_hdl(int type, const char *msg)
{
    if (py_sig_hdl == NULL) {
        return;
    }
    PyObject *arglist;
    /* build python object */
    arglist = Py_BuildValue("(is)", type, msg);
    PyObject_CallObject(py_sig_hdl, arglist);
    Py_DECREF(arglist);
}

static void pass_screen_output(){
    fclose(stdout);

    FILE* file = fopen(screen_log_name, "r");
    char rbuf[512];
    int len = 0;
    char *wbuf = screen_log_buffer;
    while (fgets(rbuf, sizeof(rbuf)-1, file)) {
        len = strlen(rbuf);
        if(wbuf - screen_log_buffer + len > MAX_LOG_BUFF_SIZE){
            *wbuf = '\0';
            call_py_sig_hdl(SCREEN_MSG, screen_log_buffer);
            wbuf = screen_log_buffer;
        }
        strcpy(wbuf, rbuf);
        wbuf += len;
    }
    fclose(file);

    *wbuf = '\0';
    call_py_sig_hdl(SCREEN_MSG, screen_log_buffer);
    
    dup2(stdout_backup, STDOUT_FILENO);
    remove(screen_log_name);
    strcpy(screen_log_name, "screen_log_XXXXXX");
}

/* flush log immediately */
static void
flush_log()
{
    if (log_handle != NULL) {
		fwrite(log_buffer, log_buffer_len, 1, log_handle);
		fflush(log_handle);
	}
	log_buffer[0] = '\0';
    start_point = log_buffer;
	log_buffer_len = 0;
}

/**
 * read from popen process
 **/
static void
popen_coredump_fp(FILE **fp) // popen() replacement
{
    char buf[1024];
    pid_t dying_pid = getpid();
	sprintf(buf, "gdb -p %d -batch -ex bt 2>/dev/null | "
			     "sed '0,/<signal handler/d'", dying_pid);
    *fp = popen(buf, "r");
    if (*fp == NULL) {
        perror("popen coredump file failed");
        exit(EXIT_FAILURE);
    }
}

static void
dump_backtrace()
{
    if(debug_mode == true){
        pass_screen_output();
    }

    int len;
    FILE *fp = NULL; 
    char rbuf[512], msg_buf[8192] = {0};
    char *wbuf = msg_buf;
    popen_coredump_fp(&fp);
    while (fgets(rbuf, sizeof(rbuf)-1, fp)) {
        len = strlen(rbuf);
        strcpy(wbuf, rbuf);
        wbuf += len;
    }
    pclose(fp);
    call_py_sig_hdl(COREDUMP_MSG, msg_buf);
    exit(0);
}

static void
recv_signal(int sig)
{
    dump_backtrace();
    flush_log();
    exit(0);
}

static int
proc_order_real(int type, int length, void *data)
{
    if (type == S_PLACE_ORDER_DEFAULT) {
        st_data_t *ord = (st_data_t *)data;
        order_t *po = (order_t *)ord->info;
        order_t *next_ord = &s_st_sig.ord[s_st_sig.ord_cnt++];
        *next_ord = *po;
        next_ord->order_id = ++s_st_sig.order_no * SERIAL_NO_MULTI + po->st_id;
        next_ord->org_ord_id = 0;
        // update order_id in data
        po->order_id = next_ord->order_id;
        if(debug_mode == true){
            printf("[LOADER] OrderID: %lld Sent single order, %s %s %s %d@%f\n", po->order_id, po->symbol,
                BUY_SELL_STR[po->direction], OPEN_CLOSE_STR[po->open_close], po->volume, po->price);
        }
        return 0;
    } else if(type == S_CANCEL_ORDER_DEFAULT) {
        st_data_t *ord = (st_data_t *)data;
        order_t *co = (order_t *)ord->info;
        order_t *next_ord = &s_st_sig.ord[s_st_sig.ord_cnt++];
        *next_ord = *co;
        next_ord->order_id = ++s_st_sig.order_no * SERIAL_NO_MULTI + co->st_id;
        //next_ord->order_id = s_st_sig.order_no * SERIAL_NO_MULTI + co->st_id;

        if(debug_mode == true){
            printf("[LOADER] OrderID: %lld Cancel single order, %s %s %s %d@%f\n", co->order_id, co->symbol,
                BUY_SELL_STR[co->direction], OPEN_CLOSE_STR[co->open_close], co->volume, co->price);
        }
        return 0;
    }
    return -1;
}

static int
send_info_real(int type, int length, void *data)
{
    /* only process the log information now */
#ifdef PROFILE
    uint64_t start, end;
    start = rdtsc();
#endif
    switch(type) {
        case S_STRATEGY_DEBUG_LOG:
        {
            if (log_handle == NULL) return 1;
			if (length + log_buffer_len >= MAX_LOG_BUFF_SIZE)
				flush_log();

            strncpy(start_point, (char*)data, length);
            if (length > 0) {
                start_point += length;
                log_buffer_len += length;
            }
        }
    }
#ifdef PROFILE
    end = rdtsc();
    add_time_record("send_info", end - start);
#endif
    return 0;
}

static int
pass_rsp_real(int type, int length, void *data)
{
    /* top level do nothing */
    return 0;
}

static int
load_strategy(const char *path, st_api_t *api)
{
    int err_no = -1;
    do {
        api->hdl = dlopen(path, RTLD_LAZY);
        if (api->hdl == NULL) {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg), "Strategy dlopen failed: %s, %s", path, dlerror());
            PyErr_Format(PyExc_TypeError, err_msg);
            break;
        }
        api->st_init = (st_data_func_t) dlsym(api->hdl, "my_st_init");
        if (!api->st_init) {
            PyErr_Format(PyExc_KeyError, "not found st_init: %s, %s", path, dlerror());
            break;
        }
        api->on_book = (st_data_func_t) dlsym(api->hdl, "my_on_book");
        if (!api->on_book) {
            PyErr_Format(PyExc_KeyError, "not found on_book: %s, %s", path, dlerror());
            break;
        }
        api->on_response = (st_data_func_t) dlsym(api->hdl, "my_on_response");
        if (!api->on_response) {
            PyErr_Format(PyExc_KeyError, "not found on_response: %s, %s", path, dlerror());
            break;
        }
        api->on_timer = (st_data_func_t) dlsym(api->hdl, "my_on_timer");
        if (!api->on_timer) {
            PyErr_Format(PyExc_KeyError, "not found on_timer: %s", path);
            break;
        }
        api->st_exit = (st_none_func_t) dlsym(api->hdl, "my_destroy");
        if (!api->st_exit) {
            PyErr_Format(PyExc_KeyError, "not found destroy: %s", path);
            break;
        }
        err_no = 0;
    } while (0);
    if (err_no != 0 && api->hdl != NULL) {
        dlclose(api->hdl);
        api->hdl = NULL;
        return err_no;
    }
    return 0;
}

static PyObject *
py_strategy_create(PyObject *self, PyObject *args)
{
    if (!set_sig_flag) {
        signal(SIGSEGV, recv_signal);
        signal(SIGABRT, recv_signal);
        set_sig_flag = true;
    }
    /* path and strategy configuration */
    const char *strategy_path;
    if (!PyArg_ParseTuple(args, "s", &strategy_path)) {
        PyErr_Format(PyExc_TypeError, "Strategy creation parameter mismatch.");
        return NULL;
    }
    /* file check */
    if (access(strategy_path, R_OK) == -1) {
        PyErr_Format(PyExc_OSError, "Strategy file can't be found:%s", strategy_path);
        return NULL;
    }
    /* load strategy api */
    int api_idx = s_st_grp.cnt;
    int ret = load_strategy(strategy_path, &s_st_grp.api[api_idx]);
    if (ret != 0) {
        return NULL;
    }
    ++s_st_grp.cnt;
    return PyLong_FromLong(api_idx);
}

static void
reset_signal_buf()
{
    s_st_sig.ord_cnt = 0;
    assert(s_st_sig.order_no < MAX_ORDER_NO);
}

static PyObject *
gather_signal_pyobject()
{
    PyObject *item;
    PyObject *signals = PyList_New(0);
    if (s_st_sig.ord_cnt == 0) {
        return signals;
    }
    /* send order*/
    int i;
    for (i = 0; i < s_st_sig.ord_cnt; i++) {
        order_t *ord = &s_st_sig.ord[i];
        // send order
        if (ord->org_ord_id == 0) {
            PyObject *d = PyDict_New();
            item = PyLong_FromLong(1);
            PyDict_SetItemString(d, "type", item);
            Py_DECREF(item);

            item = PyBytes_FromString(ord->symbol);
            PyDict_SetItemString(d, "symbol", item);
            Py_DECREF(item);

            item = PyLong_FromLong((long)ord->volume);
            PyDict_SetItemString(d, "volume", item);
            Py_DECREF(item);

            item = PyFloat_FromDouble(ord->price);
            PyDict_SetItemString(d, "price", item); 
            Py_DECREF(item);

            item = PyLong_FromLong((long)ord->direction);
            PyDict_SetItemString(d, "direction", item);
            Py_DECREF(item);
            
            item = PyLong_FromLong((long)ord->open_close);
            PyDict_SetItemString(d, "open_close", item);
            Py_DECREF(item);

            item = PyLong_FromLong((long)ord->st_id);
            PyDict_SetItemString(d, "strategy_id", item);
            Py_DECREF(item);

            item = PyLong_FromLong((long)ord->order_id);
            PyDict_SetItemString(d, "order_id", item);
            Py_DECREF(item);

            item = PyLong_FromLong((long)ord->investor_type);
            PyDict_SetItemString(d, "investor_type", item);
            Py_DECREF(item);

            item = PyLong_FromLong((long)ord->order_type);
            PyDict_SetItemString(d, "order_type", item);
            Py_DECREF(item);

            item = PyLong_FromLong((long)ord->time_in_force);
            PyDict_SetItemString(d, "time_in_force", item);
            Py_DECREF(item);

            item =  PyLong_FromLong((long)ord->exch);
            PyDict_SetItemString(d, "exchange", item);
            Py_DECREF(item);

            PyList_Append(signals, d);
            Py_DECREF(d);
        } else {
            PyObject *d = PyDict_New();
            item = PyLong_FromLong(0);
            PyDict_SetItemString(d, "type", item);
            Py_DECREF(item);

            item = PyBytes_FromString(ord->symbol);
            PyDict_SetItemString(d, "symbol", item); 
            Py_DECREF(item);

            item = PyLong_FromLong(ord->st_id);
            PyDict_SetItemString(d, "strategy_id", item);
            Py_DECREF(item);

            item = PyLong_FromLong(ord->order_id);
            PyDict_SetItemString(d, "order_id", item);
            Py_DECREF(item);

            item = PyLong_FromLong(ord->org_ord_id);
            PyDict_SetItemString(d, "org_order_id", item);
            Py_DECREF(item);

            PyList_Append(signals, d);
            Py_DECREF(d);
        }
    }
    return signals;
}

static void
destroy_strategy(int cnt)
{
    st_api_t *api;
    int i, max_idx = MIN(cnt, s_st_grp.cnt);
    for (i = 0; i < max_idx; i++)
    {
        api = &s_st_grp.api[i];
        if(api->st_exit) {
            api->st_exit();
        }
    }
    for (i = 0; i < s_st_grp.cnt; i++)
    {
        api = &s_st_grp.api[i];
        if (api->hdl) {
            dlclose(api->hdl);
        }
    }
    s_st_sig.order_no = 0;
    s_st_sig.ord_cnt = 0;
    s_st_info.msg_cnt = 0;
    memset((void *)&s_st_grp, 0, sizeof(st_grp_t));

    
#ifdef PROFILE
    stat_time_record("cprofile_strategy.log"); 
#endif
}

static PyObject *
py_set_extra(PyObject *self, PyObject *args)
{
    int debug_flag;
    const char *st_log_name;
    PyObject *tmp_hdl;
    if (!PyArg_ParseTuple(args, "zIO:set_extra", &st_log_name, &debug_flag, &tmp_hdl)) {
        PyErr_Format(PyExc_TypeError, "Set extra configuration: Parameters mismatch.");
        return NULL;
    }
    if (!PyCallable_Check(tmp_hdl)) {
        PyErr_SetString(PyExc_TypeError, "strategy signal hdl not callable");
        return NULL;
    }
    Py_XINCREF(tmp_hdl);
    py_sig_hdl = tmp_hdl;
    // Feed to strategy with an promised protocol
    if (debug_flag == 0) {
        debug_mode = false; 
    } else {
        debug_mode = true;
    }
    //printf("\n[The Strategy Debug] is %d\n", debug_flag);
    if (st_log_name != NULL && st_log_name[0] != 0) {
        strcpy(log_name, st_log_name);
        log_handle = NULL;
    } else {
        PyErr_Format(PyExc_TypeError, "Set log name: Parameters not valid.");
        return NULL;
    }
    return PyLong_FromLong(0);
}

static int
do_group_strategy_init(st_config_t *cfg_arr, int cnt)
{
    int i, ret;
    /* fill the on_book handler */
    for (i = 0; i < cnt-1; i++) {
        cfg_arr[i].proc_order_hdl = s_st_grp.api[i+1].on_book;
        cfg_arr[i].send_info_hdl = s_st_grp.api[i+1].on_book;
    }
    cfg_arr[i].proc_order_hdl = proc_order_real;
    cfg_arr[i].send_info_hdl  = send_info_real;
    /* fill the pass_rsp handler */
    for (i = 1; i < cnt; i++) {
        cfg_arr[i].pass_rsp_hdl = s_st_grp.api[i-1].on_response;
    }
    cfg_arr[0].pass_rsp_hdl = pass_rsp_real;
    /* do strategy init operation, last init strategy */
    for (i = cnt - 1; i >= 0; i--) {
        st_api_t *api = &s_st_grp.api[i];
        if (debug_mode) {
            ret = api->st_init(DEFAULT_CONFIG, STRATEGY_DEBUG, (void *)&cfg_arr[i]);
        } else {
            ret = api->st_init(DEFAULT_CONFIG, sizeof(st_config_t), (void *)&cfg_arr[i]);
        }
        if (ret != 0) {
            destroy_strategy(i);
            PyErr_Format(PyExc_TypeError, "st_init api error: %d", ret);
            return -1;
        }
    }
    return 0;
}

static PyObject *
py_strategy_init(PyObject *self, PyObject *args)
{
    PyObject *st_cfg_arr;
    if (!PyArg_ParseTuple(args, "O:init", &st_cfg_arr)) {
        PyErr_Format(PyExc_TypeError, "Strategy init failed:Parameters mismatch.");
        return NULL;
    }
    if (st_cfg_arr == NULL) {
        PyErr_Format(PyExc_ValueError, "Strategy init arguments not valid.");
        return NULL;
    }
    PyArrayObject *py_a = (PyArrayObject *) st_cfg_arr;
    npy_intp arr_size = PyArray_SIZE(py_a);
    st_config_t *cfg = (st_config_t *)PyArray_DATA(py_a);

    if (log_handle == NULL) {
        if (log_name[0] == 0) {
            PyErr_Format(PyExc_ValueError, "Should set log name first.");
            return NULL;
        }
		log_handle = fopen(log_name, "w");
		if (log_handle == NULL) {
            PyErr_Format(PyExc_ValueError, "Can't open writing log file = [%s]\n", log_name);
			return NULL;
		}
	}

    if(debug_mode == true){
        mktemp(screen_log_name);
        stdout_backup = dup(STDOUT_FILENO);
        freopen(screen_log_name,"w",stdout);
    }

    int ret = do_group_strategy_init(cfg, (int)arr_size);
    if (ret != 0) 
        return NULL;

    flush_log();
    return PyLong_FromLong(0);
}

static PyObject *
py_on_book(PyObject *self, PyObject *args)
{
    // from last to first component
    int i, type, length, qindex;
    PyArrayObject *info;
    if (!PyArg_ParseTuple(args, "IIOI:on_book", &type, &length, &info, &qindex)) {
        PyErr_Format(PyExc_TypeError, "on_book args error.");
        return NULL;
    }
    if (info == NULL) {
        PyErr_Format(PyExc_TypeError, "on_book parsed info data is null.");
        return NULL;
    }
    //void *data = (void *)PyArray_DATA(info);
    void *data = (void *)PyArray_GETPTR1(info, qindex);
    st_data_t udata;
    switch (type) {
        case DEFAULT_FUTURE_QUOTE:
            {
                numpy_future_t *nfd = (numpy_future_t *)data;
                udata.info = (void *)&(nfd->quote);
                break;
            }
        case DEFAULT_STOCK_QUOTE:
            {
                numpy_stock_t *nsd = (numpy_stock_t *)data;
                udata.info = (void *)&(nsd->quote);
                break;
            }
        case MANUAL_ORDER:
            {
                numpy_manual_order_t *moi = (numpy_manual_order_t *) data; 
                udata.info = (void *) &(moi->quote);
                break;
            }
        // TODO: support bar data
        case DEFAULT_IB_QUOTE:
            {
                numpy_book_data *nbd = (numpy_book_data *)data;
                //printf("the last_px =%lf\n", nbd->quote.last_px);
                udata.info = (void *)&(nbd->quote);
                break;
            }
    }
    reset_signal_buf();
#ifdef PROFILE
    uint64_t start, end;
    start = rdtsc();
#endif
    for (i = s_st_grp.cnt-1; i>=0; i--) {
        s_st_grp.api[i].on_book(type, length,(void *)&udata);
    }
#ifdef PROFILE
    end = rdtsc();
    add_time_record("on_book", end - start);
#endif
    /* fetch the signal triggered by strategies */
    return gather_signal_pyobject();
}

static int
consume_response(int type, int length, PyObject *data, send_data proc_f)
{
    reset_signal_buf();
    if (!PyList_Check(data)) {
        PyErr_Format(PyExc_TypeError, "Parameters mismatch: should be a python list.");
        return -1;
    }
    Py_ssize_t size = PyList_Size(data);
    st_response_t rsp_item;
    st_response_t *rsp = &rsp_item;
    st_data_t rdata = {0};
    memset(&rsp_item, 0, sizeof(st_response_t));
    for (Py_ssize_t i = 0; i < size; i++) {
        PyObject *d = PyList_GetItem(data, i);
        if (!PyDict_Check(d)) {
            PyErr_Format(PyExc_TypeError, "Parameters mismatch: item should be a python dict.");
            return -1;
        }
        strncpy(rsp->symbol, PyBytes_AsString(PyDict_GetItemString(d, "symbol")), sizeof(rsp->symbol));
        rsp->direction = (unsigned short)PyLong_AsLong(PyDict_GetItemString(d, "direction"));
        //rsp->direction -= '0';
        rsp->open_close = (unsigned short)PyLong_AsLong(PyDict_GetItemString(d, "open_close"));
        //rsp->open_close -= '0';
        rsp->order_id = (unsigned long)PyLong_AsLong(PyDict_GetItemString(d, "serial_no"));
        rsp->error_info[0] = '\0';
        rsp->error_no = 0;
        /* use this to identify it's place/cancel return */
        rsp->status = (int)PyLong_AsLong(PyDict_GetItemString(d, "entrust_status"));
        if (PyDict_GetItemString(d, "price")) {
            rsp->exe_price = PyFloat_AsDouble(PyDict_GetItemString(d, "price"));
            rsp->exe_volume = (int)PyLong_AsLong(PyDict_GetItemString(d, "volume"));
        }
        rdata.info = (void *)rsp;

        if(debug_mode == true){
            switch (rsp->status){
            case SIG_STATUS_PARTED:
                printf("[LOADER] OrderID: %lld Strategy received Partial Filled: %s %s %d@%f\n",
                    rsp->order_id, rsp->symbol,  BUY_SELL_STR[rsp->direction], rsp->exe_volume, rsp->exe_price);
                break;
            case SIG_STATUS_SUCCEED:
                printf("[LOADER] OrderID: %lld Strategy received Filled: %s %s %d@%f\n",
                    rsp->order_id, rsp->symbol, BUY_SELL_STR[rsp->direction], rsp->exe_volume, rsp->exe_price);
                break;
            case SIG_STATUS_CANCELED:
                printf("[LOADER] OrderID: %lld Strategy received Canceled\n", rsp->order_id);
                break;
            case SIG_STATUS_CANCEL_REJECTED:
                printf("[LOADER] OrderID: %lld Strategy received CancelRejected\n", rsp->order_id);
                break;
            case SIG_STATUS_REJECTED:
                printf("[LOADER] OrderID: %lld Strategy received Rejected, Error: %s\n",
                    rsp->order_id, rsp->error_info);
                break;
            case SIG_STATUS_INTRREJECTED:
                printf("[LOADER] OrderID: %lld Strategy received Rejected, Error: %s\n",
                    rsp->order_id, rsp->error_info);
                break;
            case SIG_STATUS_INIT:
                printf("[LOADER] OrderID: %lld Strategy received PendingNew\n", rsp->order_id);
                break;
            case SIG_STATUS_ENTRUSTED:
                printf("[LOADER] OrderID: %lld Strategy received Entrusted, %s %s\n",
                    rsp->order_id, rsp->symbol, BUY_SELL_STR[rsp->direction]);
                break;
            }
        }

#ifdef PROFILE
        uint64_t start, end;
        start = rdtsc();
#endif
        proc_f(type, sizeof(rdata), &rdata);
#ifdef PROFILE
        end = rdtsc();
        add_time_record("on_response", end - start);
#endif
    }
    return 0;
}

static PyObject *
py_on_response(PyObject *self, PyObject *args)
{
    int type,length;
    PyObject *info;
    if (!PyArg_ParseTuple(args, "IIO:feed_on_response", &type, &length, &info)) {
        PyErr_Format(PyExc_TypeError, "on_response args error.");
        return NULL;
    }
    if (info == NULL) {
        PyErr_Format(PyExc_TypeError, "on_response parsed info data is null.");
        return NULL;
    }
    send_data proc_f = s_st_grp.api[s_st_grp.cnt - 1].on_response;
    int ret = consume_response(type, length, info, proc_f);
    if (ret != 0) {
        return NULL;
    }
    return gather_signal_pyobject();
}

static PyObject *
py_on_timer(PyObject *self, PyObject *args)
{
    int type,length;
    PyObject *info;
    if (!PyArg_ParseTuple(args, "IIO:feed_on_timer", &type, &length, &info)) {
        PyErr_Format(PyExc_TypeError, "on_timer args error.");
        return NULL;
    }
    if (info == NULL) {
        PyErr_Format(PyExc_TypeError, "on_timer parsed info data is null.");
        return NULL;
    }
    reset_signal_buf();
    void *data = (void *)PyArray_DATA((PyArrayObject *)info);
    // argument not define now
    int i;
    for (i = s_st_grp.cnt-1; i>=0; i--) {
        s_st_grp.api[i].on_timer(type, length, data);
    }
    return gather_signal_pyobject();
}

static PyObject *
py_strategy_destroy(PyObject *self, PyObject *args)
{
    destroy_strategy(s_st_grp.cnt);
    if(debug_mode == true){
        pass_screen_output();
    }
    debug_mode = false;
    if (py_sig_hdl) {
        Py_XDECREF(py_sig_hdl);
        py_sig_hdl = NULL;
    }
    if (log_handle != NULL){
		fwrite(log_buffer, log_buffer_len, 1, log_handle);
		fclose(log_handle);
		log_handle = NULL;
        start_point = log_buffer;
		log_buffer_len = 0;
	}
    return PyLong_FromLong(0);
}


extern char doc_load[];
extern char doc_init[];
extern char doc_on_book[];
extern char doc_on_response[];
extern char doc_on_timer[];
extern char doc_destroy[];
extern char doc_set_extra[];


static PyMethodDef strategy_exposed_methods[] = {
    {"load", (PyCFunction)py_strategy_create, METH_VARARGS, doc_load},
    {"init", (PyCFunction)py_strategy_init, METH_VARARGS, doc_init},
    {"on_book", (PyCFunction)py_on_book, METH_VARARGS, doc_on_book},
    {"on_response", (PyCFunction)py_on_response, METH_VARARGS, doc_on_response},
    {"on_timer", (PyCFunction)py_on_timer, METH_VARARGS, doc_on_timer},
    {"destroy", (PyCFunction)py_strategy_destroy, METH_VARARGS, doc_destroy},
    {"set_extra", (PyCFunction)py_set_extra, METH_VARARGS, doc_set_extra},
    {NULL, NULL, 0, NULL}   /* sentinel */
};

static struct PyModuleDef st_module = {
    PyModuleDef_HEAD_INIT,
    "strategy",
    NULL,
    -1,
    strategy_exposed_methods,
    NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC
PyInit_strategy(void)
{
    PyObject *m;
    import_array();
    m = PyModule_Create(&st_module);
    if (m == NULL) {
        return NULL;
    }
    Py_INCREF(PyExc_OSError);
    PyModule_AddObject(m, "error", PyExc_OSError);
    return m;
}
