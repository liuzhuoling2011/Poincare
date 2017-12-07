#!/usr/bin/python
#-*-coding:utf-8-*-

import signal
from config import config_json
from simulator.task_handler import *


def signal_handler(signum, frame):
    if signum == 2:
        print("Recv abort signal and exit success")
        exit(0)


if __name__ == '__main__':
    signal.signal(signal.SIGINT, signal_handler)
    agent_init(config_json)

    while True:
        origin_task = agent_get_task()
        agent_do_task(origin_task)
