#! /bin/bash

#----------------------------------------------------
#  This script defines the scenarios for the ima Test Automation Framework.
#  It can be used as an example for defining other testcases.
#
#----------------------------------------------------

# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios

scenario_set_name="Tests IMA MQ Connectivity."

typeset -i n=0

xml[${n}]="MQ_CONNECTIVITY_FINAL_TEARDOWN"
scenario[${n}]="${xml[${n}]} - com.ibm.ima Runs all the teardown to clear everything for the next run, discarding the error messages."
timeouts[${n}]=300
# Set up the components for the test in the order they should start
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|final_teardown|-c|policy_config.cli"
components[${n}]="${component[1]}"
((n+=1))
