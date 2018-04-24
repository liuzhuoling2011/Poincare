#include <iconv.h>
#include <unistd.h>
#include <ctype.h>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string.h>
#include "utils.h"
#include "log.h"
#include "json11.hpp"
#include "ThostFtdcTraderApi.h"

using namespace json11;

extern bool debug_flag;
extern bool complete_data_flag;

#define CHAR_EQUAL_ZERO(a, b, c) do{\
	if (a != b) goto end;\
	if (a == 0) {c = 0; goto end;}\
}while(0)

int my_strcmp(const char * s1, const char * s2)
{
	int ret = -1;
	CHAR_EQUAL_ZERO(s1[0], s2[0], ret);
	CHAR_EQUAL_ZERO(s1[1], s2[1], ret);
	CHAR_EQUAL_ZERO(s1[2], s2[2], ret);
	CHAR_EQUAL_ZERO(s1[3], s2[3], ret);
	CHAR_EQUAL_ZERO(s1[4], s2[4], ret);
	CHAR_EQUAL_ZERO(s1[5], s2[5], ret);
	CHAR_EQUAL_ZERO(s1[6], s2[6], ret);
	CHAR_EQUAL_ZERO(s1[7], s2[7], ret);

	ret = strcmp(s1 + 8, s2 + 8);
end:
	return ret;
}

int strcmp_case(const char * one, const char * two)
{
	char *pOne = (char *)one;
	char *pTwo = (char *)two;
	if (strlen(one) < strlen(two))
		return -1;
	else if (strlen(one) > strlen(two))
		return 1;
	while (*pOne != '\0') {
		if (tolower(*pOne) != tolower(*pTwo))
			return -1;
		pOne++;
		pTwo++;
	}
	return 0;
}

uint64_t my_hash_value(const char *str_key)
{
	char *pt = (char *)str_key;
	unsigned int hash = 1315423911;

	if (pt[0] == 0) goto end;
	hash ^= ((hash << 5) + pt[0] + (hash >> 2));
	if (pt[1] == 0) goto end;
	hash ^= ((hash << 5) + pt[1] + (hash >> 2));
	if (pt[2] == 0) goto end;
	hash ^= ((hash << 5) + pt[2] + (hash >> 2));
	if (pt[3] == 0) goto end;
	hash ^= ((hash << 5) + pt[3] + (hash >> 2));

	if (pt[4] == 0) goto end;
	hash ^= ((hash << 5) + pt[4] + (hash >> 2));
	if (pt[5] == 0) goto end;
	hash ^= ((hash << 5) + pt[5] + (hash >> 2));
	if (pt[6] == 0) goto end;
	hash ^= ((hash << 5) + pt[6] + (hash >> 2));
	if (pt[7] == 0) goto end;
	hash ^= ((hash << 5) + pt[7] + (hash >> 2));

	pt = (char *)(str_key + 8);
	while (*pt != 0) {
		hash ^= ((hash << 5) + *pt + (hash >> 2));
		pt++;
	}

end:
	return hash;
}

size_t
strlcpy(char * dst, const char * src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';              /* NUL-terminate dst */
		while (*s++);
	}

	return(s - src - 1);    /* count does not include NUL */
}

void
my_strncpy(char *dst, const char *src, size_t siz)
{
	dst[0] = src[0]; if (src[0] == 0) goto end;
	dst[1] = src[1]; if (src[1] == 0) goto end;
	dst[2] = src[2]; if (src[2] == 0) goto end;
	dst[3] = src[3]; if (src[3] == 0) goto end;
	dst[4] = src[4]; if (src[4] == 0) goto end;
	dst[5] = src[5]; if (src[5] == 0) goto end;
	dst[6] = src[6]; if (src[6] == 0) goto end;
	dst[7] = src[7]; if (src[7] == 0) goto end;
	dst[8] = src[8]; if (src[8] == 0) goto end;
	dst[9] = src[9]; if (src[9] == 0) goto end;
	dst[10] = src[10]; if (src[10] == 0) goto end;
	dst[11] = src[11]; if (src[11] == 0) goto end;
	strlcpy(dst + 12, src + 12, siz - 12);
end:
	return;
}

int code_convert(char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{	
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open("utf-8", "gb2312");
	if (cd == 0) return -1;
	memset(outbuf, 0, outlen);
	if (iconv(cd, pin, &inlen, pout, &outlen) == -1) 
		return -1;
	iconv_close(cd);
	return 0;
}

#define CONFIG_FILE "./config.json"
void read_config_file(std::string& content)
{
	std::ifstream fin(CONFIG_FILE);
	std::stringstream buffer;
	buffer << fin.rdbuf();
	content = buffer.str();
	fin.close();
}


static char local_time[256];
char* get_time_record() {
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep); //取得当地时间
	sprintf(local_time, "%d%02d%02d_%02d%02d%02d", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
	return local_time;
}

void update_trader_log_name(TraderConfig& trader_config) {
	create_dir("./logs");
	std::string logname = trader_config.ASSIST_LOG;
	char* l_time = get_time_record();
	size_t pos;
	if ((pos = logname.find("[TIME]")) != std::string::npos) {
		logname.replace(pos, 6, l_time);
	}
	strlcpy(trader_config.ASSIST_LOG, logname.c_str(), 256);
}


bool read_json_config(TraderConfig& trader_config) {
	std::string json_config;
	read_config_file(json_config);
	std::string err_msg;
	Json l_json = json11::Json::parse(json_config, err_msg, JsonParse::COMMENTS);
	if (err_msg.length() > 0) {
		PRINT_ERROR("Json parse fail, please check your setting! error: %s", err_msg.c_str());
		LOG_LN("Json parse fail, please check your setting! error: %s", err_msg.c_str());
		return false;
	}

	strlcpy(trader_config.FRONT, l_json["FRONT"].string_value().c_str(), 64);
	strlcpy(trader_config.BROKER_ID, l_json["BROKER_ID"].string_value().c_str(), 64);
	strlcpy(trader_config.USER_ID, l_json["USER_ID"].string_value().c_str(), 64);
	strlcpy(trader_config.PASSWORD, l_json["PASSWORD"].string_value().c_str(), 64);

	trader_config.CONTRACT_INFO_FLAG = l_json["CONTRACT_INFO_FLAG"].bool_value();
	trader_config.ALL_CONTRACT_INFO_FLAG = l_json["ALL_CONTRACT_INFO_FLAG"].bool_value();
	
	trader_config.POSITION_FLAG = l_json["POSITION_FLAG"].bool_value();
	trader_config.ORDER_INFO_FLAG = l_json["ORDER_INFO_FLAG"].bool_value();
	trader_config.TRADE_INFO_FLAG = l_json["TRADE_INFO_FLAG"].bool_value();
	strlcpy(trader_config.ASSIST_LOG, l_json["ASSIST_LOG"].string_value().c_str(), 64);

	debug_flag = l_json["DEBUG_FLAG"].bool_value();
	complete_data_flag = l_json["COMPLETE_DATA_FLAG"].bool_value();

	create_dir("./tmp");
	update_trader_log_name(trader_config);
}

void popen_coredump_fp(FILE ** fp)
{
	char buf[8192];
	pid_t dying_pid = getpid();
	sprintf(buf, "gdb -p %d -batch -ex bt 2>/dev/null | "
		"sed '0,/<signal handler/d'", dying_pid);
	*fp = popen(buf, "r");
	if (*fp == NULL) {
		perror("popen coredump file failed");
		exit(1);
	}
}

void dump_backtrace(char * core_dump_msg)
{
	int len;
	FILE *fp = NULL;
	char rbuf[512], msg_buf[8192] = { 0 };
	char *wbuf = msg_buf;
	popen_coredump_fp(&fp);
	while (fgets(rbuf, sizeof(rbuf) - 1, fp)) {
		len = strlen(rbuf);
		strcpy(wbuf, rbuf);
		wbuf += len;
	}
	pclose(fp);
	core_dump_msg[0] = '\n';
	strlcpy(core_dump_msg + 1, msg_buf, strlen(msg_buf));
}
