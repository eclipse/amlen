#! /bin/bash

scenario_set_name="Tests IMA MQ Connectivity."

typeset -i n=0

xml[${n}]="MQ_CON_RESTART_RULES_04"
scenario[${n}]="${xml[${n}]} - 1 MQ Java message is published to an MQ topic and subscribed by an IMA MQTT consumer [1 Pub; 1 Sub] with Topic Subscription. Rule toggled 3 times before test"
timeouts[${n}]=180

# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules4|-c|policy_config.cli"
component[3]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|toggle_mqconnectivity_rules4|-c|policy_config.cli"
component[4]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.MqTopicToImaTopicWithRestartedRules,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/MQTopic/To/ISMTopic|-waitTime|10000"
component[5]=searchLogsEnd,m1,mqcon_restart_rules_04.comparetest
component[6]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules4|-c|policy_config.cli"
component[7]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_rules_topic|-c|policy_config.cli"
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]} ${component[5]} ${component[6]} ${component[7]}"
