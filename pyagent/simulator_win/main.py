# encoding: utf-8

import json
import signal

from config import config_json
from simulator import execute_task


def signal_handler(signum, frame):
    if signum == 2:
        print("Recv abort signal and exit success", frame)
        exit(0)


def get_task():
    fd = open(config_json['STRAT_CONF'])
    return json.load(fd)


if __name__ == '__main__':
    signal.signal(signal.SIGINT, signal_handler)

    origin_task = get_task()
    execute_task(origin_task)
