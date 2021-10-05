#! /bin/bash

scenario_set_name="JCA Teardown Objects"
typeset -i n=0

cluster=`grep -r -i cluster server.definition | grep -v grep`
if [[ "${cluster}" == "" ]] ; then
    echo "Not in a cluster"
    cluster=0
else
    cluster=1
fi

#----------------------------------------------------
# Scenario 1 - jca_teardown_001
#----------------------------------------------------
xml[${n}]="jca_teardown_001"
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - JCA Uninstall application"
component[1]=cAppDriverLogWait,m1,"-e|./scripts_was/was.sh","-o|-a|uninstall"
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 2 - jca_teardown_002
#----------------------------------------------------
xml[${n}]="jca_teardown_002"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - JCA Uninstall application verification"
component[1]=searchLogsEnd,m1,teardown/was.sh.uninstall.comparetest
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - jca_teardown_003
#----------------------------------------------------
xml[${n}]="jca_teardown_003"
# Need to have enough time for WAS cluster synchronization
# If the test kills the process in the middle of sync, things get messy,
# so hopefully 600 seconds is long enough for it to time out if it fails.
timeouts[${n}]=600
scenario[${n}]="${xml[${n}]} - JCA Teardown IBM MessageSight Objects"
component[1]=cAppDriverLogWait,m1,"-e|./scripts_was/was.sh","-o|-a|cleanup"
component[2]=sleep,5
component[3]=cAppDriverLogWait,m1,"-e|../jms_td_tests/HA/run-cli-primary.sh","-o|jca_policy_config.cli|cleanup"
components[${n}]="${component[1]} ${component[2]} ${component[3]}"

((n+=1))

#----------------------------------------------------
# Scenario 4 - jca_teardown_004
#----------------------------------------------------
if [[ ${WASType} =~ was8* ]] ; then
    xml[${n}]="jca_teardown_004"
    timeouts[${n}]=30
    scenario[${n}]="${xml[${n}]} - JCA Teardown Verify WAS Configuration"
    component[1]=searchLogsEnd,m1,teardown/was.sh.comparetest
    components[${n}]="${component[1]}"
    ((n+=1))
fi

#----------------------------------------------------
# Scenario 5 - jca_DS_cleanup
#----------------------------------------------------
xml[${n}]="jca_DS_cleanup"
timeouts[${n}]=120
scenario[${n}]="${xml[${n}]} - Cleanup durable subscriptions left from test app administratively"

# Administratively delete durable subscriptions created by test app
if [ ${cluster} == 1 ] ; then
    component[1]=cAppDriverLogWait,m1,"-e|../jms_td_tests/HA/run-cli-primary.sh","-o|jca_policy_config.cli|durableCluster"
else
    component[1]=cAppDriverLogWait,m1,"-e|../jms_td_tests/HA/run-cli-primary.sh","-o|jca_policy_config.cli|durableSingle"
fi
components[${n}]="${component[1]}"
((n+=1))


if [[ "${A1_LDAP}" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Cleanup and disable LDAP on M1  
    #----------------------------------------------------
    xml[${n}]="mqtt_AdminDynamic_M1_LDAP_cleanup"
    scenario[${n}]="${xml[${n}]} - disable and clean LDAP on M1"
    timeouts[${n}]=10
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi
