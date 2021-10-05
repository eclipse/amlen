#!/bin/bash

SERVER=${1-""}
LOGFILE_ADDN=${2-""}
LOGFILE="log.appliance.${LOGFILE_ADDN}.${SERVER}"
FLAG=${3-""}

if ! [ -n "$SERVER" ] ; then

    echo "Exittting. Please specify a valid appliance ip address for argument 1".
    exit 1;
fi


. /niagara/test/svt_cmqtt/svt_test.sh 

cmd=`echo "imaserver stat store
imaserver stat
advanced-pd-options memorydetail
advanced-pd-options list
status imaserver
status mqconnectivity; 
status webui
datetime get" | awk '{ printf("echo ============================================================ %s ; %s ; ", $0,$0);}'`


echo "Monitoring cmd is $cmd" > $LOGFILE


if [ "$FLAG" == "" ] ;then # default mode of operation
#---------------------------------------------
# Note: this loop / program runs until killed.
#---------------------------------------------

do_admin_loop ${SERVER} "$cmd" "$LOGFILE"

else
    iteration=0
    while((1)); do
        let iteration=$iteration+1
        echo "`date` starting iteration $iteration"
        $0 $SERVER $LOGFILE_ADDN.${iteration} > /dev/null 2>/dev/null &
        mypid=$!
        echo "`date` started as $mypid"
        start=`date +%s`
        let end=$start+3600
        cur=`date +%s`
        while(($end>$cur)); do
            sleep 60;
            cur=`date +%s`
        done
#        kill -15 $mypid
#        sleep 5
        echo "`date` kill pid $mypid"
        ps -ef | `grep basename $0`  | grep -v $FLAG  | grep -v grep | awk '{print $2}' | xargs -i kill -9 {}

        #kill -9 $mypid

        echo "`date` Gzipping results"
        find log.app* |grep -v .gz$ | xargs -i gzip {}
         
    done

fi
