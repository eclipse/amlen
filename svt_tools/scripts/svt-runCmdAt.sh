#!/bin/bash

##############################################################
#
# This bash script will send the command killall java
# to each of the client machines in the automation framework
#
# Example:
#  svt-runCmdAt.sh killall java
#     This command runs immediately and kills all java processes on all client machines
#
#  svt-runCmdAt.sh 5m killall java
#     This command sleeps 5 minutes and then kills all java processes on all client machines
#
# Example for automation framework:
#     component[15]="cAppDriverWait,m1,-e|/niagara/test/scripts/svt-runCmdAt.sh,-o|5m|killall|java,"
#     This command will run on client machine "m1" and it sleeps 5 minutes, then kills all java processes on all client machines
#
##############################################################

if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  source ../scripts/ISMsetup.sh
fi

if [[ (! -z $1)&&( $1 = [1234567890]* ) ]]; then
  echo sleep $1
  sleep $1
  shift
fi

for i in $(eval echo {1..${M_COUNT}}); do
  echo ssh $(eval echo \$\{M${i}\_HOST}) $@
  ssh $(eval echo \$\{M${i}\_HOST}) $@
done
