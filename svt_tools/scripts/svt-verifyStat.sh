#!/bin/bash 

#----------------------------------------------------------------------------
#
#  This script was intended for imaserver stat command though it should work for any bedrock cli
#  command that returns the results in table form. The first parameter specifies the column=value to be 
#  verified. for example
#
#  -o|MaxMessages=20000000|imaserver|stat|Queue|QueueName=SVTMsgEx_Queue
#  
#  Indicates to execute  
#        imaserver stat Queue QueueName=SVTMsgEx_Queue
#  and verify that
#        MaxMessages=20000000
#
# The stat command returns results in table form, such as
# ssh admin@10.10.1.135 imaserver stat Queue QueueName=SVTMsgEx_Queue
# "QueueName","Producers","Consumers","BufferedMsgs","BufferedMsgsHWM","BufferedPercent","MaxMessages","ProducedMsgs","ConsumedMsgs","RejectedMsgs","BufferedHWMPercent"
# "SVTMsgEx_Queue",0,0,0,2530,0.0,20000000,3530,0,0,0.01265
#
#  The script searches the first line of the output table for the string specified (i.e. MaxMessages), 
#  determines the column number (#5).
#  determines the value for that column from the second line of output (20000000).
#
#  If the values match "Test result is Success!" is output
#  as expected by cAppDriverLogEnd 
#
#  Example:
#  To execute at beginning of test add...
#    component[#]=cAppDriverWait,m1,"-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq20000000|-c|imaserver+stat+Queue+QueueName=SVTMsgEx_Queue,"
#
#  or to execute at end of test add...
#    component[#]=cAppDriverLogEnd,m1,"-e|../scripts/svt-verifyStat.sh,-o|-s|MaxMessages-eq20000000|-c|imaserver+stat+Queue+QueueName=SVTMsgEx_Queue,"
#
#----------------------------------------------------------------------------

getstat()
{

local VALUE
local ONE
local FIELD
local CMD
local CNT
local VAR
local VAR_TMP
local out
local f
local index
local COMP

if [ -z ${SERVER} ]; then
  SERVER=`../scripts/getPrimaryServerHostAddr.sh`
fi

display "SERVER=${SERVER}" ${LOG_FILE}

if [ -z $1 ];then
  exit
elif [[ $1 == *-eq* ]]; then
  COMP=-eq
  FIELD=${1/-eq*}
  VALUE=${1/*-eq}
elif [[ $1 == *-ne* ]]; then
  COMP=-ne
  FIELD=${1/-ne*}
  VALUE=${1/*-ne}
elif [[ $1 == *-lt* ]]; then
  COMP=-lt
  FIELD=${1/-lt*}
  VALUE=${1/*-lt}
elif [[ $1 == *-gt* ]]; then
  COMP=-gt
  FIELD=${1/-gt*}
  VALUE=${1/*-gt}
elif [[ $1 == *-le* ]]; then
  COMP=-le
  FIELD=${1/-le*}
  VALUE=${1/*-le}
elif [[ $1 == *-ge* ]]; then
  COMP=-ge
  FIELD=${1/-ge*}
  VALUE=${1/*-ge}
elif [[ $1 == *==* ]]; then
  COMP===
  FIELD=${1/==*}
  #VALUE=\"${1/*==}\"
  VALUE=${1/*==}
fi
CMD=${2//+/ }

display "$0 $FIELD $CMD" ${LOG_FILE}
#display "ssh admin@${SERVER} $CMD" ${LOG_FILE}
#out=`ssh admin@${SERVER} "$CMD"`

# run-cli uses server number so we need to convert IP to number
local SERVER_NUMBER=""
local appliance=1
while [ ${appliance} -le ${A_COUNT} ]
do
    a_host="A${appliance}_HOST"
    a_hostVal=$(eval echo \$${a_host})
    if [[ "${a_hostVal}" == "${SERVER}" ]] ; then
        SERVER_NUMBER=${appliance}
        break;
    fi 
    appliance=`expr ${appliance} + 1`
done

# remove first item
first_item=${CMD%% *}
RUN_CLI_CMD=${CMD:${#first_item}+1}
../scripts/run-cli.sh -i "verify x ${RUN_CLI_CMD}" -a ${SERVER_NUMBER} -f ${LOG_FILE} 

out=`cat reply.json`
#display "$out" ${LOG_FILE}

TYPE=`echo ${CMD} | cut -d' ' -f3`
Q_OBJECT=`echo ${CMD} | cut -d' ' -f4`
Q_NAME=${Q_OBJECT%%=*}
Q_VALUE=${Q_OBJECT##*=}
# Replace * with # to handle TopicStrings
Q_VALUE=${Q_VALUE//\*/#}

len=`echo $out | python -c "import json,sys;obj=json.load(sys.stdin);print len(obj[\"${TYPE}\"])"`
display "array_len=${len}"

local n=0
while [ ${n} -lt ${len} ]
do
    f=`echo $out | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"${TYPE}\"][${n}][\"${Q_NAME}\"]"`
    if [[ "${Q_VALUE}" == "${f}" ]] ; then
        break;
    fi
    n=`expr ${n} + 1`
done


display "import json,sys;obj=json.load(sys.stdin);print obj[\"${TYPE}\"][${n}][\"${FIELD}\"]" ${LOG_FILE}
f=`echo $out | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"${TYPE}\"][${n}][\"${FIELD}\"]"`

#index=0
#for line in ${out[@]}; do
#  if [ ${index} -eq 0 ]; then
#    display "${line}" ${LOG_FILE}
#    VAR=${line/${FIELD}*/}
#    VAR_TMP="${VAR//[^,]}" ; 
#    CNT=$((${#VAR_TMP}+1))
##    echo ${CNT}
#  else 
#    display "${line}" ${LOG_FILE}
#    f=`echo ${line}|cut -d',' -f${CNT}`
#    display "${FIELD}=${f}" ${LOG_FILE}
#    display "${FIELD}=${f}" ${SCREENOUT_FILE}
##    echo ${FIELD}=${f}
#    break;
#  fi
#  ((index++))
#done

  if [ ${f} ${COMP} ${VALUE} ]; then
    display "${f} ${COMP} ${VALUE}" ${LOG_FILE}
    display "Test result is Success!" ${LOG_FILE}
  else
    display "${FIELD} is ${f} " ${LOG_FILE}
    display "${f} ${COMP} ${VALUE} is false" ${LOG_FILE}
    exit 1
  fi

}

display(){

  local TEXT=$1
  local MYFILE=$2

  if [ "${MYFILE}" != "" ]; then
    echo ${TEXT} >> ${MYFILE}
  else
    echo ${TEXT}
  fi
}

run()
{
local OPTIONS="$@"
local LOG_FILE
local SCREENOUT_FILE
local SERVER
local c
local s

#----------------------------------------------------------------------------
# Source the ISMsetup file to get access to information for the remote machine
#----------------------------------------------------------------------------
if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  source ../scripts/ISMsetup.sh
fi

#----------------------------------------------------------------------------
# Parse Options
#----------------------------------------------------------------------------
while getopts "f:c:a:s:" option ${OPTIONS}; do
  case ${option} in
  f )
      LOG_FILE=${OPTARG}
      > ${LOG_FILE}
      SCREENOUT_FILE=${OPTARG}.screenout.log
      > ${SCREENOUT_FILE}
      ;;
  a ) SERVER=${OPTARG}
      ;;
  c ) c="${OPTARG}"
      ;;
  s ) s="${OPTARG}"
      ;;
  esac	
done

getstat "${s}" "${c}"
}

run $*
