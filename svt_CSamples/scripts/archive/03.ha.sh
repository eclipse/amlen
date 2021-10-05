#!/bin/bash


scenario_set_name="MQTTv3 Connection workload"
source ../scripts/commonFunctions.sh

typeset -i n=0

    xml[${n}]="scenario -silly test to publish messages, to an non ha pair, just to demonstrate."
    timeouts[${n}]=400
    scenario[${n}]="${xml[${n}]} - Basic MQTTv3 ha client test - publih a message"
    component[1]=cAppDriver,m1,"-e|mqttsample_array,-o|-s|${A1_IPv4_1}:16102|-a|publish|"
    components[${n}]="${component[1]}"
    ((n+=1))


#-----------------------------------
# SVT HA tests
#-----------------------------------
if (($A_COUNT>1)) ; then 
    # don't try to do HA testing unless A_COUNT reports at least 2 appliances.

    #test_template_add_test_single "SVT - setup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/haFunctions.sh,-o|-a|setupHA|-f|/dev/stdout|" 600 "" "Test result is Success"
    #test_template_add_test_single "SVT - setup HA" cAppDriver m1 "-e|../scripts/haFunctions.sh,-o|-a|setupHA|-f|setupHA.log|" 600 "" "Test result is Success"
    test_template_add_test_single "SVT - cleanup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/haFunctions.sh,-o|-a|disableHA|-f|disableHA.log" 600 "" "Test result is Success"

else
    #-----------------------------------
    # Start my SVT HA enabled tests
    #-----------------------------------

    xml[${n}]="scenario -silly test to publish messages, to an ha pair, just to demonstrate."
    timeouts[${n}]=400
    scenario[${n}]="${xml[${n}]} - Basic MQTTv3 ha client test - publih a message"
    component[1]=cAppDriver,m1,"-e|mqttsample_array,-o|-s|${A1_IPv4_1}:16102|-x|haURI=${A2_IPv4_1}:16102|-a|publish|"
    components[${n}]="${component[1]}"
    ((n+=1))

    xml[${n}]="scenario -silly test to publish 2 messages, to an ha pair, just to demonstrate."
    timeouts[${n}]=400
    scenario[${n}]="${xml[${n}]} - Basic MQTTv3 ha client test - publih a message"
    component[1]=cAppDriver,m1,"-e|mqttsample_array,-o|-s|${A1_IPv4_1}:16102|-x|haURI=${A2_IPv4_1}:16102|-a|publish|-n|2"
    components[${n}]="${component[1]}"
    ((n+=1))

    #-----------------------------------
    # End my SVT HA enabled tests
    #-----------------------------------

    test_template_add_test_single "SVT - cleanup HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/haFunctions.sh,-o|-a|disableHA|-f|/dev/stdout" 600 "" "Test result is Success"


fi

