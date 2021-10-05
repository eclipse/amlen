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


scenario_set_name="Cluster Scenarios 01"

test_template_set_prefix "cluster_01_"

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




#----------------------------------------------------

xml[${n}]="cluster_01"
scenario[${n}]="${xml[${n}]} - SVT Environment cleanup"
timeouts[${n}]=1200
test_template_initialize_test "${xml[${n}]}"

lc=0
for j in $(eval echo {1..${A_COUNT}}); do
  component[((++lc))]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh,-o|-c|./cluster.cli|-s|cleanup|-a|$j"
done

test_template_finalize_test

((n+=1))

#----------------------------------------------------
# Test Case 4 -  do setup environment
#----------------------------------------------------
xml[${n}]="SVT_IMA_ENV_03"
scenario[${n}]="${xml[${n}]} - SVT Environment setup"
timeouts[${n}]=1200
test_template_initialize_test "${xml[${n}]}"

lc=0
for j in $(eval echo {1..${A_COUNT}}); do
  component[((++lc))]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh,-o|-c|./cluster.cli|-s|setup|-a|$j"
done

test_template_finalize_test
((n+=1))


if false; then
#----------------------------------------------------
# Test Case 5 -  do setupldap environment on A1
#----------------------------------------------------
xml[${n}]="SVT_IMA_ENV_04"
scenario[${n}]="${xml[${n}]} - SVT Environment setup ldap on A1"
timeouts[${n}]=1200
test_template_initialize_test "${xml[${n}]}"

lc=0
for j in $(eval echo {1..${A_COUNT}}); do
  component[((++lc))]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh,-o|-c|../common/SVT_IMA_environment.cli|-s|setupldap|-a|$j"
done

test_template_finalize_test
((n+=1))
fi

