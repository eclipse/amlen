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
	((n+=1))

else 

component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsText_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|18000000"
component[3]=searchLogsEnd,m1,mqcon_msgcon_01.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
	((n+=1))

fi
