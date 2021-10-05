
scenario_set_name="Tests IMA MQ Connectivity."

typeset -i n=0

xml[${n}]="MQCON_MSGCON_63"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.msgconversion JMS Retained messages sent to topic with IMA transactional publishers and JMS subscriber"
timeouts[${n}]=15

# Set up the components for the test in the order they should start
component[1]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.msgconversion.JmsRetainedTransacted_ImaToIma,-o|${A1_IPv4_1}|16102|UK/ENGLAND/YORK|60000"
component[2]=searchLogsEnd,m1,${xml[${n}]}_m1.comparetest,9

#  Set test_template_compare_string[] to the "success" string within the comparetest file.  Use the index of the corresponding component[], in this case [2].
test_template_compare_string[1]="RC=0"

#  test_template_runorder defaults to 1, 2, 3,... and only needed if you want a different order
test_template_finalize_test
