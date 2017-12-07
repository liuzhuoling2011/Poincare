#!/bin/sh 

AGENT_SCRIPT='agent.sh'
SCRIPT_NAME='new_master.py'
TASK_HANDLE='main.py'

check()
{
   ps -ef |grep $SCRIPT_NAME |grep -v 'grep'
   ps -ef |grep $TASK_HANDLE |grep -v 'grep'
}


start()
{
    source ./env.sh
    echo -e "\033[32mStart agent server...\033[0m"
    RUNNING=`ps -aux | grep -c $SCRIPT_NAME | bc`
    if [ "$RUNNING " -lt 2 ]; then
        python $SCRIPT_NAME >/dev/null 2>&1 &
    fi
    sleep 5
    check;
}

make_all()
{
    cd components/match_engine
    make debug
    ################## wrapper part ###############  
    cd ../me_wrapper
    cp ../match_engine/libmatch_engine.so ./
    make debug
    echo -e "\033[32mBuild MatchEngine Successful!\033[0m"
    
    cd ../st_wrapper
    make debug
    echo -e "\033[32mBuild StrategyWrapper Successful!\033[0m"

    cd ../check/OrderCheck
    cp ../../include/strategy_interface.h ./strategy_interface.h
    cp ../../include/quote_format_define.h ./quote_format_define.h
    cp ../../include/utils/MyHash.h ./utils/MyHash.h
    make debug
    cp CheckHandler.so ../../../test/strat_so
    echo -e "\033[32mBuild CheckModule Successful!\033[0m"

    cd ../../smart_execution/SmartExecution
    cp ../../include/strategy_interface.h ./strategy_interface.h
    cp ../../include/quote_format_define.h ./
    cp ../../include/utils/MyHash.h ./utils/
    cp ../../include/utils/MyArray.h ./utils/
    make debug
    cp SmartExecution.so ../../../test/strat_so
    echo -e "\033[32mBuild SmartExecution Successful!\033[0m"
    
    ################## demo strategy part ###############  
    cd ../../strategy/sample_strategy/SDPHandler
    cp ../../../include/strategy_interface.h ./strategy_interface.h
    cp ../../../include/quote_format_define.h ./
    cp ../../../include/utils/MyHash.h ./sdp_handler/utils/
    cp ../../../include/utils/MyArray.h ./sdp_handler/utils/
    make debug
    cp sample_strategy.so ../../../../test/strat_so
    
    cd ../../sample_easy
    cp ../../include/strategy_interface.h ./
    cp ../../include/quote_format_define.h ./
    make debug
    cp sample_easy.so ../../../test/strat_so
    
    cd ../sample_pass
    cp ../../include/strategy_interface.h ./
    cp ../../include/quote_format_define.h ./
    make debug
    cp sample_pass.so ../../../test/strat_so

    cd ../sample_tradelist
    cp ../../include/strategy_interface.h ./
    cp ../../include/quote_format_define.h ./
    make debug
    cp sample_tradelist.so ../../../test/strat_so
    echo -e "\033[32mBuild All Successful!\033[0m"
}


stop_all()
{
    echo "Stop stop_master..."
    ps -ef |grep $SCRIPT_NAME |grep -v 'grep' |awk '{if($2~/^[0-9]+$/)print $2}'|xargs kill -9
    sleep 1
    echo "Stop stop_agent..."
    ps -ef |grep $TASK_HANDLE |grep -v 'grep' |awk '{if($2~/^[0-9]+$/)print $2}'|xargs kill -9
    sleep 1

    echo "Stop done!"
    check;
}

clear_all()
{
    rm -rf ./logs
    rm -rf ./__pycache__
}

run_test()
{
    source ./env.sh
    python test/test.py
}

run_redis_test()
{
    source ./env.sh
    python utils.py 1
    python main.py
}

if [ $1 = "start" ];then
    start;
elif [ $1 = "stop" ];then
    stop_all;
elif [ $1 = "restart" ];then
    stop_all
    sleep 1
    start
elif [ $1 = "check" ];then
    check;
elif [ $1 = "make" ];then
    make_all;
elif [ $1 = "clean" ];then
    find . -name '*.pyc' | xargs rm
    clear_all;
elif [ $1 = "test" ]; then
    run_test;
elif [ $1 = "rtest" ]; then
    run_redis_test;
else
   echo "Please choose: agent.sh start|stop|check|make|clean|test"
fi
