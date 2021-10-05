#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for starting SVT automation
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="SVT IMA Automation Start"

typeset -i n=0

test_template_set_prefix "svt_ima_start_"

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
# Test Case 0 - start continuous monitoring of appliance
#----------------------------------------------------
a_name="SVT Automation Starting "
appliance_monitor="m1"; 
if (($M_COUNT>=2)); then
    appliance_monitor="m2"; # appliance monitoring will run on this system to distribute work more evenly
fi

test_template_add_test_single "${a_name} - start primary appliance monitoring" cAppDriver ${appliance_monitor} "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|continuous_monitor_primary_appliance|na|1|${A1_HOST}" 300 "AF_AT_VARIATION_QUICK_EXIT=true"
if (($A_COUNT>1)) ; then # don't try to do HA testing unless A_COUNT reports at least 2 appliances.
    #----------------------------------------------------
    # Test Case 0b - start continuous monitoring of secondary appliance
    #----------------------------------------------------
    test_template_add_test_single "${a_name} - start secondary appliance monitoring" cAppDriver ${appliance_monitor} "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|continuous_monitor_secondary_appliance|na|1|${A1_HOST}|${A2_HOST}" 300 "AF_AT_VARIATION_QUICK_EXIT=true"
fi

#----------------------------------------------------
# Test Case 1 - start continuous monitoring of each client
#----------------------------------------------------
test_template_add_test_single "${a_name} - start client monitoring " cAppDriver m1 "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|continuous_monitor_client|na|1|" 300 "AF_AT_VARIATION_ALL_M=true|AF_AT_VARIATION_QUICK_EXIT=true"

#----------------------------------------------------
# Test Case 2 - cleanup any log.* files . leave continuous.* files alone.
#----------------------------------------------------
test_template_add_test_all_M_concurrent  "${a_name} - cleanup logs" "-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|process_logs" 600 "" ""

