#!/bin/bash
# Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#
shopt -s extglob


pingServer() {

   echo "Pinging ${IP} until it repsonds"
   declare -i TRY=0
   declare -i RETRY=4
   declare -i PINGOK=0
   declare -i PINGTIMEOUT=3
   while [ 1 ]
   do
      ((TRY++))
      ping -c1 ${IP} &> /dev/null
      if [[ $? != 0 ]]; then
         if [[ "${TRY}" -le ${RETRY} ]]; then
            echo " Server ${IP} is not pingable on attempt ${TRY}, sleeping and will retry in ${PINGTIMEOUT} secs.  Local Time:  "`date`
            sleep ${PINGTIMEOUT}
         else
            echo " Server ${IP} is not pingable after ${TRY} attempts, ABORTING _ FAILURE!!.  Local Time:  "`date`
            exit 69
         fi
      else
            PINGOK=1
            echo " Server ${IP} is pingable on attempt ${TRY}, Local Time:  "`date`
            break
      fi
   done


}

##########################
###  MAIN Starts HERE ####

if [ $# -eq 0 ] ; then
   echo  "This script will query each nvDIMM for Serial Number"
   echo  "Passwordless SSH setup makes this script run smoother  :-)"
   echo  "Please , provide the SSHHOST IP of the Appliance(s) you wish to query"
   read  SERVER_LIST

else
   SERVER_LIST=$@
fi 

echo "The following Appliance(s) will be scanned for nvDIMM Serial Numbers:  ${SERVER_LIST}"
echo " "

declare EVIDENCE_PATH="/var/www/html/evidence/"
if [ ! -d ${EVIDENCE_PATH} ]; then
   mkdir -p ${EVIDENCE_PATH}
fi 

declare EVIDENCE_FILE

for IP in ${SERVER_LIST}
do
   pingServer;

   SERIAL=`ssh admin@${IP} "show version" | grep "Serial number" | cut -d ":" -f2`
   SERIAL="${SERIAL##*( )}"; SERIAL="${SERIAL%%*( )}";  # Trim leading spaces, Trailing spaces using extglob fcn
   EVIDENCE_FILE="nvDIMMserial_$SERIAL.txt"

   echo "Appliance '${SERIAL}' @ ${IP} reported nvDIMM Serial Numbers on `date`"  > ${EVIDENCE_FILE}

   for SLOT in 2 5 8 11
   do
      echo "  ==>  RESULTS for nvDIMM is slot ${SLOT}:"  >> ${EVIDENCE_FILE}
      echo "1" | ssh admin@${IP} "advanced-pd-options _pcitool nveeprom ${SLOT}" |grep -A27 "T_RUN (252)"  >> ${EVIDENCE_FILE}
      echo " "  >> ${EVIDENCE_FILE}
   done
   cat ${EVIDENCE_FILE}
   cp  ${EVIDENCE_FILE}  ${EVIDENCE_PATH}${EVIDENCE_FILE}

done
echo "nvDIMM Serial number evidence files copied to ${EVIDENCE_PATH}"

shopt -u extglob
exit 0
