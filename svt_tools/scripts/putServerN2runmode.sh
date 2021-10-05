#!/bin/bash
#------------------------------------------------------------------------------
# 
# putServerMaintenance.sh
#
# Description:
#   Put an appliance server into maintenance mode
#   
#
# 
# Usage:
#   ./putServerN2runmode.sh <user@server> <MAINTENANCE|PRODUCTION>
#
# Dependencies: none
#
#------------------------------------------------------------------------------

function print_help {
    echo "Command to put a server into a running maintenance mode"
    echo " "
    echo "Usage: putServerN2runmode.sh <user@server> <MAINTENANCE|PRODUCTION>"
    echo "         user@server = the userid and server ip to put into maintence or running production"
    echo "         PRODUCTION|MAINTENANCE|CLEAN_STORE = requested server mode "
}

function verify_status {
    timeout=300
    elapsed=0
 
    while [ $elapsed -lt $timeout ] ; do
        echo -n "*"
        sleep 1
        cmd="ssh ${SERVER} status imaserver"
        reply=`$cmd`

        case $# in
            1) if [[ "$reply" =~ "$1" ]] ; then
                   echo " status '$reply' reached"
                   return 0
        	fi
 		;;
            2) if [[ "$reply" =~ "$1" || "$reply" =~ "$2" ]] ; then
                   echo " status '$reply' rearched"
                   return 0
        	fi
		;;
            3) if [[ "$reply" =~ "$1" || "$reply" =~ "$2" || "$reply" =~ "$3" ]] ; then
                   echo " status '$reply' reached"
                   return 0
        	fi
		;;
	    *)  echo "Unknown verification"
		return 1
		;;
	
        esac

        elapsed=`expr $elapsed + 1`
    done

    #
    # we timedout on status. we should fail
    #
    case $# in
        1) echo "Timed out on verifying status:'$1'"
           ;;
        2) echo "Timed out on verifying status:'$1' '$2'"
           ;;
        3) echo "Timed out on verifying status:'$1' '$2' '$3'"
           ;;
    esac

    cmd="ssh ${SERVER} status imaserver"
    reply=`$cmd`
    echo "Status: ${reply}"
    return 1
}

MAINTENANCE='Status = Running (maintenance)'
STOPPED='Status = Stopped'

#
# Below are valid production for standalone and HA
#
STANDBY='Status = Standby'
RUNNING='Status = Running (production)'
STORESTARTING='Status = StoreStarting'

if [[ $# == 2 ]]; then
    SERVER=$1
    if [[ "$2" != "MAINTENANCE" && "$2" != "PRODUCTION" && "$2" != "CLEAN_STORE" ]]; then
	print_help
	exit 1
    else
	REQMODE=$2
    fi
else
    print_help
    exit 1
fi

cmd="ssh ${SERVER} status imaserver"
reply=`$cmd`

#
# if stopped need to start to figure out status
#
if [[ "$reply" =~ "${STOPPED}" ]]; then
    ssh ${SERVER} imaserver start
    verify_status "${MAINTENANCE}" "${STANDBY}" "${RUNNING}"
fi

cmd="ssh ${SERVER} status imaserver"
reply=`$cmd`

if [[ "$reply" =~ "${MAINTENANCE}" ]]; then
    if [[ "$REQMODE" == "MAINTENANCE" ]]; then
	echo "Already running in MAINTENANCE"
    	echo "SUCCESS"
    elif [[ "$REQMODE" == "PRODUCTION" ]]; then
	# lets move to production
	echo "moving server from maintenance to production"
    	d_start=`date +%s`
    	ssh ${SERVER} imaserver runmode production
    	ssh ${SERVER} imaserver stop
    	verify_status "${STOPPED}"
    	ssh ${SERVER} imaserver start
	
	# verify we made it to production mode
    	verify_status "${STANDBY}" "${RUNNING}" "${STORESTARTING}"
    	d_done=`date +%s`
	
     	let d_total=${d_done}-${d_start}
    	echo "SUCCESS Total Time: ${d_total} seconds"
    	exit 0
    elif [[ "$REQMODE" == "CLEAN_STORE" ]]; then
	#
	# We are in maintenance mode. So we just need to perform
	# a clean store
	echo "Moving server from maintenance to clean_store"
	echo "After clean_store server will be back into maintenance mode"
    	d_start=`date +%s`
    	ssh ${SERVER} imaserver runmode clean_store
    	ssh ${SERVER} imaserver stop
    	verify_status "${STOPPED}"
    	ssh ${SERVER} imaserver start

	# verify we made it back to maintenance mode
    	verify_status "${MAINTENANCE}"
    	d_done=`date +%s`
	
     	let d_total=${d_done}-${d_start}
    	echo "SUCCESS Total Time: ${d_total} seconds"
    	exit 0

    fi
elif [[ "$reply" =~ "${RUNNING}" || "$reply" =~ "${STANDBY}" || "$reply" =~ "${STORESTARTING}" ]]; then
    if [[ "$REQMODE" == "PRODUCTION" ]]; then
	echo "server running already as production or standby"
    	echo "SUCCESS"
    elif [[ "$REQMODE" == "MAINTENANCE" ]]; then
	# lets move to maintenance
    	echo "Moving from production or standby to maintenance"
    	d_start=`date +%s`
    	ssh ${SERVER} imaserver runmode maintenance
    	ssh ${SERVER} imaserver stop
    	verify_status "${STOPPED}"
    	ssh ${SERVER} imaserver start
    	d_done=`date +%s`

    	let d_total=${d_done}-${d_start}
    	echo "SUCCESS Total Time: ${d_total} seconds"
    	exit 0
    elif [[ "$REQMODE" == "CLEAN_STORE" ]]; then
	# clean_store requires maintenance mode first
	echo "Moving server from production to clean_store"
	echo "After clean_store server will be back into maintenance mode"
    	d_main_start=`date +%s`
    	ssh ${SERVER} imaserver runmode maintenance
    	ssh ${SERVER} imaserver stop
    	verify_status "${STOPPED}"
    	ssh ${SERVER} imaserver start
    	d_main_done=`date +%s`

	# verify we made it to maintenance mode
    	verify_status "${MAINTENANCE}"
	echo "Now in maintenance. Peforming clean_store"
    	d_clean_start=`date +%s`
    	ssh ${SERVER} imaserver runmode clean_store
    	ssh ${SERVER} imaserver stop
    	verify_status "${STOPPED}"
    	d_clean_done=`date +%s`
    	ssh ${SERVER} imaserver start

	# verify we made it back to maintenance mode
    	verify_status "${MAINTENANCE}"
    	d_done=`date +%s`

	let d_clean_total=${d_clean_done}-${d_clean_start}
	let d_main_total=${d_main_done}-${d_main_start}	
    	let d_total=${d_done}-${d_main_start}
	echo "Maintenance delta: ${d_main_total}"
	echo "Clean_store delta: ${d_clean_total}"
    	echo "SUCCESS Total Time: ${d_total} seconds"
    fi
else
    echo "Server in unknown status"
    exit 1
fi
    
    
