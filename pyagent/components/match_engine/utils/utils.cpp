/*
* utils.cpp
* 
* Copyright(C) by MY Capital Inc. 2007-2999
*/
#include "utils.h"

//Define the current library version number
const char* comm_utils_version = { "Version: 2.3" };

#ifdef _WIN32
	#include <windows.h> 
	void my_sleep(int ms){
		 Sleep(ms);
	}
#else
	#include <time.h>
    #include <unistd.h>
	timespec diff(timespec start, timespec end)
	{
		timespec temp;
		if ((end.tv_nsec - start.tv_nsec) < 0) {
			temp.tv_sec = end.tv_sec - start.tv_sec - 1;
			temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
		}
		else {
			temp.tv_sec = end.tv_sec - start.tv_sec;
			temp.tv_nsec = end.tv_nsec - start.tv_nsec;
		}
		return temp;
	}

	void my_sleep(int ms) { 
		 usleep(ms * 1000); 
	}
#endif


#ifdef _WIN32  
#include <direct.h>  
#include <io.h>  
#define ACCESS _access
#define MKDIR(a) _mkdir((a))
#else
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#define ACCESS access  
#define MKDIR(a) mkdir((a),0755) 
#endif 


void  create_dir(const char *path)
{
	if (ACCESS(path, 0) != 0)
		MKDIR(path);
}

//------------------Data type related---------------------
static const std::map<std::string, char> EXCH_CHAR =
	{ { "SZSE", '0' }, { "SSE", '1' }, { "HKEX", '2' }, { "SHFE", 'A' }, { "CFFEX", 'G' }, { "DCE", 'B' },
	{ "CZCE", 'C' }, { "FUT_EXCH", 'X' }, { "SGX", 'S' }, { "SGE", 'D' }, { "CBOT", 'F' }, { "CME", 'M' },
	{ "LME", 'L' }, { "COMEX", 'O' }, { "NYMEX", 'N' }, { "BLANK_EXCH", '\0' }, { "UNDEFINED_EXCH", 'u' } };

static const std::map<char, std::string> CHAR_EXCH =
	{ { '0', "SZSE" },{ '1', "SSE" },{ '2', "HKEX" },{ 'A', "SHFE" },{ 'G', "CFFEX" },{ 'B', "DCE" },
	{ 'C', "CZCE" },{ 'X', "FUT_EXCH" },{ 'S', "SGX" },{ 'D', "SGE" },{ 'F', "CBOT" },{ 'M', "CME" },
	{ 'L', "LME" },{ 'O', "COMEX" },{ 'N', "NYMEX" },{ '\0', "BLANK_EXCH" },{ 'u', "UNDEFINED_EXCH" } };


using SymbolMap = std::map<std::string, std::string>;
const static SymbolMap symbol_map{ { "if", "IF" }, { "ic", "IC" }, { "ih", "IH" }, { "tf", "TF" }, { "t", "T" },
					{ "ag", "shag" }, { "au", "shau" }, { "al", "shal" }, { "bu", "shbu" }, { "cu", "shcu" }, { "rb", "shrb" }, 
					{ "ru", "shru" }, { "zn", "shzn" }, { "ni", "shni" }, { "fu", "shfu" }, { "hc", "shhc" }, { "pb", "shpb" }, 
					{ "wr", "shwr" }, { "sn", "shsn" },{ "a", "dla" }, { "bb", "dlbb" }, { "c", "dlc" }, { "cs", "dlcs" }, { "fb", "dlfb" },
					{ "i", "dli" }, { "j", "dlj" }, { "jd", "dljd" }, { "jm", "dljm" }, { "l", "dll" }, { "m", "dlm" }, { "p", "dlp" },
					{ "pp", "dlpp" }, { "y", "dly" }, { "b", "dlb" }, { "v", "dlv" }, { "rm", "zzrm" }, { "cf", "zzcf" }, 
					{ "sr", "zzsr" }, { "ta", "zzta" }, { "ma", "zzma" }, { "fg", "zzfg" }, { "oi", "zzoi" }, { "zc", "zzzc" }, 
					{ "jr", "zzjr" }, { "lr", "zzlr" }, { "me", "zzme" }, { "pm", "zzpm" }, { "ri", "zzri" }, { "rs", "zzrs" }, 
					{ "sf", "zzsf" }, { "sm", "zzsm" }, { "tc", "zztc" }, { "wh", "zzwh" }, {"au(t+d)","Au(T+D)"} };

using StratName_ID_Map = std::map<std::string, int>;
const static StratName_ID_Map stratname_id_map{ { "sample", 0 },
					{ "hi5", 500 },{ "hi5a", 501 },{ "hi50", 503 },{ "hi50a", 504 },{ "hi51", 510 },{ "hi51a", 511 },{ "hi52", 520 },{ "hi52a", 521 },{ "hi53", 530 },{ "hi55", 550 },{ "hi55a", 551 },
					{ "hi6", 600 },{ "hi6a", 601 },{ "hi60", 603 },{ "hi60a", 604 },{ "hi61", 610 },{ "hi61a", 611 },{ "hi62", 620 },{ "hi62a", 621 },{ "hi65", 650 },{ "hi65a", 651 },
					{ "hi70", 70 },{ "hi71", 71 },{ "hi72", 72 },{ "hi73", 73 },{ "hi74", 74 },{ "hi75", 75 },{ "hi76", 76 },
					{ "hi80", 80 },{ "hi81", 81 },{ "hi82", 82 },{ "hi83", 83 },{ "hi84", 84 },{ "hi85", 85 },{ "hi86", 86 },
					{ "hi91", 91 },{ "hi92", 92 },{ "hi93", 93 },
					{ "ip11", 1911 },{ "ip91", 1991 },{ "ip92", 1992 },{ "ip93", 1993 },{ "ip94", 1994 },{ "ip95", 1995 },{ "ip96", 1996 },{ "ip97", 1997 },
					{ "hi0", 8512 },{ "hi05", 8500 },{ "hi05a", 8501 },{ "hi050", 8503 },{ "hi050a", 8504 },{ "hi051", 8510 },{ "hi051a", 8511 },{ "hi052", 8520 },{ "hi052a", 8521 },{ "hi055", 8550 },{ "hi055a", 8551 },
					{ "hi071", 8071 },{ "hi072", 8072 },{ "hi073", 8073 },{ "hi074", 8074 },{ "hi075", 8075 },{ "hi076", 8076 },
					{ "hi05p2", 6500 }, { "hi05ap2", 6501 }, { "hi050p2", 6503 }, { "hi050ap2", 6504 }, { "hi051p2", 6510 }, { "hi051ap2", 6511 }, { "hi052p2", 6520 }, { "hi052ap2", 6521 }, { "hi055p2", 6550 }, { "hi055ap2", 6551 },
					{ "hi05p3", 7500 }, { "hi05ap3", 7501 }, { "hi050p3", 7503 }, { "hi050ap3", 7504 }, { "hi051p3", 7510 }, { "hi051ap3", 7511 }, { "hi052p3", 7520 }, { "hi052ap3", 7521 }, { "hi055p3", 7550 }, { "hi055ap3", 7551 },
					{ "hi06", 8600 }, { "hi06a", 8601 }, { "hi060", 8603 }, { "hi060a", 8604 }, { "hi061", 8610 }, { "hi061a", 8611 }, { "hi062", 8620 }, { "hi062a", 8621 }, { "hi065", 8650 }, { "hi065a", 8651 }, 
					{ "hi06p2", 6600 }, { "hi06ap2", 6601 }, { "hi060p2", 6603 }, { "hi060ap2", 6604 }, { "hi061p2", 6610 }, { "hi061ap2", 6611 }, { "hi062p2", 6620 }, { "hi062ap2", 6621 }, { "hi065p2", 6650 }, { "hi065ap2", 6651 },
					{ "hi06p3", 7600 }, { "hi06ap3", 7601 }, { "hi060p3", 7603 }, { "hi060ap3", 7604 }, { "hi061p3", 7610 }, { "hi061ap3", 7611 }, { "hi062p3", 7620 }, { "hi062ap3", 7621 }, { "hi065p3", 7650 }, { "hi065ap3", 7651 } };

using StratID_SubID_Map = std::map<int, int>;
const static StratID_SubID_Map strat_subid_id_map{ { 500, 5 },{ 501, 5 },{ 503, 5 },{ 504, 5 },{ 510, 5 },
					{ 511, 5 },{ 520, 5 },{ 521, 5 },{ 530, 5 },{ 550, 5 },{ 551, 5 },
					{ 70, 7 },{ 71, 7 },{ 72, 7 },{ 73, 7 },{ 74, 7 },{ 75, 7 },{ 76, 7 },
					{ 80, 8 },{ 81, 8 },{ 82, 8 },{ 83, 8 },{ 84, 8 },{ 85, 8 },{ 86, 8 },
					{ 91, 9 },{ 92, 9 },{ 93, 9 },
					{ 1911, 19 },{ 1991, 19 },{ 1992, 19 },{ 1993, 19 },{ 1994, 19 },{ 1995, 19 },{ 1996, 19 },{ 1997, 19 },
					{ 8512, 10 }, { 8500, 10 }, { 8501, 10 }, { 8503, 10 }, { 8504, 10 }, { 8510, 10 }, { 8511, 10 }, { 8520, 10 }, { 8521, 10 }, { 8550, 10 }, { 8551, 10 }, 
					{ 8071, 10 }, { 8072, 10 }, { 8073, 10 }, { 8074, 10 }, { 8075, 10 }, { 8076, 10 },
					{ 6500, 10 }, { 6501, 10 }, { 6503, 10 }, { 6504, 10 }, { 6510, 10 }, { 6511, 10 }, { 6520, 10 }, { 6521, 10 }, { 6550, 10 }, { 6551, 10 }, 
					{ 7500, 10 }, { 7501, 10 }, { 7503, 10 }, { 7504, 10 }, { 7510, 10 }, { 7511, 10 }, { 7520, 10 }, { 7521, 10 }, { 7550, 10 }, { 7551, 10 }, 
					{ 8600, 10 }, { 8601, 10 }, { 8603, 10 }, { 8604, 10 }, { 8610, 10 }, { 8611, 10 }, { 8620, 10 }, { 8621, 10 }, { 8650, 10 }, { 8651, 10 },
					{ 6600, 10 }, { 6601, 10 }, { 6603, 10 }, { 6604, 10 }, { 6610, 10 }, { 6611, 10 }, { 6620, 10 }, { 6621, 10 }, { 6650, 10 }, { 6651, 10 },
					{ 7600, 10 }, { 7601, 10 }, { 7603, 10 }, { 7604, 10 }, { 7610, 10 }, { 7611, 10 }, { 7620, 10 }, { 7621, 10 }, { 7650, 10 }, { 7651, 10 } };

//------------------System related---------------------
void** create_2d_array(int row, int col, int size)
{
	int point = sizeof(void *);
	int all_mem = point * row + size * row * col;
	void **new_data = (void **)malloc(all_mem);
	if (new_data != NULL){
		memset(new_data, 0, all_mem);
		void *head = (void *)((char *)new_data + point * row);
		for (int i = 0; i < row; i++){
			new_data[i] = (void *)((char *)head + i * size * col);
		}
	}

	return new_data;
}

//------------------Conversion related---------------------
void
convert_to_rss_symbol(const char* a_symbol, char* a_out_rss_symbol)
{
	char szName[MAX_SYMBOL_SIZE] = "";
	char *pt = (char *)a_symbol;
	char *ps = (char *)szName;
	while (*pt != '\0'){
		*ps++ = tolower(*pt++);
	}

	if (symbol_map.find(szName) != symbol_map.end()) {
		my_strncpy(a_out_rss_symbol, symbol_map.find(szName)->second.c_str(), MAX_SYMBOL_SIZE);
	}
	else{
		my_strncpy(a_out_rss_symbol, a_symbol, MAX_SYMBOL_SIZE);
	}
}

int convert_stratname_to_id(const char * a_name)
{
	auto l_it = stratname_id_map.find(a_name);
	if (l_it != stratname_id_map.end()) {
		return l_it->second;
	}
	else {
		return -1;
	}
}

int convert_currency_number(std::string cur_name)
{
	auto l_it = currency_int.find(cur_name);
	if (l_it != currency_int.end()) {
		return l_it->second;
	}
	else {
		return -1;
	}
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

void
my_strcpy_lower_case(char *dst, const char*src)
{
	while (*src != '\0'){
		*dst = tolower(*src);
		dst++;
		src++;
	}
	*dst = '\0';
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

size_t
strlcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}

#define CHAR_EQUAL_ZERO(a, b, c) do{\
	if (a != b) goto end;\
	if (a == 0) {c = 0; goto end;}\
}while(0)

int 
my_strcmp(const char *s1, const char *s2)
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

// Replace strcmp without case sensitivity. But if the two strings'
// length equal, we only return -1 when the strings are not equal,
// that's a bit different from strcmp
int
strcmp_case(const char *one, const char *two)
{
	char *pOne = (char *)one;
	char *pTwo = (char *)two;
	if (strlen(one) < strlen(two))
		return -1;
	else if (strlen(one) > strlen(two))
		return 1;
	while (*pOne != '\0'){
		if (tolower(*pOne) != tolower(*pTwo))
			return -1;
		pOne++;
		pTwo++;
	}
	return 0;
}

bool
case_compare(const char *one, const char *two)
{
	char *pOne = (char *)one;
	char *pTwo = (char *)two;
	if (strlen(one) != strlen(two))
		return false;
	while (*pOne != '\0'){
		if (tolower(*pOne) != tolower(*pTwo))
			return false;
		pOne++;
		pTwo++;
	}
	return true;
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
	while (*pt != 0){
		hash ^= ((hash << 5) + *pt + (hash >> 2));
		pt++;
	}

end:
	return hash;
}

void
get_product_by_symbol(const char *symbol, const char *product)
{
	char *ps = (char *)symbol;
	char *pd = (char *)product;

	while (*ps != '\0'){
		if (isalpha(*ps) || *ps == '(' ||
			*ps == '+' || *ps == ')' ){
			*pd = *ps;
			pd++;
		}
		else break;
		ps++;
	}

	*pd = '\0';
}

void
get_foreign_product_by_symbol(const char *symbol, const char *product)
{
	char *ps = (char *)symbol;
	char *pd = (char *)product;

	while (*ps != '\0') {
		if (*ps != ' ') {
			*pd = *ps;
			pd++;
		}
		else break;
		ps++;
	}

	*pd = '\0';
}

/* Windcode "000001.SH" to symbol "000001" */
void
get_symbol_by_windcode(const char *a_windcode, const char *symbol)
{
	char *ps = (char *)a_windcode;
	char *pd = (char *)symbol;

	while (*ps != '\0') {
		if (*ps != '.') {
			*pd = *ps;
			pd++;
		}
		else break;
		ps++;
	}

	*pd = '\0';
}

// Remove all space in a string. 
void
string_trim(const char* a_str, const char * a_out_str)
{
	char *pd = (char *)a_out_str;
	while (*a_str != '\0')
	{
		if (*a_str != ' ')
		{
			*pd = *a_str;
			pd++;
		}
		++a_str;
	}
	*pd = '\0';
}

char get_exch_by_name(const char *name)
{
	auto need = EXCH_CHAR.find(name);
	if (need != EXCH_CHAR.end())
		return need->second;
	else
		return '\0';
}

std::string get_exch_name_by_enum(const char name)
{
	auto need = CHAR_EXCH.find(name);
	if (need != CHAR_EXCH.end())
		return need->second;
	else
		return "DCE";
}

int get_strat_id_by_subid(int a_subid)
{
	return strat_subid_id_map.find(a_subid)->second;
}

bool
is_match(char ch, const char *delims)
{
	for (size_t i = 0; i < strlen(delims); i++){
		if (ch == delims[i])
			return true;
	}
	return false;
}

bool
split_string(std::string tmpStr, const char *delims, std::string & tmpName, double & tmpNum)
{
	if (tmpStr == "")
		return false;
	SplitVec tmpVec;
	split_string(tmpStr.c_str(), delims, tmpVec);
	tmpName = tmpVec[0];
	tmpNum = atof(tmpVec[1].c_str());
	return true;
}

void
split_string(const char *line, const char *delims, SplitVec &elems)
{
	if (line == NULL || line[0] == '\0')
		return;
	char token[TOKEN_BUFFER_SIZE];
	char *pStart = (char *)line, *pNext = pStart;
	while (*pNext != '\0' && *pNext != '\r' && *pNext != '\n')
	{
		if (is_match(*pNext, delims)){
			memset(token, 0x0, TOKEN_BUFFER_SIZE);
			memcpy(token, pStart, pNext - pStart);
			elems.emplace_back(token);
			pStart = pNext + 1;
		}
		pNext++;
	}
	memset(token, 0x0, TOKEN_BUFFER_SIZE);
	memcpy(token, pStart, pNext - pStart);
	elems.emplace_back(token);
}

void
split_string(const std::string &line, const char *delims, SplitVec &elems)
{
	split_string(line.c_str(), delims, elems);
}

bool is_endswith(char *large, const char *sub)
{
	size_t one_len = strlen(large);
	size_t two_len = strlen(sub);
	if (two_len > one_len)  return false;

	for (size_t i = 1; i <= two_len; i++){
		if (large[one_len - i] != sub[two_len - i])
			return false;
	}

	return true;
}

std::string&
my_trim(std::string &ss)
{
	const char * drop = " \r\n\t";
	if (ss.empty()){
		return ss;
	}
	ss.erase(0, ss.find_first_not_of(drop));
	ss.erase(ss.find_last_not_of(drop) + 1);
	return ss;
}

int str2int(char *str)
{
	int result = 0;
	std::string temp(str);
	str2num<int>(my_trim(temp), result);
	return result;
}

double str2double(char * str)
{
	double result = 0.0;
	std::string temp(str);
	str2num<double>(my_trim(temp), result);
	return result;
}

int
calc_increment_count(Parameter *a_parm, double epsilon)
{
	return (int)((a_parm->max - a_parm->min + epsilon) / a_parm->increment) + 1;
}

int
compare(double a, double b, double epsilon)
{	
	if (a - b > epsilon)
		return 1;
	else if (a - b < -epsilon)
		return -1;
	else
		return 0;
}

//------------------ Time & date related---------------------
int get_now_time()
{
	int result = 0;
#ifdef WIN32
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	result = sys.wHour * 10000 + sys.wMinute * 100 + sys.wSecond;
#else
	time_t timep;
	struct tm *p_tm;
	time(&timep);
	p_tm = localtime(&timep);
	result = p_tm->tm_hour * 10000 + p_tm->tm_min * 100 + p_tm->tm_sec;
#endif
	return result;
}

int get_now_date()
{
	int result = 0;
#ifdef WIN32
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	result = sys.wYear * 10000 + sys.wMonth * 100 + sys.wDay;
#else
	time_t timep;
	struct tm *p_tm;
	time(&timep);
	p_tm = localtime(&timep);
	result = (p_tm->tm_year + 1900) * 10000 + (p_tm->tm_mon + 1) * 100 + p_tm->tm_mday;
#endif
	return result;

}

/* Convert char date string, 2016-11-01 */
int str_date_to_int(const char *str_date)
{
	int result = 0;
	if (strlen(str_date) >= 10){
		int year = (str_date[0] - 48) * 1000 + (str_date[1] - 48) * 100
			+ (str_date[2] - 48) * 10 + (str_date[3] - 48);
		int month = (str_date[5] - 48) * 10 + (str_date[6] - 48);
		int day = (str_date[8] - 48) * 10 + (str_date[9] - 48);
		result = year * 10000 + month * 100 + day;
	}
	return result;
}

int get_time_diff(int time1, int time2)
{
	int time1_hour = time1 / 10000000;
	int time1_min = (time1 / 100000) % 100;
	int time1_sec = ((time1 / 1000) % 100);
	int time2_hour = time2 / 10000000;
	int time2_min = (time2 / 100000) % 100;
	int time2_sec = ((time2 / 1000) % 100);

	int hour_diff = time2_hour - time1_hour;
	int min_diff = time2_min - time1_min;
	int sec_diff = time2_sec - time1_sec;
	return (hour_diff * 3600 + min_diff * 60 + sec_diff);
}

int add_time(int int_time, int seconds)
{
	int time_hour = int_time / 10000000;
	int time_min = (int_time / 100000) % 100;
	int time_sec = ((int_time / 1000) % 100);

	int current_sec = time_hour * 3600 + time_min * 60 + time_sec;
	int sum_sec = current_sec + seconds;

	int time1_hour = sum_sec / 3600;
	int time1_min = (sum_sec % 3600) / 60;
	int time1_sec = ((sum_sec % 3600) % 60);
	return (time1_hour * 10000 + time1_min * 100 + time1_sec) * 1000;
}


int convert_itime_2_sec(int itime)
{
	int isec;
	int it1, it2, it3;
	it1 = itime / 10000000;
	it2 = (itime - it1 * 10000000) / 100000;
	it3 = (itime % 100000) / 1000;
	isec = it3 + it2 * 60 + it1 * 3600;

	return isec;
}

int get_int_time(char *str_time)
{
	if (str_time == NULL) 
		return 0;

	int result = atoi(str_time);
	if (result < 30000000){
		result = result + 240000000;
	}
	return result;
}

/*Convert int_time e.g 134523 to char time e.g "13:45:23"*/
void get_char_time(int int_time, char *des_time)
{
	int divider = 100000;
	for (int i = 0; i < 8; i++){
		if (i == 2 || i == 5){
			des_time[i] = 58;  //stands for ":"
			continue;
		}
		des_time[i] = int_time / divider % 10 + 48;
		divider /= 10;
	}
}


/* Convert char date string, 2016-11-01 */
int get_date(const char *str_date)
{
	int result = 0;
	if (strlen(str_date) >= 8){
		int year = (str_date[0] - 48) * 1000 + (str_date[1] - 48) * 100
			+ (str_date[2] - 48) * 10 + (str_date[3] - 48);
		int month = (str_date[4] - 48) * 10 + (str_date[5] - 48);
		int day = (str_date[6] - 48) * 10 + (str_date[7] - 48);
		result = year * 10000 + month * 100 + day;
	}
	return result;
}

int get_timestamp(char *src, int millisec)  // char = "13.01.01"
{
	int t;
	t = src[0] - 48;
	t = t * 10;
	t = t + (src[1] - 48);
	t = t * 10;
	t = t + (src[3] - 48);
	t = t * 10;
	t = t + (src[4] - 48);
	t = t * 10;
	t = t + (src[6] - 48);
	t = t * 10;
	t = t + (src[7] - 48);

	t = t * 1000;
	t = t + millisec;

	if (t < 30000000)
		t = t + 240000000;

	return t;
}

int get_timestamp_all_number(char *src, int millisec)  //char = "130101" or "20101"
{
	int t;
	if (strlen(src) > 8){
		t = src[0] - 48;
		t = t * 10;
		t = t + (src[1] - 48);
		t = t * 10;
		t = t + (src[2] - 48);
		t = t * 10;
		t = t + (src[3] - 48);
		t = t * 10;
		t = t + (src[4] - 48);
		t = t * 10;
		t = t + (src[5] - 48);
	}
	else {
		t = src[0] - 48;
		t = t * 10;
		t = t + (src[1] - 48);
		t = t * 10;
		t = t + (src[2] - 48);
		t = t * 10;
		t = t + (src[3] - 48);
		t = t * 10;
		t = t + (src[4] - 48);
	}

	t = t * 1000;
	t = t + millisec;

	if (t < 30000000)
		t = t + 240000000;

	return t;
}

int64_t convert_exchange_time(int64_t exch_time)
{
	if (exch_time < 30000000)
		exch_time += 240000000;
	return exch_time;
}

void merge_timestamp(char *update, int millisec, char *result)
{
	char temp[16] = "";
	result[0] = update[0]; result[1] = update[1];
	result[2] = update[3]; result[3] = update[4];
	result[4] = update[6]; result[5] = update[7];

	snprintf(temp, 4, "%03d", millisec);
	result[6] = temp[0]; result[7] = temp[1];
	result[8] = temp[2]; result[9] = '\0';
}

int get_timestamp(char *timestamp, bool is_all)
{
	int nret = 0;
	char *update = NULL;
	if (is_all){
		update = timestamp;
	}
	else{
		update = timestamp + 11;
	}

	nret = *update - 48;
	nret = nret * 10;
	nret = nret + (*(update + 1) - 48);
	nret = nret * 10;
	nret = nret + (*(update + 3) - 48);
	nret = nret * 10;
	nret = nret + (*(update + 4) - 48);
	nret = nret * 10;
	nret = nret + (*(update + 6) - 48);
	nret = nret * 10;
	nret = nret + (*(update + 7) - 48);

	if (*(update + 8) == '.'){
		nret = nret * 10;
		nret = nret + (*(update + 9) - 48);
		nret = nret * 10;
		nret = nret + (*(update + 10) - 48);
		nret = nret * 10;
		nret = nret + (*(update + 11) - 48);
	}else{
		nret = nret * 1000;
	}

	if (nret < 30000000)
		nret = nret + 240000000;
	return nret;
}

time_t convert_date_to_time_t(time_t *raw_time, int date)
{
	struct tm* timeinfo = localtime(raw_time);
	/* now modify the timeinfo to the given date: */
	timeinfo->tm_year = (date / 10000) - 1900;
	int date_with_year = date % 10000;  // MMDD
	timeinfo->tm_mon = date_with_year / 100 - 1;    //months since January - [0,11]
	timeinfo->tm_mday = date_with_year % 100;          //day of the month - [1,31] 
	timeinfo->tm_hour = 0;         //hours since midnight - [0,23]
	timeinfo->tm_min = 0;          //minutes after the hour - [0,59]
	timeinfo->tm_sec = 0;          //seconds after the minute - [0,59]
	/* call mktime: create unix time stamp from timeinfo struct */
	time_t from_date_seconds = mktime(timeinfo);

	return from_date_seconds;
}

/* Get days between two int dates, doesn't include start/end date. 20170213~20170215 has 1 day in between */
int get_days_between_int_dates(int start_date, int end_date)
{
	if (start_date == 0 || end_date == 0 || start_date >= end_date)
		return 0;
	/* Start Date */
	time_t rawtime = time(0); //or: rawtime = time(0);
	
	time_t from_date_seconds = convert_date_to_time_t( &rawtime, start_date );
	time_t end_date_seconds = convert_date_to_time_t( &rawtime, end_date );
	
	long int seconds_diff = static_cast<long int>(end_date_seconds - from_date_seconds);

	return (int)(seconds_diff / (24 * 60 * 60)) - 1;   // Divide by total seconds in a day minus the 1 day bounder.
}

//------------------ Algorithm  related---------------------
int
search_double_array(double arr[], int size, double target)
{
	for (int i = 0; i < size; i++){
		if (compare(arr[i], target, PRECISION) == 0)
			return i;
	}
	return -1;
}

void get_hours_min(int time_val, int &hours, int &min)
{
	int temp = time_val / 100000;
	hours = temp / 100;
	min = temp % 100;
}

int add_interval(int &hours, int &min)
{
	int temp = min + 100000;
	hours += temp / 60;
	min = temp % 60;
	return (hours * 100 + min) * 100000;
}


int convert_unixtime_with_nanoseconds(int64_t market_time)
{
	struct tm rcv_time = { 0 };
	const time_t l_time_t = market_time / 1000000;
	int mili_sec = (market_time % 1000000) / 1000;

	localtime_s(&rcv_time, &l_time_t);
	return rcv_time.tm_hour * 10000000 + rcv_time.tm_min * 100000 + rcv_time.tm_sec * 1000 + mili_sec;
}

// input format HHMMSSmmm
int64_t normalize_inttime_to_milliseconds(int64_t a_int_time)
{
	int64_t hours, mins, secs, msecs;
	hours = a_int_time / 10000000;
	mins = (a_int_time % 10000000) / 100000;
	secs = (a_int_time % 100000) / 1000;
	msecs = a_int_time % 1000;
	return (hours * 60 * 60 * 1000 + mins * 60 * 1000 + secs * 1000 + msecs);
}

double calc_curr_avgp(double a_notional, int a_volume, double a_pre_avg_px, double a_multiple)
{
	double dres;
	if (a_volume > 0 && a_notional > 0.0)
		dres = a_notional / a_volume;
	else
		dres = a_pre_avg_px * a_multiple;

	return dres;
}