#! /bin/bash

scenario_set_name="Tests IMA MQ Connectivity using SSL"

typeset -i n=0

xml[${n}]="MQCON_MQSSL_Setup"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} set up 10 topics with MQ Connectivity rules and send 1 million messages to each"

##bad idea, too fragile across multiple runs from multiple clients or builds
##copy script to mq machine
##su to mqm and run script
##scp keystore and stash file back to client
##remove script from mq machine
##/bad idea

#put keystore and stash to imaserver
#configure appliance to connect to mq using ssl
#send message to imaserver
#read from MQ

timeouts[${n}]=60

# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_rules_topic|-c|policy_config.cli"
component[2]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_mqssl|-c|policy_config.cli"
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.rules.ImaTopicMqttToMqTopicMqJavaTest,-o|-imaServerHost|${A1_IPv4_1}|-imaServerPort|16105|-mqHostName|${MQSERVER1_IP}|-mqPort|1415|-queueManagerName|QM_MQJMS|-numberOfMessages|1|-mqChannel|MQCLIENTS|-topicString|${MQKEY}/ISMTopic/To/MQTopic|-waitTime|5000"
component[4]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_mqssl|-c|policy_config.cli"
test_template_finalize_test

