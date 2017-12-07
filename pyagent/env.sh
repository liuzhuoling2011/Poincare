#!/bin/bash

ME_LD_PATH=$(cd `dirname "$1"`;pwd)
export LD_LIBRARY_PATH="$ME_LD_PATH/components/match_engine:$ME_LD_PATH/lib"

if [ ! -d venv ]; then
    python3.5 -m venv ./venv
    source ./venv/bin/activate
    ./venv/bin/pip install -r requirements.txt
else
    source ./venv/bin/activate
fi
