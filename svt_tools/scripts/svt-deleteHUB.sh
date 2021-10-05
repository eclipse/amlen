#!/bin/bash 

#----------------------------------------------------------------------------
# 
#  To execute at beginning of test add...
#    component[#]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.sh,"
#
#  or to execute at end of test add...
#    component[#]=cAppDriverLogEnd,m1,"-e|../scripts/svt-deleteAllSubscriptions.sh,"
#
#----------------------------------------------------------------------------


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
while getopts "f:a:h:" option ${OPTIONS}; do
  case ${option} in
  f )
      LOG_FILE=${OPTARG}
      > ${LOG_FILE}
      SCREENOUT_FILE=${OPTARG}.screenout.log
      > ${SCREENOUT_FILE}
      ;;
  a ) SERVER=${OPTARG}
      ;;
  h ) HUB=${OPTARG}
      ;;

  esac	
done

if [ -z ${HUB} ]; then
  display "Must specify: -h <HUB Name>" ${SCREENOUT_FILE}
  exit 
fi

if [ ! -z ${LOG_FILE} ]; then
  echo $0 $@ > ${LOG_FILE}
fi

if [ -z ${SERVER} ]; then
  SERVER=`../scripts/getPrimaryServerAddr.sh`
fi

display "SERVER=${SERVER}" ${SCREENOUT_FILE}
display " " ${SCREENOUT_FILE}

CMD='ssh admin@${SERVER} "imaserver list Endpoint"'
if [ ! -z ${SCREENOUT_FILE} ]; then
  OUT=`eval "${CMD}" 2>> ${SCREENOUT_FILE}`
else
  OUT=`eval "${CMD}"`
fi


for LINE in ${OUT[@]}; do
  CMD1="ssh admin@${SERVER} imaserver show Endpoint \"Name=${LINE}\" | grep MessagingPolicies |cut -d' ' -f 3"
  CMD2="ssh admin@${SERVER} imaserver show Endpoint \"Name=${LINE}\" | grep ConnectionPolicies |cut -d' ' -f 3"
  CMD3="ssh admin@${SERVER} imaserver show Endpoint \"Name=${LINE}\" | grep MessageHub "
  OUT3=`eval "${CMD3}"`

  if [[ "${OUT3}" == "MessageHub = ${HUB} " ]]; then
    CMD="ssh admin@${SERVER} imaserver delete Endpoint \"Name=${LINE}\""
    display "${CMD}" ${SCREENOUT_FILE}
    if [ ! -z ${SCREENOUT_FILE} ]; then
      OUT=`eval "${CMD}" 2>> ${SCREENOUT_FILE}`
    else
      OUT=`eval "${CMD}"`
    fi

    OUT1=`eval "${CMD1}"`
    ARR=$(echo $OUT1 | tr "," "\n")
    for X in $ARR; do
      CMD1="ssh admin@${SERVER} imaserver delete MessagingPolicy \"Name=$X\""
      display "${CMD1}" ${SCREENOUT_FILE}
      if [ ! -z ${SCREENOUT_FILE} ]; then
        OUT=`eval "${CMD}" 2>> ${SCREENOUT_FILE}`
      else
        OUT=`eval "${CMD}"`
      fi
    done

    OUT2=`eval "${CMD2}"`
    ARR=$(echo $OUT1 | tr "," "\n")
    for X in $ARR; do
      CMD2="ssh admin@${SERVER} imaserver delete CONNECTIONPolicy \"Name=$X\""
      display "${CMD2}" ${SCREENOUT_FILE}
      if [ ! -z ${SCREENOUT_FILE} ]; then
        OUT=`eval "${CMD}" 2>> ${SCREENOUT_FILE}`
      else
        OUT=`eval "${CMD}"`
      fi
    done
  fi
done

CMD="ssh admin@${SERVER} imaserver delete MessageHub \"Name=${HUB}\""
display "${CMD}" ${SCREENOUT_FILE}
if [ ! -z ${SCREENOUT_FILE} ]; then
  OUT=`eval "${CMD}" 2>> ${SCREENOUT_FILE}`
else
  OUT=`eval "${CMD}"`
fi

display "Test result is Success!" ${LOG_FILE}

