#!/bin/bash

function trace {
  timestamp=`date +"%Y-%m-%d %H:%M:%S"`
  echo -e "[${timestamp}] $1\n" >> ${LOG}
}

function run_cli_command {
  cmd="$2 ${@:3}"
  trace "${cmd}"
  reply=`ssh ${cmd} 2>&1`
  result=$?
  echo "${reply}" | tee -a ${LOG}
  echo -e "RC=${result}\n" | tee -a ${LOG}
  if [[ "$1" == "x" ]] ; then
    # ignore return code
    return
  elif [ ${result} -ne $1 ] ; then
    echo "FAILED: Expected RC: $1" | tee -a ${LOG}
    abort
  fi
}

function verify_status_individual {
  trace "Verifying status for A${1}. Status will be checked for up to $3 seconds\nExpected status: $2"
  trace "Waiting for status to update."
  statusloop "$3" "$1" "$2"

  if ! [[ "${StateDescription}" =~ "$2" ]] ; then
    if [[ "$4" == "ignore_fail" ]] ; then
      return 1
    fi
    trace "FAILURE: A$1 Status:\n${reply}."
    trace "${appliance} StateDescription:\n${StateDescription}"

    #abort
    echo "abort"
  else
    return 0
  fi
}

function statusloop {
  rc=1
  elapsed=0
  timeout=$1
  appliance=$2
  expectedA=$3
  while [ ${elapsed} -lt ${timeout} ] ; do
    echo -n "*"
    sleep 1
    cmd="curl -sSfXGET ${url[${2}]}/service/status"
    #cmd="ssh $2 status imaserver"
    reply=`${cmd}`
    StateDescription=`echo $reply | python3 -c "import json,sys;obj=json.load(sys.stdin);print(obj[\"Server\"][\"StateDescription\"])"`
    if [[ "${StateDescription}" =~ "${expectedA}" ]] ; then
      rc=0
      echo ""
      break
    fi
    elapsed=`expr ${elapsed} + 1`
  done
  trace "${appliance} Status:\n${reply}"
  trace "${appliance} StateDescription:\n${StateDescription}"
}

LOG=forceResetHA.log
echo "Entering forceResetHA" > ${LOG}

if [[ -f "../testEnv.sh" ]] ; then
  source "../testEnv.sh"
elif [[ -f "../scripts/ISMsetup.sh" ]] ; then
  source ../scripts/ISMsetup.sh
else
  echo "neither testEnv.sh or ISMsetup.sh was used" | tee -a ${LOG}
fi

echo "Checking that both servers are running in production mode" | tee -a ${LOG}

url[0]="http://${A1_IPv4_1}:${A1_PORT}/ima"
url[1]="http://${A2_IPv4_1}:${A2_PORT}/ima"
sshHost[0]="ssh ${A1_USER}@${A1_HOST}"
sshHost[1]="ssh ${A2_USER}@${A2_HOST}"
contID[0]="${A1_SRVCONTID}"
contID[1]="${A2_SRVCONTID}"

numServers=`expr ${#url[@]} - 1`
for index in `seq 0 ${numServers}` ; do
  echo "Fixing server A${index}" | tee -a ${LOG}
  thisURL=${url[${index}]}
  thisSSH=${sshHost[${index}]}
  thisSrvContID=${contID[${index}]}
  
  ${thisSSH} docker stop ${thisSrvContID}
  ${thisSSH} docker start ${thisSrvContID}
  sleep 15
  
  reply=`curl -sSfXGET ${thisURL}/service/status 2>&1`
  StateDescription=`echo $reply | python3 -c "import json,sys;obj=json.load(sys.stdin);print(obj[\"Server\"][\"StateDescription\"])"`

  if ! [[ "${StateDescription}" =~ "Running (production)" ]] ; then
    echo "Server A${index} is not running in production mode." | tee -a ${LOG}
    if [[ "${reply}" =~ "Failed connect" ]] ; then
      ${thisSSH} docker start ${thisSrvContID}
      sleep 10
    fi

    curl -sSfXPOST -d "{\"HighAvailability\":{\"EnableHA\":false,\"Group\":\"\"}}" ${thisURL}/configuration
    curl -sSfXPOST -d "{\"Service\":\"Server\",\"CleanStore\":true}" ${thisURL}/service/restart
    verify_status_individual ${index} "Running (production)" "60"
    echo "Server A${index} is now running in production mode" | tee -a ${LOG}
  else
    echo "Server A${index} is already running in production mode" | tee -a ${LOG}
  fi
done
exit 0

