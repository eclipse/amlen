#! /bin/bash

scenario_set_name="MQTT GroupID via WSTestDriver"

typeset -i n=0

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Enable for LDAP on M1 and initilize  
    #----------------------------------------------------
    xml[${n}]="mqtt_groupid_M1_LDAP_setup"
    scenario[${n}]="${xml[${n}]} - Enable and init LDAP on M1"
    timeouts[${n}]=40
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

#----------------------------------------------------
# Test Case 0 - mqtt_groupid_policyconfig_setup
#----------------------------------------------------
xml[${n}]="mqtt_groupid_policy_setup"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - run policy_config.cli for the GroupID bucket"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupGID|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Test Case 1 - mqtt_groupid01
#----------------------------------------------------
xml[${n}]="testmqtt_groupid01"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - Test GroupID behavior"
component[1]=wsDriver,m1,groupid/${xml[${n}]}.xml,RX
component[2]=wsDriver,m1,groupid/${xml[${n}]}.xml,TX
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Test Case 1b - mqtt_groupid01b
#----------------------------------------------------
xml[${n}]="testmqtt_groupid01b"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - Test GroupID durable subscription behavior"
component[1]=wsDriver,m1,groupid/${xml[${n}]}.xml,RX
component[2]=wsDriver,m1,groupid/${xml[${n}]}.xml,TX
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Test Case 2 - mqtt_groupid02
#----------------------------------------------------
xml[${n}]="testmqtt_groupid02"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - Test GroupID behavior with nested group"
component[1]=wsDriver,m1,groupid/${xml[${n}]}.xml,RX
component[2]=wsDriver,m1,groupid/${xml[${n}]}.xml,TX
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Test Case 3 - mqtt_groupid03
#----------------------------------------------------
xml[${n}]="testmqtt_groupid03"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - Test GroupID behavior with nested group negative"
component[1]=wsDriver,m1,groupid/${xml[${n}]}.xml,RX
component[2]=wsDriver,m1,groupid/${xml[${n}]}.xml,TX
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Finally - policy_cleanup
#----------------------------------------------------
xml[${n}]="mqtt_groupid_policy_cleanup"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Cleanup policy_config.cli for GroupID bucket"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupGID|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1 
    #----------------------------------------------------
    xml[${n}]="mqtt_groupid_M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean LDAP on M1"
    timeouts[${n}]=10
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi
