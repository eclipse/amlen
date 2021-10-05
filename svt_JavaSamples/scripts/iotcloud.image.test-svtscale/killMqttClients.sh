#!/bin/bash +x

killAPP () {
set -x
      while read line ;
      do
         PID=`echo $line | cut -d " " -f 2 `
         IGNORE_GREP=`echo $line | cut -d " " -f 8 `
         APP=`echo $line | cut -d " " -f 11 `
         if [[ "${APP}" == ${KILLAPP} ]] ; then
            kill -9 ${PID}
         else
            if [ "${APP}" == "" -a "${IGNORE_GREP}" != "grep" ] ; then
               echo " "
               echo "  !!! WARNING:  NO APPs were killed, the APP:${APP} did not match the expected AppName:${KILLAPP} !!!   "
               echo " "
            fi
         fi
      done < $0.data
set +x
      echo "Killed ${LINE_COUNT} occurances of ${KILLAPP} "

}

###############
###  MAIN   ###
###############

declare KILLAPP="svt/mqtt/mq/MqttSample"
declare ACTION="PROMPT"

if [[ $# > 0 ]] ; then
   KILLAPP=$1
fi

if [[ $# > 1 ]] ; then
   ACTION=$2
fi




`ps -ef | grep $KILLAPP > $0.data `
LINE_COUNT=` cat  $0.data | wc -l`
(( LINE_COUNT-- ))
if [[ "${LINE_COUNT}" -ne 0 ]] ; then 
   if [[ "${ACTION}" == "PROMPT" ]] ; then

      while  [[ ${ACTION} == "PROMPT" ]] ;
      do
         echo "Found ${LINE_COUNT} occurances of ${KILLAPP}, kill them all? (Y/n/show)"
         read y
         if [[ "${y}" == "Y" ]] ; then
            killAPP;
            ACTION="EXIT"
         elif [[ "${y}" == "show" ]] ; then
            cat "$0.data"
            echo " "
            ACTION="PROMPT"
         elif [[ "${y}" == "n" ]] ; then
            echo "No occurances of ${KILLAPP} were terminated based on response"
            ACTION="EXIT"
            fi

      done
   else
      killAPP;
   fi 
else
  echo " "
  echo "No occurances of ${KILLAPP} were found running! "
  echo " "
  echo " SYNTAX:  $0  [KILLAPP name] [PROMPT|NOPROMPT]"
  echo " DEFAULT:  $0  [${KILLAPP}] [${ACTION}]"
fi
