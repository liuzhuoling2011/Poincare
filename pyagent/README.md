## Api Reference: 
	[redis api](http://drtp.my/showdoc-master/index.php?s=/9&page_id=496)

## Usage:
	1. mount npq data
	... ```bash
    ... # for test env
	... $sudo mount -t nfs 192.168.1.41:/data1/npq/data /data
    ... # release env
    ... $sudo mount -t nfs idc-npq-server:/data /data
	... ```
	
    2. compile package
    ... ```bash
    ... $./agent.sh make
    ... ```

    3. run test
    ... ```bash
    ... $cd PyAgent
    ... $source env.sh
    ... $./agent.sh test
    ... ```bash

    4. agent env use
    ... release mode set DEBUG=False, RELEASE=True in config.py
    ... pre_release mode set DEBUG=False, RELEASE=False in config.py
    ... dev test mode set DEBUG=True, RELEASE=False in config.py

## TASK LIST:

## DOING:

## FINISHED:
    - develop an easy strategy base on the low freq strategy api
    - support index quote group query(jiahao)
    - add quote merge method
    - finish a new me and it's wrapper
    - adjust the python strategy wrappper to support this new strategy api
    - use npq currency api to fetch currency data
    - use npq constituent stock api to fetch data
    - test time consumption
    - check memory leak
    - rearrange the directory and configuration
    - applied with new settlement
