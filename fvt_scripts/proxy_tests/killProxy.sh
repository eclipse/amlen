#!/bin/bash
#

# Source the ISMsetup file to get access to information for the remote machine
if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  source ../scripts/ISMsetup.sh
fi

set -x
# P1, P2, ... default P1
if [[ $# -ge 1 ]]; then
	PX_TEMP=$1_USER
	PX_USER=$(eval echo \$${PX_TEMP})
	PX_TEMP=$1_HOST
	PX_HOST=$(eval echo \$${PX_TEMP})
	PX_TEMP=$1_PROXYROOT
	PX_PROXYROOT=$(eval echo \$${PX_TEMP})
else  
	PX_USER=${P1_USER}
	PX_HOST=${P1_HOST}
	PX_PROXYROOT=${P1_PROXYROOT}
fi
set +x

echo imaproxy processes:
ssh ${PX_USER}@${PX_HOST} ps -ef | grep imaproxy | grep -v ssh | grep -v bash | grep -v grep | awk '{print $2;}'

pidlist=`ssh ${PX_USER}@${PX_HOST} ps -ef | grep imaproxy | grep -v ssh | grep -v bash | grep -v grep | awk '{print $2;}'`
ssh ${PX_USER}@${PX_HOST} kill $pidlist

echo imaproxy processes after kill:
ssh ${PX_USER}@${PX_HOST} ps -ef | grep imaproxy | grep -v ssh | grep -v bash | grep -v grep | awk '{print $2;}'

# get a listing of all files in the directory
ssh ${PX_USER}@${PX_HOST} ls -al ${PX_PROXYROOT}
# copy all files, there might be some files due to a system trap and we definitely want those
scp ${PX_USER}@${PX_HOST}:${PX_PROXYROOT}/* .
#scp ${PX_USER}@${PX_HOST}:${PX_PROXYROOT}/*.cfg .
# Remove the old logs after copying them here
# No longer removing the logs after copying them so AF can also copy them
#ssh ${PX_USER}@${PX_HOST} rm -f ${PX_PROXYROOT}/*.log
# Rename cfg file to end in .log so it gets moved into the testcase collection directory
mv proxy.cfg proxy.cfg.log
# delete any copied extra cfg files
ssh ${PX_USER}@${PX_HOST} ls ${PX_PROXYROOT}/config
ssh ${PX_USER}@${PX_HOST} rm -f ${PX_PROXYROOT}/config/*

