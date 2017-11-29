#include <stdio.h>
#include <cstdarg>
#include <string.h>
#include "log.h"
#include "core/base_define.h"

#define MAX_LOG_BUFF_SIZE (1024 * 1024 * 8)

char       log_name[1024];
FILE*      log_handle = NULL;
static char       log_buffer[MAX_LOG_BUFF_SIZE + 1];
static char       log_buffer_temp[MAX_LOG_BUFF_SIZE + 1];
static char*      start_point = log_buffer;
static int        log_buffer_len = 0;

send_data send_log = NULL;

void flush_log()
{
	if (log_handle != NULL) {
		fwrite(log_buffer, log_buffer_len, 1, log_handle);
		fflush(log_handle);
	}
	log_buffer[0] = '\0';
	start_point = log_buffer;
	log_buffer_len = 0;
}

void write_log(bool new_line, const char *fmt_str, va_list va_args)
{
	int buffer_len = vsprintf(log_buffer_temp, fmt_str, va_args);
	if (buffer_len + log_buffer_len >= MAX_LOG_BUFF_SIZE)
		flush_log(); 

	strncpy(start_point, log_buffer_temp, buffer_len);
	if (buffer_len > 0) {
		start_point += buffer_len;
		log_buffer_len += buffer_len;
	}

	if (new_line) {
		start_point += 1;
		log_buffer[log_buffer_len++] = '\n';
	}
	log_buffer[log_buffer_len] = '\0';
}

void ctp_log(const char * fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	write_log(false, fmt, args);
	va_end(args);
}

void ctp_log_ln(const char * fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	write_log(true, fmt, args);
	va_end(args);
}