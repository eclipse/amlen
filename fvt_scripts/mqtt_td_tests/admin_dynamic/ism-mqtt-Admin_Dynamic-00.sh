#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ism Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="MQTT Admin_Dynamic-00 via WSTestDriver"

# Various tests of Administratively set values and dynamic updates. MQTT Admin Updates/Expiration/PendingDelete tests

typeset -i n=0


# Set up the components for the test in the order they should start
# What is configured here is different for each component and the options are used in run-scenarios.sh:
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

if [[ "${A1_TYPE}" == "ESX" ]] || [[ "${A1_TYPE}" =~ "KVM" ]] ||  [[ "${A1_TYPE}" =~ "VMWARE" ]]  ||  [[ "${A1_TYPE}" =~ "Bare" ]] || [[ "${A1_TYPE}" =~ "DOCKER" ]] ; then
    export TIMEOUTMULTIPLIER=1.4
    SLOWDISKPERSISTENCE="Yes"
fi

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Enable for LDAP on M1 and initilize  
    #----------------------------------------------------
    xml[${n}]="mqtt_AdminDynamic_M1_LDAP_setup"
    scenario[${n}]="${xml[${n}]} - Enable and init LDAP on M1"
    timeouts[${n}]=40
    component[1]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|start"
    component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setupm1ldap|-c|../common/m1ldap_config.cli"
    components[${n}]="${component[1]} ${component[2]}"
    ((n+=1))
fi

#----------------------------------------------------
# Setup Scenario  - mqtt message expiration setup
#----------------------------------------------------
xml[${n}]="mqtt_expiry_config_setup"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 1 - Creation of policies, endpoints, etc for mqtt Expiry testing in Admin_Dynamic bucket"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|admin_dynamic/expiry_config.cli"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Scenario  - Stop and start the server to clear the stats.. 
#----------------------------------------------------
#----------------------------------------------------
# The name of the test case and the prefix of the XML configuration file for the driver
# xml should only contain characters valid in a filename, it will be part of the logfile name.
xml[${n}]="mqtt_ClearStatsForNextTest"
timeouts[${n}]=90
scenario[${n}]="${xml[${n}]} - Clear the stats via a server restart for the next tests to check them.  "
component[1]=cAppDriver,m1,"-e|../common/serverStopRestart.sh"
components[${n}]="${component[1]} "
((n+=1))                   

if [[ ${SLOWDISKPERSISTENCE} == "Yes" ]]  ; then

#----------------------------------------------------
# Scenario 2 - mqtt_Expiry_001_DiskSlow
# special version of testcase for those cases
# where we are using DiskPersistence. 
#----------------------------------------------------
xml[${n}]="mqtt_Expiry_001_DiskSlow"
timeouts[${n}]=220
scenario[${n}]="${xml[${n}]} - SlowDisk version of Durable MQTT Message Expiry. Inactive Consumers "
component[1]=sync,m1
component[2]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,ds_setup
component[3]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,Pubs
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - mqtt_Expiry_002_DiskSlow
# special version of testcase for those cases
# where we are using DiskPersistence. 
#----------------------------------------------------
xml[${n}]="mqtt_Expiry_002_DiskSlow"
timeouts[${n}]=220
scenario[${n}]="${xml[${n}]} - SlowDisk version of Durable MQTT Q0S Subscriptions exceeding MaxMessage size with messages timing out. Active Consumers "
component[1]=sync,m1
component[2]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,Pubs
component[3]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,QoS_1Sub
component[4]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,QoS_2Sub
component[5]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,SharedSub1
component[6]=wsDriver,m2,admin_dynamic/${xml[${n}]}.xml,QoS_0Sub
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} "
((n+=1))

else

#----------------------------------------------------
# Scenario 2 - mqtt_Expiry_001
# Real Appliances version. 
#----------------------------------------------------
xml[${n}]="mqtt_Expiry_001"
timeouts[${n}]=220
scenario[${n}]="${xml[${n}]} - Durable MQTT Q0S Subscriptions exceeding MaxMessage size with messages timing out. Inactive Consumers "
component[1]=sync,m1
component[2]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,ds_setup
component[3]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,Pubs
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - mqtt_Expiry_002
#----------------------------------------------------
xml[${n}]="mqtt_Expiry_002"
timeouts[${n}]=220
scenario[${n}]="${xml[${n}]} - Durable MQTT Q0S Subscriptions exceeding MaxMessage size with messages timing out. Active Consumers "
component[1]=sync,m1
component[2]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,Pubs
component[3]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,QoS_1Sub
component[4]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,QoS_2Sub
component[5]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,SharedSub1
component[6]=wsDriver,m2,admin_dynamic/${xml[${n}]}.xml,QoS_0Sub
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} "
((n+=1))

fi

#----------------------------------------------------
# Cleanup Scenario  - mqtt message expiration cleanup
#----------------------------------------------------
xml[${n}]="mqtt_expiry_config_cleanup"
timeouts[${n}]=30
scenario[${n}]="${xml[${n}]} - Test 1 - Teardown of policies, endpoints, etc for mqtt Expiry testing in Admin_Dynamic bucket"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|admin_dynamic/expiry_config.cli"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Setup Scenario  - MQTT dynamic admin updates
#----------------------------------------------------
xml[${n}]="mqtt_AdminDynamic_config_setup"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 1 - Creation of policies, endpoints, etc for Admin Dynamic changes testing"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|admin_dynamic/Admin_Dynamic_config.cli"
components[${n}]="${component[1]} "
((n+=1))

#----------------------------------------------------
# Scenario 2 - mqtt_admin_dynamic_QoS0_001
#----------------------------------------------------
xml[${n}]="mqtt_admin_dynamic_QoS0_001"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Dynamic Policy Updates with MQTT QoS=0. CorrectPolicy on Durable Sub and PendingDelete"
component[1]=sync,m1
component[2]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,Sub
component[3]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,Pub
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 3 - mqtt_admin_dynamic_QoS2_002
#----------------------------------------------------
xml[${n}]="mqtt_admin_dynamic_QoS2_002"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Dynamic Policy Updates with MQTT QoS=2. CorrectPolicy on Durable Sub and PendingDelete"
component[1]=sync,m1
component[2]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,Sub
component[3]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,Pub
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

#----------------------------------------------------
# Scenario 4 - mqtt_allowPersistent
#----------------------------------------------------
xml[${n}]="mqtt_allowPersistent"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - AllowPersistentMessages = True"
component[1]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - mqtt_disallowPersistent
#----------------------------------------------------
xml[${n}]="mqtt_disallowPersistent"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - AllowPersistentMessages = False"
component[1]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - mqtt_allowPersistentMix
#----------------------------------------------------
xml[${n}]="mqtt_allowPersistentMix"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Allowed Publisher to disallowed subscriber and Disallowed will message"
component[1]=wsDriver,m1,admin_dynamic/mqtt_allowPersistentMix.xml,ALL
component[2]=wsDriver,m1,admin_dynamic/mqtt_disallowPersistentWill.xml,ALL
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

#----------------------------------------------------
# Scenario 5 - mqtt_allowPersistentUpdate
#----------------------------------------------------
xml[${n}]="mqtt_allowPersistentUpdate"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Connect while allowed then reconnect while not allowed"
component[1]=wsDriver,m1,admin_dynamic/${xml[${n}]}.xml,ALL
components[${n}]="${component[1]}"
((n+=1))

#----------------------------------------------------
# Setup Scenario  - mqtt dynamic admin updates
#----------------------------------------------------
xml[${n}]="mqtt_AdminDynamic_config_cleanup"
timeouts[${n}]=60
scenario[${n}]="${xml[${n}]} - Test 1 -Teardown of policies, endpoints, etc for Admin Dynamic changes testing"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|admin_dynamic/Admin_Dynamic_config.cli"
components[${n}]="${component[1]} "
((n+=1))

if [[ "$A1_LDAP" == "FALSE" ]] ; then
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
