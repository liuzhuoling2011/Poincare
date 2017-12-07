#!/usr/bin/env python3.5
# -*- coding: utf-8 -*-

import os
from raven import Client

##################### APPLIED POSITION #################
LOCAL_POSITION = 0
IDC_POSITION = 1
##################### CONFIG SWITCHES ##################
DEBUG = True
RELEASE = False
PROFILE = False
POSITION = IDC_POSITION
##################### PyAgent PART CONFIG  #############

kdb_config = {
    "port": 9000,
    "username": "fR#cV%nL@cL=uR`bD(mM)lO/iD(fE#",
    "password": "eB1*cC2&uK3#gL6@gD7^pS4#bH9^cG"
}

mysql_config = {
    "port": 3306,
    "db": "datahub_test",
    "user": "root",
    "passwd": "123456",
}

config_json = {
    "TASK_HANDLER": "main.py",
    "NETWORK_CARD": "eth0",
    "BASE_PATH": "./test",
    "LOG_DIR" : "agent_log",
    "AGENT_LOG_FILE" : "%s/agent_%s.log",
    "STRAT_CONF": "test/config/strat_config.json",
    "SETTLEMENT_PATH": "/home/rss/colin/bss_server/new_settlement",
    "REDIS_DIR": "redis",
    "REDIS_PORT": 6379,
    "REDIS_DB": 0,
}

if RELEASE:
    if POSITION == IDC_POSITION:
        kdb_config["host"] = "192.168.10.102"
        mysql_config["host"] = "192.168.10.100"
        mysql_config["db"] = "datahub"
        config_json["REDIS_HOST"] = "192.168.10.100"
        config_json["IP"] = "192.168.10.101"
        config_json["BASE_PATH"] = "/mnt/share_media"
        config_json["SETTLEMENT_PATH"] = "/home/mycapitaltrade"
        client = Client('http://d0713b31230a4351a98dd6ef47829b0c:878d6e5a85a64730b3dc9cd6b347cded@121.46.13.124:9090/3')
    else:
        kdb_config["host"] = "192.168.1.41"
        mysql_config["host"] = "127.0.0.1"
        mysql_config["db"] = "datahub"
        config_json["REDIS_HOST"] = "127.0.0.1"
        config_json["IP"] = "192.168.1.214"
        config_json["BASE_PATH"] = "/home/rss/bss_server/site/media"
        config_json["SETTLEMENT_PATH"] = "/home/rss"
        client = Client('http://4134ee7d2d794419b05c3b928343640c:7c2584ef60d94f0e954e7e2044372ea9@192.168.3.18:9000/2')
else:
    kdb_config["host"] = "192.168.1.41"
    mysql_config["host"] = "192.168.1.14"
    config_json["REDIS_HOST"] = "127.0.0.1"
    config_json["IP"] = "192.168.1.14"
    config_json["BASE_PATH"] = "/home/rss/bss_server/site/media"
    client = Client('http://4134ee7d2d794419b05c3b928343640c:7c2584ef60d94f0e954e7e2044372ea9@192.168.3.18:9000/2')

if DEBUG:
   mysql_config = {}
   config_json["BASE_PATH"] = "./test"
   config_json["REDIS_ADD_TASK_QUEUE"] = "master_add_task1"
   config_json["REDIS_STOP_TASK_MAP"] = "master_stop_task1"
   config_json["REDIS_TASK_RESULTS"] = "master_task_results1"
   config_json["REDIS_TASK_STATUS"] = "master_task_status1"
   config_json["REDIS_TASK_SCREEN_PRINT"] = "master_task_screen_print1"

   # none sentry message send when open the DEBUG switch
   class FakeClient: pass
   def none_method(self, *args, **kwargs): pass
   def print_method(self, *args, **kwargs): print("The Error is:\n\n", args, "\n\n")
   FakeClient.captureException = none_method
   FakeClient.captureMessage = print_method
   client = FakeClient()
else:
   config_json["REDIS_ADD_TASK_QUEUE"] = "new_master_add_task"
   config_json["REDIS_STOP_TASK_MAP"] = "master_stop_task"
   config_json["REDIS_TASK_RESULTS"] = "master_task_results"
   config_json["REDIS_TASK_STATUS"] = "master_task_status"
   config_json["REDIS_TASK_SCREEN_PRINT"] = "master_task_screen_print"
