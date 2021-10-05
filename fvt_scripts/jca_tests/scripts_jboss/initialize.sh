#!/bin/bash
## 
## initialize.sh
##
## Description: This script will do some basic initialization
## for the JBOSS App Server.
##   1. Copy standalone-full.unitialized.xml to standalone-full.xml
##   2. Replace IMA_IP with value of $A1_HOST
##   3. Scopy standalone-full.xml to the jboss appserver

IP=`echo ${WASIP} | cut -d: -f1`
echo "IP: ${IP}"

cp scripts_jboss/standalone-full.uninitialized.xml scripts_jboss/standalone-full.xml

sed -i s/IMA_IP/$A1_HOST/ scripts_jboss/standalone-full.xml

scp scripts_jboss/standalone-full.xml ${IP}:${WASPath}/standalone/configuration/.

sleep 3 # ghetto hack because run scenarios sucks so bad
echo "initialize.sh done"
