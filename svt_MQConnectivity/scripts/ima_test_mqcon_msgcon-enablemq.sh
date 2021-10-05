#!/bin/bash

if [[ -f "../testEnv.sh" ]]
then
  # Official ISM Automation machine assumed
  source ../testEnv.sh
else
  # Manual Runs need to build Customized ISMSetup for THIS param, SSH environment file and Tag Replacement
  source ../scripts/ISMsetup.sh
fi

source ../scripts/commonFunctions.sh

scenario_set_name="Tests IMA MQ Connectivity."

typeset -i n=0

printf -v xml[${n}] "MQCON_MSGCON_ENABLEMQ_%02d" ${n}
test_template_initialize_test "${xml[${n}]}"
scenario[${n}]="${xml[${n}]} - SVT Environment enable MQConnectivity on all MessageSight servers"
timeouts[${n}]=300
test_template_initialize_test "${xml[${n}]}"

component[1]="cAppDriverLogWait,m1,-e|../scripts/run-cli.sh,-o|-c|./policy_config.cli|-s|enablemq|-a|1"
component[2]="cAppDriverWait,m1,-e|./waitWhileMQConnectivity.py,-o|-a|${A1_HOST}:${A1_PORT}|-k|Status|-c|eq|-v|Inactive|-w|5|-m|180"

test_template_finalize_test
((n+=1))


