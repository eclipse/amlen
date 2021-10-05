#!/bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Automotive Telematics Scenarios 06"

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

source template2.sh
source ../svt_msgex/template6.sh


#----------------------------------------------------
# Test Cases
#----------------------------------------------------

echo M_COUNT=${M_COUNT}
declare MAX_COUNT=$((${M_COUNT}>6?${M_COUNT}:6))

# atelm_06_00
# test_template_add_test_single "SVT - clean_store" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|clean_store|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS" "cleanstore"

# atelm_06_01
printf -v tname "atelm_06_%02d" ${n}
test_template_define testcase=${tname} description=Valid_Group0_topic \
                     timeout=120 minutes=1 qos=2 order=false stats=false listen=false \
                     mqtt_subscribers=1 sub=10 pub=20 pubtopic=/svtGroup0/chat subtopic=/svtGroup0/chat mqtt_publishers=1 \
                     pubport=16211 subport=16212 pub_messages=100 sub_messages=100 

test_template6 ShowSummary

((n++))
# atelm_06_02
printf -v tname "atelm_06_%02d" ${n}
test_template_define testcase=${tname} description=User_belongs_to_501_groups \
                     timeout=120 minutes=1 qos=2 order=false stats=false listen=false \
                     mqtt_xsubscribers=1 sub=501 pub=10 pubtopic=/svtGroup0/chat subtopic=/svtGroup0/chat mqtt_publishers=1 \
                     pubport=16211 subport=16212 pub_messages=500 xsub_Nm=1

test_template6 ShowSummary
((n++))
# atelm_06_03
printf -v tname "atelm_06_%02d" ${n}
test_template_define testcase=${tname} description=User_belongs_to_201_groups \
                     timeout=120 minutes=1 qos=2 order=false stats=false listen=false \
                     mqtt_xsubscribers=1 sub=201 pub=10 pubtopic=/svtGroup0/chat subtopic=/svtGroup0/chat mqtt_publishers=1 \
                     pubport=16211 subport=16212 pub_messages=100 xsub_Nm=1

test_template6 ShowSummary
((n++))
# atelm_06_04
printf -v tname "atelm_06_%02d" ${n}
test_template_define testcase=${tname} description=Valid_Group1_topic \
                     timeout=120 minutes=1 qos=2 order=false stats=false listen=false \
                     mqtt_subscribers=1 sub=1000 pub=1002 pubtopic=/svtGroup1/chat subtopic=/svtGroup1/chat mqtt_publishers=1 \
                     pubport=16211 subport=16212 pub_messages=500 sub_messages=500 \
                     "sub_message=Received 500 messages" "pub_message=Published 500 messages to topic /svtGroup1/chat"

test_template6 ShowSummary
((n++))
# atelm_06_05
printf -v tname "atelm_06_%02d" ${n}
test_template_define testcase=${tname} description=Invalid_sub_topic \
                     timeout=120 minutes=1 qos=2 order=false stats=false listen=false \
                     mqtt_subscribers=1 sub=10 pub=20 pubtopic=/svtGroup0/chat subtopic=/svtGroup1/chat mqtt_publishers=1 \
                     pubport=16211 subport=16212 sub_Nm=1 pub_messages=100 \
                     "sub_message=Received 0 messages" "pub_message=Published 100 messages to topic /svtGroup0/chat"

test_template6 ShowSummary
((n++))
# atelm_06_06
printf -v tname "atelm_06_%02d" ${n}
test_template_define testcase=${tname} description=Invalid_pub_topic \
                     timeout=120 minutes=1 qos=2 order=false stats=false listen=false \
                     mqtt_subscribers=1 sub=11 pub=21 pubtopic=/svtGroup1/chat subtopic=/svtGroup0/chat mqtt_publishers=1 \
                     pubport=16211 subport=16212 sub_Nm=1 pub_Nm=1 "pub_message=Published 0 messages to topic /svtGroup1/chat" "sub_message=Received 0 messages"

test_template6 ShowSummary
((n++))
# atelm_06_07
printf -v tname "atelm_06_%02d" ${n}
test_template_define testcase=${tname} description=Fan_in_with_evil_pub_and_sub \
                     timeout=120 minutes=1 qos=2 order=false stats=false listen=false \
                     mqtt_subscribers=1 sub=10 pub=20 pubtopic=/svtGroup0/chat subtopic=/svtGroup0/chat mqtt_publishers=$((${MAX_COUNT}-3)) \
                     pubport=16211 subport=16212 evil_publishers=1 evil_subscribers=1 "evil_pubmessage=Published 0 messages to topic /svtGroup0/chat" \
                     evil_pubNm=1 evil_subNm=1 pub_messages=500 sub_messages=$(((${MAX_COUNT}-3)*500)) \
                     "pub_message=Published 500 messages to topic /svtGroup0/chat" "sub_message=Received $(((${MAX_COUNT}-3)*500)) messages"

test_template6 ShowSummary
((n++))
# atelm_06_08
printf -v tname "atelm_06_%02d" ${n}
test_template_define testcase=${tname} description=Fan_out_with_evil_pub_and_sub \
                     timeout=120 minutes=1 qos=2 order=false stats=false listen=false \
                     mqtt_subscribers=$((${MAX_COUNT}-3)) sub=14 pub=24 pubtopic=/svtGroup0/chat subtopic=/svtGroup0/chat mqtt_publishers=1 \
                     pubport=16211 subport=16212 evil_publishers=2 evil_subscribers=2 pub_messages=500 "evil_pubmessage=Published 0 messages to topic /svtGroup0/chat" \
                     evil_pubNm=1 evil_subNm=1 pub_messages=500 sub_messages=500 \
                     "pub_message=Published 500 messages to topic /svtGroup0/chat" "sub_message=Received 500 messages."


test_template6 ShowSummary
((n++))
# atelm_06_09
printf -v tname "atelm_06_%02d" ${n}
test_template_define testcase=${tname} description=Many_to_many_with_evil_pub_and_sub \
                     timeout=180 minutes=1 qos=2 order=false stats=false listen=false \
                     mqtt_subscribers=${MAX_COUNT} sub=15 subtopic=/svtGroup0/chat subport=16212 "sub_message=Received $((${MAX_COUNT}*500)) messages." sub_messages=$((${MAX_COUNT}*500)) \
                     mqtt_publishers=${MAX_COUNT} pub=25 pubtopic=/svtGroup0/chat pubport=16211 "pub_message=Published 500 messages to topic /svtGroup0/chat" pub_messages=500 
                     evil_publishers=3 "evil_pubmessage=Published 0 messages to topic /svtGroup0/chat" evil_pubNm=1 
                     evil_subscribers=3 "evil_submessage=Received 0 messages" evil_subNm=1 

test_template6 ShowSummary
#----------------------------------------------------

((n++))
# atelm_06_10
printf -v tname "atelm_06_%02d" ${n}
test_template_define testcase=${tname} description=InValid_SubGroups_subscribe \
                     timeout=120 minutes=1 qos=2 order=false stats=false listen=false \
                     mqtt_subscribers=1 sub=30 pub=60 pubtopic=/svtGroup0Sub1/chat subtopic=/svtGroup0Sub1/chat mqtt_publishers=1 \
                     pubport=16211 subport=16212 sub_Nm=1 pub_messages=100 "sub_message=Received 0 messages" "pub_message=Published 100 messages to topic /svtGroup0Sub1/chat"

test_template6 ShowSummary
((n++))
# atelm_06_11
printf -v tname "atelm_06_%02d" ${n}
test_template_define testcase=${tname} description=InValid_SubGroups_publish \
                     timeout=120 minutes=1 qos=2 order=false stats=false listen=false \
                     mqtt_subscribers=1 sub=60 pub=1060 pubtopic=/svtGroup0Sub1/chat subtopic=/svtGroup0Sub1/chat mqtt_publishers=1 \
                     pubport=16211 subport=16212 pub_Nm=1 sub_Nm=1 \
                     "pub_message=Published 0 messages to topic /svtGroup0Sub1/chat" "sub_message=Received 0 messages."

test_template6 ShowSummary
((n++))
# atelm_06_12
printf -v tname "atelm_06_%02d" ${n}
test_template_define testcase=${tname} description=Valid_SubGroups \
                     timeout=120 minutes=1 qos=2 order=false stats=false listen=false \
                     mqtt_subscribers=1 sub=51 pub=50 pubtopic=/svtGroup0Sub0/chat subtopic=/svtGroup0Sub0/chat mqtt_publishers=1 \
                     pubport=16211 subport=16212 pub_messages=500 sub_messages=500 \
                     "pub_message=Published 500 messages to topic /svtGroup0Sub0/chat" "sub_message=Received 500 messages."

test_template6 ShowSummary
((n++))
# atelm_06_13
printf -v tname "atelm_06_%02d" ${n}
test_template_define testcase=${tname} description=Valid_SubSubGroups \
                     timeout=120 minutes=1 qos=2 order=false stats=false listen=false \
                     mqtt_subscribers=1 sub=50 pub=51 pubtopic=/svtGroup0Sub0Sub1/chat subtopic=/svtGroup0Sub0Sub1/chat mqtt_publishers=1 \
                     pubport=16211 subport=16212 pub_messages=500 sub_messages=500 \
                     "pub_message=Published 500 messages to topic /svtGroup0Sub0Sub1/chat" "sub_message=Received 500 messages."


test_template6 ShowSummary
