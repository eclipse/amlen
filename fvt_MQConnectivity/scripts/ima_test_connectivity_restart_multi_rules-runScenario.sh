#! /bin/bash

scenario_set_name="Tests IMA MQ Connectivity."

typeset -i n=0

xml[${n}]="MQ_CON_MULTI_RESTART_RULES01"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQTT message is published to an IMA topic and received from an MQ Queue using MQ JAVA"
timeouts[${n}]=60
# Set up the components for the test in the order they should start
# check rule 1 works
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules1|-c|policy_config.cli"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules9|-c|policy_config.cli"
component[4]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.ImaTopicToMqQueueTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/ISMTopic/To/MQQueue|-mqQueueName|${MQKEY}.ISMTopic.To.MQQueue|-waitTime|10000"
component[5]=searchLogsEnd,m1,mqcon_rules_mr_01.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

xml[${n}]="MQ_CON_MULTI_RESTART_RULES02"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQTT message is published to an IMA topic and received from an MQ Queue using MQ JAVA"
timeouts[${n}]=60
# check rule 9 works
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqTopicSubtreeToImaSubtreeTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/MQTopicSubtree/To/ISMTopicSubtree|-waitTime|10000|-numberOfPublishers|5|-numberOfSubscribers|5"
component[2]=searchLogsEnd,m1,mqcon_rules_mr_02.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

xml[${n}]="MQ_CON_MULTI_RESTART_RULES03"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQTT message is published to an IMA topic and received from an MQ Queue using MQ JAVA"
timeouts[${n}]=60
# stop rule 1 and check rule 9 still works
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stop_mqconnectivity_rules1|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqTopicSubtreeToImaSubtreeTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/MQTopicSubtree/To/ISMTopicSubtree|-waitTime|10000|-numberOfPublishers|5|-numberOfSubscribers|5"
component[3]=searchLogsEnd,m1,mqcon_rules_mr_03.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

xml[${n}]="MQ_CON_MULTI_RESTART_RULES04"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQTT message is published to an IMA topic and received from an MQ Queue using MQ JAVA"
timeouts[${n}]=60
# start rule 1 and stop rule 9. Check rule 1 works. Restart rule 9.
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|start_mqconnectivity_rules1|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stop_mqconnectivity_rules9|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.ImaTopicToMqQueueTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/ISMTopic/To/MQQueue|-mqQueueName|${MQKEY}.ISMTopic.To.MQQueue|-waitTime|10000"
component[4]=searchLogsEnd,m1,mqcon_rules_mr_04.comparetest
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|start_mqconnectivity_rules9|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))

xml[${n}]="MQ_CON_MULTI_RESTART_RULES05"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQTT message is published to an IMA topic and received from an MQ Queue using MQ JAVA"
timeouts[${n}]=60
# check rule still 9 works
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqTopicSubtreeToImaSubtreeTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/MQTopicSubtree/To/ISMTopicSubtree|-waitTime|10000|-numberOfPublishers|5|-numberOfSubscribers|5"
component[2]=searchLogsEnd,m1,mqcon_rules_mr_05.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

xml[${n}]="MQ_CON_MULTI_RESTART_RULES06"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQTT message is published to an IMA topic and received from an MQ Queue using MQ JAVA"
timeouts[${n}]=60
# stop both rules. Check that rule 1 does not work
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stop_mqconnectivity_rules1|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|stop_mqconnectivity_rules9|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.ImaTopicToMqQueueTestNoMessages,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/ISMTopic/To/MQQueue|-mqQueueName|${MQKEY}.ISMTopic.To.MQQueue|-waitTime|10000"
component[4]=searchLogsEnd,m1,mqcon_rules_mr_06.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

xml[${n}]="MQ_CON_MULTI_RESTART_RULES07"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules.ImaTopicToMqQueueTest 1 MQTT message ????"
timeouts[${n}]=60
# Restart both rules. Check rule 1 still works.
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|start_mqconnectivity_rules1|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|start_mqconnectivity_rules9|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.ImaTopicToMqQueueTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/ISMTopic/To/MQQueue|-mqQueueName|${MQKEY}.ISMTopic.To.MQQueue|-waitTime|10000"
component[4]=searchLogsEnd,m1,mqcon_rules_mr_07.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

xml[${n}]="MQ_CON_MULTI_RESTART_RULES08"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.rules 1 MQTT message is published to an IMA topic and received from an MQ Queue using MQ JAVA"
timeouts[${n}]=60
# check rule 9 works. Teardown everything.
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqTopicSubtreeToImaSubtreeTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/MQTopicSubtree/To/ISMTopicSubtree|-waitTime|10000|-numberOfPublishers|5|-numberOfSubscribers|5"
component[2]=searchLogsEnd,m1,mqcon_rules_mr_08.comparetest
component[3]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules9|-c|policy_config.cli"
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules1|-c|policy_config.cli"
component[5]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_topic|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]}"
((n+=1))
