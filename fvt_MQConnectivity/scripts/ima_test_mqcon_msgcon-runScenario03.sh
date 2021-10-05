#! /bin/bash

scenario_set_name="Tests IMA MQ Connectivity."

typeset -i n=0

xml[${n}]="MQCON_MSGCON_03"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Text message is published to MQ topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=300

# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon2|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsText_MqToIma,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/AVON|${MQKEY}/UK/SCOTLAND/ARGYLL|60000"
component[3]=searchLogsEnd,m1,mqcon_msgcon_03.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon2|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
