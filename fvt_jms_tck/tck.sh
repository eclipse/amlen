#!/bin/bash
#set -x
##########################################################################################################
# To run these tests:
#  1. Set the CLASSPATH to point to the IMA client product jars
#  2. Run ./jck.sh from the directory where it resides
##########################################################################################################

export CLASSPATH=./lib/jmstck.jar:./lib/tsharness.jar:./lib/apiCheck.jar:./lib/javatest.jar:./classes:$CLASSPATH
doMustGather='FALSE'

# Source the ISMsetup file to get the appliance IP address (A1_HOST)
if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  source ../scripts/ISMsetup.sh
fi

# Only run TCK on FULLRUN
if [[ "${FULLRUN}" == "TRUE" ]] ; then

# Run run-scenarios.sh with empty input file so that we can verify that the server is running
source ../scripts/commonFunctions.sh
setSavePassedLogs on run_scenarios.log
RUNCMD="../scripts/run-scenarios.sh ism_jms_tck-00.sh run_scenarios.log"
echo Running command: ${RUNCMD}
$RUNCMD

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Enable for LDAP on M1 and initilize  
    #----------------------------------------------------
    ../scripts/ldap-config.sh -a start
    ../scripts/run-cli.sh -s setupm1ldap -c ../common/m1ldap_config.cli -f /dev/stdout
fi

### CLEANUP (IN CASE LAST RUN DID NOT COMPLETE)
../scripts/run-cli.sh -i "setup x delete Endpoint Name=jmstckEndpoint"  -f /dev/stdout
../scripts/run-cli.sh -i "setup x delete MessagingPolicy Name=jmstckMessagingPolicy2"  -f /dev/stdout
../scripts/run-cli.sh -i "setup x delete MessagingPolicy Name=jmstckMessagingPolicy1"  -f /dev/stdout
../scripts/run-cli.sh -i "setup x delete ConnectionPolicy Name=jmstckConnectionPolicy"  -f /dev/stdout
../scripts/run-cli.sh -i "setup x delete Queue Name=MY_QUEUE"  -f /dev/stdout
../scripts/run-cli.sh -i "setup x delete Queue Name=MY_QUEUE2"  -f /dev/stdout
../scripts/run-cli.sh -i "setup x delete Queue Name=testQ0"  -f /dev/stdout
../scripts/run-cli.sh -i "setup x delete Queue Name=testQ1"  -f /dev/stdout
../scripts/run-cli.sh -i "setup x delete Queue Name=testQ2"  -f /dev/stdout
../scripts/run-cli.sh -i "setup x delete Queue Name=testQueue2"  -f /dev/stdout
../scripts/run-cli.sh -i "setup x delete Queue Name=Q2"  -f /dev/stdout
../scripts/run-cli.sh -i "setup x delete MessageHub Name=jmstckHub"  -f /dev/stdout
../scripts/run-cli.sh -i "setup x user delete UserID=jmstck type=messaging"  -f /dev/stdout

# add the user and policies for this test (ignore return code since errors will be logged)
../scripts/run-cli.sh -i "setup 0 user add UserID=jmstck password=jmstck type=messaging"  -f /dev/stdout
#ldapsearch -xLLL -b "o=ibm"
../scripts/run-cli.sh -i "setup 0 create MessageHub Name=jmstckHub"  -f /dev/stdout
../scripts/run-cli.sh -i "setup 0 create Queue Name=MY_QUEUE"  -f /dev/stdout
../scripts/run-cli.sh -i "setup 0 create Queue Name=MY_QUEUE2"  -f /dev/stdout
../scripts/run-cli.sh -i "setup 0 create Queue Name=testQ0"  -f /dev/stdout
../scripts/run-cli.sh -i "setup 0 create Queue Name=testQ1"  -f /dev/stdout
../scripts/run-cli.sh -i "setup 0 create Queue Name=testQ2"  -f /dev/stdout
../scripts/run-cli.sh -i "setup 0 create Queue Name=testQueue2"  -f /dev/stdout
../scripts/run-cli.sh -i "setup 0 create Queue Name=Q2"  -f /dev/stdout
../scripts/run-cli.sh -i "setup 0 create ConnectionPolicy Name=jmstckConnectionPolicy Protocol=JMS"  -f /dev/stdout
../scripts/run-cli.sh -i "setup 0 create MessagingPolicy Name=jmstckMessagingPolicy1 Destination=* DestinationType=Queue "ActionList=Send,Receive,Browse" Protocol=JMS"  -f /dev/stdout
../scripts/run-cli.sh -i "setup 0 create MessagingPolicy Name=jmstckMessagingPolicy2 Destination=* DestinationType=Topic "ActionList=Publish,Subscribe" Protocol=JMS"  -f /dev/stdout
../scripts/run-cli.sh -i "setup 0 create Endpoint Name=jmstckEndpoint Enabled=True Port=16543 ConnectionPolicies=jmstckConnectionPolicy MessagingPolicies=jmstckMessagingPolicy1,jmstckMessagingPolicy2 MaxMessageSize=4MB MessageHub=jmstckHub"  -f /dev/stdout


if [[ -e /usr/bin/nawk ]]; then
  myAWK=nawk
else
  myAWK=awk
fi
cat testlist.tck | while read line; do
  testClass=`echo $line | ${myAWK} '{print $1}'`
  testName=`echo $line | ${myAWK} '{print $2}'`
  testCommand="java $BITFLAG -DIMAServer=${A1_IPv4_1} -DIMAPort=16543 -Ddeliverable.class=com.sun.ts.lib.deliverable.jms.JMSDeliverable -DIMAEnforceObjectMessageSecurity=false ${testClass} -p tstest.jte -t ${testName} -vehicle standalone"
#  testCommand="java $BITFLAG -DIMAServer=${A1_IPv4_1} -DIMAPort=16543 -DIMATraceLevel=9 -Ddeliverable.class=com.sun.ts.lib.deliverable.jms.JMSDeliverable ${testClass} -p tstest.jte -t ${testName} -vehicle standalone"
  echo "===================================================================================================================================================="
  echo "  TEST CLASS:  ${testClass}"
  echo "  TEST NAME:   ${testName}"
  echo "  COMMAND:     ${testCommand}"
  echo "===================================================================================================================================================="
  if [ ${testName} == "XXXXXXXXX" ] ; then
    echo 'FAILED: tck_${testName} - ${testClass}(Defect xxxxx)'
  else
    ${testCommand} > TEMP.OUT 2>&1
    cat TEMP.OUT
    grep STATUS:Passed TEMP.OUT
    if [ $? == 0 ] ; then
      echo "PASSED: tck_${testName} - ${testClass}"
    else
      doMustGather='TRUE'
      echo "FAILED: tck_${testName} - ${testClass}"
    fi
  fi
done

if [ ${doMustGather} == 'TRUE' ] ; then
  testPath=`pwd`
  testDir=$( basename ${testPath} )
  if [[ "${A1_TYPE}" == "DOCKER" ]] ; then
    # !!!! Docker does not support list and must-gather at this time
    # SCP all logs from server container host. Each container mounted it filesystem to the host filesystem under /mnt/nas/<containerID.
    appliance="1"
    m_userVal="${M1_USER}"
    a_hostVal="${A1_HOST}"
    a_host_ip=`echo ${a_hostVal} | cut -d' ' -f 1`
    mkdir -p A${appliance}/log
    scp -r ${m_userVal}@${a_host_ip}:/mnt/A${appliance}/log/* A${appliance}/log/.
    mkdir -p A${appliance}/messagesight/config
    scp -r ${m_userVal}@${a_host_ip}:/mnt/A${appliance}/messagesight/config/* A${appliance}/messagesight/config/.
  else
    # Do must-gather
    ssh ${A1_USER}@${A1_HOST} imaserver trace flush
    ssh ${A1_USER}@${A1_HOST} platform must-gather must-gather.tgz
    RC=$?
    if [ $RC -ne 0 ] ; then
        echo "FAILED: must-gather failed on server A${A_COUNT} ${A1_HOST}, RC = $RC"
    else
      ssh ${A1_USER}@${A1_HOST} file put must-gather.tgz scp://${M1_USER}@${M1_IPv4_1}:${M1_TESTROOT}/${testDir}/${A1_HOST}.tgz
      if [ $? -ne 0 ] ; then
        # must-gather failed, add StrictHostKeyChecking=no to ssh_config file and try again
        echo "must-gather failed... adding StrictHostKeyChecking=no to ssh_config file on server A${A_COUNT} ${A1_HOST} and trying again"
        echo 'echo StrictHostKeyChecking=no >> /etc/ssh/ssh_config' | ssh ${A1_USER}@${A1_HOST} busybox
        ssh ${A1_USER}@${A1_HOST} file put must-gather.tgz scp://${M1_USER}@${M1_IPv4_1}:${M1_TESTROOT}/${testDir}/${A1_HOST}.tgz
        RC=$?
        if [ $RC -ne 0 ] ; then
          echo "FAILED: file_put - scp of must-gather failed on server A${A_COUNT} ${A1_HOST}, RC = $RC"
        fi
      fi
    fi
  fi
fi

### CLEANUP
../scripts/run-cli.sh -i "cleanup 0 delete Endpoint Name=jmstckEndpoint"  -f /dev/stdout
../scripts/run-cli.sh -i "cleanup 0 delete MessagingPolicy Name=jmstckMessagingPolicy2"  -f /dev/stdout
../scripts/run-cli.sh -i "cleanup 0 delete MessagingPolicy Name=jmstckMessagingPolicy1"  -f /dev/stdout
../scripts/run-cli.sh -i "cleanup 0 delete ConnectionPolicy Name=jmstckConnectionPolicy"  -f /dev/stdout
../scripts/run-cli.sh -i "cleanup 0 delete Queue Name=MY_QUEUE"  -f /dev/stdout
../scripts/run-cli.sh -i "cleanup 0 delete Queue Name=MY_QUEUE2"  -f /dev/stdout
../scripts/run-cli.sh -i "cleanup 0 delete Queue Name=testQ0"  -f /dev/stdout
../scripts/run-cli.sh -i "cleanup 0 delete Queue Name=testQ1"  -f /dev/stdout
../scripts/run-cli.sh -i "cleanup 0 delete Queue Name=testQ2"  -f /dev/stdout
../scripts/run-cli.sh -i "cleanup 0 delete Queue Name=testQueue2"  -f /dev/stdout
../scripts/run-cli.sh -i "cleanup 0 delete Queue Name=Q2"  -f /dev/stdout
../scripts/run-cli.sh -i "cleanup 0 delete MessageHub Name=jmstckHub"  -f /dev/stdout
../scripts/run-cli.sh -i "cleanup x user delete UserID=jmstck type=messaging"  -f /dev/stdout

if [[ "$A1_LDAP" == "FALSE" ]] ; then
    #----------------------------------------------------
    # Clean and disable for LDAP  
    #----------------------------------------------------
    ../scripts/run-cli.sh -s cleanupm1ldap -c ../common/m1ldap_config.cli -f /dev/stdout
    ../scripts/ldap-config.sh -a stop
fi

else
    echo "SKIPPED - Only running TCK when FULLRUN=TRUE"
fi # end of only run TCK on FULLRUN
