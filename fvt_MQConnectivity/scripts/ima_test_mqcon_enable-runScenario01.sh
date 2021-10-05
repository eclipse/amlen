#! /bin/bash

scenario_set_name="IMA MQ Connectivity Enable"

typeset -i n=0

xml[${n}]="MQCON_MSGCON_ENABLE"
scenario[${n}]="${xml[${n}]} - Change MQConnectivity Service to Running"
timeouts[${n}]=15
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|enable|-c|policy_config.cli"
component[2]=searchLogsEnd,m1,mqcon_enable.comparetest
components[${n}]="${component[1]} ${component[2]}"
((n+=1))

