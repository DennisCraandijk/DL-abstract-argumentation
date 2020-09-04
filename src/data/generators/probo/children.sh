#!/bin/bash
function getcpid() {
    cpids=`pgrep -P $1|xargs`
    for cpid in $cpids;
    do
        echo "$cpid"
        getcpid $cpid
    done
}

getcpid $1
