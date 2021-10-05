#!/bin/bash
#

# Source the ISMsetup file to get access to information for the remote machine
if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  source ../scripts/ISMsetup.sh
fi

file=$1
destfile=$2
logfile=$3

# Directory has to exist when proxy starts
#ssh ${P1_USER}@${P1_HOST} mkdir ${P1_PROXYROOT}/config
echo scp ${file} ${P1_USER}@${P1_HOST}:${destfile}
scp ${file} ${P1_USER}@${P1_HOST}:${destfile}
ssh ${P1_USER}@${P1_HOST} ls -al ${destfile}
ssh ${P1_USER}@${P1_HOST} cat ${destfile}
# Rename cfg file to end in .log so it gets moved into the testcase collection directory
#echo Test Success! >> $logfile 
echo Test Success!
