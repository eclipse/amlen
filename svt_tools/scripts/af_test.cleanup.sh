#!/bin/bash -x

if [ -z "$1" ] ; then
    echo "ERROR : $1 af_test processname to cleanup is a required parameter"
    exit 1;
fi
echo "-----------------------------------------"
echo "Send sigkill (9) to all af_test $1 processes"
echo "-----------------------------------------"
ps -ef |grep af_test |grep $1 | grep -v grep | grep -v cleanup  |awk '{print $2}' |xargs -i echo "kill -9 {}" |sh -x

echo "-----------------------------------------"
echo "Completed all expected processing"
echo "-----------------------------------------"

exit 0;



