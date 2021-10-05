#!/bin/bash

#--------------------------------------------------------
# af.monitor.appliance.sh : monitor the appliance status while automation is running.
# 
# Changelog:
# 
#--------------------------------------------------------

#--------------------------------------------------------
# Inputs
#--------------------------------------------------------
SERVER=${1-""}
LOGFILE_PREFIX=${2-""}
LOGFILE="${LOGFILE_PREFIX}.${SERVER}"
FLAG=${3-""}
PERIOD=${4-3600} # default collects data in 1 hour blocks before stopping to gzip results.

if ! [ -n "$SERVER" ] ; then

    echo "Exittting. Please specify a valid appliance ip address for argument 1".
    exit 1;
fi

if [ -e /niagara/test/scripts/af_test.sh ] ;then
    . /niagara/test/scripts/af_test.sh 
else
    echo "ERROR: Missing source file /niagara/test/scripts/af_test.sh !!! "
    exit 1;
fi


cmd=`echo "imaserver stat Store
imaserver stat Server
imaserver stat Topic
imaserver stat Subscription StatType=PublishedMsgsHighest
imaserver stat Subscription StatType=PublishedMsgsLowest
imaserver stat Subscription StatType=BufferedMsgsHighest
imaserver stat Subscription StatType=BufferedMsgsLowest
imaserver stat Subscription StatType=BufferedPercentHighest
imaserver stat Subscription StatType=BufferedPercentLowest
imaserver stat Subscription StatType=BufferedHWMPercentHighest
imaserver stat Subscription StatType=BufferedHWMPercentLowest
imaserver stat Subscription StatType=RejectedMsgsHighest
imaserver stat Subscription StatType=RejectedMsgsLowest
advanced-pd-options memorydetail
advanced-pd-options list
status imaserver
status mqconnectivity;
status webui
datetime get" | awk '{ printf("echo ============================================================ %s ; %s ; ", $0,$0);}'`


if [ "$FLAG" == "" ] ;then # default mode of operation
#---------------------------------------------
# Note: this loop / program runs until killed.
#---------------------------------------------

echo "Monitoring cmd is $cmd" > $LOGFILE
af_admin_loop ${SERVER} "$cmd" "$LOGFILE"

else
#--------------------------------------------------------
# Continuous operation, gzips results periodically and restarts the process
#--------------------------------------------------------
    iteration=0
    while((1)); do
        let iteration=$iteration+1
        echo "`date` starting iteration $iteration"
        $0 $SERVER ${LOGFILE_PREFIX}.${iteration} > /dev/null 2>/dev/null &
        mypid=$!
        echo "`date` started as $mypid"
        start=`date +%s`
        let end=$start+${PERIOD}
        cur=`date +%s`
        while(($end>$cur)); do
            sleep 60;
            cur=`date +%s`
        done
        echo "`date` kill pids "
        ps -ef | grep `basename $0`  | grep -v $FLAG  | grep -v grep | grep ${LOGFILE_PREFIX}.${iteration} | grep $SERVER| awk '{print $2}' | xargs -i kill -9 {}
        echo "`date` Gzipping results for ${LOGFILE_PREFIX}.${iteration}.${SERVER} "
        gzip ${LOGFILE_PREFIX}.${iteration}.${SERVER} # must match up to LOGFILE set at top when $0 is called for each iteration
    done

fi
