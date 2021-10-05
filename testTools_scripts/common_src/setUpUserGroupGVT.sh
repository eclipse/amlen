#!/bin/bash
curDir=$(dirname ${0})
curFile=$(basename ${0})


if [[ -f "../testEnv.sh" ]]
then
  # Official ISM Automation machine assumed
  source ../testEnv.sh
else
  # Manual Runs need to build Customized ISMSetup for THIS param, SSH environment file and Tag Replacement 
  ../scripts/prepareTestEnvParameters.sh
  source ../scripts/ISMsetup.sh
fi  

#----------------------------------------------------------------------------
# Parse Options
#----------------------------------------------------------------------------
APPLIANCE=1
SECTION="ALL"
while getopts "f:a:s:" option ${OPTIONS}
  do
    case ${option} in
    f )
        CLI_LOG_FILE=${OPTARG}
        ;;
    a ) APPLIANCE=${OPTARG}
        ;;
    s ) SECTION=${OPTARG}
        ;;

    esac	
  done
LOG_FILE=${curDir}/${curFile}_${SECTION}.log
echo "Entering $0 with $# arguments: $@" > ${LOG_FILE}

../scripts/run-cli.sh -c cli_gvt_user_and_group_test.cli -s ${SECTION} -f ${LOG_FILE} -a ${APPLIANCE}
