#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

void flush_log();
void ctp_log(const char *fmt, ...);
void ctp_log_ln(const char *fmt, ...);
int get_curr_time();

#define LOG(format,...) do{\
	get_curr_time() == 0 ? ctp_log("[CTP before_start] " format, ##__VA_ARGS__) : ctp_log("[CTP %d] " format, get_curr_time(), ##__VA_ARGS__);\
}while (0)

#define LOG_LN(format,...) do{\
	get_curr_time() == 0 ? ctp_log_ln("[CTP before_start] " format, ##__VA_ARGS__) : ctp_log_ln("[CTP %d] " format, get_curr_time(), ##__VA_ARGS__);\
}while (0)

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
			printf("[DEBUG %s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_INFO(format,...) do{\
			printf(CC_CYAN "[INFO %s:%d] " format CC_RESET "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_SUCCESS(format,...) do{\
			printf(CC_GREEN "[SUCCESS %s:%d] " format CC_RESET "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_WARN(format,...) do{\
			printf(CC_YELLOW "[WARNNING %s:%d] " format CC_RESET "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_ERROR(format,...) do{\
			printf(CC_RED "[ERROR %s:%d] " format CC_RESET "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)
#else
	#define PRINT_DEBUG(format,...) do{\
			printf("[DEBUG %s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_INFO(format,...) do{\
			printf("[INFO %s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_SUCCESS(format,...) do{\
			printf("[INFO %s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_WARN(format,...) do{\
			printf("[WARNNING %s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_ERROR(format,...) do{\
			printf("[ERROR %s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)
#endif

#endif