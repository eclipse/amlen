#!/bin/bash
#------------------------------------------------------------------------------
# 
# cycleServerIMM.sh
#
# Description:
#   Power on/off server via server's IMM
#   
#
# 
# Usage:
#   ./cycleServerIMM.sh imm1=<Primary IMM IP> [imm2=<Secondary IMM IP>] <cycle_time=power cycle time seconds> <total_time=total time to cycle in seconds>
#
# Dependencies: ipmi tool must be installed
#
#------------------------------------------------------------------------------
shopt -s extglob #enables pattern lists
IMM_BAD_RC='+(Error: Unable to establish IPMI v2 / RMCP+ session|*Command not supported*)'

function print_help {
    echo "Power off server using server's IMM. This script uses ipmitool and "
    echo "must be installed. If a second IMM ip is given, the HA states of both IMM's" 
    echo " are reversed at the power cycle time delta. However, the IMM states after"
    echo " initial power off will be power on. This behavior is specific to HA testing"
    echo " either for vCenter ESX failover or appliance failover. Read below for "
    echo " more operation details on when specifing a secondary IMM ip"
    echo " " 
    echo " Using a second IMM is specific for HA testing where one server is cycled"
    echo " off according to cycle time. Therefore allowing at least one server"
    echo " to be up and  running all the time"
    echo " " 
    echo " If only primary IMM ip is given (ie no second IMM ip, no cycle or total times), then the"
    echo " script will IMMEDIATELY cycle to next state. This is default (ie 0 seconds"
    echo " for both cycle and total times). In other words if IMM1 "
    echo " is on, then the script will power off immediately. If IMM1 is off, then "
    echo " the script will power on immediately. For 2 IMM setups this will IMMEDATELY"
    echo " cause an HA failover to occur such that state of IMM1 will be switch" 
    echo " off  then switch back on after 120s (see below on second IMM HA behavior)"
    echo " "

    echo "Usage: cycleServerIMM.sh -p <Primary IMM IP> [-s <Secondary IMM IP>] -c <power cycle time seconds> -t <total time to cycle in seconds> -h print help"
    echo "         Primary IMM IP = the IMM IP address of primary server to cycle"
    echo "         Secondary IMM IP = the IMM IP address of secondary server" 
    echo " 			      Using a Secondary IMM is a special case. Such"
    echo "			      that the state of secondary should be opposite"
    echo "			      of primary. Thereby guarantee either primary"
    echo "			      or secondary is up and running at one time"
    echo "			      Also, when powering off one of the IMM's, the"
    echo "			      script will power back on the IMM that was powered off"
    echo "			      to initiate  either a resyncing or allow"
    echo "			      the off server to be online for next failover attempt"
    echo "			      For now, will power back on the off IMM after 120s"
    echo "			      Also, will power off Primary first during first cycle. Therefore,"
    echo "			      ensure Primary is in required state before first"
    echo "			      HA failover attempt"
    echo " "
    echo "	   power cycle time = (optional) time in seconds between power states"
    echo " 			      For Primary and Secondary IMM setups, this"
    echo "			      time is used for delta for cycling states,"
    echo " 			      between servers- Default 0. Minimum is 120s when"
    echo "			      specifing Secondary IMM IP (ie for HA failover"
    echo "			      due to powering back on secondary IMM)"
    echo "	   total time = (optional) total time to cycle. -Default 0." 
    echo "			Minimum is 120s (ie for HA failover due to "
    echo "			powering back on secondary IMM)"
    echo "			when specifiing Secondary IMM IP."
    echo "		 	Total time must be greater than cycle time"
    echo " "
    echo "NOTE: If either power cycle time or total time is 0, then the one "
    echo "      non-zero time is used as a delay to switch states ONCE!! This can be"
    echo "	used if total time control is done outside of script. For 2 IMM setups"
    echo "      this will cause HA failover at time of delta."
    echo " "
    echo "NOTE: script does not assume any initial state of IMM. Before invoking"
    echo "      place IMM server(s) into required initial state"
    echo " "
    echo "NOTE: 'USERID' 'PASSW0RD' are used for ipmitool authenication"
    echo " "
}

function valid_ip() {
#
# $1 = supposedly ip address string
# return 1 if true
# return 0 if false
#
    echo "validing $1 address"
    if  [ `echo $1 | grep '^[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*$'` ]; then
	echo "returning1"
	return 1
    else
	echo "returning0"
        return 0
    fi
	
}
function is_alive() {
#
# lets check upto 10mins (ie 600s) and poll every 30s
# $1 = ip address
#
    echo "testing is_alive:$1"
    timeout=30
    elapsed=0
    ADDRESS=$1
    echo "testing ${ADDRESS} is_alive" 
    while [ $elapsed -lt $timeout ] ; do
        reply=`ping -c 5 ${ADDRESS} |grep "5 packets transmitted"`
	echo "reply:$reply"
        if [[ "$reply" == *" 0% packet loss"* ]] ; then
            echo " $reply"
	    echo "server looks alive"
            return 0
        fi
	echo "sleeping for 30s before next ping"
        sleep 30
        elapsed=`expr $elapsed + 30`
    done
    
    return 1

}


function send_ipmi {
#
# set timeout to 5mins
# $1 = imm ip address to send command
# $2 = command string to send IMM
#

    echo " sending IMM $1 command:"$2"" 
    reply=`ipmitool -N 1 -I lanplus -H $1 -U USERID -P PASSW0RD $2  2>&1`
    send_reply=""

    echo "reply:$reply"
    echo "patter:$IMM_BAD_RC"
    case $reply in 
        $IMM_BAD_RC )  
             echo "error returned from ipmi:'$reply'"
             return 1
 	     ;;
        *)  echo "Good return:'$reply'"
	     send_reply=$reply
	     return 0
	     ;;
        esac

    return 1
}

function sleep_send_ipmi {
#
# same as send_ipmi above but sleep for 120seconds. This function
# is ment to run in background so not to interfere with regular processing
#
# $1 = imm ip address to send command
# $2 = command string to send IMM

#
    declare rc=1

    echo "sleep_send_ipmi:sleeping for 120s"
    sleep 120
    echo "sleep_send_ipmi: sending IMM $1 command:"$2"" 
    reply=`ipmitool -N 1 -I lanplus -H $1 -U USERID -P PASSW0RD $2  2>&1`
    send_reply=""


    echo "sleep_send_ipmi:reply:$reply"
    echo "patter:$IMM_BAD_RC"
    case $reply in 
        $IMM_BAD_RC )  
             echo "sleep_send_ipmi:error returned from ipmi:'$reply'"
             rc=1
 	     ;;
        *)  echo "sleep_send_ipmi:Good return:'$reply'"
	     send_reply=$reply
	     rc=0
	     ;;
        esac

    if [[ $rc == 0 && "$send_reply" =~ "${CHASSIS_RETURN_POWER_ON}" ]]; then
	echo "sleep_send_ipmi:successful sending $1 $2 command"
        exit 0
    else
   	echo "sleep_send_ipmi:Either cannot connect to $1 or power state is unknown"
	exit 1
    fi

    return 1
}

CHASSIS_RETURN_STATUS_ON="Chassis Power is on"
CHASSIS_RETURN_STATUS_OFF="Chassis Power is off"
CHASSIS_RETURN_POWER_OFF="Chassis Power Control: Down/Off"
CHASSIS_RETURN_POWER_ON="Chassis Power Control: Up/On"
CHASSIS_POWER_STATUS="chassis power status"
CHASSIS_POWER_ON="power on"
CHASSIS_POWER_OFF="power off"

#
# initialize options and vars to default
#
declare IMM1=""
declare IMM2=""
declare IMM1_STATE=""
declare IMM2_STATE=""
declare CYCLE_TIME=0
declare TOTAL_TIME=0
declare HA=0
declare isIP=0
#
# initialize vars
#
declare reply=""
declare valid_option=0

#----------------------------------------------------------------------------
# Parse Options
#----------------------------------------------------------------------------
while getopts "p:s:c:t:h" option ${OPTIONS}; do
  valid_option=1
  case ${option} in
  p )
      IMM1="${OPTARG}"
      valid_ip $IMM1
      if [ $? == 0 ]; then
	  echo "${IMM1} is not a valid ip"
	  print_help
	  exit 1
      fi
      ;;
  s ) 
      IMM2="${OPTARG}"
      valid_ip $IMM2
      if [ $? == 0 ]; then
	  echo "${IMM2} is not a valid ip"
	  print_help
	  exit 1
      fi
      ;;
  c )
      CYCLE_TIME=${OPTARG}
      ;;
  t )
      TOTAL_TIME=${OPTARG}
      ;;
  h )
      print_help
      exit 1
      ;;
  \? )
      print_help
      exit 1
      ;;
  esac
done

if [ $valid_option -eq 0 ]; then
	print_help
	exit 1
fi

#
# check for valid times
#
if [ $CYCLE_TIME -ne 0 -a $TOTAL_TIME -ne 0 ]; then
	if [ $CYCLE_TIME -gt $TOTAL_TIME ]; then
		echo "total time must be greater than cycle time"
		exit 1
	fi
fi

#
# Gather initial state of machine(s)
#
d_start=`date +%s`

#
# check if IMM1 address is alive
#

is_alive "${IMM1}"
if [[ $? == 1 ]]; then
    echo "FAILED:timeout occured retriving status from server"
    exit 1
fi
if [[ -n "${IMM2}" ]]; then
    is_alive "${IMM2}"
    if [[ $? == 1 ]]; then
    	echo "FAILED:timeout occured retriving status from server"
    	exit 1
    fi
fi
    
#
# check initial IMM1 state
#
echo "check initial IMM1 state:$IMM1"
send_ipmi "${IMM1}" "${CHASSIS_POWER_STATUS}"
if [[ $? == 0 && "$send_reply" =~ "${CHASSIS_RETURN_STATUS_ON}" || "$send_reply" =~ "${CHASSIS_RETURN_STATUS_OFF}" ]]; then
    IMM1_STATE="$send_reply"
else
    echo "Either cannother connect to ${IMM1} or power state is unknown"
    exit 1

fi

#
# check IMM2 state 
#
echo "check initial IMM2 state:$IMM2"
if [ -n "${IMM2}" ]; then
	is_alive "${IMM2}"
	if [[ $? == 1 ]]; then
    		echo "FAILED:timeout occured retriving status from server"
    		exit 1
	fi
	send_ipmi "${IMM2}" "${CHASSIS_POWER_STATUS}"
	if [[ $? == 0 && "$send_reply" =~ "${CHASSIS_RETURN_STATUS_ON}" || "$send_reply" =~ "${CHASSIS_RETURN_STATUS_OFF}"  ]]; then
    		IMM2_STATE="$send_reply"
	else
    		echo "Either cannot connect to ${IMM2} or power state is unknown"
    		exit 1
	fi
	
	if [ $CYCLE_TIME -ne 0 -a $CYCLE_TIME -lt 120 ]; then
		echo "For HA, cycle time must be equal or greater than 120 seconds"
		exit 1
	elif [ $TOTAL_TIME -ne 0 -a $TOTAL_TIME -lt 120 ]; then
		echo "For HA, total time must be equal or greater than 120 seconds"
	fi
	#
	# IMM2 is specified. Enable HA algo
	#
	HA=1
fi

echo "IMM1 state:$IMM1_STATE"
echo "IMM2 state:$IMM2_STATE"


#
# Loop total time
#

declare total_elapsed=0
declare cycle_elapsed=0
declare total_loops=0
declare sleep_time=0
declare NEXT_PRIMARY=${IMM1}

if (( !(TOTAL_TIME) && !(CYCLE_TIME) )); then
	echo "Total and cyle times are zero. Performing immediate power switch"
	total_loops=1
elif (( !(TOTAL_TIME) && CYCLE_TIME || TOTAL_TIME && !(CYCLE_TIME) )); then
	sleep_time=`expr $CYCLE_TIME + $TOTAL_TIME`	
	echo "Will sleep for $sleep_time then perform switch"
else
	
	total_loops=`expr $TOTAL_TIME / $CYCLE_TIME`
	sleep_time=$CYCLE_TIME
fi

echo "cycle sleep_time:$sleep_time"
#
# Loop till total loops is reached
# 
if (( HA ));then
	echo " "
	echo " "
	echo "performing HA failover algo. Power off will cycle between both"
	echo "IMM's. After power off the same IMM will be powered back on n 120secs"
	echo " "
else
	echo "Cycling single IMM1"
fi

d_start=`date +%s`
cycle_elapsed=1

echo "Looping for $total_loops loops"
while [ $cycle_elapsed -le $total_loops ]; do 
	echo "sleeping till cycle time:$CYCLE_TIME seconds"
	echo "loop:$cycle_elapsed"

       	sleep $sleep_time 





	#
	# woke up. time to switch states
	#

	case $HA in

	0 ) 
		if [[ "${IMM1_STATE}" == "${CHASSIS_RETURN_STATUS_ON}" ]]; then
			send_ipmi "${IMM1}" "${CHASSIS_POWER_OFF}"
			if [[ $? == 0 && "$send_reply" =~ "${CHASSIS_RETURN_POWER_OFF}" ]]; then
    				IMM1_STATE="${CHASSIS_RETURN_STATUS_OFF}"
				echo "imm1_state:$IMM1_STATE"
			else
    				echo "Either cannot connect to ${IMM1} or power state is unknown"
				exit 1
			fi
			echo "rc:$? send_reply:$send_reply"
		elif [[ "${IMM1_STATE}" == "${CHASSIS_RETURN_STATUS_OFF}" ]]; then
			send_ipmi "${IMM1}" "${CHASSIS_POWER_ON}"
			if [[ $? == 0 && "$send_reply" =~ "${CHASSIS_RETURN_POWER_ON}" ]]; then
        			IMM1_STATE="${CHASSIS_RETURN_STATUS_ON}"
				echo "imm1_state:$IMM1_STATE"
			else
        			echo "Either cannot connect to ${IMM1} or power state is unknown"
				exit 1
			fi
			echo "rc:$? send_reply:$send_reply"
		else
    			echo "Either cannot connect to ${IMM1} or power state is unknown"
    			exit 1
		fi
		;;
	1 ) 
		#
		# before continuing, get lastest chassis status since we backgrouned
		# the power on of the powered off imm
		#	
		send_ipmi "${IMM1}" "${CHASSIS_POWER_STATUS}"
		if [[ $? == 0 && "$send_reply" =~ "${CHASSIS_RETURN_STATUS_ON}" || "$send_reply" =~ "${CHASSIS_RETURN_STATUS_OFF}"  ]]; then
    			IMM1_STATE="$send_reply"
		else
    			echo "Either cannot connect to ${IMM1} or power state is unknown"
    			exit 1
		fi

		send_ipmi "${IMM2}" "${CHASSIS_POWER_STATUS}"
		if [[ $? == 0 && "$send_reply" =~ "${CHASSIS_RETURN_STATUS_ON}" || "$send_reply" =~ "${CHASSIS_RETURN_STATUS_OFF}"  ]]; then
    			IMM2_STATE="$send_reply"
		else
    			echo "Either cannot connect to ${IMM2} or power state is unknown"
    			exit 1
		fi

		#
		# For HA both IMM's should be in power on state. We just need
		# to track which one we powered off last and power off the other
		# next time
		#
	 	if [[ "${IMM1}" == "${NEXT_PRIMARY}" ]]; then
			#
			# if IMM1 state is off for some reason then no need to 
			# power off
			#
			if [[ "${IMM1_STATE}" =~ "${CHASSIS_RETURN_STATUS_OFF}" ]]; then
				send_reply="${CHASSIS_RETURN_POWER_OFF}"
			else
				send_ipmi "${IMM1}" "${CHASSIS_POWER_OFF}"
			fi

			if [[ $? == 0 && "$send_reply" =~ "${CHASSIS_RETURN_POWER_OFF}" ]]; then
    				IMM1_STATE="${CHASSIS_RETURN_STATUS_OFF}"
				echo "imm1_state:$IMM1_STATE"
				#
				# backgroun function sleep 120s and send power on
				#
				sleep_send_ipmi "${IMM1}" "${CHASSIS_POWER_ON}" &
    				IMM1_STATE="${CHASSIS_RETURN_STATUS_ON}"
				NEXT_PRIMARY="$IMM2"
				echo "imm1_state:$IMM1_STATE"
				
			else
    				echo "Either cannot connect to $IMM1 or power state is unknown"
				exit 1
			fi
		elif [[ "${IMM2}" == "${NEXT_PRIMARY}" ]]; then
			#
			# if IMM2 state is off for some reason then no need to 
			# power off
			#
			if [[ "${IMM2_STATE}" == "${CHASSIS_RETURN_STATUS_OFF}" ]]; then
				send_reply="${CHASSIS_RETURN_POWER_OFF}"
			else
				send_ipmi "${IMM2}" "${CHASSIS_POWER_OFF}"
			fi

			if [[ $? == 0 && "$send_reply" =~ "${CHASSIS_RETURN_POWER_OFF}" ]]; then
        			IMM2_STATE="${CHASSIS_RETURN_STATUS_OFF}"
				echo "imm2_state:$IMM2_STATE"
				#
				# background function sleep 120s and send power on
				#
				sleep_send_ipmi "${IMM2}" "${CHASSIS_POWER_ON}" &
    				IMM2_STATE="${CHASSIS_RETURN_STATUS_ON}"
				NEXT_PRIMARY="$IMM1"
				echo "imm1_state:$IMM2_STATE"
			else
        			echo "Either cannot connect to $IMM2 or power state is unknown"
				exit 1

			fi
		else
    			echo "Either cannot connect to $IMM2 or power state is unknown"
    			exit 1
		fi
		;;
		 
	esac

       	cycle_elapsed=`expr $cycle_elapsed + 1`
done

d_done=`date +%s`

let d_total=${d_done}-${d_start}
echo "SUCCESS: Total time total seconds:$d_total"
