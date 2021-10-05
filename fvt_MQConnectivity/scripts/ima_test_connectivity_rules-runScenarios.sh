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


xml[${n}]="MQ_CON_RULES_02MR"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQTT message is published to an IMA topic and subscribed by an MQ JAVA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=150
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules2mr|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.ImaTopicToMultipleMqTopicsTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/ISMTopic/To/MultipleMQTopics|-waitTime|5000"
component[4]=searchLogsEnd,m1,mqcon_rules_02mr.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules2mr|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_topic|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))


xml[${n}]="MQ_CON_RULES_03"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQ Java message is published to an MQ Queue topic and subscribed by an MQTT consumer on IMA [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|settrace|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules3|-c|policy_config.cli"
component[4]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqQueueMqJavaToImaTopicMqtt,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/MQQueue/To/ISMTopic|-mqQueueName|${MQKEY}.MQQueue.To.ISMTopic|-waitTime|10000"
component[5]=searchLogsEnd,m1,mqcon_rules_03.comparetest
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules3|-c|policy_config.cli"
component[7]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_topic|-c|policy_config.cli"
component[8]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|settrace|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]} ${component[8]}"
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


xml[${n}]="MQ_CON_RULES_04_LONG"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQ Java message is published to an MQ topic and subscribed by an IMA MQTT consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=150
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules4_long|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqTopicMqJavaToImaTopicMqttTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/MQTopic/To/ISMTopic/Longer/Topic/String/To/Test/Length/Defect|-waitTime|5000"
component[4]=searchLogsEnd,m1,mqcon_rules_04_long.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules4_long|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_topic|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))


xml[${n}]="MQ_CON_RULES_05"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQTT message is published to each of 5 IMA topics and received from an MQ Queue using MQ JAVA"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules5|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.ImaTopicSubtreeToMqQueueTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/ISMTopicSubtree/To/MQQueue|-mqQueueName|${MQKEY}.ISMTopicSubtree.To.MQQueue|-waitTime|10000|-numberOfPublishers|5"
component[4]=searchLogsEnd,m1,mqcon_rules_05.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules5|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_topic|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))


xml[${n}]="MQ_CON_RULES_06"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQTT message is published to each of 5 IMA topics and subscribed by an MQ JAVA consumer [5 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules6|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.ImaTopicSubtreeToMqTopicTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/ISMTopicSubtree/To/MQTopic|-waitTime|10000|-numberOfPublishers|5"
component[4]=searchLogsEnd,m1,mqcon_rules_06.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules6|-c|policy_config.cli"
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


xml[${n}]="MQ_CON_RULES_08_LONG"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQ Java message is published to each of 5 MQ topics and subscribed by an IMA consumer [5 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules8_long|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqTopicSubtreeToImaTopicTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/MQTopicSubtree/To/ISMTopic/Longer/Topic/String/To/Test/Length/Defect|-waitTime|10000|-numberOfPublishers|5"
component[4]=searchLogsEnd,m1,mqcon_rules_08_long.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules8_long|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_topic|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))


xml[${n}]="MQ_CON_RULES_09"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQ Java message is published to each of 5 MQ topics and subscribed by an MQTT IMA consumer [5 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules9|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqTopicSubtreeToImaSubtreeTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/MQTopicSubtree/To/ISMTopicSubtree|-waitTime|10000|-numberOfPublishers|5|-numberOfSubscribers|5"
component[4]=searchLogsEnd,m1,mqcon_rules_09.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules9|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_topic|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))


xml[${n}]="MQ_CON_RULES_09_LONG"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQ Java message is published to each of 5 MQ topics and subscribed by an MQTT IMA consumer [5 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules9_long|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqTopicSubtreeToImaSubtreeTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/MQTopicSubtree/To/ISMTopicSubtree/Longer/Topic/String/To/Test/Length/Defect|-waitTime|10000|-numberOfPublishers|5|-numberOfSubscribers|5"
component[4]=searchLogsEnd,m1,mqcon_rules_09_long.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules9_long|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_topic|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))


xml[${n}]="MQ_CON_RULES_10"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 JMS message is published to an IMA queue and received from MQ queue using MQ JAVA"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_queue|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules10|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.ImaQueueToMqQueueTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16104|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-imaQueueName|${MQKEY}.ISMQueue.To.MQQueue|-mqQueueName|${MQKEY}.ISMQueue.To.MQQueue|-waitTime|10000"
component[4]=searchLogsEnd,m1,mqcon_rules_10.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules10|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_queue|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))


xml[${n}]="MQ_CON_RULES_11"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 JMS message is published to an IMA queue and subscribed by an MQ JAVA consumer [1 Pub; 1 Sub] with Topic Subscription"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_queue|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules11|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.ImaQueueToMqTopicTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16104|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-imaQueueName|${MQKEY}.ISMQueue.To.MQTopic|-mqTopicString|${MQKEY}/ISMQueue/To/MQTopic|-waitTime|10000"
component[4]=searchLogsEnd,m1,mqcon_rules_11.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules11|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_queue|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))


xml[${n}]="MQ_CON_RULES_12"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQ Java message is published to an MQ queue and received from an IMA queue using JMS"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_queue|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules12|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqQueueToImaQueue,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16104|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-imaQueueName|${MQKEY}.MQQueue.To.ISMQueue|-mqQueueName|${MQKEY}.MQQueue.To.ISMQueue|-waitTime|5000"
component[4]=searchLogsEnd,m1,mqcon_rules_12.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules12|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_queue|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))


xml[${n}]="MQ_CON_RULES_13"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQ Java message is published to an MQ topic and collected from IMA queue [1 Pub; 1 Sub]"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_queue|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules13|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqTopicToImaQueueTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16104|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/MQTopic/To/ISMQueue|-imaQueueName|${MQKEY}.MQTopic.To.ISMQueue|-waitTime|10000"
component[4]=searchLogsEnd,m1,mqcon_rules_13.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules13|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_queue|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))


xml[${n}]="MQ_CON_RULES_13_LONG"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQ Java message is published to an MQ topic and collected from IMA queue [1 Pub; 1 Sub]"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_queue|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules13_long|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqTopicToImaQueueTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16104|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/MQTopic/To/ISMQueue/Longer/Topic/String/To/Test/Length/Defect|-imaQueueName|${MQKEY}.MQTopic.To.ISMQueue.Long|-waitTime|10000"
component[4]=searchLogsEnd,m1,mqcon_rules_13_long.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules13_long|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_queue|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))


xml[${n}]="MQ_CON_RULES_14"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQ Java message is published to each of 5 MQ topics and received from an IMA queue using JMS"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
# 14 	WebSphere MQ topic subtree to IBM Messaging Appliance queue
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_queue|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules14|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqTopicSubtreeToImaQueueTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16104|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-imaQueueName|${MQKEY}.MQTSubtree.To.ISMQueue|-topicString|${MQKEY}/MQTSubtree/To/ISMQueue|-waitTime|10000|-numberOfPublishers|5"
component[4]=searchLogsEnd,m1,mqcon_rules_14.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules14|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_queue|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))


xml[${n}]="MQ_CON_RULES_14_LONG"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQ Java message is published to each of 5 MQ topics and received from an IMA queue using JMS"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
# 14 	WebSphere MQ topic subtree to IBM Messaging Appliance queue
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_queue|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules14_long|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqTopicSubtreeToImaQueueTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16104|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-imaQueueName|${MQKEY}.MQTSubtree.To.ISMQueue.Long|-topicString|${MQKEY}/MQTSubtree/To/ISMQueue/Longer/Topic/String/To/Test/Length/Defect|-waitTime|10000|-numberOfPublishers|5"
component[4]=searchLogsEnd,m1,mqcon_rules_14_long.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules14_long|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_queue|-c|policy_config.cli"
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


xml[${n}]="MQ_CON_BAD_RULES_04"
# Set up the components for the test in the order they should start
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQ Java message is published to an MQ topic and subscribed by an IMA MQTT consumer [1 Pub; 1 Sub] with Topic Subscription. Bad QMC present"
timeouts[${n}]=180
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_bad_rules04|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqTopicToImaTopicWithBadRule,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/MQTopic/To/ISMTopic|-waitTime|5000"
component[4]=searchLogsEnd,m1,mqcon_bad_rules_04.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_bad_rules04|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_topic|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))


xml[${n}]="MQ_CON_BAD_RULES_13"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQ Java message is published to an MQ topic and collected from IMA queue [1 Pub; 1 Sub]"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_queue|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_bad_rules13|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqTopicToImaQueueTestWithBadRule,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16104|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/MQTopic/To/ISMQueue|-imaQueueName|${MQKEY}.MQTopic.To.ISMQueue|-waitTime|10000"
component[4]=searchLogsEnd,m1,mqcon_bad_rules_13.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_bad_rules13|-c|policy_config.cli"
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_queue|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]}"
((n+=1))
