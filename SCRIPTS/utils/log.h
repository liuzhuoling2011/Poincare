#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

void flush_log();
void ctp_log(const char *fmt, ...);
void ctp_log_ln(const char *fmt, ...);
int  get_debug_flag();

#define LOG(format,...) do{\
	ctp_log(format, ##__VA_ARGS__);\
}while (0)

#define LOG_LN(format,...) do{\
	ctp_log_ln(format, ##__VA_ARGS__);\
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
			if(get_debug_flag()) fprintf(stderr, "[DEBUG %s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_LOG_DEBUG(format,...) do{\
			if(get_debug_flag()) ctp_log_ln(format, ##__VA_ARGS__);\
			if(get_debug_flag()) fprintf(stderr, "[DEBUG %s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_INFO(format,...) do{\
			fprintf(stderr, CC_CYAN "[INFO %s:%d] " format CC_RESET "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_LOG_INFO(format,...) do{\
			if(get_debug_flag()) ctp_log_ln(format, ##__VA_ARGS__);\
			if(get_debug_flag()) fprintf(stderr, CC_CYAN "[INFO %s:%d] " format CC_RESET "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_SUCCESS(format,...) do{\
			fprintf(stderr, CC_GREEN "[SUCCESS %s:%d] " format CC_RESET "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_WARN(format,...) do{\
			fprintf(stderr, CC_YELLOW "[WARNNING %s:%d] " format CC_RESET "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_ERROR(format,...) do{\
			fprintf(stderr, CC_RED "[ERROR %s:%d] " format CC_RESET "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)
#else
	#define PRINT_LOG_DEBUG(format,...) do{\
			printf("[DEBUG %s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

	#define PRINT_LOG_INFO(format,...) do{\
			printf("[INFO %s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);\
		}while (0)

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