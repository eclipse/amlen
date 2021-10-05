#!/bin/bash

#-------------------------------------------
# The purpose of this script is to automatically
# configure Websphere MQ for SVT automotive test
# scenario. This script also documents implicitly
# the steps that are needed to configure Websphere
# MQ for that test scenario.
#
# Prerequisites:
# 1) This script requires that the rpms for Websphere 
# MQ have already been installed on the
# target system. If the rpms have not been installed
# an error will be encountered. 
# 2) This script must be executed by a user id with 
# the appropriate Websphere MQ permissions, for example,
# the mquser id would be sufficient.
#
# Usage Steps:
# 1. ssh to the target system where MQ is installed.
# 2. ./configureMQ.sh # run to configure MQ for SVT
#     automotive test schenario.
#
# Further details available in RTC Task 12906, 12902
#-------------------------------------------

export PATH=$PATH:/opt/mqm/bin

SVT_QUEUE_MANAGER=SVTBRIDGE.QUEUE.MANAGER

declare -a const_command_array=(
0 "shutdown queue manager" "endmqm -i $SVT_QUEUE_MANAGER"
0 "delete queue manager" "dltmqm  $SVT_QUEUE_MANAGER"
0 "create queue manager" "crtmqm -q $SVT_QUEUE_MANAGER"
0 "start queue manager" "strmqm"
0 "configure mq" "echo \"* define queues
* define listener
define listener (listener1) trptype (tcp) control (qmgr) port (1414)
* start listener
start listener (listener1)
* define channels
define channel (SVTBRIDGE.CHANNEL) chltype (SVRCONN) trptype (tcp) mcauser('mqm') hbint(60)
* disable channel authentication
ALTER QMGR CHLAUTH (DISABLED) 
DEFINE QL(SYSTEM.ISM.SYNCQ)
DEFINE QL(LOOP_MQQ1)
DEFINE QL(LOOP_MQQ10)
DEFINE QL(LOOP_MQQ5)
\" | runmqsc"
NULL NULL NULL
NULL NULL NULL
NULL NULL NULL
);


let const_command_array_width=3;

let x=0;
let errcount=0;
while [[ "${const_command_array[$x]}" != "NULL" ]] ; do
	echo -n "command is ${const_command_array[$x+2]} : "
	echo "${const_command_array[$x+2]}" | sh
	rc=$?
	if [ "$rc" != "${const_command_array[$x]}" ] ; then
		if ! echo "${const_command_array[$x+1]}" |grep -E '(shutdown queue manager|delete queue manager)' > /dev/null ; then
			echo ""
			echo "`date` ERROR: exit code $rc did not match expecation ${const_command_array[$x]}"
			echo ""
			let errcount=$errcount+1;
		else
			echo ""
			echo "`date` WARNING: exit code $rc did not match expecation ${const_command_array[$x]}"
			echo ""
		fi
	else
		echo ""
		echo "`date` SUCCESS: exit code $rc"
		echo ""
	fi
	if echo "${const_command_array[$x+1]}" |grep -E 'shutdown queue manager' > /dev/null ;then
		while((1)); do 
			data=`ps -ef |grep SVTBRIDGE.QUEUE.MANAGER |grep -v grep`; 
			if [ -n "$data" ] ;then
				echo "$data" | awk '{print $2}' |xargs -i kill -9 {} ;
			else 
				break; 
			fi;
		done
	fi

	let x=$x+$const_command_array_width
done


if [ "$errcount" == "0" ]; then
	echo "---------------------------------------------------------"
	echo "Successfully configured MQ for SVT Automotive Scenario v1"
	echo "---------------------------------------------------------"
	exit 0;
else
	echo "---------------------------------------------------------"
	echo "$errcount Failures while configuring MQ for SVT Automotive Scenario v1"
	echo "---------------------------------------------------------"
	exit 1;
fi

exit 0;

