#include "redis_handler.h"

RedisHandler::RedisHandler(const char* ip, int port){
    redis = redisConnect(ip, port);
#ifdef REDIS_DEBUG
    if (redis == NULL || redis->err) {
        if (redis) {
            printf("Connection error: %s\n", redis->errstr);
            redisFree(redis);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
    }
#endif
    printf("Connect to redisServer:%s Success\n", ip); 
}

RedisHandler::~RedisHandler(){
    redisFree(redis);
}


RedisList::RedisList(RedisHandler* handler, const char* key){
    redis = handler->redis;
    strcpy(this->key, key);
}

int RedisList::size(){
    reply = (redisReply*)redisCommand(redis, "llen %s", key);
#ifdef REDIS_DEBUG
    if ( reply->type == REDIS_REPLY_ERROR ) {
        printf( "Error: %s\n", reply->str );
    } else if (reply->type != REDIS_REPLY_INTEGER) {
        fprintf(stderr, "Invalid LLEN reply from Redis\n");
    }
    printf("llen: %lld\n", reply->integer);
#endif
    int l_size = reply->integer;
    freeReplyObject(reply);  
    return l_size;  
}

void RedisList::rpush(char* value){
    reply = (redisReply*)redisCommand(redis, "rpush %s %s", key, value);
#ifdef REDIS_DEBUG
    if ( reply->type == REDIS_REPLY_ERROR ) {
        printf( "Error: %s\n", reply->str );
    } else if (reply->type != REDIS_REPLY_INTEGER) {
        fprintf(stderr, "Invalid RPUSH reply from Redis\n");
    }
#endif
    freeReplyObject(reply);  
}

char* RedisList::blpop(){
    reply = (redisReply*)redisCommand(redis, "blpop %s 0", key);
#ifdef REDIS_DEBUG
    if ( reply->type == REDIS_REPLY_ERROR ) {
        printf( "Error: %s\n", reply->str );
    } else if (reply->type != REDIS_REPLY_ARRAY || reply->elements < 2) {
        fprintf(stderr, "Invalid BLPOP reply from Redis\n");
    }
    //printf("Redis: %s => %s\n", reply->element[0]->str, reply->element[1]->str);
#endif
    return reply->element[1]->str;
}

void RedisList::freeStr(){
    freeReplyObject(reply);  
}


RedisHash::RedisHash(RedisHandler* handler, const char* name){
    redis = handler->redis;
    strcpy(this->name, name);
}

int RedisHash::size(){
    reply = (redisReply*)redisCommand(redis, "hlen %s", name);
#ifdef REDIS_DEBUG
    if ( reply->type == REDIS_REPLY_ERROR ) {
        printf( "Error: %s\n", reply->str );
    } else if (reply->type != REDIS_REPLY_INTEGER) {
        fprintf(stderr, "Invalid HLEN reply from Redis\n");
    }
    printf("hlen: %lld\n", reply->integer);        
#endif
    int l_size = reply->integer;
    freeReplyObject(reply); 
    return l_size;
}

void RedisHash::remove(char* key){
    reply = (redisReply*)redisCommand(redis, "hdel %s %s", name, key);
#ifdef REDIS_DEBUG
    if ( reply->type == REDIS_REPLY_ERROR ) {
        printf( "Error: %s\n", reply->str );
    } else if (reply->type != REDIS_REPLY_INTEGER) {
        fprintf(stderr, "Invalid HDEL reply from Redis\n");
    }
#endif
    freeReplyObject(reply); 
}

void RedisHash::set(char* key, char* value){
    reply = (redisReply*)redisCommand(redis, "hset %s %s %s", name, key, value);
#ifdef REDIS_DEBUG
    if ( reply->type == REDIS_REPLY_ERROR ) {
        printf( "Error: %s\n", reply->str );
    } else if (reply->type != REDIS_REPLY_INTEGER) {
        fprintf(stderr, "Invalid HSET reply from Redis\n");
    }
#endif
    freeReplyObject(reply); 
}

char* RedisHash::get(char* key){
    reply = (redisReply*)redisCommand(redis, "hget %s %s", name, key);
#ifdef REDIS_DEBUG
    if ( reply->type == REDIS_REPLY_ERROR ) {
        printf( "Error: %s\n", reply->str );
    } else if (reply->type != REDIS_REPLY_STRING) {
        fprintf(stderr, "Invalid HGET reply from Redis\n");
    }
    printf("hget %s\n", reply->str);
#endif
    return reply->str;
}

void RedisHash::getall(){
    reply = (redisReply*)redisCommand(redis, "hgetall %s", name);
#ifdef REDIS_DEBUG
    if ( reply->type == REDIS_REPLY_ERROR ) {
        printf( "Error: %s\n", reply->str );
    } else if ( reply->type != REDIS_REPLY_ARRAY ) {
        printf( "Unexpected type: %d\n", reply->type );
    } else {
        for (int i = 0; i < reply->elements; i = i + 2 ) {
            hash.insert(std::make_pair(reply->element[i]->str, reply->element[i + 1]->str));
            //printf( "Result: %s = %s \n", reply->element[i]->str, reply->element[i + 1]->str );
        }
    }
#endif
}

bool RedisHash::exist(char* key){
    getall();
    bool flag = false;
    if(hash.find(key) != hash.end())
        flag = true;
    freeStr();
    return flag;
}

void RedisHash::freeStr(){
    freeReplyObject(reply); 
}