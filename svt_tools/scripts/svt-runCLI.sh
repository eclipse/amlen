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
  declare TEXT=$1
  declare MYFILE=$2

  if [ "${MYFILE}" != "" ]; then
    echo ${TEXT} >> ${MYFILE}
  else
    echo ${TEXT}
  fi
}

declare _USER=
declare _SERVER=
declare _COMMAD=
declare _C1=
declare _C2=
declare _CMD=
declare _OUT=
declare _SPACE=
declare _QUOTE=
declare _SLASH=
declare _COMMA=
declare _DOLLAR=

#----------------------------------------------------------------------------
# Parse Options
#----------------------------------------------------------------------------
while getopts "f:a:e:u:s:q:c:d:" option ${OPTIONS}; do
  case ${option} in
  f )
      LOG_FILE=${OPTARG}
      > ${LOG_FILE}
      SCREENOUT_FILE=${OPTARG}.screenout.log
      > ${SCREENOUT_FILE}
      ;;
  a ) _SERVER=${OPTARG}
      ;;
  e ) _COMMAND=${OPTARG}
      ;;
  u ) _USER=${OPTARG}
      ;;
  s ) _SPACE=${OPTARG}
      ;;
  q ) _QUOTE=${OPTARG}
      ;;
  c ) _COMMA=${OPTARG}
      ;;
  d ) _DOLLAR=${OPTARG}
      ;;

  esac	
done

if [ -z ${_SERVER} ]; then
  _SERVER=`../scripts/getPrimaryServerHostAddr.sh`
fi

if [ -z ${_USER} ]; then
  _USER=admin
fi

if [ ! -z ${_SPACE} ]; then
  _C1=$(sed -e"s/${_SPACE}/ /g" <<< ${_COMMAND})
else
  _C1="${_COMMAND}"
fi

if [ ! -z ${_COMMA} ]; then
  _C2=$(sed -e"s/${_COMMA}/,/g" <<< ${_C1})
else
  _C2="${_C1}"
fi

if [ ! -z ${_QUOTE} ]; then
  _C3=$(sed -e"s/${_QUOTE}/\"/g" <<< ${_C2})
else
  _C3="${_C2}"
fi

if [ ! -z ${_DOLLAR} ]; then
  _CMD=$(sed -e"s/${_DOLLAR}/\$/g" <<< ${_C3})
else
  _CMD="${_C3}"
fi

if [ "${_CMD}" != "" ]; then
  display "ssh ${_USER}@${_SERVER} \"${_CMD}\"" ${LOG_FILE}
  #_OUT=`ssh ${_USER}@${_SERVER} "${_CMD}"`

  # remove imaserver from CMD
  first_item=${_CMD%% *}
  _CMD=${_CMD:${#first_item}+1}
  _OUT=`../scripts/run-cli.sh -i "verify x ${_CMD}" -f /dev/stdout`

  display "${_OUT}" ${LOG_FILE}
fi

