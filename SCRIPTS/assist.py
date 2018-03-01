# -*- coding:utf-8 -*-

import time
import json
import os
import shutil
from multiprocessing import cpu_count
import subprocess

data_output = {}
BUY_SELL = ["买", "卖"]
OPEN_CLOSE = ["开仓", "平仓", "强平", "平今", "平昨"]
STATUS = ["报单", "", "", "", "", "撤单"]

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
        shutil.copy("./poincare_assist", folder)
        shutil.copy("./libthosttraderapi.so", folder)
        task_config = account
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

            if 'contracts' in task_info:
                contracts_info = task_info['contracts']
                contracts_info = sorted(contracts_info, key=lambda d:d['symbol'])
                log_info += '持仓信息:\n'
                for pos in contracts_info:
                    log_info += '合约名: %6s 今仓多仓: %s 今仓空仓: %s 昨仓多仓: %s 昨仓空仓: %s\n' % \
                        (pos["symbol"], pos["tpos_long_val"], pos["tpos_short_val"], \
                        pos["ypos_long_val"], pos["ypos_short_val"])

            if 'orderlist' in task_info:
                orderlist_info = task_info['orderlist']
                orderlist_info = sorted(orderlist_info, key=lambda d:(d['InsertDate'], d['InsertTime']))
                log_info += '报单信息:\n'
                for order in orderlist_info:
                    log_info += '合约名: %6s 买卖: %s 开平: %s 成交价格: %8.2f 手数: %3d 报单时间: %s 报单编号: %6s 报单状态: %s 状态信息: %s\n' % \
                        (order['InstrumentID'], BUY_SELL[order['Direction'] - ord('0')], OPEN_CLOSE[order['OffsetFlag'] - ord('0')],
                        order['Price'], order['Volume'], order['InsertDate'] + ' ' + order['InsertTime'], 
                        '0' if len(order['OrderSysID'].strip()) == 0 else order['OrderSysID'].strip(), 
                        STATUS[order['OrderStatus'] - ord('0')], order['StatusMsg'])
           
            if 'tradelist' in task_info:
                tradelist_info = task_info['tradelist']
                tradelist_info = sorted(tradelist_info, key=lambda d:(d['TradeDate'], d['TradeTime']))
                log_info += '交易信息:\n'
                for trade in tradelist_info:
                    log_info += '合约名: %6s 买卖: %s 开平: %s 成交价格: %8.2f 手数: %3d 成交时间: %s 报单编号: %6s 成交编号: %s\n' % \
                        (trade['InstrumentID'], BUY_SELL[trade['Direction'] - ord('0')], OPEN_CLOSE[trade['OffsetFlag'] - ord('0')],
                        trade['Price'], trade['Volume'], trade['TradeDate'] + ' ' + trade['TradeTime'], trade['OrderSysID'].strip(), trade['TradeID'].strip())
            log_info += '\n' + '-' * 50 + '\n'
            logfile.write(log_info)

