#!/bin/bash
#------------------------------------------------------------------------------
# Script for running messagesight-must-gather.sh
# Works with both Docker and RPM installs
#------------------------------------------------------------------------------



function printUsage {
  echo "must-gather-test.sh Usage"
  echo "  ./must-gather-test.sh [-h] -a <appliance ID> [-f <LOG> ]"
  echo ""
  echo "  appliance ID: The ISMsetup.sh A_COUNT index for the server to control"
  echo "  LOG:          The log file for script output"
  echo ""
}

#------------------------------------------------------------------------------
# START OF MAIN
#------------------------------------------------------------------------------
LOG=must-gather-test.log
# Set default applance ID to 1
appliance=1
# Working directory on the Server where must-gather will be executed
WorkDir="/must-gather-test"

while getopts "a:f:h" opt; do
  case ${opt} in
  a )
    appliance=${OPTARG}
    ;;
  f )
    LOG=${OPTARG}
    ;;
  h )
    printUsage
    exit 0
    ;;
  * )
    exit 1
    ;;
  esac
done

echo "Entering serverControl.sh with $# arguments: $@" | tee -a ${LOG}
if [ -f ../testEnv.sh ] ; then
  source ../testEnv.sh
elif [ -f ../scripts/ISMsetup.sh ] ; then
  source ../scripts/ISMsetup.sh
fi

aUser="A${appliance}_USER"
aUserVal=$(eval echo -e \$${aUser})
aHost="A${appliance}_HOST"
aHostVal=$(eval echo -e \$${aHost})
aPort="A${appliance}_PORT"
aPortVal=$(eval echo -e \$${aPort})
aType="A${appliance}_TYPE"
aTypeVal=$(eval echo -e \$${aType})
aSrvContID="A${appliance}_SRVCONTID"
aSrvContIDVal=$(eval echo -e \$${aSrvContID})


echo "APPLIANCE: ${appliance}" | tee -a ${LOG}
echo -e "User=${aUserVal} Host=${aHostVal} Port=${aPortVal} Type=${aTypeVal} SrvContID=${aSrvContIDVal}\n" | tee -a ${LOG}

statusResponse=`curl -sSf -XGET http://${aHostVal}:${aPortVal}/ima/service/status 2>/dev/null`
echo "Current server status:" | tee -a ${LOG}
echo -e ${statusResponse} | tee -a ${LOG}

# Make sure there is no data from previous runs
cmd="ssh ${aUserVal}@${aHostVal} rm -rf ${WorkDir}"
echo "A${appliance}: $cmd" | tee -a ${LOG}
reply=`$cmd`
RC=$?
echo $reply | tee -a ${LOG}
echo "RC=$RC" | tee -a ${LOG}
		
cmd="ssh ${aUserVal}@${aHostVal} mkdir -p ${WorkDir}"
echo "A${appliance}: $cmd" | tee -a ${LOG}
reply=`$cmd`
RC=$?
echo $reply | tee -a ${LOG}
echo "RC=$RC" | tee -a ${LOG}

if [ $RC -eq 0 ] ; then
    if [[ "${aTypeVal}" == "DOCKER" ]] ; then
        cmd="ssh ${aUserVal}@${aHostVal} docker cp ${aSrvContIDVal}:/opt/ibm/imaserver/bin/messagesight-must-gather.sh ${WorkDir}/."
        echo "A${appliance}: $cmd" | tee -a ${LOG}
        reply=`$cmd`
        RC=$?
        echo $reply | tee -a ${LOG}
        echo "RC=$RC" | tee -a ${LOG}
        if [ $RC -eq 0 ] ; then
            cmd="ssh ${aUserVal}@${aHostVal} ${WorkDir}/messagesight-must-gather.sh -d ${aSrvContIDVal} -w ${WorkDir}"
            echo "A${appliance}: $cmd" | tee -a ${LOG}
            reply=`$cmd`
            RC=$?
            echo $reply | tee -a ${LOG}
            echo "RC=$RC" | tee -a ${LOG}
        fi
    else
        cmd="ssh ${aUserVal}@${aHostVal} /opt/ibm/imaserver/bin/messagesight-must-gather.sh -w ${WorkDir}"
        echo "A${appliance}: $cmd" | tee -a ${LOG}
        reply=`$cmd`
        RC=$?
        echo $reply | tee -a ${LOG}
        echo "RC=$RC" | tee -a ${LOG}
    fi

    if [ $RC -eq 0 ] ; then
        cmd="scp ${aUserVal}@${aHostVal}:${WorkDir}/messagesight-must-gather-*.tar.gz must-gather/."
        echo "A${appliance}: $cmd" | tee -a ${LOG}
        reply=`$cmd`
        RC=$?
        echo $reply | tee -a ${LOG}
        echo "RC=$RC" | tee -a ${LOG}
    fi
fi

if [ $RC -eq 0 ] ; then
  echo "Test result is Success!" | tee -a ${LOG}
fi


