
scenario_set_name="Tests IMA MQ Connectivity."

typeset -i n=0

xml[${n}]="MQCON_MSGCON_29"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion 1 JMS Base message is published to IMA topic and subscribed by an IMA MQTT consumer [1 Pub; 1 Sub]"
timeouts[${n}]=15

# Set up the components for the test in the order they should start
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsBaseToMqtt_ImaToIma,-o|${A1_IPv4_1}|16102|UK/ENGLAND/DERBY|60000"
component[2]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9

#  Set test_template_compare_string[] to the "success" string within the comparetest file.  Use the index of the corresponding component[], in this case [2].
test_template_compare_string[1]="RC=0"

#  test_template_runorder defaults to 1, 2, 3,... and only needed if you want a different order
test_template_finalize_test
