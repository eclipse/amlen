#!/bin/bash +x
echo "Entering ${0} with $# arguments: $@"
# based off of scp_file.sh

if [ -f ../testEnv.sh ] ; then
  source ../testEnv.sh
elif [ -f ../scripts/ISMsetup.sh ] ; then
  source ../scripts/ISMsetup.sh
fi
# ASSUMES:  A1, A2, A3, A4, A5 paths for /mnt/ mounts to DOCKER /var/messagesight
srcSrv=$1   # Source Server with file, pick one: [A1-A5]
srcFile=$2  # fully qualified path with file name:  /var/messagesight/data/export/Export.data.tar (wildcards might work)
dstPath=$3  # fully qualified path to PUT srcFile: /mnt/A3/messagesight/data/import/  (will be created if does not exist)
#set -x 

tmp="${srcSrv}_USER"
srcUser=$(eval echo \$${tmp})
tmp="${srcSrv}_HOST"
srcHost=$(eval echo \$${tmp})
###tmp="${dstSrv}_USER"
###dstUser=$(eval echo \$${tmp})
###tmp="${dstSrv}_HOST"
###dstHost=$(eval echo \$${tmp})


if [ ${dstPath} <> "." ]; then
   ssh ${M1_USER}@${M1_HOST} " mkdir -p ${dstPath}"
fi

RUNCMD="ssh ${srcUser}@${srcHost} ls -al ${srcFile}"
REPLY=`${RUNCMD}`
echo ${REPLY}
echo "files will be SCP'ed"
set -x
RUNCMD="scp -r ${srcUser}@${srcHost}:${srcFile} ${dstPath}/ "

echo "TO RUN: ${RUNCMD}"
REPLY=`${RUNCMD}`
RC=$?
echo ${REPLY}
echo "RC=${RC}"
set +x
if [ ${RC} -ne 0 ] ; then
  echo "${0} FAILED to scp file: ${srcFile} to ${dstPath}"
fi
