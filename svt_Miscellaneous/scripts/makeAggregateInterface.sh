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
buildAggregateInterface () {
   printMsg "  ENTER: buildAggregateInterface()  ........."
   unset AGG_ETH_RAW
   unset AGG_ETH
   AGG_INTF="${1}"
   AGG_POLICY="${2}"


   getAggregateDefinition  ${THE_HOSTID} ${AGG_INTF} ;

   echo "Clean the existing IPs in list: ${AGG_ETH_LIST}"


   for ETH in ${AGG_ETH_LIST}
   do
      if [ "${ETH}" != "mgt0" ]; then
         ssh admin@${THE_HOST} "edit ethernet-interface ${ETH}; reset ip; aggregate-candidate true; exit"
         ssh admin@${THE_HOST} "enable ethernet-interface ${ETH}"
         ssh admin@${THE_HOST} "show ethernet-interface ${ETH}"
         ssh admin@${THE_HOST} "status ethernet-interface ${ETH}"
         echo "--------------------------------------------------"
      else
         echo "WARNING:  mgt0 was listed to have IP Reset - that is not gonna happen."
      fi

   done
   



   if [ "${AGG_POLICY}" == "active-backup" ]; then
      RC_MSG=`ssh admin@${THE_HOST} "create aggregate-interface ${AGG_ETH[0]}; member ${AGG_ETH_LIST}; primary-member ${AGG_ETH[1]}; aggregation-policy ${AGG_POLICY}; ip; address ${AGG_IP}; exit; exit;"`
      RC=$?
   elif [ "${AGG_POLICY}" == "lacp" ]; then
      RC_MSG=`ssh admin@${THE_HOST} "create aggregate-interface ${AGG_ETH[0]}; member ${AGG_ETH_LIST}; primary-member ${AGG_ETH[1]}; aggregation-policy ${AGG_POLICY}; ip; address ${AGG_IP}; exit; exit;"`
      RC=$?
#     SWITCH CONFIG
      for INDEX in $(eval echo {1..$((${#AGG_ETH[*]}-1))})
      do
          echo "SWITCH: ${SWITCH_IP[${INDEX}]}  PORT: ${SWITCH_PORT[${INDEX}]}  --  LACP MODE ACTIVE"
          set -x
          LACP_MSG=`sshpass -p ${SWITCH_PSWD[${INDEX}]} ssh ${SWITCH_UID[${INDEX}]}@${SWITCH_IP[${INDEX}]} "enable; configure terminal; interface port ${SWITCH_PORT[${INDEX}]}; lacp mode active; lacp key ${AGG_KEY}; port-channel min-links 1; exit; exit; write; disable; show interface port ${SWITCH_PORT[${INDEX}]} lacp; exit; logoff;" `
          LACP_RC=$?
          set +x
#            printf "\n   SWITCH RC: %s  with MSG: \n %s" ${LACP_RC} ${LACP_MSG}
          printf "\n   SWITCH RC: %s \n" ${LACP_RC} 

      done
   else
      RC_MSG=`ssh admin@${THE_HOST} "create aggregate-interface ${AGG_ETH[0]}; member ${AGG_ETH_LIST}; primary-member ${AGG_ETH[1]}; aggregation-policy ${AGG_POLICY}; ip; address ${AGG_IP}; exit; exit;"`
      RC=$?
   fi

   if [ "${RC}" -ne 0 ]; then
      echo "ERROR:  CREATE AGGREGATE-INTERFACE ${AGG_ETH[0]} FAILED with RC='${RC}' and MSG='${RC_MSG}'  Aborting!"
      exit ${RC}
   else
      printMsg "Created aggregate-interface ${AGG_ETH[0]}"
      ssh admin@${THE_HOST} "show aggregate-interface ${AGG_ETH[0]};"
      ssh admin@${THE_HOST} "status aggregate-interface ${AGG_ETH[0]};"
   fi

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

if [ -z "${THE_HOST}" ];then
   echo "ERROR:  Failed to get the Appliance ID, A1_HOST or A2_HOST. ABORTING! "
   exit 69
else
   echo "Clean and Configure Aggregate Interfaces on ${THE_HOST} Appliance."
fi

declare -a AGG_ETH_RAW=()
declare -a AGG_ETH=()

# Blantantly delete any preexisting aggregate-interfaces, none are ever expected actually.
deleteAggregates;

# Verify all the Ethernet Interfaces are present
EXPECTED_ETH_INTF=( eth0 eth1 eth2 eth3 eth4 eth5 eth6 eth7 ha0 ha1 mgt0 mgt1 )
ETH_INTF_RAW=` ssh admin@${THE_HOST} "list ethernet-interface" `
declare -a ETH_INTF=`echo ${ETH_INTF_RAW}` 

if [ "${ETH_INTF[*]}" != "${EXPECTED_ETH_INTF[*]}" ]; then
   echo "ERROR:  There is a serious network failure!  NOT ALL ETH Interfaces discovered!"
   echo "        Expect Eth List:  '${EXPECTED_ETH_INTF[*]}'.    YET FOUND THIS:"
   ssh admin@${THE_HOST} "list ethernet-interface"
   exit 99
fi

echo "Build the list of ETH members in Aggregate Interface"

# AGG_IP_LIST is built from IPV4 IPs in buildAggregateInterface()
# must be EXPORTED for getAggregateDefinition()
declare -ax AGG_IP_LIST=()

# build A1_AGG'1'_ aggregate definition
THIS_ETH_COUNT=$(eval echo \$${THE_HOSTID}_AGG1_LINK)
if [ ! -z "${THIS_ETH_COUNT}" -a "${THIS_ETH_COUNT}" -ne 0 ]; then
   buildAggregateInterface 1 "lacp";
fi

# build A1_AGG'2'_ aggregate definition
THIS_ETH_COUNT=$(eval echo \$${THE_HOSTID}_AGG2_LINK)
if [ ! -z "${THIS_ETH_COUNT}" -a "${THIS_ETH_COUNT}" -ne 0 ]; then
   buildAggregateInterface 2 "active-backup";
fi

# Verify the Aggregate Interfaces are ready for Operation by pinging
for IP in ${AGG_IP_LIST}
do
   checkIPOperational ${IP}
done

echo "PASS: ${0} completed Successfully"
exit 0



