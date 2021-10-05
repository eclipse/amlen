#! /bin/bash

scenario_set_name="Tests IMA MQ Connectivity."

typeset -i n=0

xml[${n}]="MQ_CON_RULES_01"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQTT message is published to an IMA topic and received from an MQ Queue using MQ JAVA"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules1|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.ImaTopicToMqQueueTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/ISMTopic/To/MQQueue|-mqQueueName|${MQKEY}.ISMTopic.To.MQQueue|-waitTime|10000"
component[4]=searchLogsEnd,m1,mqcon_rules_01.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules1|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_topic|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))


xml[${n}]="MQ_CON_RULES_02"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQTT message is published to an IMA topic and subscribed by an MQ JAVA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=150
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules2|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.ImaTopicMqttToMqTopicMqJavaTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/ISMTopic/To/MQTopic|-waitTime|5000"
component[4]=searchLogsEnd,m1,mqcon_rules_02.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules2|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_topic|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))

xml[${n}]="MQ_CON_RULES_04"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQ Java message is published to an MQ topic and subscribed by an IMA MQTT consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=150
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules4|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqTopicMqJavaToImaTopicMqttTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/MQTopic/To/ISMTopic|-waitTime|5000"
component[4]=searchLogsEnd,m1,mqcon_rules_04.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules4|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_topic|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))

xml[${n}]="MQ_CON_RULES_07"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQTT Java message is published to each of 5 IMA topics and subscribed by an MQ JAVA consumer [5 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules7|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.ImaTopicSubtreeToMqTopicSubtreeTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/ISMTopicSubtree/To/MQTopicSubtree|-waitTime|10000|-numberOfPublishers|5"
component[4]=searchLogsEnd,m1,mqcon_rules_07.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules7|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_topic|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))


xml[${n}]="MQ_CON_RULES_08"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQ Java message is published to each of 5 MQ topics and subscribed by an IMA consumer [5 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules8|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqTopicSubtreeToImaTopicTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/MQTopicSubtree/To/ISMTopic|-waitTime|10000|-numberOfPublishers|5"
component[4]=searchLogsEnd,m1,mqcon_rules_08.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules8|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_topic|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))

xml[${n}]="MQ_CON_BAD_RULES_02"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQTT message is published to an IMA topic and subscribed by an MQ JAVA consumer [1 Pub; 1 Sub] with Topic Subscription. Bad QMC present"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_bad_rules02|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.ImaTopicToMqTopicWithBadRule,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/ISMTopic/To/MQTopic|-waitTime|10000"
component[4]=searchLogsEnd,m1,mqcon_bad_rules_02.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_bad_rules02|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_topic|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))
