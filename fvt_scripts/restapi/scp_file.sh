#!/bin/bash +x
echo "Entering scp_file.sh with $# arguments: $@"

if [ -f ../testEnv.sh ] ; then
  source ../testEnv.sh
elif [ -f ../scripts/ISMsetup.sh ] ; then
  source ../scripts/ISMsetup.sh
fi
# ASSUMES:  A1, A2, A3, A4, A5 paths for /mnt/ mounts to DOCKER /var/messagesight
srcSrv=$1   # Source Server with file, pick one: [A1-A5]
srcFile=$2  # fully qualified path with file name:  /var/messagesight/data/export/Export.data.tar (wildcards might work)
dstSrv=$3   # Destination Server to PUT file, pick one: [A1-A5]
dstPath=$4  # fully qualified path to PUT srcFile: /mnt/A3/messagesight/data/import/  (will be created if does not exist)
#set -x 

tmp="${srcSrv}_USER"
srcUser=$(eval echo \$${tmp})
tmp="${srcSrv}_HOST"
srcHost=$(eval echo \$${tmp})
tmp="${dstSrv}_USER"
dstUser=$(eval echo \$${tmp})
tmp="${dstSrv}_HOST"
dstHost=$(eval echo \$${tmp})

ssh ${dstUser}@${dstHost} " mkdir -p ${dstPath}"

RUNCMD="ssh ${srcUser}@${srcHost} ls -al ${srcFile}"
REPLY=`${RUNCMD}`
TIMESTAMP=`date`
echo ${REPLY}
echo "${TIMESTAMP} files will be SCP'ed"

RUNCMD="scp -r ${srcUser}@${srcHost}:${srcFile} ${dstUser}@${dstHost}:${dstPath}/ "

echo "TO RUN: ${RUNCMD}"
REPLY=`${RUNCMD}`
RC=$?
echo ${REPLY}
echo "RC=${RC}"
set +x
if [ ${RC} -ne 0 ] ; then
  echo "scp_file.sh FAILED to scp file: ${srcFile} to ${dstPath}"
fi

TIMESTAMP=`date`
echo "${TIMESTAMP} ${0} ended"
