#!/bin/bash
echo "Entering copy_file.sh with $# arguments: $@"

if [ -f ../testEnv.sh ] ; then
  source ../testEnv.sh
elif [ -f ../scripts/ISMsetup.sh ] ; then
  source ../scripts/ISMsetup.sh
fi

srcFile=$1
dstFile=$2

if [[ "${A1_TYPE}" == "DOCKER" ]] ; then 
    RUNCMD="ssh ${A1_USER}@${A1_HOST} docker exec ${A1_SRVCONTID} bash -c \"cp -f ${srcFile} ${dstFile}\""
else
    RUNCMD="ssh ${A1_USER}@${A1_HOST} bash -c \"cp -f ${srcFile} ${dstFile}\""
fi
echo ${RUNCMD}
REPLY=`${RUNCMD}`
RC=$?
echo ${REPLY}
echo "RC=${RC}"

if [ ${RC} -ne 0 ] ; then
  echo "get_file.sh FAILED to get file"
fi
