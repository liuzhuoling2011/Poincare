#ifndef __REDIS_HANDLER_H__
#define __REDIS_HANDLER_H__

#include <string.h> 
#include <map>
#include "hiredis.h"

#define REDIS_DEBUG

struct ptrCmp
{
    bool operator()( const char * s1, const char * s2 ) const {
        return strcmp( s1, s2 ) < 0;
    }
};

class RedisHandler{
public:
    RedisHandler(const char* ip, int port);
    ~RedisHandler();
    redisContext* redis;
};

class RedisList{
public:
    RedisList(RedisHandler* handler, const char* key);
    int size();
    void rpush(char* value);
    char* blpop();
    void freeStr();
private:
    char key[64];
    redisContext* redis;
    redisReply* reply;
};

class RedisHash{
public:
    RedisHash(RedisHandler* handler, const char* name);
    int size();
    void remove(char* key);
    void set(char* key, char* value);
    char* get(char* key);
    void getall();
    bool exist(char* key);
    void freeStr();
private:
    char name[64];
    std::map<char*, char*, ptrCmp> hash;
    redisContext* redis;
    redisReply* reply;
};

#endif