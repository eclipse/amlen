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


scenario_set_name="MsgExpry Scenarios 01"

test_template_set_prefix "jmqtt_07_"

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



#----------------------------
# setup ha and cluster
#----------------------------
printf -v tname "cluster_01_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - setup ha and cluster"
timeouts[${n}]=600

component[1]="cAppDriverLogWait,m1,-e|./setup_all.py,"

test_template_finalize_test

((++n))

#----------------------------
# 
#----------------------------
printf -v tname "cluster_01_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - "
timeouts[${n}]=600

component[1]="cAppDriverLogWait,m1,-e|./failover_all.py,"

test_template_finalize_test

((++n))

#----------------------------
# 
#----------------------------
printf -v tname "cluster_01_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - "
timeouts[${n}]=600

component[1]="cAppDriverLogWait,m1,-e|./failback_all.py,"

test_template_finalize_test

((++n))

#----------------------------
# 
#----------------------------
printf -v tname "cluster_01_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - "
timeouts[${n}]=600

component[1]="cAppDriverLogWait,m1,-e|./remove_one.py,"

test_template_finalize_test

((++n))

#----------------------------
# 
#----------------------------
printf -v tname "cluster_01_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - "
timeouts[${n}]=600

component[1]="cAppDriverLogWait,m1,-e|./add_one.py,"

test_template_finalize_test

((++n))

#----------------------------
# 
#----------------------------
printf -v tname "cluster_01_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - "
timeouts[${n}]=600

component[1]="cAppDriverLogWait,m1,-e|./disablecluster_all.py,"

test_template_finalize_test

((++n))

#----------------------------
# 
#----------------------------
printf -v tname "cluster_01_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - "
timeouts[${n}]=600

component[1]="cAppDriverLogWait,m1,-e|./disablehanocluster_all.py,"

test_template_finalize_test

((++n))

#----------------------------
# 
#----------------------------
printf -v tname "cluster_01_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - "
timeouts[${n}]=600

component[1]="cAppDriverLogWait,m1,-e|./enableclusternoha_all.py,"

test_template_finalize_test

((++n))

#----------------------------
# 
#----------------------------
printf -v tname "cluster_01_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - "
timeouts[${n}]=600

component[1]="cAppDriverLogWait,m1,-e|./disableclusternoha_all.py,"

test_template_finalize_test

((++n))

#----------------------------
# 
#----------------------------
printf -v tname "cluster_01_%02d" ${n}
xml[${n}]=${tname}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - "
timeouts[${n}]=600

component[1]="cAppDriverLogWait,m1,-e|./cleanstore_all.py,"

test_template_finalize_test

((++n))
