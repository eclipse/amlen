#! /bin/bash

scenario_set_name="IMA MQ Connectivity Restart"

typeset -i n=0

xml[${n}]="MQCON_MSGCON_RESTART"
scenario[${n}]="${xml[${n}]} - Restart MQConnectivity Service"
timeouts[${n}]=15
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|restart|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))

