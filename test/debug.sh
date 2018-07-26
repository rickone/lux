#!/bin/sh

uname=`uname`
debug="gdb --args"
if [ "$uname" = "Darwin" ]; then
    debug="lldb --"
fi

$debug ../bin/luxd --config=default.conf --start=$1 --local_log=../logs/$1.log --extra=$2
