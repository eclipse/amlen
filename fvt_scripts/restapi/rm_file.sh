#!/bin/bash +x
echo "Entering rm_file.sh with $# arguments: $@"

if [ -f ../testEnv.sh ] ; then
  source ../testEnv.sh
elif [ -f ../scripts/ISMsetup.sh ] ; then
  source ../scripts/ISMsetup.sh
fi
# ASSUMES:  A1, A2, A3, A4, A5 paths for /mnt/ mounts to DOCKER /var/messagesight

dstSrv=$1   # Destination Server to REMOVE file, pick one: [A1-A5]
dstPath=$2  # fully qualified path to REMOVE ALL Files (include * at end, if don't want DIR deleted): /mnt/A3/messagesight/data/import/*  
#set -x 

tmp="${dstSrv}_USER"
dstUser=$(eval echo \$${tmp})
tmp="${dstSrv}_HOST"
dstHost=$(eval echo \$${tmp})

RUNCMD="ssh  ${dstUser}@${dstHost} ls -als ${dstPath}" 
REPLY=`${RUNCMD}`
echo ${REPLY}

RUNCMD="ssh  ${dstUser}@${dstHost} rm -fr ${dstPath}"

TIMESTAMP=`date`
echo "${TIMESTAMP} TO RUN: ${RUNCMD}"
REPLY=`${RUNCMD}`
RC=$?
echo ${REPLY}
echo "RC=${RC}"

if [ ${RC} -ne 0 ] ; then
  echo "rm_file.sh FAILED to remove files on ${dstSrv} from:  ${dstPath}"
fi

TIMESTAMP=`date`
echo "${TIMESTAMP} ${0} ended"
