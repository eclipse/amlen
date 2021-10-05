#! /bin/bash 
#

##################################
# setup_monitor_pids() 
##################################
function setup_monitor_pids() {

   currentdir=`basename $PWD`
   fullpath=$PWD
   declare -g pidarray=""
   declare -g pidfilearray=""
   declare -g machinearray=""
   declare -i pid_id=0


   # FORMAT == M1;HA_JMS_RA_WAS-execution_01-cAppDriverRC-manualrunHAQueue-M1-1.log ! machine;filename
   # SemiColons and Exclamation Marks
   # parse pidlogfile string into pidfilearray:  http://www.cyberciti.biz/faq/bash-for-loop-array/
   # or http://stackoverflow.com/questions/10586153/bash-split-string-into-array
   IFS='#' read -a pidfilearray  <<< "$pidlogfiles"

   ## loop on each file in the pidfilearray
   for pid in "${pidfilearray[@]}"
   do
   #Build some paths for filename  with $M#
      echo "PID[$pid_id]= ${pid}"
      machine="`echo  ${pid} | cut -d ';' -f 1 `"
      filename="`echo ${pid} | cut -d ';' -f 2 `"
      this_user=$(eval echo \$${machine}_USER)
      this_host=$(eval echo \$${machine}_HOST)
   
      this_testroot=$(eval echo \$${machine}_TESTROOT)
      this_command="cat ${this_testroot}/${currentdir}/$filename"
      this_file=`ssh ${this_user}@${this_host} "${this_command}" `
      if [[ "${this_file}" == "" ]];then
         echo ""
         echo "ERROR BUILDING MONITORING PIDs Array:  ${this_command}, Possible No Such File?"
         echo ""
      fi

      while read line
      do
         if [[ "${line}" == "The process id is reported as:"* ]]; then
            thisPid=`echo ${line} | cut -d : -f 2 `
            machinearray[$pid_id]="${this_user}@${this_host}"
            pidarray[$pid_id]=$thisPid
         fi

      done <<< "$this_file"
      ((pid_id+=1))

   done
   ((pid_id-=1))

   echo "PIDARRARY: ${pidarray[@]}"
   echo "PIDARRARY Elements: ${#pidarray[*]}"
   echo "MACHINEARRARY: ${machinearray[@]}"
   echo "MACHINEARRARY Elements: ${#machinearray[*]}"

}

#############################
# monitor_pids() 
# THIS should be called multiple times - maybe with each HA failover 
# Cycle thru pidarray and verify pids are still active.
#
#  Assumes the following variables have been defined and initialized
#   monitorPids	- 1 means PIDs are to be monitored for automation runs so know when to stop failover script
#   pidarray[]	- Array of PIDs read from log file
#   machinearray[]	- Array of ssh 'UID@IP' where the PIDs are running, used to grep PIDs
#
# Returns:
#  0 = Some PIDs are still active (or not tracking PIDs)
#  1 = All tracked PIDs have completed, ready to exit
#
#############################
function monitor_pids() {

dead_pid=0
rc=0
if [ ${monitorPids} == "1" ]; then

   for index in ${!pidarray[*]}
   do
      this_command="ps -ef | grep ${pidarray[index]} | grep -v grep"
      ps_running=`ssh ${machinearray[index]}  " ${this_command} " `
      # check results of PID check
      if [ "${ps_running}" == "" ]; then
         ((dead_pid+=1))
      fi
   done

   if [ ${dead_pid} == ${#pidarray[*]} ]; then
      echo ""
      echo "Done Monitoring PIDS, all appear to be completed!"
      echo ""
      rc=1
   fi
fi
return $rc
}

#############################
#
#
#############################
function ping-imaserver {

  echo "   Pinging ${IMASERVER} until it responds (takes up to 10 mins)..."
  # Some how appliance can ping successfully when it is being restarted
#
  sleep 10
  while [ 1 ]
  do
    # If IP Pings successfully, break out of While Loop
    ping -c1 $IMASERVER &> /dev/null && break
    sleep 10
  done

}
#############################
# Returns:
#   0-Primary/Standby identified
#   1-UNSYNC, Maintenance (Unusable, Unrecoverable state)
#   2-Stopped
#   9-UNKNOWN Status
#############################
function status-imaserver {
#set -x
  echo "   Status ${IMASERVER} until it Syncs..."
  unsync=0              # How many times the status was unsynced     
  stop_count=0          # How many times the status was stopped 
  stop_error=2		# MAX times the status can be stopped before aborting
  unsync_error=28       # MAX times the status can be unsynced before aborting
  rc=0
  while [ 1 ]
  do
    HAROLE=`ssh -nx admin@${IMASERVER} "status imaserver" | grep "HARole = "`
    STATUS=`ssh -nx admin@${IMASERVER} "status imaserver" | grep "Status = "`

    if [[ "${STATUS}" == *"Maintenance"* ]]; then
        UNSYNC=`echo ${IMASERVER}`
        echo "IMAServer: ${IMASERVER} HARole is: '"${HAROLE}"' and Status is: '"${STATUS}"' which is unexpected, aborting."
        rc=1
        break      
    elif [[ "${STATUS}" == *"Stopped" ]]; then
      STANDBY=`echo ${IMASERVER}`
      echo 'IMAServer: '${STANDBY}'  is  '${STATUS}
      stop_count=$(( ${stop_count}+1 ))
      if [ ${stop_count} -eq ${stop_error} ]; then
        rc=2
        break
      fi
    elif [[ "${HAROLE}" == *"PRIMARY" &&  "${STATUS}" == *"Running"* ]]; then
      PRIMARY=`echo ${IMASERVER}`
      echo 'IMAServer: '${PRIMARY}' '${HAROLE}' and '${STATUS}
      break
    elif  [[ "${HAROLE}" == *"STANDBY" &&  "${STATUS}" == *"Standby" ]]; then
      STANDBY=`echo ${IMASERVER}`
      echo 'IMAServer: '${STANDBY}' '${HAROLE}' and '${STATUS}
      break
    elif  [[ "${HAROLE}" == *"UNSYNC"* ]]; then
      unsync=$(( ${unsync}+1 ))
      echo '   IMAServer: '${IMASERVER}'  '${HAROLE}' and '${STATUS}' '${unsync}' time(s).  ['${PRIMARY}']TIME: ' ` ssh -nx admin@${PRIMARY} "datetime get" `
      if [[ "${unsync}" == "${unsync_error}" ]]; then
        UNSYNC=`echo ${IMASERVER}`
        echo "IMAServer: ${IMASERVER} HARole is: '"${HAROLE}"' and Status is: '"${STATUS}"' which is unexpected, aborting."
        rc=1
        break
      fi
    else 
      # HA Role is unexpected - has been blank and other values
      unsync=$(( ${unsync}+1 ))
      echo '   IMAServer: '${IMASERVER}'  '${HAROLE}' and '${STATUS}' '${unsync}' time(s).  ['${PRIMARY}']TIME: ' ` ssh -nx admin@${PRIMARY} "datetime get" `
      if [[ "${unsync}" == "${unsync_error}" ]]; then
        UNSYNC=`echo ${IMASERVER}`
        echo "IMAServer: ${IMASERVER} HARole is: '"${HAROLE}"' and Status is: '"${STATUS}"' which is unexpected, aborting."
        rc=9
        break
      fi
    fi
    sleep 15
  done
  return ${rc}
set +x
}
#############################
# returns: THIS NEEDS TO BE RETHOUGH, might need stat for Publisher thruput active
# Problem is what if Subscriber keeps up with Producers, there is no bufferMsgs, so how do you know clients are active?
#############################
function bufferedMsgs {

  echo "   Stat Subscription ${PRIMARY}..."
  rc=0
  STAT_1=`ssh -nx admin@${PRIMARY} "imaserver stat Subscription"`
  sleep 3
  STAT_2=`ssh -nx admin@${PRIMARY} "imaserver stat Subscription"`

SUB_1=""
SUB1=""
BUFF_1=""
BUFF1=""
complete="0" 

index=1
  for line in ${STAT_1}
  do
    SUB_1="`echo ${line} | cut -d , -f 1-3`"
    BUFF_1="`echo ${line} | cut -d , -f 5`"
    if [[ "${BUFF_1}" == *"BufferedMsgs"* ]]; then
       continue
    else
       SUB1[${index}]=${SUB_1}
       BUFF1[${index}]=${BUFF_1}
    fi
  index=$(( ${index} +1 ))
  done

# Parse the Second Stat to compare progress
  index=1
  for line in ${STAT_2}
  do
    SUB_2="`echo ${line} | cut -d , -f 1-3`"
    BUFF_2="`echo ${line} | cut -d , -f 5`"
    if [[ "${BUFF_2}" == *"BufferedMsgs"* ]]; then
       continue
    else
       # If still receiveing messages on this subscription
       if [[ "${BUFF_2}" -eq "${BUFF1[${index}]}" ]]; then 
         complete=$(( ${complete} +1 ))
       fi
    fi
    index=$(( ${index} +1 ))
  done
  index=$(( ${index} -1))

  if [[ "${complete}" -eq "${index}" ]]; then
    rc=1
    echo "Stat Subscription: " ${STAT_2}
    echo "We're Done Here, no more msgs being received!"
#    exit ${rc}
  fi

  return ${rc}
}
#############################
function checkCore {

  echo "   Check for Core Files on ${PRIMARY}..."
  CORE=`ssh -nx admin@${PRIMARY} "advanced-pd-options list"`
  if [[ "${CORE}" == *"bt."* ]]; then
    echo "CORE FILES Found on Primary;" ${PRIMARY}
    echo   ${CORE}
    exit
  fi

  echo "   Check for Core Files on ${STANDBY}..."
  CORE=`ssh -nx admin@${STANDBY} "advanced-pd-options list"`
  if [[ "${CORE}" == *"bt."* ]]; then
    echo "CORE FILES Found on Standby: " ${STANDBY}
    echo  ${CORE}
    exit
  fi

  return 
}
#############################
# Assumes IMASERVER has the IP address of the appliance to stop IMA
#
function imaserver-stop {

  echo "   Stopping imaserver ${IMASERVER}..."
  ssh admin@${STANDBY} "imaserver stop"
  status-imaserver
  RC=$?
  while [[ "${RC}" -ne "2" ]]; 
  do
    echo "Waiting for imaserver to stop" ${IMASERVER}
    ssh admin@${STANDBY} "imaserver stop"
    status-imaserver
    RC=$?
  done

  return 
}
#############################
# Begin Main
#############################
#export IMASERVER='9.3.177.87'
#export IMASERVER='9.3.177.212'
IMASERVERLIST=`$(eval echo $\{${THIS}_TESTROOT\})/scripts/getServerList`

DEBUG="TRUE"
DEBUG="FALSE"
if [ "${DEBUG}" == "TRUE" ]; then
   echo " R U N N I N G   I N   D E B U G   M O D E !!!  Only be one failover and some work to do at the end."
fi

# if there was input, expects M#:logile,M#:logfile, ... from automation
monitorPids=0
if [ $# -gt 0 ]; then
   pidlogfiles=$@
   echo "PIDLOGFILES=$pidlogfiles"
   monitorPids=1
   setup_monitor_pids
fi

LOOP_CONTROL=0
FAILOVER_COUNT=1
while [ ${LOOP_CONTROL} -ne ${FAILOVER_COUNT} ]
do

PRIMARY=" "
STANDBY=" "
UNSYNC=" "

   for IMASERVER in ${IMASERVERLIST}
   do

      # Is the Server Booted?
      ping-imaserver
      # Is the Server the Primary?
      status-imaserver
      RC=$?
      if [[ "${RC}" -ne "0" ]]; then
        echo " "
        echo "THERE IS A PROBLEM WITH IMASERVER "${IMASERVER}".  Check the status and collect logs!"
        exit -1
      fi

   done

   # Are IMA Servers clean of core files?
   checkCore
   echo " "
   echo "Primary= ${PRIMARY}, STANDBY= ${STANDBY}... Ready for Device Restart in 30 seconds after a few stats!"
#   if [ "${DEBUG}" == "TRUE" ];then
      echo " "
      echo 'DumpTopicTree: ' ` ssh admin@${PRIMARY} "advanced-pd-options dumptopictree level=7" `
#   fi
   echo " "

   echo 'STAT Queue: '
   QUEUESTATS=` ssh admin@${PRIMARY} "imaserver stat Queue ResultCount=100" `
   echo ${QUEUESTATS}

   echo 'STAT Subscriptions: ' 
   SUBS=` ssh admin@${PRIMARY} "imaserver stat Subscription ResultCount=100" `
   echo ${SUBS}
   FIND_DEL_SUB=` echo ${SUBS} | grep "c0...187" `
   RC=$?
   if [ "${RC}" == "0" ];then
      echo " "
      echo " ### STOP THE FAILOVER, Deleted Subscription Resurrected!  ###"
      LOOP_CONTROL=${FAILOVER_COUNT}
   fi
   echo " "
   echo 'STAT Conn Hi Thru: ' ` ssh admin@${PRIMARY} "imaserver stat Connection  StatType=HighestThroughputMsgs " `
   echo " "
   echo 'STAT Conn LowThru: ' ` ssh admin@${PRIMARY} "imaserver stat Connection  StatType=LowestThroughputMsgs  "  | column -s',' -t `
   echo " "
   echo 'STAT Conn Count: ' ` ssh admin@${PRIMARY} "imaserver stat "  |  grep "ActiveConnections =" | cut -d" " -f3 `
   echo " "
   echo 'STAT Memory: ' ` ssh admin@${PRIMARY} "imaserver stat Memory " `
   echo 'STAT Store:  ' ` ssh admin@${PRIMARY} "imaserver stat Store  " `
   echo 'STAT Flash:  ' ` ssh admin@${PRIMARY} "status flash          " `
   echo 'STAT Uptime: ' ` ssh admin@${PRIMARY} "status uptime         " `
   echo " "
   sleep 30 
#set -x
#   # If still receiving messages?  Not a good check, what if subscriber keeps up with producer, then no buffer...
#   bufferedMsgs
#   if [[ $? == 1 ]]; then
      echo 'PRIMARY ['${PRIMARY}'] TIME: ' ` ssh admin@${PRIMARY} "datetime get" `
#      if [ "${DEBUG}" == "TRUE" ];then
         # This is necessary when this machine eventually restarts and attempts to Sync with New Primary to avoid Split Brain StartupMode error
#         ssh admin@${PRIMARY} " imaserver update HighAvailability StartupMode=AutoDetect "
#         ssh admin@${PRIMARY} "device shutdown"
#      else
         ssh admin@${PRIMARY} "device restart"
#      fi
      echo "Failover initiated for ${FAILOVER_COUNT} times"
      # wait a bit for the machine to really stop....(maybe should ping until it fails?)
      # to kill some time, if there were PIDs to monitor for automation, do that now
      monitor_pids
      if [ ${rc} -ne 0 ]; then
         # FORCE Exit of this script with this loop
         LOOP_CONTROL=${FAILOVER_COUNT}
         # If exiting, need to wait a bit for the failover that was just initiated to complete -- or else the HA Pair might not be ready for cleanup.
         sleep 500
         continue
      fi
      sleep 60
#   else
#      exit
#   fi
#set +x

#   if [ "${DEBUG}" == "TRUE" ];then
      echo 'DumpTopicTree (on new PRIMARY): ' ` ssh admin@${STANDBY} "advanced-pd-options dumptopictree level=7" `
      sleep 5
      echo 'NEW PRIMARY ['${STANDBY}'] TIME: ' ` ssh admin@${STANDBY} "datetime get" `
#      echo "Stopping NEW Primary IMA Server @ ${STANDBY} and doing must-gather"
      # This is necessary so the IMA Server can be restarted before the old primary is restarted... (see StartupMode note above)
#      ssh admin@${STANDBY} "imaserver update HighAvailability StartupMode=StandAlone "
#      IMASERVER=${STANDBY}
#      imaserver-stop
#      ssh admin@${STANDBY} "platform must-gather ${STANDBY}.loop${FAILOVER_COUNT}.37093.mg"
      echo " "
#      echo "NOW, restart ${PRIMARY}, IMASERVER STOP, PLATFORM MUST-GATHER"
      #Force Exit 
#      LOOP_CONTROL=${FAILOVER_COUNT}
#   fi

   let FAILOVER_COUNT=${FAILOVER_COUNT}+1
done
