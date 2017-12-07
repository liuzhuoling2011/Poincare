#if !defined(_ME_LOADER_H_52AE9284_9CB5_402C_8669_5BC763408576)
#define _ME_LOADER_H_52AE9284_9CB5_402C_8669_5BC763408576

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/ndarraytypes.h>
#include <numpy/arrayobject.h>
#include <Python.h>
#include <errno.h>
#include "order_types_def.h"
#include "../include/msp_intf.h"

#define MAX_PRODUCT_CNT (4096)

typedef struct {
        void *me;
        void *conf_arg;
} me_handle_t;

typedef struct {
        int default_mt; // default match_engine type
        int pro_cnt;    // product info config count
        product_info  pi_cfg[MAX_PRODUCT_CNT];
        int par_cnt;    // parameters config count
        mode_config   mp_cfg[MAX_PRODUCT_CNT];
} me_config_t;

#endif
