#include <stdio.h>
#include <cstdarg>
#include "log.h"
#include "sdp_handler/core/base_define.h"

#define MAX_LOG_BUFF_SIZE (1024 * 1024)

send_data send_log = NULL;
int int_time = 0;
bool debug_mode = false;

char log_buffer[MAX_LOG_BUFF_SIZE];

void write_log(bool new_line, const char *fmt_str, va_list va_args) {
	int buffer_len = vsprintf(log_buffer, fmt_str, va_args);
	if (new_line) {
		log_buffer[buffer_len++] = '\n';
	}
	log_buffer[buffer_len] = '\0';
	send_log(S_STRATEGY_DEBUG_LOG, buffer_len, (void*)log_buffer);
}

void check_log(const char * fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	write_log(false, fmt, args);
	va_end(args);
}

void check_log_ln(const char * fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	write_log(true, fmt, args);
	va_end(args);
}

int get_curr_time()
{
	return int_time;
}

bool is_debug_mode()
{
	return debug_mode;
}