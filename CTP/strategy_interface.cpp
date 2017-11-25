#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "unistd.h"
#include "dlfcn.h"
#include "strategy_interface.h"

#define MAX_LOG_BUFF_SIZE     8 * 1024 * 1024

static char       log_name[1024];
static FILE*      log_handle = NULL;
static char       log_buffer[MAX_LOG_BUFF_SIZE + 1];
static char*      start_point = log_buffer;
static int        log_buffer_len = 0;

/* flush log immediately */
static void flush_log()
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
		exit(1);
	}
}

static void
dump_backtrace()
{
	int len;
	FILE *fp = NULL;
	char rbuf[512], msg_buf[4096] = { 0 };
	char *wbuf = msg_buf;
	popen_coredump_fp(&fp);
	while (fgets(rbuf, sizeof(rbuf) - 1, fp)) {
		len = strlen(rbuf);
		strcpy(wbuf, rbuf);
		wbuf += len;
	}
	fclose(fp);
	printf("core dump: %s\n", msg_buf);
	exit(0);
}

static void
recv_signal(int sig)
{
	dump_backtrace();
	flush_log();
	//todo
	exit(0);
}
#if 0
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
#ifdef DEBUG
		printf("OrderID: %lld Sent single order, %s %s %s %d@%f\n", po->order_id, po->symbol,
			(po->direction == ORDER_BUY ? "Buy" : "Sell"), (po->open_close == ORDER_OPEN ? "Open" : "Close"),
			po->volume, po->price);
#endif
		return 0;
	}
	else if (type == S_CANCEL_ORDER_DEFAULT) {
		st_data_t *ord = (st_data_t *)data;
		order_t *co = (order_t *)ord->info;
		order_t *next_ord = &s_st_sig.ord[s_st_sig.ord_cnt++];
		*next_ord = *co;
		next_ord->order_id = ++s_st_sig.order_no * SERIAL_NO_MULTI + co->st_id;
		//next_ord->order_id = s_st_sig.order_no * SERIAL_NO_MULTI + co->st_id;

#ifdef DEBUG
		printf("OrderID: %lld Cancel single order, %s %s %s %d@%f\n", co->order_id, co->symbol,
			(co->direction == ORDER_BUY ? "Buy" : "Sell"), (co->open_close == ORDER_OPEN ? "Open" : "Close"),
			co->volume, co->price);
#endif
		return 0;
	}
	return -1;
}

static int
send_info_real(int type, int length, void *data)
{
	/* only process the log information now */
	switch (type) {
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
		api->st_init = (st_data_func_t)dlsym(api->hdl, "my_st_init");
		if (!api->st_init) {
			PyErr_Format(PyExc_KeyError, "not found st_init: %s, %s", path, dlerror());
			break;
		}
		api->on_book = (st_data_func_t)dlsym(api->hdl, "my_on_book");
		if (!api->on_book) {
			PyErr_Format(PyExc_KeyError, "not found on_book: %s, %s", path, dlerror());
			break;
		}
		api->on_response = (st_data_func_t)dlsym(api->hdl, "my_on_response");
		if (!api->on_response) {
			PyErr_Format(PyExc_KeyError, "not found on_response: %s, %s", path, dlerror());
			break;
		}
		api->on_timer = (st_data_func_t)dlsym(api->hdl, "my_on_timer");
		if (!api->on_timer) {
			PyErr_Format(PyExc_KeyError, "not found on_timer: %s", path);
			break;
		}
		api->st_exit = (st_none_func_t)dlsym(api->hdl, "my_destroy");
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

static void
destroy_strategy(int cnt)
{
	st_api_t *api;
	int i, max_idx = MIN(cnt, s_st_grp.cnt);
	for (i = 0; i < max_idx; i++)
	{
		api = &s_st_grp.api[i];
		if (api->st_exit) {
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
	debug_mode = false;
	s_st_sig.order_no = 0;
	s_st_sig.ord_cnt = 0;
	s_st_info.msg_cnt = 0;
	memset((void *)&s_st_grp, 0, sizeof(st_grp_t));
#ifdef PROFILE
	stat_time_record("cprofile_strategy.log");
#endif
}

static int
do_group_strategy_init(st_config_t *cfg_arr, int cnt)
{
	int i, ret;
	/* fill the on_book handler */
	for (i = 0; i < cnt - 1; i++) {
		cfg_arr[i].proc_order_hdl = s_st_grp.api[i + 1].on_book;
		cfg_arr[i].send_info_hdl = s_st_grp.api[i + 1].on_book;
	}
	cfg_arr[i].proc_order_hdl = proc_order_real;
	cfg_arr[i].send_info_hdl = send_info_real;
	/* fill the pass_rsp handler */
	for (i = 1; i < cnt; i++) {
		cfg_arr[i].pass_rsp_hdl = s_st_grp.api[i - 1].on_response;
	}
	cfg_arr[0].pass_rsp_hdl = pass_rsp_real;
	/* do strategy init operation, last init strategy */
	for (i = cnt - 1; i >= 0; i--) {
		st_api_t *api = &s_st_grp.api[i];
		if (debug_mode) {
			ret = api->st_init(DEFAULT_CONFIG, STRATEGY_DEBUG, (void *)&cfg_arr[i]);
		}
		else {
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
	PyArrayObject *py_a = (PyArrayObject *)st_cfg_arr;
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

	int ret = do_group_strategy_init(cfg, (int)arr_size);
	if (ret != 0)
		return NULL;

	flush_log();
	return PyLong_FromLong(0);
}
#endif
int my_st_init(int type, int length, void *cfg) {
	signal(SIGSEGV, recv_signal);
	signal(SIGABRT, recv_signal);
	return 0;
}

int my_on_book(int type, int length, void *book) {
	return 0;

}

int my_on_response(int type, int length, void *resp) {
	return 0;

}

int my_on_timer(int type, int length, void *info) {
	
	return 0;
}

void my_destroy() {

}