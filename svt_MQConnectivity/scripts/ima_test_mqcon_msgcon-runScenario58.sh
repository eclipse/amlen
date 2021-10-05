
scenario_set_name="Tests IMA MQ Connectivity."

typeset -i n=0

xml[${n}]="MQCON_MSGCON_58"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion MQTT Retained message sent to IMA topic with MQ JMS consumer"
timeouts[${n}]=150

# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon3|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.MqttRetainedToJms_ImaToMq,-o|${A1_IPv4_1}|16102|${MQSERVER1_IP}|1424|MQC_SVT_MSGCONVERSION|mquser|/opt/mqm_v7.5/bin|UK/ENGLAND/OXFORD|UK/SCOTLAND/MORAY|60000"
component[3]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9
component[4]=cAppDriverLogEnd,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon3|-c|policy_config.cli"

#  Set test_template_compare_string[] to the "success" string within the comparetest file.  Use the index of the corresponding component[], in this case [2].
test_template_compare_string[2]="RC=0"

#  test_template_runorder defaults to 1, 2, 3,... and only needed if you want a different order
test_template_finalize_test
