#!/bin/bash

#----------------------------------------------------------------------------
# 
#  To execute at beginning of test add...
#    component[#]=cAppDriverLogWait,m1,"-e|../scripts/svt-logEnd.sh,-o|-c|../scripts/getServerAddr"
#
#----------------------------------------------------------------------------

unset LOG_FILE
unset SCREENOUT_FILE
unset CMD
unset option
unset result

#----------------------------------------------------------------------------
# Parse Options
#----------------------------------------------------------------------------
while getopts "f:c:" option ${OPTIONS}; do
  case ${option} in
  f )
      LOG_FILE="${OPTARG}"
      SCREENOUT_FILE="${OPTARG}".screenout.log
      ;;
  c ) CMD=${OPTARG}
      ;;
  esac	
done

eval "${CMD}" > ${SCREENOUT_FILE} 2>&1
result=$?

if [ ${result} -eq 0 ]; then
  echo "Test result is Success!" >> ${LOG_FILE}
else
  echo "Test result is "${result} >> ${LOG_FILE}
fi

unset LOG_FILE
unset SCREENOUT_FILE
unset CMD
unset option
unset result
