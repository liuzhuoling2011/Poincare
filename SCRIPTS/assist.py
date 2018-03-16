# -*- coding:utf-8 -*-

import time
import json
import os
import shutil
from multiprocessing import cpu_count
import subprocess

data_output = {}

if __name__ == '__main__':
    config_file = open('config.json')
    config_json = json.load(config_file)
    
    for account in config_json['accounts']:
        folder = './' + account["USER_ID"] + '/'
        if os.path.isdir(folder):
            shutil.rmtree(folder)
            os.mkdir(folder)   
        else:
            os.mkdir(folder)
        shutil.copy("./" + config_json['HANDLER'], folder)
        shutil.copy("./libthosttraderapi.so", folder)
        task_config = account
        task_config['ALL_CONTRACT_FLAG'] = config_json['ALL_CONTRACT_FLAG']
        task_config['CONTRACT_POS_FLAG'] = config_json['CONTRACT_POS_FLAG']
        task_config['ORDER_INFO_FLAG'] = config_json['ORDER_INFO_FLAG']
        task_config['TRADE_INFO_FLAG'] = config_json['TRADE_INFO_FLAG']
        task_config['WRITE_LOG'] = config_json['WRITE_LOG']
        task_config['ASSIST_LOG'] = config_json['ASSIST_LOG']
        with open(folder + 'config.json','w') as fconfig:
            fconfig.write(json.dumps(task_config, indent=4))
        cmd_start_handler = 'cd %s;./%s &' % (folder, config_json['HANDLER'])
        p = subprocess.Popen(cmd_start_handler, shell=True, stdout=subprocess.PIPE)  
        data_output[account["USER_ID"]] = (p.stdout.read()).decode('gbk')
        shutil.rmtree(folder)
        
    #print(json.dumps(data_output, indent=4))
    with open('results.log','w') as logfile:
        for info in data_output:
            task_info = json.loads(data_output[info])
            account_info = task_info['account']
            static_power = account_info["PreBalance"] - account_info["Withdraw"] + account_info["Deposit"]
            dynamic_power = static_power + account_info["CloseProfit"] + account_info["PositionProfit"] - account_info["Commission"]
            log_info = '\n账户名: %s 静态权益: %s 动态权益: %s 持仓盈亏: %s 平仓盈亏: %s 手续费: %s 占用保证金: %s 可用现金: %s\n' % \
                (account_info["AccountID"], static_power, dynamic_power, account_info["PositionProfit"], account_info["CloseProfit"], \
                account_info["Commission"], account_info["ExchangeMargin"], account_info["Available"],)

            log_info += '\n' + '-' * 50 + '\n'
            logfile.write(log_info)

