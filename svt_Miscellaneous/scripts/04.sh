#!/bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Miscellaneous Scenarios 04"

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

# misc_04_00
printf -v xml[${n}] "misc_04_%02d" ${n}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - Restart servers"
timeouts[${n}]=900

component[1]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|stopServer|-i|2|-t|300"
test_template_compare_string[1]="Server is stopped"
component[2]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|stopServer|-i|1|-t|300"
test_template_compare_string[2]="Server is stopped"

component[3]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|startServer|-i|1"
test_template_compare_string[5]="serverControl.sh exited successfully"
component[4]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|startServer|-i|2"
test_template_compare_string[5]="serverControl.sh exited successfully"

component[5]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|checkStatus|-i|1|-s|production|-t|240"
test_template_compare_string[5]="Status = Running (production)"
component[6]=cAppDriverWait,m1,"-e|../scripts/serverControl.sh","-o|-a|checkStatus|-i|2|-s|production|-t|240"
test_template_compare_string[6]="Status = Running (production)"

test_template_finalize_test
((n+=1))
