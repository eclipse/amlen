#! /bin/bash

scenario_set_name="Tests IMA MQ Connectivity."

typeset -i n=0

xml[${n}]="MQCON_MSGCON_01"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Text message is published to IMA topic and subscribed by a MQ consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=300
# Set up the components for the test in the order they should start
if [[ "${A1_TYPE}" == "ESX" ]] || [[ "${A1_TYPE}" =~ "KVM" ]] ||  [[ "${A1_TYPE}" =~ "VMWARE" ]]  ||  [[ "${A1_TYPE}" =~ "Bare" ]] ; then
   #slow disk persistence, longer time needed to send messages to IMA Server
   #fewer messages are sent.
    export TIMEOUTMULTIPLIER=1.5
   component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
   component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsText_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|27000000"
   component[3]=searchLogsEnd,m1,mqcon_msgcon_01.comparetest
   component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
   components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
else 
   component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
   component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsText_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|18000000"
   component[3]=searchLogsEnd,m1,mqcon_msgcon_01.comparetest
   component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
   components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
fi
((n+=1))


xml[${n}]="MQCON_MSGCON_02"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Text message is published to IMA topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=15
# Set up the components for the test in the order they should start
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsText_ImaToIma,-o|${A1_IPv4_1}|16102|${MQKEY}/UK/ENGLAND/DERBY|60000"
component[2]=searchLogsEnd,m1,mqcon_msgcon_02.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))


xml[${n}]="MQCON_MSGCON_03"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Text message is published to MQ topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=300
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon2|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsText_MqToIma,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/AVON|${MQKEY}/UK/SCOTLAND/ARGYLL|60000"
component[3]=searchLogsEnd,m1,mqcon_msgcon_03.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon2|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))


xml[${n}]="MQCON_MSGCON_04"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Bytes message is published to IMA topic and subscribed by a MQ consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=300
# Set up the components for the test in the order they should start
if [[ "${A1_TYPE}" == "ESX" ]] || [[ "${A1_TYPE}" =~ "KVM" ]] ||  [[ "${A1_TYPE}" =~ "VMWARE" ]]  ||  [[ "${A1_TYPE}" =~ "Bare" ]] ; then
   #slow disk persistence, longer time needed to send messages to IMA Server
   #fewer messages are sent.
   export TIMEOUTMULTIPLIER=1.5
   component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
   component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsBytes_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000"
   component[3]=searchLogsEnd,m1,mqcon_msgcon_04.comparetest
   component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
   components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
else
   component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
   component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsBytes_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000"
   component[3]=searchLogsEnd,m1,mqcon_msgcon_04.comparetest
   component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
   components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
fi
((n+=1))


xml[${n}]="MQCON_MSGCON_05"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Bytes message is published to IMA topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=15
# Set up the components for the test in the order they should start
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsBytes_ImaToIma,-o|${A1_IPv4_1}|16102|UK/ENGLAND/DERBY|60000"
component[2]=searchLogsEnd,m1,mqcon_msgcon_05.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))


xml[${n}]="MQCON_MSGCON_06"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Bytes message is published to MQ topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon2|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsBytes_MqToIma,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/AVON|${MQKEY}/UK/SCOTLAND/ARGYLL|60000"
component[3]=searchLogsEnd,m1,mqcon_msgcon_06.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon2|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))


xml[${n}]="MQCON_MSGCON_07"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Text message with headers is published to IMA topic and subscribed by a MQ consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=120
# Set up the components for the test in the order they should start
if [[ "${A1_TYPE}" == "ESX" ]] || [[ "${A1_TYPE}" =~ "KVM" ]] ||  [[ "${A1_TYPE}" =~ "VMWARE" ]]  ||  [[ "${A1_TYPE}" =~ "Bare" ]] ; then
   #slow disk persistence, longer time needed to send messages to IMA Server
   #fewer messages are sent.
   export TIMEOUTMULTIPLIER=1.5
   component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
   component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsHeaders_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000"
   component[3]=searchLogsEnd,m1,mqcon_msgcon_07.comparetest
   component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
   components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
else
   component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
   component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsHeaders_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000"
   component[3]=searchLogsEnd,m1,mqcon_msgcon_07.comparetest
   component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
   components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
fi
((n+=1))


xml[${n}]="MQCON_MSGCON_08"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Text message with headers is published to IMA topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=15
# Set up the components for the test in the order they should start
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsHeaders_ImaToIma,-o|${A1_IPv4_1}|16102|UK/ENGLAND/DERBY|60000"
component[2]=searchLogsEnd,m1,mqcon_msgcon_08.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))


xml[${n}]="MQCON_MSGCON_09"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Text message with headers is published to MQ topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon2|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsHeaders_MqToIma,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/AVON|${MQKEY}/UK/SCOTLAND/ARGYLL|60000"
component[3]=searchLogsEnd,m1,mqcon_msgcon_09.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon2|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))


xml[${n}]="MQCON_MSGCON_10"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Map message is published to IMA topic and subscribed by a MQ consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=120
# Set up the components for the test in the order they should start
if [[ "${A1_TYPE}" == "ESX" ]] || [[ "${A1_TYPE}" =~ "KVM" ]] ||  [[ "${A1_TYPE}" =~ "VMWARE" ]]  ||  [[ "${A1_TYPE}" =~ "Bare" ]] ; then
   #slow disk persistence, longer time needed to send messages to IMA Server
   #fewer messages are sent.
   export TIMEOUTMULTIPLIER=1.5
   component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
   component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsMap_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000"
   component[3]=searchLogsEnd,m1,mqcon_msgcon_10.comparetest
   component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
   components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
else
   component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
   component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsMap_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000"
   component[3]=searchLogsEnd,m1,mqcon_msgcon_10.comparetest
   component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
   components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
fi
((n+=1))


xml[${n}]="MQCON_MSGCON_11"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Map message is published to IMA topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=15
# Set up the components for the test in the order they should start
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsMap_ImaToIma,-o|${A1_IPv4_1}|16102|UK/ENGLAND/DERBY|60000"
component[2]=searchLogsEnd,m1,mqcon_msgcon_11.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))


xml[${n}]="MQCON_MSGCON_12"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Map message is published to MQ topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=120
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon2|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsMap_MqToIma,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/AVON|${MQKEY}/UK/SCOTLAND/ARGYLL|60000"
component[3]=searchLogsEnd,m1,mqcon_msgcon_12.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon2|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))


xml[${n}]="MQCON_MSGCON_13"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Object message is published to IMA topic and subscribed by a MQ consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=120
# Set up the components for the test in the order they should start
if [[ "${A1_TYPE}" == "ESX" ]] || [[ "${A1_TYPE}" =~ "KVM" ]] ||  [[ "${A1_TYPE}" =~ "VMWARE" ]]  ||  [[ "${A1_TYPE}" =~ "Bare" ]] ; then
   #slow disk persistence, longer time needed to send messages to IMA Server
   #fewer messages are sent.
   export TIMEOUTMULTIPLIER=1.5
   component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
   component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsObject_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000","-s|JVM_ARGS=-DIMAEnforceObjectMessageSecurity=false"
   component[3]=searchLogsEnd,m1,mqcon_msgcon_13.comparetest
   component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
   components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
else
   component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
   component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsObject_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000","-s|JVM_ARGS=-DIMAEnforceObjectMessageSecurity=false"
   component[3]=searchLogsEnd,m1,mqcon_msgcon_13.comparetest
   component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
   components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
fi
((n+=1))


xml[${n}]="MQCON_MSGCON_14"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Object message is published to IMA topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=15
# Set up the components for the test in the order they should start
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsObject_ImaToIma,-o|${A1_IPv4_1}|16102|UK/ENGLAND/DERBY|60000","-s|JVM_ARGS=-DIMAEnforceObjectMessageSecurity=false"
component[2]=searchLogsEnd,m1,mqcon_msgcon_14.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))


xml[${n}]="MQCON_MSGCON_15"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Object message is published to MQ topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon2|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsObject_MqToIma,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/AVON|${MQKEY}/UK/SCOTLAND/ARGYLL|60000","-s|JVM_ARGS=-DIMAEnforceObjectMessageSecurity=false"
component[3]=searchLogsEnd,m1,mqcon_msgcon_15.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon2|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))


xml[${n}]="MQCON_MSGCON_16"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Stream message is published to IMA topic and subscribed by a MQ consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=120
# Set up the components for the test in the order they should start
if [[ "${A1_TYPE}" == "ESX" ]] || [[ "${A1_TYPE}" =~ "KVM" ]] ||  [[ "${A1_TYPE}" =~ "VMWARE" ]]  ||  [[ "${A1_TYPE}" =~ "Bare" ]] ; then
   #slow disk persistence, longer time needed to send messages to IMA Server
   #fewer messages are sent.
   export TIMEOUTMULTIPLIER=1.5
   component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
   component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsStream_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000"
   component[3]=searchLogsEnd,m1,mqcon_msgcon_16.comparetest
   component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
   components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
else
   component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
   component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsStream_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000"
   component[3]=searchLogsEnd,m1,mqcon_msgcon_16.comparetest
   component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
   components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
fi
((n+=1))


xml[${n}]="MQCON_MSGCON_17"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Stream message is published to IMA topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=15
# Set up the components for the test in the order they should start
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsStream_ImaToIma,-o|${A1_IPv4_1}|16102|UK/ENGLAND/DERBY|60000"
component[2]=searchLogsEnd,m1,mqcon_msgcon_17.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))


xml[${n}]="MQCON_MSGCON_18"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Stream message is published to MQ topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon2|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsStream_MqToIma,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/AVON|${MQKEY}/UK/SCOTLAND/ARGYLL|60000"
component[3]=searchLogsEnd,m1,mqcon_msgcon_18.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon2|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))


xml[${n}]="MQCON_MSGCON_19"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Base message is published to IMA topic and subscribed by a MQ consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=120
# Set up the components for the test in the order they should start
if [[ "${A1_TYPE}" == "ESX" ]] || [[ "${A1_TYPE}" =~ "KVM" ]] ||  [[ "${A1_TYPE}" =~ "VMWARE" ]]  ||  [[ "${A1_TYPE}" =~ "Bare" ]] ; then
   #slow disk persistence, longer time needed to send messages to IMA Server
   #fewer messages are sent.
   export TIMEOUTMULTIPLIER=1.5
   component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
   component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsBase_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000"
   component[3]=searchLogsEnd,m1,mqcon_msgcon_19.comparetest
   component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
   components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
else
   component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
   component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsBase_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000"
   component[3]=searchLogsEnd,m1,mqcon_msgcon_19.comparetest
   component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
   components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
fi
((n+=1))


xml[${n}]="MQCON_MSGCON_20"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Base message is published to IMA topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=15
# Set up the components for the test in the order they should start
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsBase_ImaToIma,-o|${A1_IPv4_1}|16102|UK/ENGLAND/DERBY|60000"
component[2]=searchLogsEnd,m1,mqcon_msgcon_20.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))


xml[${n}]="MQCON_MSGCON_21"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Base message is published to MQ topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon2|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsBase_MqToIma,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/AVON|${MQKEY}/UK/SCOTLAND/ARGYLL|60000"
component[3]=searchLogsEnd,m1,mqcon_msgcon_21.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon2|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

