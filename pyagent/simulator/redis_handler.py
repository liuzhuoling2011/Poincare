# -*-coding:utf-8-*-

"""
 *  Description: 和redis进行交互模块
"""

import redis


class RedisHandler:

    def __init__(self, config_json):
        pool = redis.ConnectionPool(host=config_json['REDIS_HOST'], port=config_json['REDIS_PORT'],
                                    db=config_json['REDIS_DB'])
        self.__rds = redis.Redis(connection_pool=pool)
        print('Connect to redisServer:%s Success' % config_json['REDIS_HOST'])

    def get_handler(self):
        return self.__rds


class RedisList:

    def __init__(self, handler, key):
        self.__rds = handler
        self.__key = key

    def setkey(self, key):
        self.__key = key

    def getkey(self):
        return self.__key

    def empty(self):
        return self.__rds.llen(self.__key) == 0

    def size(self):
        return self.__rds.llen(self.__key)

    def show(self):
        return self.range(0, -1)

    def lpush(self, value):
        self.__rds.lpush(self.__key, value)

    def rpush(self, value):
        self.__rds.rpush(self.__key, value)

    def lpop(self):
        result = self.__rds.lpop(self.__key)
        if isinstance(result, bytes):
            return result.decode()
        return result

    def rpop(self):
        result = self.__rds.rpop(self.__key)
        if isinstance(result, bytes):
            return result.decode()
        return result

    def blpop(self):
        result = self.__rds.blpop(self.__key, 0)[1]
        if isinstance(result, bytes):
            return result.decode()
        return result

    def brpop(self):
        result = self.__rds.brpop(self.__key, 0)[1]
        if isinstance(result, bytes):
            return result.decode()
        return result

    def set(self, index, value):
        self.__rds.lset(self.__key, index, value)

    def remove(self, value, number=0):
        self.__rds.lrem(self.__key, value, number)

    def clear(self):
        while self.size() > 0:
            self.lpop()

    def range(self, start, end):
        result = []
        for val in self.__rds.lrange(self.__key, start, end):
            if isinstance(val, bytes):
                result.append(val.decode())
        return result

    def index(self, index):
        result = self.__rds.lindex(self.__key, index)
        if isinstance(result, bytes):
            return result.decode()
        return result


class RedisHash:

    def __init__(self, handler, key):
        self.__rds = handler
        self.__key = key

    def setkey(self, key):
        self.__key = key

    def getkey(self):
        return self.__key

    def empty(self):
        return self.__rds.hlen(self.__key) == 0

    def size(self):
        return self.__rds.hlen(self.__key)

    def show(self):
        return self.getall()

    def show_keys(self):
        result = []
        for key in self.__rds.hkeys(self.__key):
            if isinstance(key, bytes):
                result.append(key.decode())
        return result

    def show_vals(self):
        result = []
        for val in self.__rds.hvals(self.__key):
            if isinstance(val, bytes):
                result.append(val.decode())
        return result

    def exists(self, value):
        self.__rds.hexists(self.__key, value)

    def set(self, key, value):
        self.__rds.hset(self.__key, key, value)

    def get(self, key):
        result = self.__rds.hget(self.__key, key)
        if isinstance(result, bytes):
            return result.decode()
        return result

    def getall(self):
        result = {}
        for key, val in self.__rds.hgetall(self.__key).items():
            if isinstance(key, bytes):
                key = key.decode()
            if isinstance(val, bytes):
                result[key] = val.decode()
        return result

    def increase_val_int(self, value, amount=1):
        self.__rds.hincrby(self.__key, value, amount)

    def increase_val_float(self, value, amount=1.0):
        self.__rds.hincrbyfloat(self.__key, value, amount)

    def remove(self, value):
        self.__rds.hdel(self.__key, value)

    def clear(self):
        for key in self.show_keys():
            self.remove(key)


class RedisSet:

    def __init__(self, handler, key):
        self.__rds = handler
        self.__key = key

    def setkey(self, key):
        self.__key = key

    def getkey(self):
        return self.__key

    def empty(self):
        return self.__rds.scard(self.__key) == 0

    def size(self):
        return self.__rds.scard(self.__key)

    def show(self):
        return self.__rds.smembers(self.__key)

    def add(self, *values):
        map(lambda val: self.__rds.sadd(self.__key, val), values)

    def diff(self, *rds_set):
        return self.__rds.sdiff(self.__key, *tuple(map(lambda l_set: l_set.getkey(), rds_set)))

    def intersection(self, *rds_set):
        return self.__rds.sinter(self.__key, *tuple(map(lambda l_set: l_set.getkey(), rds_set)))

    def union(self, *rds_set):
        return self.__rds.sunion(self.__key, *tuple(map(lambda l_set: l_set.getkey(), rds_set)))

    def exists(self, value):
        self.__rds.sismember(self.__key, value)

    def random(self, number=1):
        return self.__rds.srandmember(self.__key, number)

    def remove(self, value):
        self.__rds.srem(self.__key, value)

    def clear(self):
        for value in self.show():
            self.remove(value)


if __name__ == '__main__':
    rds_handler = RedisHandler().get_handler()

    pass

    # rds = RedisList(rds_handler, "newlist")
    # rds.lpush(1)
    # rds.rpush(2)
    # rds.lpush(3)
    # rds.rpush(4)
    # rds.lpush(4)
    # print rds.empty()
    # print rds.size()
    # print rds.show()
    # print rds.index(2)
    # rds.remove(1, 2)
    # print rds.range(0, -1)
    # rds.clear()
    # print rds.show()

    # rds = RedisHash(rds_handler, "newhash")
    # rds.set(4, 'hello')
    # rds.multi_set({5:'lzl', 6:'jojo', 'num':12, 'f':1.2})
    # print rds.empty()
    # print rds.size()
    # print rds.show_keys()
    # print rds.show_vals()
    # print rds.show()
    # print rds.increase_val_int('num', 2)
    # print rds.get('num')
    # print rds.multi_get(5, 6)
    # rds.increase_val_float('f', 2.1)
    # rds.remove(4)
    # print rds.show()
    # rds.clear()
    # print rds.show()

    # rds = RedisSet(rds_handler, "newset")
    # rds1 = RedisSet(rds_handler, "newset1")
    # rds2 = RedisSet(rds_handler, "newset2")
    # rds.add(4, 5, 6, 7, 8, 9)
    # rds1.add(1, 2, 3, 4, 5, 6)
    # rds2.add(5, 6, 7, 8)
    # print rds.empty()
    # print rds.size()
    # print rds.show()
    # print rds.random(2)
    # rds.remove(4)
    # print rds.show()
    # print rds.diff(rds1, rds2)
    # print rds.union(rds1, rds2)
    # print rds.intersection(rds1, rds2)
    # rds.clear()
    # print rds.show()
