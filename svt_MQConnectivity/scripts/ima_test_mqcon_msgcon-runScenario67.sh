
scenario_set_name="Tests IMA MQ Connectivity under stress conditions"

typeset -i n=0

xml[${n}]="MQCON_MSGCON_67_Setup"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} set up 10 topics with MQ Connectivity rules and send 1 million messages to each"
timeouts[${n}]=60

# Set up the components for the test in the order they should start
component[1]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon1|-c|scenario67_policy_config.cli"
component[2]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon2|-c|scenario67_policy_config.cli"
component[3]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon3|-c|scenario67_policy_config.cli"
component[4]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon4|-c|scenario67_policy_config.cli"
component[5]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon5|-c|scenario67_policy_config.cli"
component[6]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon6|-c|scenario67_policy_config.cli"
component[7]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon7|-c|scenario67_policy_config.cli"
component[8]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon8|-c|scenario67_policy_config.cli"
component[9]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon9|-c|scenario67_policy_config.cli"
component[10]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_msgCon10|-c|scenario67_policy_config.cli"
test_template_finalize_test

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))

xml[${n}]="MQCON_MSGCON_67_Send_messages"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} set up 10 topics with MQ Connectivity rules and send 1 million messages to each"
timeouts[${n}]=600
component[1]=javaAppDriverRC,m1,"-e|svt.jms.ism.JMSSample,-o|-a|publish|-t|UK/ENGLAND/WINCHESTER|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
component[2]=javaAppDriverRC,m1,"-e|svt.jms.ism.JMSSample,-o|-a|publish|-t|UK/SCOTLAND/ARGYLL|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
component[3]=javaAppDriverRC,m1,"-e|svt.jms.ism.JMSSample,-o|-a|publish|-t|UK/ENGLAND/OXFORD|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
component[4]=javaAppDriverRC,m1,"-e|svt.jms.ism.JMSSample,-o|-a|publish|-t|UK/SCOTLAND/BUTE|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
component[5]=javaAppDriverRC,m1,"-e|svt.jms.ism.JMSSample,-o|-a|publish|-t|UK/ENGLAND/LIVERPOOL|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
component[6]=javaAppDriverRC,m1,"-e|svt.jms.ism.JMSSample,-o|-a|publish|-t|UK/SCOTLAND/ANGUS|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
component[7]=javaAppDriverRC,m1,"-e|svt.jms.ism.JMSSample,-o|-a|publish|-t|US/MARYLAND/BALTIMORE|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
component[8]=javaAppDriverRC,m1,"-e|svt.jms.ism.JMSSample,-o|-a|publish|-t|US/COLORADO/DENVER|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
component[9]=javaAppDriverRC,m1,"-e|svt.jms.ism.JMSSample,-o|-a|publish|-t|US/CALIFORNIA/SACREMENTO|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
component[10]=javaAppDriverRC,m1,"-e|svt.jms.ism.JMSSample,-o|-a|publish|-t|US/IOWA/AMES|-s|tcp://${A1_IPv4_1}:16102|-n|1000000"
test_template_finalize_test

###Uncomment the single pound-sign line ('#') if multiple test cases are in this Test Bucket, repeat this block for each test case.
((n+=1))

xml[${n}]="MQCON_MSGCON_67_Teardown"
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} MQ Connectivity rules and endpoints are removed"
timeouts[${n}]=30

component[1]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon1|-c|scenario67_policy_config.cli"
component[2]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon2|-c|scenario67_policy_config.cli"
component[3]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon3|-c|scenario67_policy_config.cli"
component[4]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon4|-c|scenario67_policy_config.cli"
component[5]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon5|-c|scenario67_policy_config.cli"
component[6]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon6|-c|scenario67_policy_config.cli"
component[7]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon7|-c|scenario67_policy_config.cli"
component[8]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon8|-c|scenario67_policy_config.cli"
component[9]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon9|-c|scenario67_policy_config.cli"
component[10]=cAppDriver,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_msgCon10|-c|scenario67_policy_config.cli"

test_template_finalize_test
