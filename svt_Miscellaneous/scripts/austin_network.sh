#!/bin/bash  -x
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

########################################################################
#  GET_IP_10NET()
### 
GET_IP_10NET () {
   
#   IP_10NET=`echo ${ETH:(${#ETH})-1:1  }`  
# CSC Convention above, but need the Austin Lab Convention in automation
   ETH_TYPE=`echo ${ETH:0:(${#ETH})-1 }`
   ETH_ID=`echo ${ETH:(${#ETH})-1:1  }`
   if [ "${ETH_TYPE}" == "eth" ]; then
      if [ "${IP_F3}" == "179" ]; then
         IP_10NET=${a179_IP_F3[${ETH_ID}] }
      else
         IP_10NET=${a177_IP_F3[${ETH_ID}] }
      fi

   elif [ "${ETH_TYPE}" == "mgt" ]; then
      LAST=$((${#a179_IP_F3[*]}-1))
      if [ "${IP_F3}" == "179" ]; then
         IP_10NET=${a179_IP_F3[LAST] }
      else
         IP_10NET=${a177_IP_F3[LAST] }
      fi

   elif [ "${ETH_TYPE}" == "ha" ]; then 
      if [ "${IP_F3}" == "179" ]; then
         IP_10NET=${a179_IP_F3[0] }
      else
         IP_10NET=${a177_IP_F3[0] }
      fi
      
   else
      echo "ERROR:  Ethernet Interface name was not recognized and could not be processed."
      echo "        Expected names are 'eth', 'mgt', or 'ha', but found:  ${ETH}"
      echo "        TERMINATING!"
      exit 99
   fi
}

########################################################################
#  GET_IP_F4
###
GET_IP_F4 () {
   IP_F3=`echo ${A1_HOST} | cut -d "." -f 3`
   IP_F4=`echo ${A1_HOST} | cut -d "." -f 4`
   length=${#A1_HOST}-${#IP_F4}
   GATEWAY_IP="${A1_HOST:0:${length}}1"
#set -x   
   if [ "${IP_F3}" == "179" ]; then
      IMM_IP_F3="176"
   else
      IMM_IP_F3="174"
   fi 
   length=${length}-${#IP_F3}-1

   export A1_HOST=${A1_HOST}
   if [ "${A1_IMM}" == "" ]; then
      export A1_IMM="${A1_HOST:0:${length}}${IMM_IP_F3}.${IP_F4}"
   fi
   export IP_F4=${IP_F4}
#set +x
}


### MAIN ###########################################################################################

if [ $# -ne 1 ]; then
   echo "What is the IP of the Appliance to be tested?"
   read  A1_HOST
else
   A1_HOST=$1
fi 
echo "Processing Ethernet Interfaces on appliance ${A1_HOST}"

declare IP_10NET=""

declare IP_F3=""
declare -a a177_IP_F3=( 1 3 5 7 9 11 13 15 17 )
declare -a a179_IP_F3=( 0 2 4 6 8 10 12 14 16 )

declare IP_F4=""
declare GATEWAY_IP=""
# PREREQ:  Establish the values for the variable above
GET_IP_F4;

EXPECTED_ETH_INTF=( eth0 eth1 eth2 eth3 eth4 eth5 eth6 eth7 ha0 ha1 mgt0 mgt1 )
ETH_INTF_RAW=` ssh admin@${A1_HOST} "list ethernet-interface" `
declare -a ETH_INTF=`echo ${ETH_INTF_RAW}` 

if [ "${ETH_INTF[*]}" != "${EXPECTED_ETH_INTF[*]}" ]; then
   echo "ERROR:  There is a serious network failure!  NOT ALL ETH Interfaces discovered!"
   echo "        Expect Eth List:  '${EXPECTED_ETH_INTF[*]}'.    YET FOUND THIS:"
   ssh admin@${A1_HOST} "list ethernet-interface"
   exit 99
fi
echo "Clean the existing IPs out in case there are collisions"
for ETH in ${ETH_INTF}
do
   if [ "${ETH}" != "mgt0" ]; then
      ssh admin@${A1_HOST} "edit ethernet-interface ${ETH}; ip; reset address; reset ipv4-default-gateway; exit; exit"
   fi
done

echo "Build the desired IP defintions"
declare IP=""
declare IP_SUBNET=""
declare GATEWAY_STANZA=""

for  ETH in ${ETH_INTF} 
do
   if [ "${ISM_AUTOMATION}" == "TRUE" ] ; then
      IP_SUBNET="20"
   else
      IP_SUBNET="23"
   fi
   GATEWAY_STANZA=""
   if [[ "${ETH}" == "eth"* ]]; then
      GET_IP_10NET;
      IP="10.10.${IP_10NET}.${IP_F4}"
   elif [ "${ETH}" == "mgt0" ]; then
      IP="${A1_HOST}"
      IP_SUBNET="24"
      GATEWAY_STANZA="ipv4-default-gateway; gateway ${GATEWAY_IP}; exit"
   elif [ "${ETH}" == "mgt1" ]; then
      GET_IP_10NET;
      IP="10.10.${IP_10NET}.${IP_F4}"
   elif [ "${ETH}" == "ha0" ]; then
      GET_IP_10NET;
      IP="10.11.${IP_10NET}.${IP_F4}"
   elif [ "${ETH}" == "ha1" ]; then
      GET_IP_10NET;
      IP="10.12.${IP_10NET}.${IP_F4}"
   else
      echo "ERROR:  WHAT IS ETH ${ETH}"
      GET_IP_10NET;
      IP="10.10.${IP_10NET}.${IP_F4}"
   fi
#set -x
   ssh admin@${A1_HOST} "edit ethernet-interface ${ETH}; ip; reset address; reset ipv4-default-gateway; address ${IP}/${IP_SUBNET}; ${GATEWAY_STANZA}; exit; exit"
   ssh admin@${A1_HOST} "enable ethernet-interface ${ETH}"  
   export A1_${ETH}=${IP}
#set +x

   echo "Pinging ${ETH} @ ${IP} until it repsonds"
   declare -i TRY=0
   declare -i PINGOK=0
   declare -i PINGTIMEOUT=5
   declare -i PINGTRY=9
   while [ 1 ]
   do
      ((TRY++))
      ping -c1 ${IP} &> /dev/null
      if [[ $? != 0 ]]; then
         if [[ "${TRY}" -le "${PINGTRY}" ]]; then
            echo " Server ${IP} is not pingable on attempt ${TRY}, sleeping and will retry in ${PINGTIMEOUT} secs.  Local Time:  "`date`
            sleep ${PINGTIMEOUT}
         else
            echo " Server ${IP} is not pingable after ${TRY} attempts, ABORTING _ FAILURE.  Local Time:  "`date`
            exit 69
         fi
      else
            PINGOK=1
            echo " Server ${IP} is pingable on attempt ${TRY}, Local Time:  "`date`
            break
      fi
   done


done



