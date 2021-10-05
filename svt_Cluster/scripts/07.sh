#!/bin/bash

if [[ -f "../testEnv.sh" ]]
then
  # Official ISM Automation machine assumed
  source ../testEnv.sh
else
  # Manual Runs need to build Customized ISMSetup for THIS param, SSH environment file and Tag Replacement
  source ../scripts/ISMsetup.sh
fi

source ../scripts/commonFunctions.sh
#source template6.sh

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios


scenario_set_name="Cluster Scenarios 07"

test_template_set_prefix "cluster_07_"

typeset -i n=0

# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#   Tool SubController:
#               component[x]=<subControllerName>,<machineNumber_ismSetup>,<config_file>
# or    component[x]=<subControllerName>,<machineNumber_ismSetup>,"-o \"-x <param> -y <params>\" "
#       where:
#   <SubControllerName>
#               SubController controls and monitors the test case running on the target machine.
#   <machineNumber_ismSetup>
#               m1 is the machine 1 in ismSetup.sh, m2 is the machine 2 and so on...
#
# Optional, but either a config_file or other_opts must be specified
#   <config_file> for the subController
#               The configuration file to drive the test case using this controller.
#       <OTHER_OPTS>    is used when configuration file may be over kill,
#                       parameters are passed as is and are processed by the subController.
#                       However, Notice the spaces are replaced with a delimiter - it is necessary.
#           The syntax is '-o',  <delimiter_char>, -<option_letter>, <delimiter_char>, <optionValue>, ...
#       ex: -o_-x_paramXvalue_-y_paramYvalue   or  -o|-x|paramXvalue|-y|paramYvalue
#
#   DriverSync:
#       component[x]=sync,<machineNumber_ismSetup>,
#       where:
#               <m1>                    is the machine 1
#               <m2>                    is the machine 2
#
#   Sleep:
#       component[x]=sleep,<seconds>
#       where:
#               <seconds>       is the number of additional seconds to wait before the next component is started.
#
#



#----------------------------------------------------
# Test Cases
#----------------------------------------------------

echo M_COUNT=${M_COUNT}
echo A_COUNT=${A_COUNT}



PPORT=16102
SPORT=16102

for QOS in 0 1 2 ; do

########################3
# n=2
########################3

declare MESSAGES=5000
declare MINUTES=20
timeouts[$n]=1260

declare lc=0

printf -v tname "cluster_07_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - Fan In of $((${MESSAGES}*${A_COUNT}/2*$((${M_COUNT}-1)))) messages"

component[$((++lc))]="javaAppDriverWait,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:${SPORT}+tcp://${A2_IPv4_1}:${SPORT}|-i|u0000002|-u|u0000002|-p|imasvtest|-c|true|-e|false|-Ns|1"

component[$((++lc))]="javaAppDriver,m1,-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|tcp://${A1_IPv4_1}:${SPORT}+tcp://${A2_IPv4_1}:${SPORT}|-i|u0000002|-u|u0000002|-p|imasvtest|-c|false|-O|-e|true|-n|$((${MESSAGES}*${A_COUNT}/2*$((${M_COUNT}-1))))|-Nm|${MINUTES}|-receiverTimeout|0:0"
test_template_compare_string[${lc}]="Message Order Pass"


count=3
for i in $(eval echo {2..${M_COUNT}}); do
  j=1; while [ $j -lt ${A_COUNT} ]; do
    k=$(($j+1))
    AP=$(eval echo \$\{A${j}_IPv4_1\})
    AS=$(eval echo \$\{A${k}_IPv4_1\})
    printf -v id "u%07d" ${count}
    component[$((++lc))]="javaAppDriver,m$i,-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|${QOS}|-t|/svtGroup0/chat|-s|tcp://${AP}:${PPORT}+tcp://${AS}:${PPORT}|-i|${id}|-u|${id}|-p|imasvtest|-O|-n|${MESSAGES}|-w|500"
    test_template_compare_string[${lc}]="Published ${MESSAGES} messages"
    ((j+=2))
    ((count+=1))
  done
done

component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000002|-k|PublishedMsgs|-c|lt|-v|10000|-w|5|-m|180"
component[$((++lc))]="cAppDriverLogWait,m1,-e|./failover.py,-o|-a|1"

component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000002|-k|PublishedMsgs|-c|lt|-v|5000|-w|5|-m|90"
component[$((++lc))]="cAppDriverLogWait,m1,-e|./failover.py,-o|-a|7"

component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-i|u0000002|-k|PublishedMsgs|-c|lt|-v|10000|-w|5|-m|180"
component[$((++lc))]="cAppDriverLogWait,m1,-e|./failover.py,-o|-a|2"

component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-i|u0000002|-k|PublishedMsgs|-c|lt|-v|5000|-w|5|-m|180"
component[$((++lc))]="cAppDriverLogWait,m1,-e|./failover.py,-o|-a|8"



for i in $(eval echo {1..${M_COUNT}}); do
  component[$((++lc))]="searchLogsEnd,m$i,${xml[$n]}_m$i.comparetest,9"
done

test_template_finalize_test




  echo
  echo ================================================================================================
  echo
  echo xml[${n}]=\"${xml[${n}]}\"
  echo test_template_initialize_test \"${xml[${n}]}\"
  echo scenario[${n}]=\"${scenario[${n}]}\"
  echo timeouts[${n}]=${timeouts[${n}]}

  echo
  for i in ${!component[@]}; do
    echo component[$i]=\"${component[$i]}\"
  done

  echo
  for i in ${!test_template_compare_string[@]}; do
    echo test_template_compare_string[${i}]=\"${test_template_compare_string[${i}]}\"
  done

  echo
  for i in ${!test_template_metrics_v1[@]}; do
    echo test_template_metrics_v1[${i}]=\"${test_template_metrics_v1[${i}]}\"
  done

  echo
  echo test_template_runorder=\"${test_template_runorder}\"
  echo test_template_finalize_test
  echo
  echo ================================================================================================
  echo


((++n))

done

unset lc
unset MESSAGES
unset MINUTES
