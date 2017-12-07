#!/usr/bin/python
#-*-coding:utf-8-*-

"""
 *  Description: 基础的工具集
"""

import os
import sys
import json
import random, string
import datetime
import shutil
import re

from config import config_json
from simulator.redis_handler import RedisHandler, RedisHash, RedisList


redis_handler = RedisHandler(config_json)


def get_random_int(max=20):
    return random.randint(1, max)


def get_random_str():
    return ''.join(random.sample(string.ascii_letters, get_random_int()))


##========================clear logs==================================
save_log_days = 7
path_home = os.path.dirname(os.path.realpath(__file__))


def get_expiration_time():
    now_time = datetime.datetime.now()
    return now_time - datetime.timedelta(days=save_log_days)


def get_last_time(file_path):
    timestamp = os.path.getmtime(file_path)
    return datetime.datetime.fromtimestamp(timestamp)


def remove_file(path):
    if os.path.isfile(path):
        os.remove(path)
    elif os.path.isdir(path):
        shutil.rmtree(path, True)

def delete_agent_logs(validity):
    del_task = []
    agent_logs = os.path.join(path_home, 'agent_log')
    all_file = os.listdir(agent_logs)
    for name in all_file:
        file_path = os.path.join(agent_logs, name)
        last_time = get_last_time(file_path)
        if last_time < validity:
            task = re.findall(r"^agent_(.+?)_screen.log$", name)
            if task:
                del_task.append(task[0])
            remove_file(file_path)
    return del_task


def delete_strat_logs(validity):
    strat_logs = os.path.join(path_home, 'logs')
    all_file = os.listdir(strat_logs)
    for name in all_file:
        file_path = os.path.join(strat_logs, name)
        last_time = get_last_time(file_path)
        if last_time < validity:
            remove_file(file_path)

def process_logs():
    validity = get_expiration_time()
    delete_strat_logs(validity)
    return delete_agent_logs(validity)

##=========================end clear=================================


def show_redis_db():
    redis_task_queue = RedisList(redis_handler.get_handler(), config_json['REDIS_ADD_TASK_QUEUE'])
    redis_stop_task_map = RedisHash(redis_handler.get_handler(), config_json['REDIS_STOP_TASK_MAP'])
    redis_task_status = RedisHash(redis_handler.get_handler(), config_json['REDIS_TASK_STATUS'])
    redis_task_results = RedisList(redis_handler.get_handler(), config_json['REDIS_TASK_RESULTS'])
    redis_task_screen_print = RedisHash(redis_handler.get_handler(), config_json['REDIS_TASK_SCREEN_PRINT'])

    if os.path.exists(config_json['REDIS_DIR']) is False:
        os.mkdir(config_json['REDIS_DIR'])

    TaskStatus = {0:'InProcess', 1:'Stop', 2:'Finished', 3:'InQueue', 4:'Failed', 97:"undefined_so_type", 98:"Cpp_So", 99:"Python_So"}
    SortedStatus = {0: 5, 2: 3, 1: 4, 4: 4}

    print("Writing task queue......")
    with open(os.path.join(config_json['REDIS_DIR'], 'task_queue.log'),'w') as logfile:
        for task in redis_task_queue.show():
            logfile.write(task + '\n')

    print("Writing task status......")
    with open(os.path.join(config_json['REDIS_DIR'], 'task_status.log'),'w') as logfile:
        result = {key: json.loads(item) for key,item in redis_task_status.getall().items()}
        result = sorted(result.items(), key=lambda d:(SortedStatus.get(d[1]['status'], d[1]['status']), d[1]['time']))
        for key, json_value in result:
            info =  'TIME:       ' + json_value['time'] + '\nTASK_ID:    ' + json_value['task_id'] \
                    +'\nIP:         ' + json_value['ip'] + '\nPID:        '+ str(json_value['pid']) \
                    +'\nSTATUS:     ' + TaskStatus[json_value['status']]  +'\nLOG_NAME:   ' + json_value['log_name'] + \
                    '\nTASK:       ' + json_value.get('task', '{}')
            logfile.write(info)
            logfile.write('\n============================================================================================================\n')

    print("Writing task screen print......")
    with open(os.path.join(config_json['REDIS_DIR'], 'task_screen_print.log'),'w') as logfile:
        for key,value in redis_task_screen_print.getall().items():
            log = 'TASKID:\t' + key + '\n' + value
            logfile.write(log)
            logfile.write('\n============================================================================================================\n')

    print("Writing task results......")
    with open(os.path.join(config_json['REDIS_DIR'], 'task_results.log'),'w') as logfile:
        for result in redis_task_results.show():
            logfile.write(result + '\n')

    print("Writing stop task......")
    with open(os.path.join(config_json['REDIS_DIR'], 'stop_task.log'),'w') as logfile:
        keys = sorted(redis_stop_task_map.getall().keys())
        logfile.write(' '.join(keys))

    print("Finished, All results are in the redis folder!")

def send_task():
    # st_name = ['Hi5', 'Hi5A', 'Hi50', 'Hi50A', 'Hi51', 'Hi51A', 'Hi52', 'Hi52A', 'Hi53', 'Hi55', 'Hi55A', 'Hi6', 'Hi6A', 'Hi60',
    #                'Hi60A', 'Hi61', 'Hi61A', 'Hi62', 'Hi62A', 'Hi65', 'Hi65A', 'Hi70', 'Hi71', 'Hi72', 'Hi73', 'Hi74', 'Hi75', 'Hi76',
    #                'Hi80', 'Hi81', 'Hi82', 'Hi83', 'Hi84', 'Hi85', 'Hi86', 'Hi91', 'Hi92', 'Hi93']
    trading_days = [20160405, 20160406, 20160407, 20160408, 20160411, 20160412, 20160413, 20160414, 20160415, 20160418, 20160419, 20160420, 20160421, 20160422, 20160425, 20160426, 20160427, 20160428, 20160429, 20160503, 20160504, 20160505, 20160506, 20160509, 20160510, 20160511, 20160512, 20160513, 20160516, 20160517, 20160518, 20160519, 20160520, 20160523, 20160524, 20160525, 20160526, 20160527, 20160530, 20160531, 20160601, 20160602, 20160603, 20160606, 20160607, 20160608, 20160613, 20160614, 20160615, 20160616, 20160617, 20160620, 20160621, 20160622, 20160623, 20160624, 20160627, 20160628, 20160629, 20160630, 20160701, 20160704, 20160705, 20160706, 20160707, 20160708, 20160711, 20160712, 20160713, 20160714, 20160715, 20160718, 20160719, 20160720, 20160721, 20160722, 20160725, 20160726, 20160727, 20160728, 20160729, 20160902, 20160905, 20160906, 20160907, 20160908, 20160909, 20160912, 20160913, 20161017, 20161018, 20161019, 20161020, 20161021, 20161024, 20161025, 20161026, 20161027, 20161028, 20161031, 20161101, 20161102, 20161103, 20161104, 20161107, 20161108, 20161109, 20161110, 20161111, 20161114, 20161115, 20161116, 20161117, 20161118, 20161121, 20161122, 20161123, 20161124, 20161125, 20161128, 20161129, 20161130, 20161201, 20161202, 20161205, 20161206, 20161207, 20161208, 20161209, 20161212, 20161213, 20161214, 20161215, 20161216, 20161219, 20161220, 20161221, 20161222, 20161223, 20161226, 20161227, 20161228, 20161229, 20161230, 20170103, 20170104, 20170105, 20170106, 20170109, 20170110, 20170111, 20170112, 20170113, 20170116, 20170117, 20170118, 20170123, 20170125, 20170126, 20170203, 20170206, 20170207, 20170208, 20170209, 20170210, 20170213, 20170214, 20170215, 20170216, 20170217, 20170220, 20170221, 20170222, 20170223, 20170224, 20170227, 20170228, 20170301, 20170302, 20170303, 20170306, 20170307, 20170308, 20170309, 20170310, 20170313, 20170314, 20170315, 20170316, 20170317, 20170320, 20170321, 20170322, 20170323, 20170324, 20170327, 20170328, 20170329, 20170330, 20170331, 20170405, 20170406, 20170407, 20170410, 20170411, 20170412, 20170413, 20170414, 20170417, 20170418, 20170419, 20170420, 20170421, 20170424, 20170425, 20170426, 20170427, 20170428, 20170502, 20170503, 20170504, 20170505, 20170508, 20170509, 20170510, 20170511]

    redis_task_queue = RedisList(redis_handler.get_handler(), config_json['REDIS_ADD_TASK_QUEUE'])

    fd = open(config_json['STRAT_CONF'])
    con_json = json.load(fd)
    for trader_day in trading_days[0:1]:
        con_json['task_id'] = get_random_str()
        # con_json['start_date'] = 20170901
        # con_json['end_date'] = 20170901
        #for name in st_name:
        #con_json['strat_item'][0]['strat_name'] = [name.lower()]
        redis_task_queue.rpush(json.dumps(con_json))
    print("Successful sent task!")

def stop_task(task_id):
    redis_stop_task_map = RedisHash(redis_handler.get_handler(), config_json['REDIS_STOP_TASK_MAP'])
    redis_stop_task_map.set(task_id, 1)
    print("Successful sent stop signal!")

def clear_redis_db(del_task):
    redis_task_queue = RedisList(redis_handler.get_handler(), config_json['REDIS_ADD_TASK_QUEUE'])
    redis_stop_task_map = RedisHash(redis_handler.get_handler(), config_json['REDIS_STOP_TASK_MAP'])
    redis_task_status = RedisHash(redis_handler.get_handler(), config_json['REDIS_TASK_STATUS'])
    redis_task_results = RedisList(redis_handler.get_handler(), config_json['REDIS_TASK_RESULTS'])
    redis_task_screen_print = RedisHash(redis_handler.get_handler(), config_json['REDIS_TASK_SCREEN_PRINT'])

    #Attention!!! below two redis keys are important
    # redis_task_queue.clear()
    # redis_task_results.clear()
    # redis_stop_task_map.clear()
    # for task in del_task:
    #     redis_task_status.remove(task)
    #     redis_task_screen_print.remove(task)

    redis_task_queue.clear()
    redis_task_results.clear()
    redis_stop_task_map.clear()
    redis_task_status.clear()
    redis_task_screen_print.clear()
    print("Successful clear redis!")

MSG = \
'''
==============================================
Current We Support These Functions:
1. Send Task To Redis
2. Stop Task (need to provide task_id)
3. Show Redis Data
4. Clear Redis Data
Else. Quit
==============================================
'''

def function_selector():
    print(MSG)
    choice = input("Please input your choice: ");
    if choice == '1':
        send_task()
    elif choice == '2':
        task_ids = input("Please input task_id: ").split(' ');
        for id in task_ids:
            stop_task(id)
    elif choice == '3':
        show_redis_db()
    elif choice == '4':
        del_task = process_logs()
        clear_redis_db(del_task)
    else:
        exit()


if __name__ == '__main__':
    if len(sys.argv) == 1:
        while True:
            function_selector()
    elif sys.argv[1] == '1':
        send_task()
    elif sys.argv[1] == '2':
        task_ids = input("Please input task_id: ").split(' ');
        for id in task_ids:
            stop_task(id)
    elif sys.argv[1] == '3':
        show_redis_db()
    elif sys.argv[1] == '4':
        del_task = process_logs()
        clear_redis_db(del_task)
    else:
        exit()
