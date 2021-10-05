#! /bin/bash

function doPOST {
  payload="$1"
  result=`curl -sSf -XPOST -d "${payload}" ${WHERE}/configuration`
  rc=$?
  echo RESULT: ${result}
  if [ ${rc} -ne 0 ] ; then
    echo "FAILURE: POST ${WHERE} ${payload}"
    ((FAILURES+=1))
  fi
}

function doPUT {
  payload="$1"
  destination="$2"
  result=`curl -sSf -XPUT -T "${payload}" ${WHERE}/file/${destination}`
  rc=$?
  echo RESULT: ${result}
  if [ ${rc} -ne 0 ] ; then
    echo "FAILURE: PUT ${WHERE} ${payload}"
    ((FAILURES+=1))
  fi
}

function doDELETE {
  objtype=$1
  objname=$2
  result=`curl -sSf -XDELETE ${WHERE}/configuration/${objtype}/${objname}`
  rc=$?
  echo RESULT: ${result}
  if [ ${rc} -ne 0 ] ; then
    echo "FAILURE: DELETE ${WHERE}/configuration/${objtype}/${objname}"
    ((FAILURES+=1))
  fi
}

function createElements {
  # Loop for 100 times to create 100 of each type element
  FAILURES=0

  doPUT "${M1_TESTROOT}/common/imaserver-crt.pem" "imaserver-crt.pem"
  doPUT "${M1_TESTROOT}/common/imaserver-key.pem" "imaserver-key.pem"
  doPOST "{\"CertificateProfile\":{\"HASSLCertProfile\":{\"Certificate\":\"imaserver-crt.pem\",\"Key\":\"imaserver-key.pem\"}}}"

  port=22001
  for i in {1..100}
  do
    # SSL Certificate and Security Profile Setup
    doPOST "{\"SecurityProfile\":{\"HASSLSecureProf$i\":{\"UsePasswordAuthentication\":false,\"MinimumProtocolMethod\":\"TLSv1\",\"UseClientCertificate\":false,\"Ciphers\":\"Fast\",\"CertificateProfile\":\"HASSLCertProfile\"}}}"
    doPOST "{\"MessageHub\":{\"HAHub$i\":{\"Description\":\"HAHub$i\"}}}"
    doPOST "{\"ConnectionPolicy\":{\"HAConnectionPolicy$i\":{\"ClientID\":\"mqtt*\",\"UserID\":\"HAUser$i\",\"Protocol\":\"MQTT\"}}}"
    doPOST "{\"TopicPolicy\":{\"HATopicPolicy$i\":{\"Topic\":\"/dest$i/*\",\"ActionList\":\"Publish,Subscribe\",\"Protocol\":\"MQTT\"}}}"
    doPOST "{\"Endpoint\":{\"HAEndpoint$i\":{\"Enabled\":true,\"SecurityProfile\":\"HASSLSecureProf$i\",\"Port\":${port},\"ConnectionPolicies\":\"HAConnectionPolicy$i\",\"TopicPolicies\":\"HATopicPolicy$i\",\"MessageHub\":\"HAHub$i\",\"MaxMessageSize\":\"200MB\"}}}"
    doPOST "{\"Queue\":{\"HAQueue$i\":{}}}"
    doPOST "{\"TopicMonitor\":[\"/HAString$i/#\"]}"
    #doCLICommand group add GroupID=HAGroup$i
    #doCLICommand user add UserID=HAUser$i type=messaging password=test GroupMembership=HAGroup$i

    port=`expr $port + 1`
  done
  #doCLICommand create QueueManagerConnection Name=HA_QMC_AJ QueueManagerName=QM_MQJMS "\"ConnectionName=1.1.1.1(1415)\"" ChannelName=CHNLJMS
  #doCLICommand create DestinationMappingRule Name=HA_DMR_AJ QueueManagerConnection=HA_QMC_AJ RuleType=2 Source=FOOTBALL/ARSENAL Destination=FOOTBALL/ARSENAL Enabled=True
  
  if [[ $FAILURES -ne 0 ]] ; then
    echo Got $FAILURES failures in the CLI commands
  fi

}

function deleteElements {
  # Loop for 100 times to delete 100 of each type element
  FAILURES=0
  

  for i in {1..100}
  do
    doDELETE TopicMonitor "/HAString$i/%23"
    doDELETE Queue HAQueue$i
    doDELETE Endpoint HAEndpoint$i
    doDELETE TopicPolicy HATopicPolicy$i
    doDELETE ConnectionPolicy HAConnectionPolicy$i
    doDELETE MessageHub HAHub$i
    doDELETE SecurityProfile HASSLSecureProf$i
    #doCLICommand user delete UserID=HAUser$i type=messaging
    #doCLICommand group delete GroupID=HAGroup$i
  done
  #doCLICommand update DestinationMappingRule Name=HA_DMR_AJ QueueManagerConnection=HA_QMC_AJ RuleType=2 Source=FOOTBALL/ARSENAL Destination=FOOTBALL/ARSENAL Enabled=False
  #doCLICommand delete DestinationMappingRule Name=HA_DMR_AJ
  #doCLICommand delete QueueManagerConnection Name=HA_QMC_AJ
  
  doDELETE CertificateProfile HASSLCertProfile


  if [[ $FAILURES -ne 0 ]] ; then
    echo Got $FAILURES failures in the CLI commands
  fi

}


# Begin the main process

# Source the ISMsetup file to get access to information for the remote machine
if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
elif [[ -f "../scripts/ISMsetup.sh" ]] ; then
  source ../scripts/ISMsetup.sh
else
  echo "No env file to source"
fi

if [[ "$1" == "?" ]] ; then
  echo "Usage is: "
  echo "     ./HAadmintest1.sh [ D  [ 1 ] ]"
  echo "   if the first parameter is 'D' then delete all the objects" 
  echo "     on m2, unless the second parameter is '1'"
elif [[ "$1" == "D" ]] ; then
  if [[ "$2" == "1" ]] ; then
    WHERE="http://${A1_HOST}:${A1_PORT}/ima/v1"
  else
    WHERE="http://${A2_HOST}:${A2_PORT}/ima/v1"
  fi
  START=$(date +%s)
# start your script work here
  deleteElements
# your logic ends here
  END=$(date +%s)
  DIFF=$(( $END - $START ))
  echo "It took $DIFF seconds to delete"
else
  if [[ "$1" == "2" ]] ; then
    WHERE="http://${A2_HOST}:${A2_PORT}/ima/v1"
  else
    WHERE="http://${A1_HOST}:${A1_PORT}/ima/v1"
  fi
  START=$(date +%s)
# start your script work here
  createElements
# your logic ends here
  END=$(date +%s)
  DIFF=$(( $END - $START ))
  echo "It took $DIFF seconds to create"
fi

if [ ${FAILURES} -gt 0 ] ; then
  exit 1
else
  exit 0
fi
