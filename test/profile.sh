#!/bin/sh

../bin/stmd --config=default.conf --start=$1 --local_log=../logs/$1.log --profile=../profile/$1.profile
