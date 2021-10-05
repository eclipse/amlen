#!/bin/bash  -x
. ./commonAggregateInterface.sh
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
buildEthernetInterface () {
#set -x
   # 'EXPECTED_ETH_INTF" list is mapped to the ISMsetup.sh IPV4_#/IPV6_# Index values 
   #                                  ( eth0 eth1 eth2 eth3 eth4 eth5 eth6 eth7 ha0 ha1 mgt0 mgt1 )
   declare -a EXPECTED_ETH_INTF_INDEX=( 1    2    3    4    5    6    7    8    HA0 HA1 HOST 9 )
   declare    IPV4_NETMASK="/23"
   declare    IPV6_NETMASK="/10"
   declare -a AGG_ETH_RAW=""
   declare -a AGG_ETH=""
   AGG_INTF="${1}"

   THIS_ETH_COUNT=$(eval echo \$${THE_HOSTID}_AGG${AGG_INTF}_LINK)
   declare -i AGG_ETH_COUNT=`echo ${THIS_ETH_COUNT}`
   for ETH in $(eval echo "{1..${THIS_ETH_COUNT}}");
   do
      THIS_LINE=$(eval echo \$${THE_HOSTID}_AGG${AGG_INTF}_LINK${ETH})
      THIS_ETH=`echo ${THIS_LINE} | cut -d ',' -f 1`
      for (( i=0 ;  i <= ${#EXPECTED_ETH_INTF_INDEX[*]} ; i++ ))
      do
         if [ "${EXPECTED_ETH_INTF[${i}]}"  != "${THIS_ETH}" ]; then
             continue;
         fi
         
         IDX=${EXPECTED_ETH_INTF_INDEX[${i}]}
         if [ "${THIS_ETH}" == "mgt0" ]; then
            THIS_IPV4=$(eval echo \$${THE_HOSTID}_HOST)${IPV4_NETMASK}
            THIS_IPV6=""
         else
#            THIS_IPV4=$(eval echo \$${THE_HOSTID}_IPv4_${IDX}${IPV4_NETMASK})
            THIS_IPV4=$(eval echo \$${THE_HOSTID}_IPv4_${IDX})
            THIS_IPV4=`echo ${THIS_IPV4}${IPV4_NETMASK}`
#            THIS_IPV6=$(eval echo \$${THE_HOSTID}_IPv6_${IDX}${IPV6_NETMASK})
            THIS_IPV6=$(eval echo \$${THE_HOSTID}_IPv6_${IDX})
            THIS_IPV6=`echo ${THIS_IPV6}${IPV6_NETMASK}`
         fi

         printMsg "${THIS_ETH} is defined by A1_IPV*_${IDX} -  ${THIS_IPV4} ${THIS_IPV6} "
         RC_MSG=`ssh admin@${THE_HOST} "edit ethernet-interface ${THIS_ETH}; reset ip; reset aggregate-candidate; ip; address ${THIS_IPV4} ${THIS_IPV6}; exit; exit; "`
         RC=$?
# defect 85683        if [ "${RC}" -ne 0 ]; then
         if [ "${RC_MSG}" != "Entering \"ip\" mode" ]; then
            echo  "ERROR:  FAILED to CREATE ethernet-interface ${THIS_ETH} with IP(s): ${THIS_IPV4} ${THIS_IPV6}"
            echo  "RC=${RC} and Msg: ${RC_MSG}"
            exit ${RC}
         fi

         RC_MSG=`ssh admin@${THE_HOST} "enable ethernet-interface ${THIS_ETH}"`
         RC=$?
         if [ "${RC}" -ne 0 ]; then
            echo  "ERROR:  FAILED to ENABLE ethernet-interface ${THIS_ETH}. "
            echo  "RC=${RC} and Msg: ${RC_MSG}"
            exit ${RC}
         fi

         echo "Created ethernet-interface ${THIS_ETH} with IP(s): ${THIS_IPV4} ${THIS_IPV6}"
         
         ssh admin@${THE_HOST} "show ethernet-interface ${THIS_ETH}; "
         ssh admin@${THE_HOST} "status ethernet-interface ${THIS_ETH}; "

         break;
      done

   done
}


### MAIN ###########################################################################################
DEBUG=1
set +x
if [ -z "${A1_HOST}" ]; then
   printMsg "SOURCE ISMsetup.sh to get A1_HOST's value, this is unexpected!"
   . /niagara/test/scripts/ISMsetup.sh
fi
#set -x

if [ $# -ne 1 ]; then
   THE_HOST=${A1_HOST}
   THE_HOSTID="A1"
else
   THE_HOST=$(eval echo \$${1}_HOST)
   THE_HOSTID=${1}
fi 

declare RESTORE_ETH=0
if [ "$2" == "TRUE" ]; then
   RESTORE_ETH=1
fi

if [ -z "${THE_HOST}" ];then
   echo "ERROR:  Failed to get the Appliance ID, A1_HOST or A2_HOST. ABORTING! "
   exit 69
else
   echo "Clean and Configure Aggregate Interfaces on ${THE_HOST} Appliance."
fi


# AGG_IP_LIST is built from IPV4 IPs in buildEthernetInterface()
# must be EXPORTED for getAggregateDefinition()
declare -ax AGG_IP_LIST=()

# Blantantly delete any preexisting aggregate-interfaces, none are ever expected actually.
deleteAggregates;

# Verify all the Ethernet Interfaces are present
declare -a EXPECTED_ETH_INTF=( eth0 eth1 eth2 eth3 eth4 eth5 eth6 eth7 ha0 ha1 mgt0 mgt1 )
ETH_INTF_RAW=` ssh admin@${THE_HOST} "list ethernet-interface" `
declare -a ETH_INTF=`echo ${ETH_INTF_RAW}` 

if [ "${ETH_INTF[*]}" != "${EXPECTED_ETH_INTF[*]}" ]; then
   echo "ERROR:  There is a serious network failure!  NOT ALL ETH Interfaces discovered!"
   echo "        Expect Eth List:  '${EXPECTED_ETH_INTF[*]}'.    YET FOUND THIS:"
   ssh admin@${THE_HOST} "list ethernet-interface"
   exit 99
fi

echo "Build the list of ETH members in Aggregate Interface"
# rebuild eths of A1_AGG'1'_ aggregate definition
THIS_ETH_COUNT=$(eval echo \$${THE_HOSTID}_AGG1_LINK)
if [ ! -z "${THIS_ETH_COUNT}" -a "${THIS_ETH_COUNT}" -ne 0 ]; then
   buildEthernetInterface 1 ;
fi

# rebuild eths of A1_AGG'2'_ aggregate definition
THIS_ETH_COUNT=$(eval echo \$${THE_HOSTID}_AGG2_LINK)
if [ ! -z "${THIS_ETH_COUNT}" -a "${THIS_ETH_COUNT}" -ne 0 ]; then
   buildEthernetInterface 2 ;
fi

for IP in ${AGG_IP_LIST}
do
   checkIPOperational ${IP}
done

echo "PASS: ${0} completed Successfully"
exit 0



