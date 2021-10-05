#!/bin/bash

# -s = server number
# -f = logfile
# -r = restore previous config
# -i = sync interval
# -d = forwarding delay

SYNC_INTERVAL=10
FORWARDING_DELAY=40

while getopts "f:s:r:i:d:" option ${OPTIONS}
do
    case ${option} in
    f )
        LOG_FILE=${OPTARG}
        ;;
    s )
        SERVER=${OPTARG}
        ;;
    r )
        RESTORE="1"
        ;;
    i )
        SYNC_INTERVAL=${OPTARG}
        ;;
    d )
        FORWARDING_DELAY=${OPTARG}
        ;;
    esac	
done

if [ -z ${SERVER} ]; then
  echo "No server number supplied in -s arg" | tee -a ${LOG_FILE}
  echo "Test result is Failure!" | tee -a ${LOG_FILE}
  exit 1
fi

USER="A${SERVER}_USER"
USR=${!USER}

HOST="A${SERVER}_HOST"
HST=${!HOST}

if [ -z ${USR} ]; then
    echo "Cannot find user env var for ${USER}" | tee -a ${LOG_FILE}
    echo "Test result is Failure!" | tee -a ${LOG_FILE}
   exit 1
fi

if [ -z ${HST} ]; then
    echo "Cannot find host env var for ${USER}" | tee -a ${LOG_FILE}
    echo "Test result is Failure!" | tee -a ${LOG_FILE}
    exit 1
fi

echo "user is ${USR}, host is ${HST}" | tee -a ${LOG_FILE}

theFile="/mnt/A${SERVER}/messagesight/data/config/server.cfg"
savedFile="${theFile}.ORIG"
backup="cp ${theFile} ${savedFile}"
restore="mv ${savedFile} ${theFile}"
echo1="echo \"\" >> ${theFile}"
echo2="echo \"Engine.ClusterRetainedSyncInterval = ${SYNC_INTERVAL}\" >> ${theFile}"
echo3="echo \"Engine.ClusterRetainedForwardingDelay = ${FORWARDING_DELAY}\" >> ${theFile}"

if [ -z $RESTORE ]; then
    echo "updating config" | tee -a ${LOG_FILE}
    cmd="ssh ${USR}@${HST} ${backup} && ${echo1} && ${echo2} && ${echo3}"
else
    echo "restoring config" | tee -a ${LOG_FILE}
    cmd="ssh ${USR}@${HST} ${restore}"
fi

`${cmd}`
rc=$?

if [ "${rc}" -ne "0" ]; then
    echo "Test result is Failure!" | tee -a ${LOG_FILE}
    exit 1
else
    echo "Test result is Success!" | tee -a ${LOG_FILE}
    exit 2
fi
