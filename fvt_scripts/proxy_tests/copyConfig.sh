#!/bin/bash
#

# Source the ISMsetup file to get access to information for the remote machine
if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  source ../scripts/ISMsetup.sh
fi

if [[ "$1" == "-f" ]]
then
  file=proxysam2.cfg
  logfile=$2
else
  file=$1
  logfile=$3
fi

# Directory has to exist when proxy starts
#ssh ${P1_USER}@${P1_HOST} mkdir ${P1_PROXYROOT}/config
echo scp ${file} ${P1_USER}@${P1_HOST}:${P1_PROXYROOT}/config/proxysam2.cfg
scp ${file} ${P1_USER}@${P1_HOST}:${P1_PROXYROOT}/config/proxysam2.cfg
ssh ${P1_USER}@${P1_HOST} ls -al ${P1_PROXYROOT}/config
ssh ${P1_USER}@${P1_HOST} cat ${P1_PROXYROOT}/config/proxysam2.cfg
# Rename cfg file to end in .log so it gets moved into the testcase collection directory
echo Test Success! >> $logfile 
