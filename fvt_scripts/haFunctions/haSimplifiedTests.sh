#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="HA Simplification Tests"

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

# Start off with a clean_store to try and make it at least somewhat similar
# to starting these tests on 2 fresh appliances.

#----------------------------------------------------
# Scenario 0 - clean_store
#----------------------------------------------------
# A1 = Running
# A2 = Running
xml[${n}]="clean_store"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - clean_store both appliances"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|cleanStore|-m1|1"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|cleanStore|-m1|2"
component[3]=sleep,30
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

# We should be able to configure HA without needing to clean the store on
# a fresh pair of appliances. Or in this case, because we just cleaned both stores...

#----------------------------------------------------
# Scenario 1 - autoSetup
#----------------------------------------------------
# A1 = Running
# A2 = Running
xml[${n}]="autoSetup"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - auto HA setup"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|setupHA"
components[${n}]="${component[1]}"
((n+=1))

# Test a couple failovers after the simplified HA setup.



#----------------------------------------------------
# Scenario 2 - Kill primary
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="killPrimary"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Kill primary server in HA pair"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|stopPrimary"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - Start standby
#----------------------------------------------------
# A1 = Stopped
# A2 = Primary
xml[${n}]="startStandby"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Start standby server in HA pair"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - Swap primary
#----------------------------------------------------
# A1 = Standby
# A2 = Primary
xml[${n}]="swapPrimary"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - swap primary in HA pair"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|swapPrimary"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - Crash Primary
#----------------------------------------------------

# A1 = Primary
# A2 = Standby
xml[${n}]="crashPrimary-StartStandby-SwapPrimary"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - crash primary in HA pair, start standby, swap primary"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|crashPrimary"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|swapPrimary"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))


#----------------------------------------------------
# Scenario 6 - Stop Standby
#----------------------------------------------------

# A1 = Primary
# A2 = Standby
xml[${n}]="stopStandbyThenStartStandby"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - stop standby, start standby"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|stopStandby"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 7 - stop both
#----------------------------------------------------

# A1 = Primary
# A2 = Standby
xml[${n}]="stopBothThenStartBoth"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - stop both,start both"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|stopBoth"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|startBoth"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 8 - Crash Secondary
#----------------------------------------------------

# A1 = Primary
# A2 = Standby
xml[${n}]="crashStandbyThenStartStandby"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - crash standby,start standby"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|crashStandby"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|startStandby"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))




#----------------------------------------------------
# Scenario 9 - Crash Both
#----------------------------------------------------

# A1 = Primary
# A2 = Standby
xml[${n}]="crashBoth"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - crash both"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|crashBoth"
components[${n}]="${component[1]}"
((n+=1))


#----------------------------------------------------
# Scenario 10 - Start Both 
#----------------------------------------------------

# A1 = Primary
# A2 = Standby
xml[${n}]="startBoth"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - start both"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|startBoth"
components[${n}]="${component[1]}"
((n+=1))









#----------------------------------------------------
# Scenario 11 - disableHA
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="disableHA"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - disable HA"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|disableHA"
components[${n}]="${component[1]}"
((n+=1))





#----------------------------------------------------
# Scenario 6 - clean_store
#----------------------------------------------------
# A1 = Running
# A2 = Running
#xml[${n}]="clean_store_again"
#timeouts[${n}]=400
#scenario[${n}]="${xml[${n}]} - clean_store both appliances again"
#component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py,-o|-a|clean_store"
#components[${n}]="${component[1]}"
#((n+=1))

#----------------------------------------------------
# Scenario 7 - Mismatched Group name test
#----------------------------------------------------
# A1 = Running
# A2 = Running
#xml[${n}]="mismatched_HA_group"
#timeouts[${n}]=400
#scenario[${n}]="${xml[${n}]} - Test bringing up HA pair with mismatched Group name"
#component[1]=cAppDriver,m1,"-e|./haGroupTests.sh"
#components[${n}]="${component[1]}"
#((n+=1))

