#!/bin/bash
#------------------------------------------------------------------------------
# 
# clean_store.sh
#
# Description:
#   This script follows the documented procedure for cleaning the store on
#   the MessageSight appliance.
#   It also has the option to first corrupt the store by going into busybox
#   and replacing the shared memory file. This option should only be used
#   when running in a KVM.
# 
# Usage:
#   ./clean_store.sh [corrupt|clean]
#
#------------------------------------------------------------------------------

function verify_status_docker {
    timeout=30
    elapsed=0
    cmd="curl -X GET -f -s -S http://A1_HOST:A1_PORT/ima/v1/service/status"
    echo $cmd
    while [ $elapsed -lt $timeout ] ; do
        echo -n "*"
        sleep 1
        reply=`$cmd`
        RC=$?
        if [ $RC -eq 0 ] ; then
            StateDescription=`echo $reply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Server\"][\"StateDescription\"]"`
            if [[ "$StateDescription" =~ "$1" ]] ; then
                echo " $reply"
                echo " StateDescription=$StateDescription"	
                echo " Test result is Success!"			
                return
            fi
        fi
        elapsed=`expr $elapsed + 1`
    done

    cmd="curl -X GET -f -s -S http://A1_HOST:A1_PORT/ima/v1/service/status"
    reply=`$cmd`
    echo "Status: ${reply}"
}

function verify_status {
    timeout=30
    elapsed=0
    while [ $elapsed -lt $timeout ] ; do
        echo -n "*"
        sleep 1
        cmd="ssh ${SERVER} status imaserver"
        reply=`$cmd`
        if [[ "$reply" =~ "$1" ]] ; then
            echo " $reply"
            return
        fi
        elapsed=`expr $elapsed + 1`
    done

    cmd="ssh ${SERVER} status imaserver"
    reply=`$cmd`
    echo "Status: ${reply}"
}

function corrupt_store {
    echo 'cp /lib64/libismadmin.so /dev/shm/com.ibm.ism\:\:0\:store' | ssh ${SERVER} busybox
    ssh ${SERVER} imaserver stop force
    verify_status "${STOPPED}"
    ssh ${SERVER} imaserver start
    verify_status "${MAINTENANCE}"
}

if [[ !( "$1" == "clean" || "$1" == "corrupt" ) ]]; then
    echo "Usage: clean_store.sh <MODE>"
    echo "mode:    clean|corrupt"
    echo "         clean = run clean_store with a non-corrupted store"
    echo "         corrupt = run clean_store with a corrupted store"
    exit 1
fi

if [[ "A1_TYPE" == "DOCKER" ]] || [[ "A1_TYPE" == "RPM" ]] ; then
	# TODO: add in corrupt store piece?
	cmd="curl -X POST -f -s -S -d {\"Service\":\"Server\",\"CleanStore\":true} http://A1_HOST:A1_PORT/ima/v1/service/restart"
        echo $cmd
        reply=`$cmd`
        RC=$?
        echo $reply
        echo "RC=$RC"
	# give the server a chance to stop before checking status
	sleep 5
	verify_status_docker "Running (production)"
else
	SERVER=A1_USER@A1_HOST
	versioncmd=`echo "show imaserver" | ssh ${SERVER}`
	if [[ "${versioncmd}" == "" || "${versioncmd}" =~ "version is 1.0" ]] ; then
	    # Old status messages 1.0.x.x
	    MAINTENANCE='Status = Maintenance'
	    RUNNING='Status = Running'
	    STOPPED='Status = Stopped'
	else
	    # New status messages version 1.1+
	    MAINTENANCE='Status = Running (maintenance)'
	    RUNNING='Status = Running (production)'
	    STOPPED='Status = Stopped'
	fi
	
	
	if [[ "$1" = "corrupt" ]]; then
	    echo "Corrupting the store . . ."
	    corrupt_store
	fi
	
	d_start=`date +%s`
	ssh ${SERVER} imaserver runmode maintenance
	ssh ${SERVER} imaserver stop
	verify_status "${STOPPED}"
	ssh ${SERVER} imaserver start
	d_mset=`date +%s`
	verify_status "${MAINTENANCE}"
	ssh ${SERVER} imaserver runmode clean_store 
	d_cs_set=`date +%s`
	ssh ${SERVER} imaserver stop
	verify_status "${STOPPED}"
	d_stop1=`date +%s`
	ssh ${SERVER} imaserver start
	d_start1=`date +%s`
	verify_status "${MAINTENANCE}"
	d_mcheck=`date +%s`
	ssh ${SERVER} imaserver runmode production 
	d_prod_set=`date +%s`
	ssh ${SERVER} imaserver stop
	verify_status "${STOPPED}"
	d_stop2=`date +%s`
	ssh ${SERVER} imaserver start
	d_start2=`date +%s`
	verify_status "${RUNNING}"
	d_rcheck=`date +%s`
	
	let d_total=${d_rcheck}-${d_start}
	echo "Total Time: ${d_total} seconds"
	echo "Begin Timestamp: ${d_start}"
	echo "After setting the maintenance flag: ${d_mset}"
	echo "After setting the clean_store flag: ${d_cs_set}"
	echo "After the first imaserver stop: ${d_stop1}"
	echo "After the first imaserver start call: ${d_start1}"
	echo "After confirming that status is maintenance: ${d_mcheck}"
	echo "After setting the production flag: ${d_prod_set}"
	echo "After the second imaserver stop: ${d_stop2}"
	echo "After the second imaserver start call: ${d_start2}"
	echo "After confirming that status is running: ${d_rcheck}"
fi	
