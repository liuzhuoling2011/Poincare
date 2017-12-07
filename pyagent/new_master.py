import os
import time
import json
from config import config_json
from multiprocessing import cpu_count
from subprocess import call, check_output


if __name__ == '__main__':
    CPU_NUM = cpu_count() * 0.75
    cmd_check = 'ps -ef | grep -c %s' % config_json['TASK_HANDLER']
    cmd_start_handler = 'python ./%s &' % config_json['TASK_HANDLER']
    while True:
        cur_process_num = int(check_output(cmd_check, shell=True)) - 2
        if cur_process_num < CPU_NUM:
            call(cmd_start_handler, shell=True)
        time.sleep(2)
