#!/bin/bash 

#----------------------------------------------------------------------------
# 
#  To execute at beginning of test add...
#    component[#]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.sh,"
#
#  or to execute at end of test add...
#    component[#]=cAppDriverLogEnd,m1,"-e|../scripts/svt-deleteAllMqttClients.sh,"
#
#----------------------------------------------------------------------------

if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  if [[ -f "../scripts/ISMsetup.sh"  ]]
  then
    source ../scripts/ISMsetup.sh
  else
    source /niagara/test/scripts/ISMsetup.sh
  fi
fi


unset LOG_FILE
unset SCREENOUT_FILE
unset SERVER
unset OUT
unset CMD
unset SN

display(){
  TEXT=$1
  SCREENOUT_FILE=$2

  if [ ! -z ${SCREENOUT_FILE} ]; then
    echo ${TEXT} >> ${SCREENOUT_FILE}
  else
    echo ${TEXT}
  fi
}


#----------------------------------------------------------------------------
# Parse Options
#----------------------------------------------------------------------------
while getopts "f:a:" option ${OPTIONS}; do
  case ${option} in
  f )
      LOG_FILE=${OPTARG}
      > ${LOG_FILE}
      SCREENOUT_FILE=${OPTARG}.screenout.log
      > ${SCREENOUT_FILE}
      ;;
  a ) SERVER=${OPTARG}
      ;;

  esac
done

if [ ! -z ${LOG_FILE} ]; then
  echo $0 $@ > ${LOG_FILE}
fi

if [ -z ${SERVER} ]; then
  SERVER=`../scripts/getPrimaryServerHostAddr.sh`
fi

display "SERVER=${SERVER}" ${SCREENOUT_FILE}
display " " ${SCREENOUT_FILE}

if [[ "${A1_TYPE}" == "DOCKER" ]]; then
  CMD='curl -i -H "Accept: application/json" -X GET http://${SERVER}:${A1_PORT}/ima/v1/monitor/MQTTClient'
else
  CMD='ssh admin@'${SERVER}' "imaserver stat MQTTClient ResultCount=100" | tail -n +2'
fi

display "${CMD}" ${SCREENOUT_FILE}
if [ ! -z ${SCREENOUT_FILE} ]; then
  OUT=`eval "${CMD}" 2>> ${SCREENOUT_FILE}`
else
  OUT=`eval "${CMD}"`
fi
display " " ${SCREENOUT_FILE}

while [ ${#OUT[@]} -gt 0 ]; do
  if [[ "${A1_TYPE}" == "DOCKER" ]]; then
    for LINE in ${OUT[@]}; do
       if [[ ${LINE} =~ "ClientID" ]]; then
       SN=`echo ${LINE}|cut -d'"' -f4`
       CMD='curl -i -H "Accept: application/json" -X DELETE http://${SERVER}:${A1_PORT}/ima/v1/service/MQTTClient/${SN}'
       display "${CMD}" ${SCREENOUT_FILE}
       if [ ! -z ${SCREENOUT_FILE} ]; then
         eval ${CMD} >> ${SCREENOUT_FILE} 2>&1
       else
         eval ${CMD}
       fi
       display " " ${SCREENOUT_FILE}
       fi
    done
  else
    for LINE in ${OUT[@]}; do
       if [[ ${LINE} =~ [,] ]]; then
         SN=`echo ${LINE}|cut -d'"' -f2`
         CMD='ssh admin@'${SERVER}' imaserver delete MQTTClient \"Name='${SN}'\"'
         display "${CMD}" ${SCREENOUT_FILE}
         if [ ! -z ${SCREENOUT_FILE} ]; then
           eval ${CMD} >> ${SCREENOUT_FILE} 2>&1
         else
           eval ${CMD}
         fi
         display " " ${SCREENOUT_FILE}
       fi
    done
  fi

  if [ ${#OUT[@]} -lt 100 ]; then
     break
  fi

  if [[ "${A1_TYPE}" == "DOCKER" ]]; then
    CMD='curl -i -H "Accept: application/json" -X GET http://${SERVER}:${A1_PORT}/ima/v1/monitor/MQTTClient'
  else
    CMD='ssh admin@'${SERVER}' "imaserver stat MQTTClient ResultCount=100" | tail -n +2'
  fi
  display "${CMD}" ${SCREENOUT_FILE}
  if [ ! -z ${SCREENOUT_FILE} ]; then
    OUT=`eval "${CMD}" 2>> ${SCREENOUT_FILE}`
  else
    OUT=`eval "${CMD}"`
  fi
  display " " ${SCREENOUT_FILE}
done

display "Test result is Success!" ${LOG_FILE}

unset LOG_FILE
unset SCREENOUT_FILE
unset SERVER
unset OUT
unset CMD
unset SN

