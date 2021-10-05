#!/bin/bash

server=$1

if [ -n "$server" ] ;then

    . ./svt_test.sh
    while((1)); do
    
    #echo "dmesg -c" | ssh  admin@mar425 "busybox" | grep -v "WARNING: This command is available only for development builds." | awk '{printf ("DMESG:%s %s" , '$server' , $0);}' | add_log_time
    echo "dmesg -c" | ssh  admin@${server} "busybox" | grep -v "WARNING: This command is available only for development builds." |  awk '{printf ("DMESG:'${server}': %s\n" , $0);}' | add_log_time
    #echo "dmesg -c" | ssh  admin@mar425 "busybox" | grep -v "WARNING: This command is available only for development builds." | add_log_time
    
    sleep 1;
    
    done
    
else
    echo "ERROR: must input server to monitor"
fi
