# -*-coding:utf-8-*-

import json
from enum import Enum


class TaskStatus(Enum):
    InProcess = 0
    Stop = 1
    Finished = 2
    InQueue = 3
    Failed = 4


class CallbackMsgType(Enum):
    Coredump_Msg = 0
    Screen_Msg = 1
    PyAgent_Msg = 2
    PyAgent_Flush_Msg = 3


class JsonType(Enum):
    Json_Task = 1,
    Json_Resp = 2,
    Json_Config = 3


class Status:
    def __init__(self, ip, pid, time):
        self.__ip = str(ip)
        self.__pid = pid
        self.cur_time = time
        self.status = TaskStatus.InProcess.value
        self.task_id = ''
        self.st_name = ''
        self.log_name = ''
        self.task = ''

    def output(self):
        return json.dumps({
            'time': self.cur_time,
            'task_id': self.task_id,
            'st_name': self.st_name,
            'ip': self.__ip,
            'pid': self.__pid,
            'status': self.status,
            'log_name': self.log_name,
            'task': self.task
        })
