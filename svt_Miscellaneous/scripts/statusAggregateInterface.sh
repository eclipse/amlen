#!/bin/bash -x 
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
do_loops () {
   printMsg  "  ENTER: do_loops()  ........."
#set -x
   LAST_LOOP=${1}
   for (( loop=0 ; loop <= ${LAST_LOOP} ; loop++ ))
   do

      for INDEX_INTF in $(eval echo {0..$((${#AGG_ETH[*]}-1))})
      do


         if [ ${INDEX_INTF} -eq 0 ]; then
            THIS_INTF="aggregate-interface"
         else
            THIS_INTF="ethernet-interface"
         fi

         rx_bytes=0
         rx_packets=0
         tx_bytes=0
         tx_packets=0

         RC_MSG_RAW=`ssh admin@${THE_HOST} "status ${THIS_INTF} ${AGG_ETH[${INDEX_INTF}]}"`
         RC=$?
         if [ "${RC}" -ne 0 ]; then
            echo "ERROR:  STATUS ${THIS_INTF} ${AGG_ETH[${INDEX_INTF}]} FAILED with RC='${RC}' and MSG='${RC_MSG}'  Aborting!"
            exit ${RC}
         else

            OLD_IFS=${IFS}
            IFS=' '
            IFS=${IFS:0:1}    #12 @ http://stackoverflow.com/questions/10586153/bash-split-string-into-array
      
            declare -a RC_MSG=( ${RC_MSG_RAW} ) ;
            IFS=${OLD_IFS}

   
            for x in "${RC_MSG[@]}"
            do
#               printMsg "   Parsing STATUS element[] = ${x}" 
      
               if [[ "${x}" =~ "rx_bytes"* ]]; then
                  this_rx_bytes=`echo ${x} | grep  rx_bytes | cut -d : -f 2`
                  rx_bytes=`echo ${this_rx_bytes}`
                  printf " %s = %s\n" ${x} ${rx_bytes}              
               fi


               if [[ "${x}" =~ "rx_packets"* ]]; then
                  this_rx_packets=`echo ${x} | grep  rx_packets | cut -d : -f2`
                  rx_packets=`echo ${this_rx_packets}`
                  printf " %s = %s\n" ${x} ${rx_packets}              
               fi
   
               if [[ "${x}" =~ "tx_bytes"* ]]; then
                  this_tx_bytes=`echo ${x} | grep  tx_bytes | cut -d : -f 2`
                  tx_bytes=`echo ${this_tx_bytes}`
                  printf " %s = %s\n" ${x} ${tx_bytes}              
               fi


               if [[ "${x}" =~ "tx_packets"* ]]; then
                  this_tx_packets=`echo ${x} | grep  tx_packets | cut -d : -f 2`
                  tx_packets=`echo ${this_tx_packets}`
                  printf " %s = %s\n" ${x} ${tx_packets}              
               fi
            done

            if [ ${rx_packets} -gt ${RX_PACKETS[${INDEX_INTF}]} ]; then
               echo "Loop ${loop}: ${AGG_ETH[${INDEX_INTF}]} received $((rx_packets - RX_PACKETS[${INDEX_INTF}] )) new packets - Total: ${rx_packets}; new bytes $((rx_bytes - RX_BYTES[${INDEX_INTF}] )) - Total:  ${rx_bytes} "
               RX_BYTES[${INDEX_INTF}]=${rx_bytes}
               RX_PACKETS[${INDEX_INTF}]=${rx_packets}

            elif [ ${rx_packets} -lt ${RX_PACKETS[${INDEX_INTF}]} ]; then
               echo "${AGG_ETH[${INDEX_INTF}]} reset RX statistics with failover on loop ${loop}: new 1st packet: ${RX_PACKETS[${INDEX_INTF}]}"
               RX_BYTES[${INDEX_INTF}]=${rx_bytes}
               RX_PACKETS[${INDEX_INTF}]=${rx_packets}

            else
               echo "${AGG_ETH[${INDEX_INTF}]} did not receive new RX packets on loop ${loop}: last packet: ${RX_PACKETS[${INDEX_INTF}]}"
         
            fi
   
   
            if [ ${tx_packets} -gt ${TX_PACKETS[${INDEX_INTF}]} ]; then
               echo "Loop ${loop}: ${AGG_ETH[${INDEX_INTF}]} sent     $((tx_packets - TX_PACKETS[${INDEX_INTF}] )) new packets - Total: ${tx_packets}; new bytes $((tx_bytes - TX_BYTES[${INDEX_INTF}] )) - Total:  ${tx_bytes} "
               TX_BYTES[${INDEX_INTF}]=`echo ${tx_bytes}`
               TX_PACKETS[${INDEX_INTF}]=`echo ${tx_packets}`

            elif [ ${tx_packets} -lt ${TX_PACKETS[${INDEX_INTF}]} ]; then
               echo "${AGG_ETH[${INDEX_INTF}]} reset TX statistics with failover on loop ${loop}: new 1st packet: ${TX_PACKETS[${INDEX_INTF}]}"
               TX_BYTES[${INDEX_INTF}]=${tx_bytes}
               TX_PACKETS[${INDEX_INTF}]=${tx_packets}

            else
               echo "${AGG_ETH[${INDEX_INTF}]} did not send new TX packets on loop ${loop}: last packet: ${TX_PACKETS[${INDEX_INTF}]}"
         
            fi
#set +x
            echo " --- --- `date`"
   
         fi
      done
      echo " --- --- "

      sleep 5
#read y
   done

}

########################################################################
#  enable_interface ()
#  Inputs:
#  $1 - ETH : INDEX IN LIST TO the Ethernet Interface to enable
### 
enable_interface () {
   printMsg  "  ENTER: enable_interface ()  ........."
   printMsg " ---  Attempt to Enable eth-intf # $1 in list: ${AGG_ETH[*]}"
   printf "\n \n TRY: ENABLE ETHERNET INTERFACE %s \n" ${AGG_ETH[${1}]}   

   RC_MSG=`ssh admin@${THE_HOST} "enable ethernet-interface ${AGG_ETH[${1}]}"`
   RC=$?
   if [ "${RC}" -ne 0 ]; then
      echo "ERROR:  ENABLE ETHERNET-INTERFACE FAILED with RC='${RC}' and MSG='${RC_MSG}'  Aborting!"
      exit ${RC}
   else
      printf "\n Successfully ENABLED ETHERNET INTERFACE %s \n\n" ${AGG_ETH[${1}]}   
   fi

   RC_MSG=`ssh admin@${THE_HOST} "show ethernet-interface ${AGG_ETH[${1}]}"`
   RC=$?

   if [ DEBUG ]; then echo "show eth-intf:  ${AGG_ETH[${1}]}"; echo "${RC_MSG}"; fi;
   if [ "${RC}" -ne 0 ]; then
      echo "ERROR:  SHOW ETHERNET-INTERFACE ${AGG_ETH[${1}]} FAILED with RC='${RC}' and MSG='${RC_MSG}'  Aborting!"
      exit ${RC}
   else
      if [[ "${RC_MSG}" =~ .*ethernet-interface\ ${AGG_ETH[${1}]}:\ \[Up\].* ]]; then
         printf "\n Successfully ENABLED ETHERNET INTERFACE %s \n\n" ${AGG_ETH[${1}]}
      else
         echo "ERROR:  ENABLE ETHERNET-INTERFACE ${AGG_ETH[${1}]} REALLY FAILED here is STATUS='${RC_MSG}'  Aborting!"
         exit 99
      fi   
   fi
  

}

########################################################################
#  disable_interface ()
#  Inputs:
#  $1 - ETH : INDEX IN LIST TO the Ethernet Interface to disable
### 
disable_interface () {
   printMsg  "  ENTER: disable_interface ()  ........."
   printMsg " ---  Attempt to Disable eth-intf # $1 in list: ${AGG_ETH[*]}"
   printf "\n \n TRY: DISABLE ETHERNET INTERFACE %s \n" ${AGG_ETH[${1}]}   

   RC_MSG=`ssh admin@${THE_HOST} "disable ethernet-interface ${AGG_ETH[${1}]}"`
   RC=$?
   if [ "${RC}" -ne 0 ]; then
      echo "ERROR:  DISABLE ETHERNET-INTERFACE FAILED with RC='${RC}' and MSG='${RC_MSG}'  Aborting!"
      exit ${RC}
   else
      printf "\n Successfully DISABLED ETHERNET INTERFACE %s \n\n" ${AGG_ETH[${1}]}   
   fi

   RC_MSG=`ssh admin@${THE_HOST} "show ethernet-interface ${AGG_ETH[${1}]}"`
   RC=$?

   if [ DEBUG ]; then echo "show eth-intf:  ${AGG_ETH[${1}]}"; echo "${RC_MSG}"; fi;
   if [ "${RC}" -ne 0 ]; then
      echo "ERROR:  SHOW ETHERNET-INTERFACE ${AGG_ETH[${1}]} FAILED with RC='${RC}' and MSG='${RC_MSG}'  Aborting!"
      exit ${RC}
   else
      if [[ "${RC_MSG}" =~ .*ethernet-interface\ ${AGG_ETH[${1}]}:\ \[Down\].* ]]; then
         printf "\n Successfully DISABLED ETHERNET INTERFACE %s \n\n" ${AGG_ETH[${1}]}
      else
         echo "ERROR:  DISABLE ETHERNET-INTERFACE ${AGG_ETH[${1}]} REALLY FAILED here is STATUS='${RC_MSG}'  Aborting!"
         exit 99
      fi   
   fi

}

########################################################################
#  restore_interface ()
#  Inputs:
#  $1 - ETH : INDEX IN LIST TO the Ethernet Interface to enable
### 
restore_interface () {
   printMsg  "  ENTER: restore_interface ()  ........."
   getAggregateMembers;

   printf "\n \n TRY: RE-ADD ETHERNET INTERFACE # %s, %s, to Aggregate: %s. Current List:  \n" ${1} ${AGG_ETH[${1}]} ${AGG_ETH[0]}
   echo "${AGG_MEMBERS[@]} "

   set -x
   RC_MSG=`ssh admin@${THE_HOST} "edit aggregate-interface ${AGG_ETH[0]};reset member; member ${AGG_ETH_LIST[*]};exit"`
   RC=$?
   set +x 

   if [ "${RC}" -ne 0 ]; then
      echo "ERROR:  EDIT AGGREGATE-INTERFACE FAILED with RC='${RC}' and MSG='${RC_MSG}'  Aborting!"
      exit ${RC}
   else
      printf "\n Successfully ADDED ETHERNET INTERFACE %s \n\n" ${AGG_ETH[${1}]}   
   fi

   set -x 
      RC_MSG=`ssh admin@${THE_HOST} "show aggregate-interface ${AGG_ETH[0]}" `
      RC=$?
   set +x

#   if [ "${RC}" -ne 0 ]; then
#      echo "ERROR:  SHOW AGGREGATE-INTERFACE ${AGG_ETH[0]} FAILED with RC='${RC}' and MSG='${RC_MSG}'  Aborting!"
#      exit ${RC}
#   else
#      for ITEM in ${RC_MSG}
#      do
#         if [[ "${ITEM}" =~ "member ${AGG_ETH_LIST}"* ]]; then
#            printf "\n Successfully READDED ETHERNET INTERFACE %s \n\n" ${AGG_ETH[${1}]}
#         else
#            echo "ERROR:  EDIT AGGREGATE-INTERFACE ${AGG_ETH[0]} REALLY FAILED here is STATUS='${RC_MSG}'  Aborting!"
#            exit 99
#         fi   
#      done
#   fi 

#read y
}

########################################################################
# remove_interface ()
#  Inputs:
#   $1 - ETH : INDEX IN LIST TO the Ethernet Interface to disable
#
#  Outputs:
#
#  Returns:
#   CURRENT_MEMBERS - list of 'active' ethernet interface members in the LACP Agg.
#
### 
remove_interface () {
   printMsg  "  ENTER: remove_interface ()  ........."
#set -x
   getAggregateMembers;

   printf "\n \n TRY: REMOVE ETHERNET INTERFACE # %s, %s, from the Agg %s list: \n"  ${1} ${AGG_ETH[${1}]} ${AGG_ETH[0]}
   echo "${AGG_MEMBERS[@]}"

   declare -ax CURRENT_MEMBERS=()
   for MEMBER in ${AGG_MEMBERS[@]}
   do
      if [ "${MEMBER}" == \"${AGG_ETH[${1}]}\" ];then
         continue
      else
         CURRENT_MEMBERS=(${CURRENT_MEMBERS[@]} ${MEMBER})
      fi
   done

   set -x
   RC_MSG=`ssh admin@${THE_HOST} "edit aggregate-interface ${AGG_ETH[0]}; reset member; member ${CURRENT_MEMBERS[@]}; exit;" `
   RC=$?
   set +x
   if [ "${RC}" -ne 0 ]; then
      echo "ERROR:  EDIT AGGREGATE-INTERFACE FAILED with RC='${RC}' and MSG='${RC_MSG}'  Aborting!"
      exit ${RC}
   else
      printf "\n Successfully EDITed AGGREGATE INTERFACE %s \n\n" ${AGG_ETH[0]}   
   fi

   set -x
   RC_MSG=`ssh admin@${THE_HOST} "show aggregate-interface ${AGG_ETH[0]}"`
   RC=$?
   set +x

#   if [ "${RC}" -ne 0 ]; then
#      echo "ERROR:  SHOW AGGREGATE-INTERFACE ${AGG_ETH[0]} FAILED with RC='${RC}' and MSG='${RC_MSG}'  Aborting!"
#      exit ${RC}
#   else
#      unset found_it
#      for ITEM in ${RC_MSG}
#      do
 #        if [[ "${ITEM}" == "member" ]]; then
#            j=0
#            for i in $(eval echo "1..${#CURRENT_MEMBERS[@]}" )
#            do 
#               shift
#               if [ "${ITEM}" == ${CURRENT_MEMBERS[${i}]} ]; then
#                  ((j+=1))
#                  continue
#               else
#                  break
#               fi
# 
 #           done
#            if [ ${i} == ${j} ]; then
#               printf "\n Successfully REMOVED ETHERNET INTERFACE %s from list: %s \n\n" ${AGG_ETH[${1}]} ${CURRENT_MEMBERS[@]}
#               found_it=1
#            fi
#         else
#            continue
#         fi   
#      done
#      if [ -z ${found_it} ]; then
#         echo "ERROR:  EDIT AGGREGATE-INTERFACE ${AGG_ETH[0]} REALLY FAILED here is STATUS='${RC_MSG}'  Aborting!"
#         exit 99
#      fi
#   fi

#read y
}



### MAIN ###########################################################################################
DEBUG=0
set +x
if [ -z "${A1_HOST}" ]; then
   printMsg "SOURCE ISMsetup.sh to get A1_HOST's value, this is unexpected!"
   . /niagara/test/scripts/ISMsetup.sh
fi

if [ $# -ne 1 ]; then
   THE_HOST=${A1_HOST}
   THE_HOSTID="A1"
else
   THE_HOST=$(eval echo \$${1}_HOST)
   THE_HOSTID=${1}
fi 

declare -i AGG_INDEX=1
if [ $# -ge 2 ]; then
   AGG_INDEX=${2}
fi

declare -i CLIENT_INDEX=1
if [ $# -ge 3 ]; then
   CLIENT_INDEX=${3}
fi

if [ -z "${THE_HOST}" ];then
   echo "ERROR:  Failed to get the Appliance ID, A1_HOST or A2_HOST. ABORTING! "
   exit 69
else
   echo "Monitor Statistics on Aggregate/Ethernet Interfaces on ${THE_HOST} Appliance."
fi

##### Define Variables whose value is set is function calls....
declare -ax AGG_IP_LIST=()  	#getAggregateDefinition()
declare -ax AGG_MEMBERS=()  	#getAggregateMembers()

getAggregateDefinition ${THE_HOSTID}  ${AGG_INDEX}

THE_IP=${AGG_IP_LIST[0]}

echo "Appliance: ${THE_HOST}, Aggregate-members ${AGG_ETH_LIST[*]} - `date`" 
echo "AGG_ETH: Count= ${#AGG_ETH[*]} Elements: ${AGG_ETH[*]}; IP: ${THE_IP}" 
#
declare -a MCAST=(      $(for i in $(eval echo  {0..${#AGG_ETH[*]}}); do echo 0; done) )
declare -a RX_BYTES=(   $(for i in $(eval echo  {0..${#AGG_ETH[*]}}); do echo 0; done) )
declare -a RX_PACKETS=( $(for i in $(eval echo  {0..${#AGG_ETH[*]}}); do echo 0; done) )
declare -a TX_BYTES=(   $(for i in $(eval echo  {0..${#AGG_ETH[*]}}); do echo 0; done) )
declare -a TX_PACKETS=( $(for i in $(eval echo  {0..${#AGG_ETH[*]}}); do echo 0; done) )

declare -i rx_bytes=0
declare -i rx_packets=0
declare -i tx_bytes=0
declare -i tx_packets=0

declare -i MAX_LOOPS=4

# Make some traffic even if MQTT is running....
if [ DEBUG ]; then
   ping -s 10000 ${THE_IP} &
   PID1=$!
   echo "$PID1 is pinging ${THE_IP}"
fi

getClientid ${THE_HOSTID} ${AGG_INDEX} ${CLIENT_INDEX} ;

WAIT_ON_SUB_READY=true
while ( ${WAIT_ON_SUB_READY} )
do
   echo "   ...Checking if Subscriber is ready..."
   PUBSTAT=`java svt.mqtt.mq.MqttSample -s tcp://${TARGET_IP}:16102 -a subscribe -e true -t /stat${TARGET_IP} -n 1 -receiverTimeout 1:1 -i ${CLIENT_STATID} -v`
   printMsg "----------------------------------------------------------------------------------"
   printMsg "${PUBSTAT}"
   printMsg "----------------------------------------------------------------------------------"
   if [[ "${PUBSTAT}" =~ .*\ \ MSG:Subscriber\ is\ READY.* ]]; then
      WAIT_ON_SUB_READY=false
      echo " TIME TO START MONITORING!!! YEA!!!"
   else
      echo "DID NOT FIND EXPECTED TEXT: 'Subscriber is READY'.  Try again, turn on DEBUG to dump PUBSTAT"
      
      DEBUG=1
      sleep 2
   fi
done 



PUBLISHING=true
getAggregatePolicy ${AGG_ETH[0]}

while ( ${PUBLISHING} )
do

   do_loops ${MAX_LOOPS} 

   for INDEX_ETH in $(eval echo {1..$((${#AGG_ETH[*]}-1))})
   do

      if [ "${AGG_POLICY}" == "\"active-backup\"" ]; then

         disable_interface ${INDEX_ETH}
         do_loops ${MAX_LOOPS}

         enable_interface ${INDEX_ETH}
         do_loops ${MAX_LOOPS}
    
      elif [ "${AGG_POLICY}" == "\"lacp\"" ]; then

         remove_interface ${INDEX_ETH}
         do_loops ${MAX_LOOPS}

         restore_interface ${INDEX_ETH}
         do_loops ${MAX_LOOPS}

      else
         echo "ERROR:  FAILED TO recognized the AGG_POLICY: ${AGG_POLICY}.  Aborting!"
         exit 89
      fi

   done
   printf "\n  ...Checking if Subscriber is done...\n"
set -x
   PUBSTAT=`java svt.mqtt.mq.MqttSample -s tcp://${TARGET_IP}:16102 -a subscribe -e true -t /stat${TARGET_IP} -receiverTimeout 1:1 -i ${CLIENT_STATID} -v `
set +x
   printMsg "----------------------------------------------------------------------------------"
   printMsg "${PUBSTAT}"
   printMsg "----------------------------------------------------------------------------------"
   if [[ "${PUBSTAT}" =~ .*\ \ MSG:Subscriber\ Completed.* ]]; then
      PUBLISHING=false
      echo " TIME TO QUIT!!! YEA!!!"

   fi

done

# Stop the PING traffic
if [ DEBUG ]; then
   kill -9 $PID1
fi

echo "PASS: Network Status Complete for $0 $@"

exit
