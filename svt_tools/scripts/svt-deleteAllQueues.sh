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



unset LOG_FILE
unset SCREENOUT_FILE
unset SERVER
unset OUT
unset LINE
unset CN
unset SN
unset CMD


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

CMD='ssh admin@'${SERVER}' "imaserver stat Queue ResultCount=100" | tail -n +2'
display "${CMD}" ${SCREENOUT_FILE}
if [ ! -z ${SCREENOUT_FILE} ]; then
  OUT=`eval "${CMD}" 2>> ${SCREENOUT_FILE}`
else
  OUT=`eval "${CMD}"`
fi
display " " ${SCREENOUT_FILE}

while [ ${#OUT[@]} -gt 0 ]; do
  for LINE in ${OUT[@]}; do
    if [ ${#LINE} -gt 3 ]; then
      SN=`echo ${LINE}| cut -d"," -f1`
      if [[ "${SN}" == \"*\" ]]; then
         SN=${SN:1:$((${#SN}-2))}
      fi

      CMD='ssh admin@'${SERVER}' "imaserver delete Queue \"Name='${SN}'\" "DiscardMessages=True" "' 
      display "${CMD}" ${SCREENOUT_FILE}
      if [ ! -z ${SCREENOUT_FILE} ]; then
        eval ${CMD} >> ${SCREENOUT_FILE} 2>&1
      else
        eval ${CMD}
      fi
      display " " ${SCREENOUT_FILE}
    fi
  done

  if [ ${#OUT[@]} -lt 100 ]; then
     break
  fi

  CMD='ssh admin@'${SERVER}' imaserver stat Queue ResultCount=100 | tail -n +2'
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
unset LINE
unset CN
unset SN
unset CMD
