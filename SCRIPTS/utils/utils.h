#ifndef	_UTILS_H_
#define	_UTILS_H_

#include <stdio.h>
#include "CTP_define.h"
#include "base_define.h"

#define delete_mem(cls_obj) do{\
	if(cls_obj != nullptr) { delete cls_obj; cls_obj = nullptr; }\
}while(0)

#ifndef _WIN32
/* High Precision Time Compare */
#define HP_TIMING_NOW(Var) \
 ({ uint64_t _hi, _lo; \
  asm volatile ("rdtsc" : "=a" (_lo), "=d" (_hi)); \
  (Var) = _hi << 32 | _lo; })

/*  Example: Compare clock time
unsigned long start, end;
HP_TIMING_NOW(start);
Do something...
HP_TIMING_NOW(end);

printf("%ld\n", end - start);
*/

#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#define ACCESS access  
#define MKDIR(a) mkdir((a),0755) 
#else
#define HP_TIMING_NOW(Var)

#include <direct.h>  
#include <io.h>  
#define ACCESS _access
#define MKDIR(a) _mkdir((a))
#endif

/*
* Copy src to string dst of size siz with high efficiency.
* At most siz-1 characters will be copied.
* Always NUL terminates.
*/
void my_strncpy(char *dst, const char *src, size_t siz);

size_t strlcpy(char * dst, const char * src, size_t siz);

/*
* Compare two strings with high efficiency.
* Returns 0 for equal, else for not equal.
*/
int my_strcmp(const char *s1, const char *s2);

// Replace strcmp without case sensitivity. But if the two strings'
// length equal, we only return -1 when the strings are not equal,
// that's a bit different from strcmp
int strcmp_case(const char *one, const char *two);

uint64_t my_hash_value(const char *str_key);

int code_convert(char *inbuf, size_t inlen, char *outbuf, size_t outlen);

bool read_json_config(TraderConfig& trader_config);

inline void create_dir(const char *path) {
	if (ACCESS(path, 0) != 0) MKDIR(path);
}

void popen_coredump_fp(FILE **fp);

void dump_backtrace(char * core_dump_msg);
#endif								   