#!/bin/bash -x

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

declare _COMMAD=
declare _C1=
declare _C2=
declare _CMD=
declare _OUT=
declare _SPACE=
declare _QUOTE=
declare _TIC=
declare _SLASH=
declare _COMMA=
declare _DOLLAR=
declare _HYPHEN=
declare _SEMICOLON=
declare _BACKGROUND=

#----------------------------------------------------------------------------
# Parse Options
#----------------------------------------------------------------------------
while getopts "f:b:e:s:q:t:c:d:h:z:" option ${OPTIONS}; do
  case ${option} in
  f )
      LOG_FILE=${OPTARG}
      > ${LOG_FILE}
      SCREENOUT_FILE=${OPTARG}.screenout.log
      > ${SCREENOUT_FILE}
      ;;
  b ) _BACKGROUND=${OPTARG}
      ;;
  e ) _COMMAND=${OPTARG}
      ;;
  s ) _SPACE=${OPTARG}
      ;;
  q ) _QUOTE=${OPTARG}
      ;;
  t ) _TIC=${OPTARG}
      ;;
  c ) _COMMA=${OPTARG}
      ;;
  d ) _DOLLAR=${OPTARG}
      ;;
  h ) _HYPHEN=${OPTARG}
      ;;
  z ) _SEMICOLON=${OPTARG}
      ;;

  esac	
done

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

if [ ! -z ${_TIC} ]; then
  _C4=$(sed -e"s/${_TIC}/'/g" <<< ${_C3})
else
  _C4="${_C3}"
fi

if [ ! -z ${_HYPHEN} ]; then
  _C5=$(sed -e"s/${_HYPHEN}/-/g" <<< ${_C4})
else
  _C5="${_C4}"
fi

if [ ! -z ${_SEMICOLON} ]; then
  _C6=$(sed -e"s/${_SEMICOLON}/;/g" <<< ${_C5})
else
  _C6="${_C5}"
fi

if [ ! -z ${_DOLLAR} ]; then
  _CMD=$(sed -e"s/${_DOLLAR}/\$/g" <<< ${_C6})
else
  _CMD="${_C6}"
fi

if [ "${_CMD}" != "" ]; then
  if [ "${_BACKGROUND}" != "" ]; then
    display "\"${_CMD} &\"" ${LOG_FILE}
    $(eval echo ${_CMD}) &
    sleep 2s
  else
    display "\"${_CMD}\"" ${LOG_FILE}
    _OUT=`$(eval echo ${_CMD})`
    display "${_OUT}" ${LOG_FILE}
  fi
fi

