#!/bin/sh

../bin/luxd --config=default.conf --start=$1 --local_log=../logs/$1.log --extra=$2
