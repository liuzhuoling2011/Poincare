# -*- coding: utf-8 -*-

import json
import os
import signal
import sys
import time

file_path = os.path.abspath(__file__)
pare_path = os.path.dirname(file_path)
base_path = os.path.dirname(pare_path)

sys.path.append(base_path)
sys.path.append(os.path.join(base_path, "lib"))

from config import config_json

sys.path.append(config_json['SETTLEMENT_PATH'])
print(sys.path, config_json['SETTLEMENT_PATH'])

from simulator.simu_handler import execute_task


def rsp_handler(msg):
    print("print from python", msg)


def signal_handler(signum, frame):
    if signum == 2:
        print("Recv abort signal and exit success")
        exit(0)


def extra_handler(msg_type, msg):
    print("extra msg is", msg_type, msg)


def run_test(cfg_file):
    signal.signal(signal.SIGINT, signal_handler)
    cwd = os.getcwd()
    task_obj = None
    with open(cfg_file, "r+") as f:
        task_json = f.read()
        task_obj = json.loads(task_json)
        if 'h5h78.so' in task_obj['so_path'] and cwd.find('test') == -1:
            os.chdir('./test')
    if task_obj:
        print(task_obj)
        t1 = time.time()
        execute_task(task_obj, 'agent_log', rsp_handler, extra_handler)
        print("time consumption %s seconds" % (time.time() - t1))


def run_one_test():
    # config_file = os.path.join(pare_path, "config", "win_task.json")
    config_file = os.path.join(pare_path, "config", "easy_check_task.json")
    # config_file = os.path.join(pare_path, "config", "easy_task.json")
    # config_file = os.path.join(pare_path, "config", "test.json")
    # config_file = os.path.join(pare_path, "config", "ltdelay_task.json")
    # config_file = os.path.join(pare_path, "config", "ltdelay_check_task.json")
    # config_file = os.path.join(pare_path, "config", "strat_config.json")
    # config_file = os.path.join(pare_path, "config", "vwap.json")
    # config_file = os.path.join(pare_path, "config", "stock.json")
    run_test(config_file)


def run_all_test():
    test_cases = [
        'easy_check_task.json',
        'easy_task.json',
        'test.json',
        'ltdelay_task.json',
        'ltdelay_check_task.json',
        'strat_config.json',
        'vwap.json',
        'stock.json'
    ]
    for case in test_cases:
        filename = os.path.join(pare_path, "config", case)
        run_test(filename)


def func_level_profiler():
    from cProfile import Profile
    from pstats import Stats
    profiler = Profile()
    profiler.runcall(run_one_test)
    stats = Stats(profiler)
    stats.strip_dirs()
    stats.sort_stats('cumulative')
    stats.print_stats()


def trace_the_malloc():
    import tracemalloc
    tracemalloc.start()
    snapshot1 = tracemalloc.take_snapshot()
    run_one_test()
    snapshot2 = tracemalloc.take_snapshot()
    top_stats = snapshot2.compare_to(snapshot1, 'lineno')

    print("[ Top 10 differences ]")
    for stat in top_stats[:20]:
        print(stat)


if __name__ == "__main__":
    run_one_test()
    # run_all_test()
    # func_level_profiler()
    # trace_the_malloc()
