#! /bin/bash

# This is a script to start the Sync Driver server
# Taken from llmtest2.sh and modified

# Functions
printUsage(){
	echo "Usage: ./sync.sh [start|kill|restart] [logfile]"
	echo "Give the start option to start the SyncDriver server"
	echo "Give the kill option to kill the SyncDriver server"
	echo "Give the restart option to kill the SyncDriver server, then restart it"
	# Print the status
	if [ -f sync.pid ]
	then
		PID=`cat sync.pid`
		if [ `ps -ef | grep -v grep | grep -c " ${PID} "` -gt 0 ]
		then
			echo "Sync Server is running - PID: ${PID}"
		else
			echo "Sync Server is not running"
		fi
	else
		echo "Sync Server is not running"
	fi
		
}

startSync(){
    # Start the Sync Server
    echo "Starting Sync Server"
    j_path=$(java 2>&1 | grep Usage | wc -l)
    if [[ ${j_path} -ne 0 ]]
    then
        java test.llm.TestServer ${SYNCPORT} >> ${LOGFILE} 2>&1 &
        syncServID=$!
    echo "Server ID ${syncServID}"
        sleep 5
        if [ `ps -ef | grep -v grep | grep -c " ${syncServID} "` -gt 0 ]
        then
            echo "Sync Server Started (${syncServID})"
            echo ${syncServID} > sync.pid
        else
            echo "Sync Server Failed to Start"
            exit -1
        fi
    fi
}

killSync(){
    # Kill the Sync Server
    echo "Killing the Sync Server"
    PID=`cat sync.pid`
    if [[ -z ${PID} ]] ; 
    then
        echo "No PID found for Sync Server in \"sync.pid\" file.  Kill of Sync Server did not occur."
        exit -1
    fi
 
    kill -9 ${PID}
    if [ `ps -ef | grep -v grep | grep -c " ${PID} "` -eq 0 ]
    then
        echo "Sync Server is no longer running"
        rm -f sync.pid
    else
        echo "Failed to kill Sync Server"
        exit -1
    fi
}

# Handle the input
if [ $# -ne 2 ]
then
	printUsage
	exit -1
fi

OPTION=$1
LOGFILE=$2

# Source the ISMsetup file to get access to information for the remote machine
if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  source ../scripts/ISMsetup.sh
fi

if [ "${OPTION}" == "start" ]
then
	startSync
elif [ "${OPTION}" == "kill" ]
then
	killSync
elif [ "${OPTION}" == "restart" ]
then
	# Restart the sync server
	killSync
	startSync
else
	echo "Invalid option ${OPTION} given to sync.sh"
	printUsage
fi
