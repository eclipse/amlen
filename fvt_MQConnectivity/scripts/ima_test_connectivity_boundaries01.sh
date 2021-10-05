#! /bin/bash

scenario_set_name="Tests IMA MQ Connectivity."

typeset -i n=0

xml[${n}]="MQ_CON_BOUNDARIES_01"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.boundaries MQTT messages are published to a topic which is associated with a destination mapping rule in order to check that the max messages value is being enforced"
timeouts[${n}]=300
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup_mqconnectivity_boundaries01|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.boundaries.Mqtt_ImaTestDestinationMappingMaxMessges,-o|${A1_IPv4_1}|16123|DESTINATIONBOUNDARYTEST|10000|5000"
component[3]=searchLogsEnd,m1,mqcon_boundary_01a.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

xml[${n}]="MQ_CON_BOUNDARIES_02"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.boundaries MQTT messages are published to a topic which is associated with a destination mapping rule in order to check that the max messages value is being enforced"
timeouts[${n}]=300
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|changemaxmessagesto_200_mqconnectivity_boundaries01|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.boundaries.Mqtt_ImaTestDestinationMappingMaxMessges,-o|${A1_IPv4_1}|16123|DESTINATIONBOUNDARYTEST|10000|200"
component[3]=searchLogsEnd,m1,mqcon_boundary_01b.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

xml[${n}]="MQ_CON_BOUNDARIES_03"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.boundaries MQTT messages are published to a topic which is associated with a destination mapping rule in order to check that the max messages value is being enforced"
timeouts[${n}]=300
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|changemaxmessagesto_20000_mqconnectivity_boundaries01|-c|policy_config.cli"
component[2]=sleep,10
component[3]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.boundaries.Mqtt_ImaTestDestinationMappingMaxMessges,-o|${A1_IPv4_1}|16123|DESTINATIONBOUNDARYTEST|10000|20000|-waitTime|5000"
component[4]=searchLogsEnd,m1,mqcon_boundary_01c.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

xml[${n}]="MQ_CON_BOUNDARIES_04"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.boundaries MQTT messages are published to a topic which is associated with a destination mapping rule in order to check that the max messages value is being enforced"
timeouts[${n}]=300
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|changemaxmessagesto_5000_mqconnectivity_boundaries01|-c|policy_config.cli"
component[2]=javaAppDriverRC,m1,"-e|com.ibm.ima.mqcon.boundaries.Mqtt_ImaTestDestinationMappingMaxMessges,-o|${A1_IPv4_1}|16123|DESTINATIONBOUNDARYTEST|10000|5000"
component[3]=searchLogsEnd,m1,mqcon_boundary_01d.comparetest
components[${n}]="${component[1]} ${component[2]} ${component[3]}"
((n+=1))

xml[${n}]="MQ_CON_BOUNDARIES_05"
scenario[${n}]="${xml[${n}]} - com.ibm.ima.mqcon.boundaries MQTT messages are published to a topic which is associated with a destination mapping rule in order to check that the max messages value is being enforced"
timeouts[${n}]=300
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|teardown_mqconnectivity_boundaries01|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))
