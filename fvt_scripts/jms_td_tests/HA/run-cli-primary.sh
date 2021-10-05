#!/bin/bash
#------------------------------------------------------------------------------
# run-cli-primary.sh
#
# $1 = exe_file
# $2 = set_name
# $3 = logfile
#
# This script is mainly used in case of test failures to make sure that
# run-cli scripts are run against the primary server.
# If we run the commands against the wrong server, objects won't actually
# be deleted, since a lot of messaging object commands aren't allowed on
# standby.
#------------------------------------------------------------------------------

if [[ $# != 4 ]] ; then
    echo "run-cli-primary.sh expects 4 arguments"
    echo "Ex. ./run-cli-primary.sh config.cli setup -f logfile"
    exit 1
fi

exe_file=$1
set_name=$2
logfile=$4
echo "Entering run-cli-primary.sh with 4 arguments: $exe_file $set_name $3 $logfile"

if [[ -f "../testEnv.sh" ]] ; then
    source "../testEnv.sh"
else
    source ../scripts/ISMsetup.sh
fi

a1output=`curl -sS http://${A1_HOST}:${A1_PORT}/ima/service/status`
a2output=`curl -sS http://${A2_HOST}:${A2_PORT}/ima/service/status`

if [[ "${a1output}" =~ "Running (production)" ]] ; then
  primary=1
elif [[ "${a2output}" =~ "Running (production)" ]] ; then
  primary=2
else
    echo -e "\n>>>> RC=1 There is no primary server running <<<<"
    exit 1
fi

hasColon=`echo ${M1_IPv6_1} | grep ":"`
if [[ "${hasColon}" != "" ]]; then
    export new_M1_IPv6_1="[${M1_IPv6_1}]"
else
    export new_M1_IPv6_1="${M1_IPv6_1}"
fi

PID=`../scripts/run-cli.sh -s ${set_name} -c ${exe_file} -f ${logfile} -a ${primary}`
