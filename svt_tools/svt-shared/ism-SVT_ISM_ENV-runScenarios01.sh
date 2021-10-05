#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="SVT IMA Admin Objects for SSL Environment"

test_template_set_prefix "SVT_IMA_ENV_"

typeset -i n=0

# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#   Tool SubController:
#		component[x]=<subControllerName>,<machineNumber_ismSetup>,<config_file>
# or	component[x]=<subControllerName>,<machineNumber_ismSetup>,"-o \"-x <param> -y <params>\" "
#	where:
#   <SubControllerName>
#		SubController controls and monitors the test case running on the target machine.
#   <machineNumber_ismSetup>
#		m1 is the machine 1 in ismSetup.sh, m2 is the machine 2 and so on...
#		
# Optional, but either a config_file or other_opts must be specified
#   <config_file> for the subController 
#		The configuration file to drive the test case using this controller.
#	<OTHER_OPTS>	is used when configuration file may be over kill,
#			parameters are passed as is and are processed by the subController.
#			However, Notice the spaces are replaced with a delimiter - it is necessary.
#           The syntax is '-o',  <delimiter_char>, -<option_letter>, <delimiter_char>, <optionValue>, ...
#       ex: -o_-x_paramXvalue_-y_paramYvalue   or  -o|-x|paramXvalue|-y|paramYvalue
#
#   DriverSync:
#	component[x]=sync,<machineNumber_ismSetup>,
#	where:
#		<m1>			is the machine 1
#		<m2>			is the machine 2
#
#   Sleep:
#	component[x]=sleep,<seconds>
#	where:
#		<seconds>	is the number of additional seconds to wait before the next component is started.
#

#----------------------------------------------------
# Note - I don't want this environment setup to fail "easily" so I 
# did not put any searchLogs in it. If the cli commands pass 
# without returning 0, i consider that "good enough"
#----------------------------------------------------

#----------------------------------------------------
# Test Case 0 -  disable HA if it was enabled (usually this would only happen if previous Automation run terminated unexpectedly)
#----------------------------------------------------
# test_template_add_test_single "Disable HA if it was enabled" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|svt_disable_ha|na|1|${A1_HOST}|${A2_HOST}" 600 "AF_AT_VARIATION_QUICK_EXIT=true"


#----------------------------------------------------
# Test Case 0 - AllowSingleNIC
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="AllowSingleNIC"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - AllowSingleNIC"
component[1]=cAppDriverWait,m1,"-e|../scripts/updateAllowSingleNIC.sh"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 1 - Disable HA
#----------------------------------------------------
xml[${n}]="jmqtt_DisableHA"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Cleanup to nonHA environment"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py","-o|-a|disableHA|-t|300|-f|disableHA.log"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 2 -  do clean store
#----------------------------------------------------
# xml[${n}]="SVT_IMA_ENV_01"
# scenario[${n}]="${xml[${n}]} - SVT Environment cleanstore "
# timeouts[${n}]=1200
#
# component[1]=cAppDriver,m1,"-e|../scripts/clean_store.py,-o|clean"
# components[${n}]="${component[1]} "
# 
# ((n+=1))

#----------------------------------------------------
# Test Case 3 -  do cleanup environment
#----------------------------------------------------

xml[${n}]="SVT_IMA_ENV_02"
scenario[${n}]="${xml[${n}]} - SVT Environment cleanup"
timeouts[${n}]=1200

component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh,-o|-c|../common/SVT_IMA_environment.cli|-s|cleanup"
components[${n}]="${component[1]} "

((n+=1))

#----------------------------------------------------
# Test Case 4 -  do setup environment
#----------------------------------------------------
xml[${n}]="SVT_IMA_ENV_03"
scenario[${n}]="${xml[${n}]} - SVT Environment setup"
timeouts[${n}]=1200
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh,-o|-c|../common/SVT_IMA_environment.cli|-s|setup"
components[${n}]="${component[1]} "
((n+=1))


#----------------------------------------------------
# Test Case 5 -  do setupldap environment
#----------------------------------------------------
xml[${n}]="SVT_IMA_ENV_04"
scenario[${n}]="${xml[${n}]} - SVT Environment setup ldap on all MessageSights"
timeouts[${n}]=1200
test_template_initialize_test "${xml[${n}]}"

lc=0
for j in $(eval echo {1..${A_COUNT}}); do
  component[((++lc))]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh,-o|-c|../common/SVT_IMA_environment.cli|-s|setupldap|-a|$j"
done

test_template_finalize_test
((n+=1))


#----------------------------------------------------
# Test Case 4 - 4+MCOUNT (SVT_IMA_ENV_04 - SVT_IMA_ENV_(4+MCOUNT)) -  
#			Setup LTPA, client certificate cache - 
# Note : this test_template macro automatically increments n
# Note: this test may take a long time the first time, but all subsequent
# times it should be very fast after the cache has been initialized.
#
# The cache is not reintialized after each automation run, but persists until
# the master data cache changes or an expected file is missing.
#
# TODO: try to make it smarter about where to create the directory / or /home or silently fail 
#----------------------------------------------------
# test_template_add_test_all_Ms  "SVT Environment data cache setup" "-e|${M1_TESTROOT}/scripts/SVT.setup.data.sh,-o|" 3600 "" "" "SVT_IMA_ENV_"   




