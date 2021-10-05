#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

scenario_set_name="messagesight-must-gather test - 00"

typeset -i n=0


#----------------------------------------------------
# must_gather_test_001
#----------------------------------------------------
xml[${n}]="must_gather_test_001"
timeouts[${n}]=300
scenario[${n}]="${xml[${n}]} - Creation of Connection Factory Administered Objects and Policy Setup"
component[1]=cAppDriverLog,m1,"-e|./must-gather/must_gather_test.sh"
components[${n}]="${component[1]} "
((n+=1))

