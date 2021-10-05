#!/bin/bash -x
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
      IP_10NET=0
   else
      IMM_IP_F3="174"
      IP_10NET=1
   fi 
   length=${length}-${#IP_F3}-1

   export A1_HOST=${A1_HOST}
   if [ "${A1_IMM}" == "" ]; then
      export A1_IMM="${A1_HOST:0:${length}}${IMM_IP_F3}.${IP_F4}"
   fi
   export IP_F4=${IP_F4}
#set +x
}


##### MAIN 

if [ $# -ne 1 ]; then
   echo "Provide the appliance's HOST IP address"
   read A1_HOST
else
   A1_HOST=$1
fi

GET_IP_F4;

if [ "${M1_IMA_SDK}" == "" ]; then
   export M1_IMA_SDK=/niagara/application/client_ship/lib
fi
#export  IP_F4=${IP_F4}
 
#export  A1_ HOST=${A1_HOST}
#export  A1_IMM=9.3.${IMM_IP_F3}.${IP_F4}
export  A1_ha0=10.11.${IP_10NET}.${IP_F4}
export  A1_ha1=10.12.${IP_10NET}.${IP_F4}
i=0
while [ "${i}" -le "7" ]
do
   ETH=`echo "A1_eth${i}"`
   export  ${ETH}=10.10.${IP_10NET}.${IP_F4}
   ((IP_10NET+=2))
   ((i+=1))
done

export A1_mgt1=10.10.${IP_10NET}.${IP_F4}


