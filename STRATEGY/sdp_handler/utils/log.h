#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include "strategy_interface.h"

#define LOG(format,...) do{\
	get_curr_time() == 0 ? check_log("[SDP before_start] " format, ##__VA_ARGS__) : check_log("[SDP %d] " format, get_curr_time(), ##__VA_ARGS__);\
}while (0)

#define LOG_LN(format,...) do{\
	get_curr_time() == 0 ? check_log_ln("[SDP before_start] " format, ##__VA_ARGS__) : check_log_ln("[SDP %d] " format, get_curr_time(), ##__VA_ARGS__);\
}while (0)

void check_log(const char *fmt, ...);
void check_log_ln(const char *fmt, ...);
int get_curr_time();
bool is_debug_mode();

#ifndef _WIN32
	#define CC_RED "\033[31m"
	#define CC_GREEN "\033[32m"
	#define CC_BLUE "\033[34m"
	#define CC_YELLOW "\033[33m"
	#define CC_CYAN "\033[36m"
	#define CC_WHITE "\033[37m"
	#define CC_BLACK "\033[30m"
	#define CC_RESET "\033[0m"

	#define PRINT_DEBUG(format,...) do{\
			if(is_debug_mode()) printf("[SDP %s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_INFO(format,...) do{\
			if(is_debug_mode()) printf(CC_CYAN "[SDP %s:%d] " format CC_RESET "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_SUCCESS(format,...) do{\
			printf(CC_GREEN "[SUCCESS %s:%d] " format CC_RESET "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_WARN(format,...) do{\
			printf(CC_YELLOW "[SDP %s:%d] " format CC_RESET "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_ERROR(format,...) do{\
			printf(CC_RED "[SDP %s:%d] " format CC_RESET "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)
#else
	#define PRINT_DEBUG(format,...) do{\
			printf("[SDP %s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_INFO(format,...) do{\
			printf("[SDP %s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_SUCCESS(format,...) do{\
			printf("[INFO %s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_WARN(format,...) do{\
			printf("[SDP %s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_ERROR(format,...) do{\
			printf("[SDP %s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)
#endif

#endif