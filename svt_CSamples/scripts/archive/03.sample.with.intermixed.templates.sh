#!/bin/bash





scenario_set_name="MQTTv3 Connection workload"
source ../scripts/commonFunctions.sh

typeset -i n=0

#-----------------------------------
# SVT HA tests
#-----------------------------------
if (($A_COUNT>1)) ; then 
    # don't try to do HA testing unless A_COUNT reports at least 2 appliances.

    test_template_add_test_single cmqtt_03_ "SVT - setup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/haFunctions.sh,-o|-a|setupHA|-f|/dev/stdout|" 600 "" "Test result is Success"

    #-----------------------------------
    # Start my SVT HA enabled tests
    #-----------------------------------

#----------------------------------------------------
# Test Case 0 - 
#----------------------------------------------------
# The prefix of the XML configuration file for the driver
#TODO!: xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="cmqtt_03_01"
scenario[${n}]="${xml[${n}]} - mqttsample 69 messages with Topic Filter Subscription"
timeouts[${n}]=800
# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|mqttsample_array","-o|-s|10.10.10.10:16102|-a|subscribe|"
component[2]=searchLogsEnd,m1,blar,9
components[${n}]="${component[1]} ${component[2]} "

((n+=1))

    

    #-----------------------------------
    # End my SVT HA enabled tests
    #-----------------------------------

    test_template_add_test_single cmqtt_03_ "SVT - cleanup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/haFunctions.sh,-o|-a|disableHA|-f|/dev/stdout" 600 "" "Test result is Success"

fi

