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
printMsg () {
   if [ DEBUG ];then
      echo "`date` :: $@"
   fi

}

########################################################################
# getAggregatePolicy ()
#  Inputs:
#   ASSUMES: Aggregate Interface name (generally from AGG_ETH[0])
#
#  Outputs:
#   RC of Error from Show Aggregate Interface
#   89  - Failure to recognize the active AGG_POLICY value 
#
#  Returns:
#   AGG_POLICY - Return value of 'aggregate-policy'
#
### 
getAggregatePolicy () {
   printMsg  "  ENTER: getAggregatePolicy ()  ........." 
   export AGG_POLICY=""
   RC_MSG=`ssh admin@${THE_HOST} "show aggregate-interface ${AGG_ETH[0]}" `
   RC=$?

   if [ DEBUG ]; then echo "show agg-intf:  ${AGG_ETH[0]}"; echo "${RC_MSG}"; fi;
   if [ "${RC}" -ne 0 ]; then
      echo "ERROR:  SHOW AGGREGATE-INTERFACE ${AGG_ETH[0]} FAILED with RC='${RC}' and MSG='${RC_MSG}'  Aborting!"
      exit ${RC}
   else
#set -x
      AGG_POLICY=`echo $RC_MSG | awk -F'[ ]+' '{for (i=1;i<NF;i++) if ($i=="aggregation-policy") print $(i+1) }'`

#      for ITEM in ${RC_MSG}
#      do
#         if [[ "${ITEM}" =~ "aggregation-policy"* ]]; then
#            AGG_POLICY=`echo ${ITEM} |cut -d ' ' -f 2 `
#            printMsg "... AGG_POLICY=${AGG_POLICY}"
#            break
#         fi   
#      done

      if [ -z "${AGG_POLICY}" ]; then
         echo "ERROR:  Unable to obtain the AGG_POLICY from ${AGG_ETH[0]}"
         exit 89
      fi
set +x
   fi
   printMsg "AGG_POLICY: ${AGG_POLICY}"
}

########################################################################
# getAggregateMembers ()
#  Inputs:
#   ASSUMES: Aggregate Interface name (generally from AGG_ETH[0])
#     The invoker has defined:
#       declare -ax AGG_MEMBERS=()  	#getAggregateMembers()
#
#  Outputs:
#   RC of Error from Show Aggregate Interface
#   89  - Failure to recognize the MEMBER list value 
#
#  Returns:
#   AGG_MEMBERS - Return value of 'member'
#
### 
getAggregateMembers () {
   printMsg  "  ENTER: getAggregateMembers ()  ........." 

   unset AGG_MEMBERS
   set -x
   RC_MSG=`ssh admin@${THE_HOST} "show aggregate-interface ${AGG_ETH[0]}" `
   RC=$?
   set +x

   if [ "${RC}" -ne 0 ]; then
      echo "ERROR:  SHOW AGGREGATE-INTERFACE ${AGG_ETH[0]} FAILED with RC='${RC}' and MSG='${RC_MSG}'  Aborting!"
      exit ${RC}
   else
#set -x
      LIST=`echo $RC_MSG | awk -F'[ ]+' '{for (i=1;i<NF;i++) if ($i=="member") for (j=i+1;j<=NF;j++){out=out" "$j}; print out }'`
      for ITEM in ${LIST}
      do
         echo "ITEM:${ITEM}"
         if  [[ "${ITEM:0:1}" == '"' ]]; then
            AGG_MEMBERS=(${AGG_MEMBERS[@]} ${ITEM} )
            printMsg "... AGG_MEMBERS=${AGG_MEMBERS[@]}"
         else  # first parameter with a  starting quote is a new KEY, not a value of 'member'
            break
         fi

      done 

      if [ "${#AGG_MEMBERS[@]}" == 0 ]; then
         echo "ERROR:  Unable to obtain the AGG_MEMBERS from ${AGG_ETH[0]}"
         exit 89
      fi
#set +x
   fi
   echo "GOT:  AGG_MEMBERS: ${AGG_MEMBERS[@]}"

}

########################################################################
#  checkIPOperational
#     ping the NW Interface IP address to verify it is operational
# 
#  Input:
#  $1 - IP Address 
#
###
checkIPOperational () {
   printMsg "  ENTER:  checkIPOperational()  ........."

   IP=${1}
   echo "Pinging ${IP}  until it repsonds - Local Time: "`date`
   declare -i TRY=0
   declare -i TRYMAX=15
   declare -i PINGOK=0
   declare -i PINGTIMEOUT=3
   while [ 1 ]
   do
      ((TRY++))
      ping -c1 ${IP} &> /dev/null
      if [[ $? != 0 ]]; then
         if [[ "${TRY}" -le ${TRYMAX} ]]; then
            printMsg " Server ${IP} is not pingable on attempt ${TRY}, sleeping and will retry in ${PINGTIMEOUT} secs.  Local Time:  "`date`
            sleep ${PINGTIMEOUT}
         else
            echo " Server ${IP} is not pingable after ${TRY} attempts, ABORTING _ FAILURE.  Local Time:  "`date`
            exit 79
         fi
      else
            PINGOK=1
            echo " Server ${IP} is pingable on attempt ${TRY}, Local Time:  "`date`
            break
      fi
   done

}

########################################################################
#  deleteAggregates () - Deletes existing Aggregate-Interfaces
#
#  Assumes:  THE_HOSTID
### 
deleteAggregates () {
   printMsg "  ENTER:  deleteAggregates()  ........."

   AGG_INDEX=1
   AGG_INTF_RAW=` ssh admin@${THE_HOST} "list aggregate-interface" `
   declare -a AGG_INTF=`echo ${AGG_INTF_RAW}`
  

   if [ ! -z "${AGG_INTF}" ]; then

      for AGG in ${AGG_INTF}
      do
         # MUST Remove IP before Aggregrate can be deleted
         RC_MSG=`ssh admin@${THE_HOST} "edit aggregate-interface ${AGG}; reset ip; exit"`
         RC=$?
         if [ "${RC}" -ne 0 ]; then
            echo "ERROR:  'RESET IP' AGGREGATE-INTERFACE FAILED with RC='${RC}' and MSG='${RC_MSG}'  Aborting!"
            exit ${RC}
         fi

         RC_MSG=`ssh admin@${THE_HOST} "delete aggregate-interface ${AGG}"`
         RC=$?
         if [ "${RC}" -ne 0 ]; then
            echo "ERROR:  DELETE AGGREGATE-INTERFACE FAILED with RC='${RC}' and MSG='${RC_MSG}'  Aborting!"
            exit ${RC}
         fi
         printMsg "Deleted aggregate-interface ${AGG}"
      done
   fi 

   while ( true )
   do   
      THIS_ETH_COUNT=$(eval echo \$${THE_HOSTID}_AGG${AGG_INDEX}_LINK )
      if [ ! -z "${THIS_ETH_COUNT}" ]; then

         # Get the SWITCH CONFIG
         getAggregateDefinition ${THE_HOSTID} ${AGG_INDEX};

         for INDEX in $(eval echo "{1..${THIS_ETH_COUNT}}")
         do
            echo "SWITCH: ${SWITCH_IP[${INDEX}]}  PORT: ${SWITCH_PORT[${INDEX}]}  --  LACP MODE OFF"
            if [ DEBUG ]; then set -x; fi;
            LACP_MSG=`sshpass -p ${SWITCH_PSWD[${INDEX}]} ssh ${SWITCH_UID[${INDEX}]}@${SWITCH_IP[${INDEX}]} "enable; configure terminal; interface port ${SWITCH_PORT[${INDEX}]}; lacp mode off; exit; exit; write; disable; show interface port ${SWITCH_PORT[${INDEX}]} lacp; exit; logoff; " ` 
            LACP_RC=$?    # actually the sshpass RC, poo!
            if [ DEBUG ]; then set +x; fi;
#            printf "\n   SWITCH RC: %s  with MSG: \n %s" ${LACP_RC} ${LACP_MSG}
            printf "\n   SWITCH RC: %s \n" ${LACP_RC} 

         done
         (( AGG_INDEX+=1))
      else
         break
      fi
   done

}


########################################################################
#  getAggregateDefinition
#     Build the variables in the OUTPUT list
#
#  ASSUMES:
#  AGG_IP_LIST pre exists as an ARRAY type and EXPORTed to the Env.
#  declare -ax AGG_IP_LIST=()
# 
#  Input:
#   $1 - THE_HOSTID = [ A1|A2 ] as used in ISMsetup.sh
#   $2 - AGG_INTF: [ 1|2 ] INDEX for this Aggregate Information in ISMsetup.sh
#
#
#  Output:
#   AGG_ETH_LIST : List of ethernet interface names
#   AGG_ETH[]    : Array of ALL InterfaceNames agg name is index [0], eth names follow [1], [2], ...
#   AGG_IPV4     : ISMSetup IPv4 Address for Aggregate
#   AGG_IPV6     : ISMSetup IPv6 Address for Aggregate
#   AGG_IP_LIST  : Aggregate IPv4 without NetMask (Used for PINGS)  Accumulates over ALL Aggregates
#   AGG_IP       : Aggregate IPv4 and IPv6 with NetMask  (Used for Creates)
#   AGG_KEY      : Used for the SWITCH Config, must be unique, using IP Address part 3 & 4 (key length limitation)
#   SWITCH_PORT  : Array of Switch ports by LINK
#   SWITCH_IP    : Array of Switch IPs by LINK
#   SWITCH_UID   : Array of Switch user ids by LINK
#   SWITCH_PSWD  : Array of Switch passwords by LINK
#
###
getAggregateDefinition () {
   printMsg "  ENTER:  getAggregateDefinition()  ........."

   declare THE_HOSTID=$1
   declare AGG_INTF=$2
   unset AGG_ETH
   unset AGG_ETH_RAW


   THIS_ETH_COUNT=$(eval echo \$${THE_HOSTID}_AGG${AGG_INTF}_LINK)
   declare -i AGG_ETH_COUNT=`echo ${THIS_ETH_COUNT}`
   for ETH in $(eval echo "{1..${THIS_ETH_COUNT}}");
   do
      THIS_LINE=$(eval echo \$${THE_HOSTID}_AGG${AGG_INTF}_LINK${ETH})
      THIS_ETH=`echo ${THIS_LINE} | cut -d ',' -f 1`
      if [ ${ETH} -eq 1 ]; then
         AGG_ETH[0]=${THIS_ETH}_agg
      fi
      printMsg "${THIS_ETH} is LINK${ETH} "

      SWITCH_PORT[${ETH}]=`echo ${THIS_LINE} | cut -d ',' -f 2`
      SWITCH_IP[${ETH}]=`echo ${THIS_LINE} | cut -d ',' -f 3`
      SWITCH_UID[${ETH}]=`echo ${THIS_LINE} | cut -d ',' -f 4`
      SWITCH_PSWD[${ETH}]=`echo ${THIS_LINE} | cut -d ',' -f 5`

      AGG_ETH[${ETH}]=${THIS_ETH}
      AGG_ETH_RAW="${AGG_ETH_RAW}  ${THIS_ETH}"
   done

   export  AGG_ETH_LIST=`echo ${AGG_ETH_RAW}`

   printMsg "AGG_ETH_RAW=${AGG_ETH_RAW}; AGG_ETH_LIST Count= ${#AGG_ETH_LIST[*]} Elements: ${AGG_ETH_LIST[*]}  "
   echo "AGG_ETH=${AGG_ETH}; AGG_ETH Count= ${#AGG_ETH[*]} Elements: ${AGG_ETH[*]}  "

   export  AGG_IPV4=$(eval echo \$${THE_HOSTID}_AGG${AGG_INTF}_IPV4)
   export  AGG_IPV6=$(eval echo \$${THE_HOSTID}_AGG${AGG_INTF}_IPV6)
   export  AGG_IP=""
#   export  AGG_IP_LIST is cumulative acrosss all calls, do not reset it value inside this method.

   if [ ! -z "${AGG_IPV4}" ]; then
      AGG_IP="${AGG_IPV4} "
      AGG_IP_NOMASK=`echo ${AGG_IPV4} | cut -d '/' -f 1`
      AGG_IP_LIST="${AGG_IP_LIST} ${AGG_IP_NOMASK}"  
      AGG_KEY=`echo  ${AGG_IP_NOMASK} | cut -d '.' -f 3``echo  ${AGG_IP_NOMASK} | cut -d '.' -f 4`
   fi
   if [ ! -z "${AGG_IPV6}" ]; then
      AGG_IP="${AGG_IP} ${AGG_IPV6} "
   fi



}

########################################################################
# getClientid ()
#  Inputs:
#   $1 - The HOST ID [A1 | A2 ...] of the Appliance
#   $2 - Index of Aggregate Interface 
#   $3 - Index of the Client in ISMsetup.sh
#
#  Outputs:
#   CLIENT_HOST  - 'short hostname' of M${3}
#   CLIENT_PUBID - 
#   CLIENT_SUBID - 
#   CLIENT_STATID - 
#
#  Return Code: 
#   0  - success
#   99 - invoke error, missing parameters
### 
getClientid () {
   printMsg  "  ENTER: getClientid ()  ........."
   if [ -z $3 ]; then
      echo "ERROR on invoke of getClientid (), missing third parameter"
      exit 99
   fi
    
   TARGET_IP_NETMASK=$(eval echo \$${1}_AGG${2}_IPV4)
   TARGET_IP=`echo ${TARGET_IP_NETMASK}  | cut -d '/' -f 1`

   CLIENT_FULL_HOST=$(eval echo \$M${3}_HOSTNAME)
   export CLIENT_HOST=`echo ${CLIENT_FULL_HOST} | cut -d '.' -f 1`
   export CLIENT_PUBID="P"${CLIENT_HOST}.${TARGET_IP}
   export CLIENT_SUBID="S"${CLIENT_HOST}.${TARGET_IP}
   export CLIENT_STATID="X"${CLIENT_HOST}.${TARGET_IP}

}


export  MSG_COUNT=50000
export  QOS=2
#export  NQ="0:0:0:${MSG_COUNT}"		# "minQOS 0:maxQOS 0:QOS 1:QOS 2" - expected msg count based on QOS
