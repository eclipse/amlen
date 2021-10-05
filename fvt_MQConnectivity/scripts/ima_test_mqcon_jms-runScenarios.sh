#! /bin/bash

scenario_set_name="Tests IMA MQ Connectivity."

typeset -i n=0

xml[${n}]="MQCON_JMS_01"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.jms 3 JMS Text messages are published to IMA topic and subscribed by a MQ consumer [1 Pub; 1 Sub] with client ACK"
timeouts[${n}]=120
if [[ "${A1_TYPE}" == "ESX" ]] || [[ "${A1_TYPE}" =~ "KVM" ]] ||  [[ "${A1_TYPE}" =~ "VMWARE" ]]  ||  [[ "${A1_TYPE}" =~ "Bare" ]] ; then
#slow disk persistence, longer time needed to send messages to IMA Server
#fewer messages are sent.
    export TIMEOUTMULTIPLIER=1.5
	component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
	component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsAcks_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000"
	component[3]=searchLogsEnd,m1,mqcon_jms_01.comparetest
	component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
	components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
	((n+=1))
else
	component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
	component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsAcks_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000"
	component[3]=searchLogsEnd,m1,mqcon_jms_01.comparetest
	component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
	components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
	((n+=1))
fi

xml[${n}]="MQCON_JMS_02"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.jms 3 JMS Text messages are published to IMA topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with client ACK"
timeouts[${n}]=15
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsAcks_ImaToIma,-o|${A1_IPv4_1}|16102|UK/ENGLAND/DERBY|60000"
component[2]=searchLogsEnd,m1,mqcon_jms_02.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

xml[${n}]="MQCON_JMS_03"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.jms 3 JMS Text message is published to MQ topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with client ACK"
timeouts[${n}]=60
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon2|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsAcks_MqToIma,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/AVON|${MQKEY}/UK/SCOTLAND/ARGYLL|60000"
component[3]=searchLogsEnd,m1,mqcon_jms_03.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon2|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

xml[${n}]="MQCON_JMS_04"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.jms 2 JMS Text messages are published to IMA topic and subscribed by a durable MQ consumer [1 Pub; 1 Sub]"
timeouts[${n}]=160
if [[ "${A1_TYPE}" == "ESX" ]] || [[ "${A1_TYPE}" =~ "KVM" ]] ||  [[ "${A1_TYPE}" =~ "VMWARE" ]]  ||  [[ "${A1_TYPE}" =~ "Bare" ]] ; then
#slow disk persistence, longer time needed to send messages to IMA Server
#fewer messages are sent.
    export TIMEOUTMULTIPLIER=1.5
	component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
	component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsDurableSubscriber_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000"
	component[3]=searchLogsEnd,m1,mqcon_jms_04.comparetest
	component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
	components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
	((n+=1))
else
	component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
	component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsDurableSubscriber_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000"
	component[3]=searchLogsEnd,m1,mqcon_jms_04.comparetest
	component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
	components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
	((n+=1))
fi

xml[${n}]="MQCON_JMS_05"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.jms 2 JMS Text messages are published to IMA topic and subscribed by an IMA durable consumer [1 Pub; 1 Sub]"
timeouts[${n}]=80
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsDurableSubscriber_ImaToIma,-o|${A1_IPv4_1}|16102|UK/ENGLAND/DERBY|60000"
component[2]=searchLogsEnd,m1,mqcon_jms_05.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

xml[${n}]="MQCON_JMS_06"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.jms 2 JMS Text message are published to MQ topic and subscribed by an durable IMA consumer [1 Pub; 1 Sub]"
timeouts[${n}]=80
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon2|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsDurableSubscriber_MqToIma,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/AVON|${MQKEY}/UK/SCOTLAND/ARGYLL|60000"
component[3]=searchLogsEnd,m1,mqcon_jms_06.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon2|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

xml[${n}]="MQCON_JMS_07"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.jms 2 JMS Text messages are published to IMA topic and subscribed by a MQ consumer [1 Pub; 1 Sub] using a selector to obtain just 1 message"
timeouts[${n}]=120
if [[ "${A1_TYPE}" == "ESX" ]] || [[ "${A1_TYPE}" =~ "KVM" ]] ||  [[ "${A1_TYPE}" =~ "VMWARE" ]]  ||  [[ "${A1_TYPE}" =~ "Bare" ]] ; then
#slow disk persistence, longer time needed to send messages to IMA Server
#fewer messages are sent.
    export TIMEOUTMULTIPLIER=1.5
	component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
	component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsSelectors_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000"
	component[3]=searchLogsEnd,m1,mqcon_jms_07.comparetest
	component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
	components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
	((n+=1))
else
	component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
	component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsSelectors_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000"
	component[3]=searchLogsEnd,m1,mqcon_jms_07.comparetest
	component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
	components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
	((n+=1))
fi

xml[${n}]="MQCON_JMS_08"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.jms 2 JMS Text messages are published to IMA topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with a selector to obtain just 1 message"
timeouts[${n}]=60
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsSelectors_ImaToIma,-o|${A1_IPv4_1}|16102|UK/ENGLAND/DERBY|60000"
component[2]=searchLogsEnd,m1,mqcon_jms_08.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

xml[${n}]="MQCON_JMS_09"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.jms 2 JMS Text messages are published to MQ topic and subscribed by an IMA consumer [1 Pub; 1 Sub] with a selector to obtain just 1 message"
timeouts[${n}]=60
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon2|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsSelectors_MqToIma,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/AVON|${MQKEY}/UK/SCOTLAND/ARGYLL|60000"
component[3]=searchLogsEnd,m1,mqcon_jms_09.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon2|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

xml[${n}]="MQCON_JMS_10"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.jms 3 JMS Text messages are published to IMA topic and subscribed by a MQ consumer [1 Pub; 1 Sub] but only 2 messages are committed"
timeouts[${n}]=60
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsTransactions_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|60000"
component[3]=searchLogsEnd,m1,mqcon_jms_10.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

xml[${n}]="MQCON_JMS_11"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.jms 3 JMS Text messages are published to IMA topic and subscribed by an IMA consumer [1 Pub; 1 Sub] but only two messages are committed"
timeouts[${n}]=15
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsTransactions_ImaToIma,-o|${A1_IPv4_1}|16102|UK/ENGLAND/DERBY|60000"
component[2]=searchLogsEnd,m1,mqcon_jms_11.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

xml[${n}]="MQCON_JMS_12"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.jms 3 JMS Text messages are published to MQ topic and subscribed by an IMA consumer [1 Pub; 1 Sub] but only 2 messages are committed"
timeouts[${n}]=60
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon2|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsTransactions_MqToIma,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/AVON|${MQKEY}/UK/SCOTLAND/ARGYLL|60000"
component[3]=searchLogsEnd,m1,mqcon_jms_12.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon2|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

xml[${n}]="MQCON_JMS_13"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.jms 4 JMS Text messages are published to IMA topic and subscribed by a MQ aysnchronous consumer [1 Pub; 1 Sub] but only 1 message should be received"
timeouts[${n}]=300
if [[ "${A1_TYPE}" == "ESX" ]] || [[ "${A1_TYPE}" =~ "KVM" ]] ||  [[ "${A1_TYPE}" =~ "VMWARE" ]]  ||  [[ "${A1_TYPE}" =~ "Bare" ]] ; then
#slow disk persistence, longer time needed to send messages to IMA Server
#fewer messages are sent.
    export TIMEOUTMULTIPLIER=1.5
	component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
	component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsAsynchronous_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|360000"
	component[3]=searchLogsEnd,m1,mqcon_jms_13.comparetest
	component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
	components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
	((n+=1))
else 
	component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|policy_config.cli"
	component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsAsynchronous_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WINCHESTER|${MQKEY}/UK/SCOTLAND/DUNDEE|360000"
	component[3]=searchLogsEnd,m1,mqcon_jms_13.comparetest
	component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|policy_config.cli"
	components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
	((n+=1))
fi

xml[${n}]="MQCON_JMS_14"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.jms 4 JMS Text messages are published to IMA topic and subscribed by a MQ aysnchronous consumer [1 Pub; 1 Sub] but only 1 message should be received"
timeouts[${n}]=15
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsAsynchronous_ImaToIma,-o|${A1_IPv4_1}|16102|UK/ENGLAND/DERBY|60000"
component[2]=searchLogsEnd,m1,mqcon_jms_14.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

xml[${n}]="MQCON_JMS_15"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.jms 4 JMS Text messages are published to IMA topic and subscribed by a MQ aysnchronous consumer [1 Pub; 1 Sub] but only 1 message should be received"
timeouts[${n}]=60
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon2|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsAsynchronous_MqToIma,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/AVON|${MQKEY}/UK/SCOTLAND/ARGYLL|60000"
component[3]=searchLogsEnd,m1,mqcon_jms_15.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon2|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

xml[${n}]="MQCON_JMS_16"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.jms 5 JMS Text messages are published to IMA queue and subscribed by a MQ consumer from a topic [1 Pub; 1 Sub]"
timeouts[${n}]=240
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon3|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsQueueToTopic_ImaToMq,-o|${A1_IPv4_1}|16202|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/TINTAGEL|${MQKEY}/UK/SCOTLAND/OBAN|60000","-s|JVM_ARGS=-DIMAEnforceObjectMessageSecurity=false"
component[3]=searchLogsEnd,m1,mqcon_jms_16.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon3|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

xml[${n}]="MQCON_JMS_17"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.jms 5 JMS Text messages are published to MQ queue and subscribed by an IMA consumer from a topic [1 Pub; 1 Sub]"
timeouts[${n}]=60
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon4|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.jms.JmsQueueToTopic_MqToIma,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1415|QM_MQJMS|UK/ENGLAND/WHITBY|${MQKEY}/UK/SCOTLAND/PERTH|60000","-s|JVM_ARGS=-DIMAEnforceObjectMessageSecurity=false"
component[3]=searchLogsEnd,m1,mqcon_jms_17.comparetest
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon4|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))
