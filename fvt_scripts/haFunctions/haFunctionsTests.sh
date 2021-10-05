#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="haFunctions.sh tests"

typeset -i n=0

# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
#
#   Tool SubController:
#		component[x]=<subControllerName>,<machineNumber_ismSetup>,<config_file>
# or	component[x]=<subControllerName>,<machineNumber_ismSetup>,"-o \"-x <param> -y <params>\" "
#	where:
#   <SubControllerName>
#		SubController controls and monitors the test case runningon the target machine.
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
# Clean_store prep
#----------------------------------------------------
# A1 = Running
# A2 = Running
xml[${n}]="clean_store"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - clean_store"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|clean_store"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 0 - autoSetup
#----------------------------------------------------
# A1 = Running
# A2 = Running
xml[${n}]="autoSetup"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - autoSetup"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|autoSetup"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 1 - disableHA
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="disableHA"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - disableHA"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|disableHA"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - setupHA
#----------------------------------------------------
# A1 = Running as standalone server
# A2 = Running as standalone server
xml[${n}]="setupHA"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Configure HA with A1 as primary and A2 as standby"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|setupHA"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - stopPrimary
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="stopPrimary"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - stop primary server A1. A2 should become primary"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|stopPrimary"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - startStandby
#----------------------------------------------------
# A1 = Dead
# A2 = Primary
xml[${n}]="startStandby"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - start A1 into standby mode"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|startStandby"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - crashBothServers
#----------------------------------------------------
# A1 = Standby
# A2 = Primary
xml[${n}]="crashBothServers"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - crash both servers in the pair"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|crashBothServers"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 6 - startAndPickPrimary
#----------------------------------------------------
# A1 = Dead
# A2 = Dead
xml[${n}]="startAndPickPrimary"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - start both servers with A2 as primary"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|startAndPickPrimary|-p|2"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 - disableHA2
#----------------------------------------------------
# A1 = Standby
# A2 = Primary
xml[${n}]="disableHA2"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Disable HA"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|disableHA"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 8 - setupPrimary
#----------------------------------------------------
# A1 = Running as individual server
# A2 = Running as individual server
xml[${n}]="setupPrimary"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Configure A1 as a standalone primary server"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|setupPrimary"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 9 - setupStandby
#----------------------------------------------------
# A1 = Primary
# A2 = Running as standalone server
xml[${n}]="setupStandby"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Configure A2 as standby server for A1"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|setupStandby"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 10 - stopStandby
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="stopStandby"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Stop standby server A2"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|stopStandby"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 11 - startStandby
#----------------------------------------------------
# A1 = Primary
# A2 = Dead
xml[${n}]="startStandby2"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Start standby server A2"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|startStandby"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 12 - stopPrimary2
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="stopPrimary2"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - crash primary server A1"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|stopPrimary"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 13 - stopPrimaryNoCheck
#----------------------------------------------------
# A1 = Dead
# A2 = Primary
xml[${n}]="stopPrimaryNoCheck2"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - crash primary server A2"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|stopPrimaryNoCheck"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 14 - startAndPickPrimary
#----------------------------------------------------
# A1 = Dead
# A2 = Dead
xml[${n}]="startAndPickPrimary2"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - restart both servers with most recent primary as primary"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|startAndPickPrimary"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 15 - swapPrimary
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="swapPrimary"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Swap primary server"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|swapPrimary"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 16 - disableHA3
#----------------------------------------------------
# A1 = Standby
# A2 = Primary
xml[${n}]="disableHA3"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - disableHA"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.sh,-o|-a|disableHA"
components[${n}]="${component[1]}"
((n+=1))
