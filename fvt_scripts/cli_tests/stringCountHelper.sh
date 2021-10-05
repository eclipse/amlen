#! /bin/bash
# wc Helper

echo -e "\nexecuting stringCountHelper with the following parameters: \"$1\" \"$2\""

count=`grep -o "$1" "$2" | wc -w`

echo "ResultCount=${count}"

# We have to sleep so that run-scenarios.sh can verify that it is running by getting the PID
sleep 0.1
