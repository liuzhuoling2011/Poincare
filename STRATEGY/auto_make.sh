#!/bin/sh

if [ $# -ge 1 ]; then
    echo "Make $1 strategy..."
    cp -f $1 strategy.cpp
    make clean; make debug
else
    echo "Make default strategy..."
    make clean; make debug
fi
