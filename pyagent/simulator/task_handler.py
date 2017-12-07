# -*- coding: utf-8 -*-

"""
 *  Description: 任务执行模块，和回测模块进行交互
"""

import json
import os
import sys
from datetime import datetime

from config import config_json

file_path = os.path.abspath(__file__)
pare_path = os.path.dirname(file_path)
base_path = os.path.dirname(pare_path)

sys.path.append(base_path)
sys.path.append(os.path.join(base_path, "lib"))
sys.path.append(config_json['SETTLEMENT_PATH'])

from simulator.agent_defines import *
from simulator.redis_handler import RedisHandler, RedisHash, RedisList
from simulator.simu_handler import execute_task, need_stop_now


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


class TaskManager:
    def __init__(self):
        self.JSON_CONFIG = {}
        self.JSON_TASK = {}
        self.TASK_ID = ''
        self.AGENT_LOG_FILE = ''
        self.AGENT_LOG_BUFFER = ''
        self.SCREEN_BUFFER = ''
        self.DEBUG_FLAG = False
        self.EXECUTE_TASK = None
        self.NEED_STOP_NOW = None
        self.RDS_QUEUE = None
        self.RDS_STOP = None
        self.RDS_STATUS = None
        self.RDS_RESULTS = None
        self.RDS_SCREEN = None
        self.TASK_STATUS = None

    def get_task_status(self):
        return self.TASK_STATUS.status

    def set_task_status(self, status):
        self.TASK_STATUS.status = status

    def set_task_status_in_redis(self):
        self.RDS_STATUS.set(self.TASK_ID, self.TASK_STATUS.output())

    def push_result_to_redis(self, result_json):
        self.RDS_RESULTS.rpush(result_json)

    def push_screen_log_to_redis(self, screen_log):
        self.SCREEN_BUFFER += screen_log
        self.RDS_SCREEN.set(self.TASK_ID, self.SCREEN_BUFFER)

    def write_resp_log(self, resp_log):
        if self.DEBUG_FLAG == True:
            self.AGENT_LOG_BUFFER += '--------------->process_result_func:\n'
            self.AGENT_LOG_BUFFER += resp_log
            self.AGENT_LOG_BUFFER += '\n'

    def is_get_stop_signal(self):
        if self.TASK_ID in self.RDS_STOP.getall():
            return True
        return False

    def remove_stop_signal_in_redis(self):
        self.RDS_STOP.remove(self.TASK_ID)

    def execute_task(self, agent_log_file):
        self.EXECUTE_TASK(self.JSON_TASK, agent_log_file, process_result_func, process_callback_result)


task_manager = TaskManager()


def agent_init(g_config):
    redis_handler = RedisHandler(g_config)
    task_manager.EXECUTE_TASK = execute_task
    task_manager.NEED_STOP_NOW = need_stop_now
    task_manager.JSON_CONFIG = g_config
    task_manager.RDS_QUEUE = RedisList(redis_handler.get_handler(), g_config['REDIS_ADD_TASK_QUEUE'])
    task_manager.RDS_STOP = RedisHash(redis_handler.get_handler(), g_config['REDIS_STOP_TASK_MAP'])
    task_manager.RDS_STATUS = RedisHash(redis_handler.get_handler(), g_config['REDIS_TASK_STATUS'])
    task_manager.RDS_RESULTS = RedisList(redis_handler.get_handler(), g_config['REDIS_TASK_RESULTS'])
    task_manager.RDS_SCREEN = RedisHash(redis_handler.get_handler(), g_config['REDIS_TASK_SCREEN_PRINT'])

    now = datetime.now()
    task_manager.TASK_STATUS = Status(g_config['IP'], os.getpid(), now.strftime('%Y%m%d %H:%M:%S'))

    log_dir = './' + g_config['LOG_DIR']
    if os.path.exists(log_dir) is False:
        os.mkdir(log_dir)

    return True


def agent_get_task():
    return task_manager.RDS_QUEUE.blpop()


def agent_do_task(task):
    l_task = json.loads(task)
    l_task['base_path'] = task_manager.JSON_CONFIG['BASE_PATH']
    task_manager.TASK_ID = task_manager.TASK_STATUS.task_id = str(l_task['task_id'])
    task_manager.TASK_STATUS.st_name = str(l_task['strat_item']['strat_name'])
    task_manager.TASK_STATUS.task = json.dumps(l_task)
    task_manager.DEBUG_FLAG == l_task.get('flag_debug_mode', 0)
    task_manager.JSON_TASK = l_task
    task_manager.AGENT_LOG_FILE = task_manager.JSON_CONFIG['AGENT_LOG_FILE'] % (
    task_manager.JSON_CONFIG['LOG_DIR'], task_manager.TASK_ID)

    if task_manager.is_get_stop_signal():  # Stop task before process
        task_manager.set_task_status(TaskStatus.Stop.value)
        task_manager.remove_stop_signal_in_redis()
    else:
        task_manager.set_task_status(TaskStatus.InProcess.value)
        task_manager.set_task_status_in_redis()
        task_manager.execute_task(task_manager.AGENT_LOG_FILE)

    task_manager.set_task_status_in_redis()


def process_callback_result(msg_type, msg):
    if msg_type == CallbackMsgType.Coredump_Msg.value:
        task_manager.set_task_status(TaskStatus.Failed.value)
        response = json.dumps({'task_id': task_manager.TASK_ID, 'status': TaskStatus.Failed.value,
                               'msg': 'execution coredump:\n %s' % msg})
        task_manager.write_resp_log(response)
        task_manager.push_result_to_redis(response)  # update redis results
        task_manager.set_task_status(TaskStatus.Failed.value)  # update redis status
        task_manager.set_task_status_in_redis()
        task_manager.push_screen_log_to_redis(msg)  # update redis screen log
        if task_manager.DEBUG_FLAG == True:
            with open(task_manager.AGENT_LOG_FILE, 'w') as log_file:  # write agent log
                log_file.write(task_manager.AGENT_LOG_BUFFER)
    elif msg_type == CallbackMsgType.Screen_Msg.value:
        task_manager.push_screen_log_to_redis(msg)
    elif msg_type == CallbackMsgType.PyAgent_Msg.value:
        task_manager.AGENT_LOG_BUFFER += msg
    elif msg_type == CallbackMsgType.PyAgent_Flush_Msg.value:
        if task_manager.DEBUG_FLAG == True:
            with open(task_manager.AGENT_LOG_FILE, 'w') as log_file:
                log_file.write(task_manager.AGENT_LOG_BUFFER)


def process_result_func(strat_resp):
    task_manager.write_resp_log(strat_resp)
    if isinstance(strat_resp, bytes):
        strat_resp = strat_resp.decode()
    json_result = json.loads(strat_resp)
    strat_status = json_result['status']
    if strat_status == TaskStatus.Finished.value:
        if task_manager.get_task_status() == TaskStatus.InProcess.value:  # Other signal will be ignored
            task_manager.set_task_status(TaskStatus.Finished.value)
            response = json.dumps(
                {'task_id': task_manager.TASK_ID, 'status': task_manager.get_task_status(), 'msg': 'Task finished!'})
            task_manager.write_resp_log(response)
            task_manager.push_result_to_redis(response)
    elif strat_status == TaskStatus.InProcess.value:
        task_manager.push_result_to_redis(strat_resp)
        if task_manager.is_get_stop_signal():  # When received strategy resp, check stop command
            task_manager.NEED_STOP_NOW()
            task_manager.remove_stop_signal_in_redis()
            task_manager.set_task_status(TaskStatus.Stop.value)
    elif strat_status == TaskStatus.Stop.value:
        task_manager.set_task_status(TaskStatus.Stop.value)
    elif strat_status == TaskStatus.Failed.value:
        task_manager.set_task_status(TaskStatus.Failed.value)
    task_manager.set_task_status_in_redis()
    return 0
