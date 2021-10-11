#!/bin/bash
#------------------------------------------------------------------------------
# Set A1 to be primary with Group=groupone
# Set A2 to be standby with Group=grouptwo
# Ensure they cannot pair, then update A2 Group=groupone
# Restart the servers and ensure they come up paired
# Reset back to non-HA mode
#
# Use a low discovery timeout so that the servers don't take 10 minutes to
# fail and go into maintenance mode.
#------------------------------------------------------------------------------

function run_cli_command {
  cmd="${@}"
  reply=`ssh ${cmd} 2>&1`
  rc=$?
  echo -e "ssh ${cmd}\n${reply}\n${RC}"
}

function restart_servers {
  ${curlPrefix} POST -d '{"Service":"Server"}' ${server1}/service/restart
  ${curlPrefix} POST -d '{"Service":"Server"}' ${server2}/service/restart
}

function verify_status {
  timeout=$3
  failed=0

  for server in ${servers} ; do
    elapsed=0
    expected=$1
    shift
    rc=1
    while [ ${elapsed} -lt ${timeout} ] ; do
      echo -n "*"
      sleep 1
      cmd="${curlPrefix} GET ${server}/service/status"
      reply=`${cmd}`
      StateDescription=`echo $reply | python3 -c "import json,sys;obj=json.load(sys.stdin);print(obj[\"Server\"][\"StateDescription\"])"`
      if [[ "${StateDescription}" =~ "${expected}" ]] ; then
        rc=0
        break
      fi
      elapsed=`expr ${elapsed} + 1`
    done
    echo -e "${server} Status:\n${reply}\n"
    echo -e "${server} StateDescription:${StateDescription}"

    if [ ${rc} -eq 1 ] ; then
      failed=1
      echo "FAILURE: Status for ${server} was not as expected"
      echo -e "Expected:\n${expected}\nActual:\n${StateDescription}\n"
    fi
  done

  if [ ${failed} -eq 1 ] ; then
    echo "abort"
  fi
}

function abort {
  for server in ${servers} ; do
    run_cli_command "${server} imaserver status"
    if [[ "${reply}" =~ "${STOPPED}" ]] ; then
      run_cli_command "${server} imaserver start"
    fi
    # Give the server some time to restart
    sleep 30
    # Disable HA and set back to production mode and hope things work OK?
    # With any luck we will never get here anyways
    run_cli_command "${server} imaserver update HighAvailability EnableHA=False"
    run_cli_command "${server} imaserver runmode production"
    run_cli_command "${server} imaserver stop force"
    sleep 3
    run_cli_command "${server} imaserver start"
  done
}

RUNNING="Running (production)"
STANDBY="Standby"
MAINTENANCE="Running (maintenance)"
STOPPED="Status = Stopped"

server1="http://${A1_HOST}:${A1_PORT}/ima"
server2="http://${A2_HOST}:${A2_PORT}/ima"
servers="${server1} ${server2}"
curlPrefix="curl -sSfX"

# Enable HA with mismatched Group names
${curlPrefix} POST -d "{\"HighAvailability\":{\"LocalDiscoveryNIC\":\"${A1_IPv4_1}\",\"LocalReplicationNIC\":\"${A1_IPv4_1}\",\"RemoteDiscoveryNIC\":\"${A2_IPv4_1}\",\"EnableHA\":true,\"Group\":\"${MQKEY}HAGroupOne\",\"DiscoveryTimeout\":30}}" ${server1}/configuration
${curlPrefix} POST -d "{\"HighAvailability\":{\"LocalDiscoveryNIC\":\"${A2_IPv4_1}\",\"LocalReplicationNIC\":\"${A2_IPv4_1}\",\"RemoteDiscoveryNIC\":\"${A1_IPv4_1}\",\"EnableHA\":true,\"Group\":\"${MQKEY}HAGroupTwo\",\"DiscoveryTimeout\":30}}" ${server2}/configuration
restart_servers

# Verify that the machines do not pair
verify_status "${MAINTENANCE}" "${MAINTENANCE}" 45

# Update A2 Group to match A1
${curlPrefix} POST -d "{\"HighAvailability\":{\"Group\":\"${MQKEY}HAGroupOne\"}}" ${server2}/configuration
restart_servers

# Verify that the machines are paired
verify_status "${RUNNING}" "${STANDBY}" 45

# Update group dynamically on primary (Should succeed)
${curlPrefix} POST -d "{\"HighAvailability\":{\"Group\":\"${MQKEY}HAGroupPrimary\"}}" ${server1}/configuration
${curlPrefix} GET ${server1}/configuration/HighAvailability
if ! [[ "${reply}" =~ "\"Group\": \"${MQKEY}HAGroupPrimary\"" ]] ; then
  echo "FAILURE: Group was not updated!"
fi

# Update group dynamically on standby (Should fail)
${curlPrefix} POST -d "{\"HighAvailability\":{\"Group\":\"${MQKEY}HAGroupStandby\"}}" ${server2}/configuration
${curlPrefix} GET ${server2}/configuration/HighAvailability
if [[ "${reply}" =~ "\"Group\": \"${MQKEY}HAGroupStandby\"" ]] ; then
  echo "FAILURE: Group was updated!"
fi

# Disable HA
${curlPrefix} POST -d "{\"HighAvailability\":{\"Group\":\"\",\"EnableHA\":false,\"DiscoveryTimeout\":600}}" ${server1}/configuration
${curlPrefix} POST -d "{\"HighAvailability\":{\"Group\":\"\",\"EnableHA\":false,\"DiscoveryTimeout\":600}}" ${server2}/configuration
restart_servers

# Verify the servers are no longer in HA
verify_status "${RUNNING}" "${RUNNING}" 45
