#! /bin/bash

function doGET {
  objtype=$1
  objname=$2
  result=`curl -sSf -XGET ${WHERE}/configuration/${objtype}/${objname}`
  rc=$?
  echo RESULT: ${result}
  if [ ${rc} -ne 0 ] ; then
    echo "FAILURE: GET ${WHERE}/${objtype}/${objname}"
    ((FAILURES+=1))
  fi
}

# Get the variables
if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  source ../scripts/ISMsetup.sh
fi

FAILURES=0
WHERE="http://${A1_HOST}:${A1_PORT}/ima/v1"
scriptrc=0

  doGET CertificateProfile
  output=`curl -sSf -XGET ${WHERE}/configuration/CertificateProfile | grep "HASSLCert"`
  rc=$?
  echo ${rc}
  echo ${output}
  if [[ "${output}" != "" ]] ; then
    echo incorrect response
    scriptrc=1 
  fi

  output=`curl -sSf -XGET ${WHERE}/configuration/SecurityProfile | grep "HASSLSec"`
  rc=$?
  echo ${rc}
  echo ${output}
  if [[ "${output}" != "" ]] ; then
    echo incorrect response 
    scriptrc=1 
  fi

  output=`curl -sSf -XGET ${WHERE}/configuration/TopicMonitor | grep "HAString"`
  rc=$?
  echo ${rc}
  echo ${output}
  if [[ "${output}" != "" ]] ; then
    echo incorrect response 
    scriptrc=1 
  fi

  output=`curl -sSf -XGET ${WHERE}/configuration/Queue | grep "HAQueue"`
  rc=$?
  echo ${rc}
  echo ${output}
  if [[ "${output}" != "" ]] ; then
    echo incorrect response 
    scriptrc=1 
  fi

  #ssh ${WHERE} imaserver group list type=messaging 2>&1  | grep "No group has been found" 
  #rc=$?
  #if [[ $rc -ne 0 ]] ; then
          #echo incorrect response 
    #doCLICommand group list type=messaging
    #echo result=$result
          #scriptrc=1 
  #fi

  #ssh ${WHERE} imaserver user list type=messaging 2>&1  | grep "No user has been found" 
  #rc=$?
  #if [[ $rc -ne 0 ]] ; then
          #echo incorrect response 
    #doCLICommand user list type=messaging
    #echo result=$result
          #scriptrc=1 
  #fi

  output=`curl -sSf -XGET ${WHERE}/configuration/MessageHub | grep "HAHub"`
  rc=$?
  echo ${rc}
  echo ${output}
  if [[ "${output}" != "" ]] ; then
    echo incorrect response 
    scriptrc=1 
  fi

  output=`curl -sSf -XGET ${WHERE}/configuration/ConnectionPolicy | grep "HAConnectionP"`
  rc=$?
  echo ${rc}
  echo ${output}
  if [[ "${output}" != "" ]] ; then
    echo incorrect response 
    scriptrc=1 
  fi

  output=`curl -sSf -XGET ${WHERE}/configuration/TopicPolicy | grep "HATopicP"`
  rc=$?
  echo ${rc}
  echo ${output}
  if [[ "${output}" != "" ]] ; then
    echo incorrect response 
    scriptrc=1 
  fi

  output=`curl -sSf -XGET ${WHERE}/configuration/Endpoint | grep "HAEndpoint"`
  rc=$?
  echo ${rc}
  echo ${output}
  if [[ "${output}" != "" ]] ; then
    echo incorrect response 
    scriptrc=1 
  fi
  
  #ssh ${WHERE} imaserver list QueueManagerConnection 2>&1  | grep "HA_QMC_AJ" 
  #rc=$?
  #if [[ $rc -ne 1 ]] ; then
    #echo incorrect response 
    #doCLICommand imaserver list QueueManagerConnection
    #echo result=$result
    #scriptrc=1 
  #fi
  
  #ssh ${WHERE} imaserver list DestinationMappingRule 2>&1  | grep "HA_DMR_AJ" 
  #rc=$?
  #if [[ $rc -ne 1 ]] ; then
    #echo incorrect response 
    #doCLICommand imaserver list DestinationMappingRule
    #echo result=$result
    #scriptrc=1 
  #fi

exit $scriptrc
