#!/bin/bash
#------------------------------------------------------------------------------
# Script for controlling a server
# Works with both Docker and RPM installs
#------------------------------------------------------------------------------


# Stops the server using the REST API
function stopServer {
  echo "[`date +"%Y/%m/%d %T:%3N %Z"`] >>> stopServer" | tee -a ${LOG}
  
  curl -sSf -XPOST -d '{"Service":"Server"}' http://${aHostVal}:${aPortVal}/ima/service/stop

  sleep 5
  
  checkStopped
  exitSuccess

}

# Starts the server for Docker or RPM
# If -s option specified, check that the server comes up into
# the expected status
function startServer {
  echo "[`date +"%Y/%m/%d %T:%3N %Z"`] >>> startServer" | tee -a ${LOG}

  if [[ "${aTypeVal}" == "DOCKER" ]] ; then
    ssh ${aUserVal}@${aHostVal} docker start ${aSrvContIDVal}

  elif [[ "${aTypeVal}" == "RPM" ]] ; then
    ssh ${aUserVal}@${aHostVal} "systemctl start imaserver"

  fi

  sleep 10

  if [[ "${expectedStatus}" != "" ]] ; then
    if [[ "${timeout}" == "" ]] ; then
      timeout=15
    fi
    checkStatus
  fi
  exitSuccess

}

# Kills the server for Docker or RPM
function killServer {
  echo "[`date +"%Y/%m/%d %T:%3N %Z"`] >>> killServer" | tee -a ${LOG}

  if [[ "${aTypeVal}" == "DOCKER" ]] ; then
    ssh ${aUserVal}@${aHostVal} docker kill ${aSrvContIDVal}

  elif [[ "${aTypeVal}" == "RPM" ]] ; then
    pid=`ssh ${aUserVal}@${aHostVal} ps -ef | grep imaserver | grep -v grep | awk -F' ' '{ print $2 }'`
    #echo imaserver PID=${pid} | tee -a ${LOG}
    if [[ "${pid}" == "" ]] ; then
      echo "[`date +"%Y/%m/%d %T:%3N %Z"`] imaserver process is not running" | tee -a ${LOG}
      exitFail
    fi
    
#    ssh ${aUserVal}@${aHostVal} kill -9 ${pid}
    ssh ${aUserVal}@${aHostVal} "systemctl stop imaserver"    
    
  fi

  sleep 5
    
  checkStopped
  exitSuccess
    
}

#runs tcpkill 
function runTcpkil {
  echo "[`date +"%Y/%m/%d %T:%3N %Z"`] >>> runTcpkill" | tee -a ${LOG}
  
  iface=`ssh ${aUserVal}@${aHostVal} ip addr | grep ${aHostVal} | awk -F ' ' '{print $7}'`
  echo "running tcpkill on interface ${iface} port ${tkport}" | tee -a ${LOG}
  ssh ${aUserVal}@${aHostVal} tcpkill -i ${iface} port ${tkport} >> ${LOG} &
  #25001 - control port, 25002 - messagingport
  
  #checks if tcpkill is running, not necessarily if it is killing correctly
  pid=`ssh ${aUserVal}@${aHostVal} ps -ef | grep tcpkill | grep -v grep | awk -F' ' '{ print $2 }'`
  echo tcpkill PID=${pid} | tee -a ${LOG}
#if [[ "${pid}" == "" ]] ; then
    #echo "tcpkill process is not running" | tee -a ${LOG}
    #exitFail
#fi
  echo "tcpkill running.." | tee -a ${LOG}
  exitSuccess

}

function endTcpkilProcess {
  echo "[`date +"%Y/%m/%d %T:%3N %Z"`] >>> endTcpkillProcess" | tee -a ${LOG}
  
  pid=`ssh ${aUserVal}@${aHostVal} ps -ef | grep tcpkill | grep -v grep | awk -F' ' '{ print $2 }'`
  echo tcpkill PID=${pid} | tee -a ${LOG}
  if [[ "${pid}" == "" ]] ; then
      echo "tcpkill process is not running" | tee -a ${LOG}
      exitFail
  fi
  echo "killing tcpkill processes.." | tee -a ${LOG}
  #ssh ${aUserVal}@${aHostVal} ps -ef | grep tcpkill | grep -v grep | awk '{print $2}' | xargs kill | tee -a ${LOG}
  ssh ${aUserVal}@${aHostVal} kill -9 ${pid} | tee -a ${LOG}
  echo "done killing tcpkill processes" | tee -a ${LOG}
  ssh ${aUserVal}@${aHostVal} ps -ef | grep tcpkill | grep -v grep | tee -a ${LOG} 

  pid=`ssh ${aUserVal}@${aHostVal} ps -ef | grep tcpkill | grep -v grep | awk -F' ' '{ print $2 }'`
  echo tcpkill PID=${pid} | tee -a ${LOG}
  if [[ "${pid}" != "" ]] ; then
      echo "tcpkill process still running" | tee -a ${LOG}
      exitFail
  fi

  exitSuccess

}


# Restarts the server using the REST API
function restartServer {
  echo "[`date +"%Y/%m/%d %T:%3N %Z"`] >>> restartServer" | tee -a ${LOG}

  payload='{"Service":"Server"}'
  if [[ "$1" == "clean" ]] ; then
    payload="{\"Service\":\"Server\",\"CleanStore\":true}"
  elif [[ "$1" == "maintenanceStart" ]] ; then
    payload="{\"Service\":\"Server\",\"Maintenance\":\"start\"}"
  elif [[ "$1" == "maintenanceStop" ]] ; then
    payload="{\"Service\":\"Server\",\"Maintenance\":\"stop\"}"
  elif [[ "$1" == "resetConfig" ]] ; then
    payload="{\"Service\":\"Server\",\"Reset\":\"config\"}"
  fi
  curl -sSf -XPOST -d "${payload}" http://${aHostVal}:${aPortVal}/ima/service/restart

  sleep 10

  if [[ "${expectedStatus}" != "" ]] ; then
    if [[ "${timeout}" == "" ]] ; then
      timeout=60
    fi
    checkStatus
  fi
  exitSuccess

}

# Check that the server is running using service/status REST API
function checkRunning {
  echo "[`date +"%Y/%m/%d %T:%3N %Z"`] >>> checkRunning" | tee -a ${LOG}

  statusResponse=`curl -sSf -XGET http://${aHostVal}:${aPortVal}/ima/service/status 2>/dev/null`
  echo "[`date +"%Y/%m/%d %T:%3N %Z"`] checkRunning > server status:" | tee -a ${LOG}
  echo -e "${statusResponse}\n" | tee -a ${LOG}
  StateDescription=`echo $statusResponse| python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Server\"][\"StateDescription\"]"`

  if [[ "${StateDescription}" =~ "Running" ]] || [[ "${StateDescription}" =~ "Standby" ]] ; then
    echo "[`date +"%Y/%m/%d %T:%3N %Z"`] Server is running" | tee -a ${LOG}
    exitSuccess
  else
    echo "[`date +"%Y/%m/%d %T:%3N %Z"`] FAILURE: Server is not running" | tee -a ${LOG}
    exitFail
  fi

}

# Check that the server is stopped
function checkStopped {
  echo "[`date +"%Y/%m/%d %T:%3N %Z"`] >>> checkStopped" | tee -a ${LOG}

  sleep 3
  if [[ "${timeout}" == "" ]] ; then
    timeout=1
  fi
  
  done=0
  elapsed=0
  while [ ${elapsed} -lt ${timeout} ] ; do

    # Should not connect to admin endpoint
    statusResponse=`curl -sSf -XGET http://${aHostVal}:${aPortVal}/ima/service/status 2>/dev/null`
    echo "[`date +"%Y/%m/%d %T:%3N %Z"`] Status at start of checkStopped:" | tee -a ${LOG}
    echo -e "${statusResponse}\n" | tee -a ${LOG}
  
    if [[ "${statusResponse}" == "" ]] ; then
      done=1
      break
    fi

    # Process or container should not be running
    if [[ "${aTypeVal}" == "DOCKER" ]] ; then
      running=`ssh ${aUserVal}@${aHostVal} docker ps -a | grep ${aSrvContIDVal} | grep Exited`
      echo "[`date +"%Y/%m/%d %T:%3N %Z"`] checkStopped DOCKER > docker ps -a output:" | tee -a ${LOG}
      echo -e "${running}" | tee -a ${LOG}
    
      if [[ "${running}" != "" ]] ; then
        done=1
        break
      fi

    elif [[ "${aTypeVal}" == "RPM" ]] ; then
      running=`ssh ${aUserVal}@${aHostVal} ps -ef | grep imaserver | grep -v grep`
      echo "[`date +"%Y/%m/%d %T:%3N %Z"`] checkStopped RPM > ps -ef output:" | tee -a ${LOG}
      echo -e "${running}\n" | tee -a ${LOG}
    
      if [[ "${running}" == "" ]] ; then
        done=1
        break
      fi
    
    fi
  
    sleep 1
    ((elapsed+=1))
  done
  
  if [ ${done} -ne 1 ] ; then
    echo "[`date +"%Y/%m/%d %T:%3N %Z"`] FAILURE: Server is not stopped" | tee -a ${LOG}
    exitFail
  fi
  echo "[`date +"%Y/%m/%d %T:%3N %Z"`] Server is stopped" | tee -a ${LOG}
  exitSuccess
  
}

# Check that a server is running with a specific status
function checkStatus {
  echo "[`date +"%Y/%m/%d %T:%3N %Z"`] >>> checkStatus" | tee -a ${LOG}

  done=0
  elapsed=0
  while [ ${elapsed} -lt ${timeout} ] ; do
    echo $(date) "[`date +"%Y/%m/%d %T:%3N %Z"`] Elapsed:" ${elapsed} | tee -a ${LOG}
    statusResponse=`curl -sSf -XGET http://${aHostVal}:${aPortVal}/ima/service/status 2>/dev/null`
    StateDescription=`echo $statusResponse | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Server\"][\"StateDescription\"]"`

    if [[ "${StateDescription}" =~ "${expectedStatus}" ]] ; then
      done=1
      break
    fi

    sleep 1
    ((elapsed+=1))
  done

  echo "[`date +"%Y/%m/%d %T:%3N %Z"`] Status at end of checkStatus loop:" | tee -a ${LOG}
  echo -e "${statusResponse}\n" | tee -a ${LOG}
  sleep 3
  if [ ${done} -ne 1 ] ; then
    echo "[`date +"%Y/%m/%d %T:%3N %Z"`] FAILURE: did not reach expected server status" | tee -a ${LOG}
    exitFail
  fi
  exitSuccess

}

# Set the status string for checkStatus action
function setExpectedStatus {
  case ${status} in
  production )
    expectedStatus="Running (production)"
    ;;
  maintenance )
    expectedStatus="Running (maintenance)"
    ;;
  stopped )
    expectedStatus=""
    ;;
  standby )
    expectedStatus="Standby"
    ;;
  esac
}

function exitSuccess {
  statusResponse=`curl -sSf -XGET http://${aHostVal}:${aPortVal}/ima/service/status 2>/dev/null`
  echo "[`date +"%Y/%m/%d %T:%3N %Z"`] Exiting successfully. Server status:" | tee -a ${LOG}
  echo -e "${statusResponse}\n" | tee -a ${LOG}

  echo "[`date +"%Y/%m/%d %T:%3N %Z"`] serverControl.sh exited successfully" | tee -a ${LOG}
  exit 0
}

function exitFail {
  statusResponse=`curl -sSf -XGET http://${aHostVal}:${aPortVal}/ima/service/status 2>/dev/null`
  echo "[`date +"%Y/%m/%d %T:%3N %Z"`] Exiting unsuccessfully. Server status:" | tee -a ${LOG}
  echo -e "${statusResponse}\n" | tee -a ${LOG}

  echo "[`date +"%Y/%m/%d %T:%3N %Z"`] serverControl.sh exited with errors" | tee -a ${LOG}
  exit 1
}

function printUsage {
  echo "serverControl.sh Usage"
  echo "  ./serverControl.sh [-h] -a <action> -i <appliance ID> [-f <LOG> -s <status> -t <timeout> -p <port>]"
  echo ""
  echo "  action:       The action to take against the server"
  echo "                Available actions: stopServer, killServer, startServer,"
  echo "                                   restartServer, restartMaintenance, restartProduction"
  echo "                                   checkRunning, checkStopped, checkStatus"
  echo "  appliance ID: The ISMsetup.sh A_COUNT index for the server to control"
  echo "  status:       The expected status for checkStatus action"
  echo "                Available statuses: production, maintenance, stopped, standby"
  echo "  timeout:      The timeout for checkStatus action"
  echo "  port:         The port to kill for tcpkill action"
  echo "  LOG:          The log file for script output"
  echo ""
  exitSuccess
}

#------------------------------------------------------------------------------
# START OF MAIN
#------------------------------------------------------------------------------
LOG=serverControl.log

while getopts "a:f:hi:s:t:p:" opt; do
  case ${opt} in
  a )
    action=${OPTARG}
    ;;
  f )
    LOG=${OPTARG}
    ;;
  h )
    printUsage
    ;;
  i )
    appliance=${OPTARG}
    ;;
  s )
    status=${OPTARG}
    ;;
  t )
    timeout=${OPTARG}
    ;;
  p )
    tkport=${OPTARG}
    ;;
  * )
    exitFail
    ;;
  esac
done

echo "[`date +"%Y/%m/%d %T:%3N %Z"`] Entering serverControl.sh with $# arguments: $@" >> ${LOG}

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

if [[ "${action}" == "" ]] ; then
  echo "Error: -a <action> is a required field" | tee -a ${LOG}
  echo "" | tee -a ${LOG}
  printUsage
fi
if [[ "${appliance}" == "" ]] ; then
  echo "Error: -i <appliance ID> is a required field" | tee -a ${LOG}
  echo "" | tee -a ${LOG}
  printUsage
fi

echo "APPLIANCE: ${appliance}" | tee -a ${LOG}
echo "ACTION: ${action}" | tee -a ${LOG}
echo -e "User=${aUserVal} Host=${aHostVal} Port=${aPortVal} Type=${aTypeVal} SrvContID=${aSrvContIDVal}\n" | tee -a ${LOG}

statusResponse=`curl -sSf -XGET http://${aHostVal}:${aPortVal}/ima/service/status 2>/dev/null`
echo "[`date +"%Y/%m/%d %T:%3N %Z"`] Current server status:" | tee -a ${LOG}
echo -e "${statusResponse}\n" | tee -a ${LOG}

case ${action} in
stopServer )
  stopServer
  ;;
startServer )
  if [[ "${status}" != "" ]] ; then
    setExpectedStatus
  fi
  startServer
  ;;
killServer )
  killServer
  ;;
restartServer )
  if [[ "${status}" != "" ]] ; then
    setExpectedStatus
  fi
  restartServer
  ;;
restartCleanStore )
  if [[ "${status}" != "" ]] ; then
    setExpectedStatus
  fi
  restartServer "clean"
  ;;
restartMaintenance )
  status=maintenance
  setExpectedStatus
  restartServer "maintenanceStart"
  ;;
restartProduction )
  status=production
  setExpectedStatus
  restartServer "maintenanceStop"
  ;;
restartReset )
  status=production
  setExpectedStatus
  restartServer "resetConfig"
  ;;
checkRunning )
  checkRunning
  ;;
checkStopped )
  checkStopped
  ;;
runTcpkil )
  runTcpkil
  if [[ "${tkport}" == "" ]] ; then
    echo "Error: -p <port> is a required field" | tee -a ${LOG}
    echo "" | tee -a ${LOG}
    printUsage
  fi
  ;;
endTcpkilProcess )
  endTcpkilProcess
  ;;
checkStatus )
  if [[ "${status}" == "" ]] ; then
    echo "Error: -s <status> is a required field" | tee -a ${LOG}
    echo "" | tee -a ${LOG}
    printUsage
  fi
  if [[ "${timeout}" == "" ]] ; then
    echo "Error: -t <timeout> is a required field" | tee -a ${LOG}
    echo "" | tee -a ${LOG}
    printUsage
  fi
  setExpectedStatus
  checkStatus
  ;;
* )
  echo "FAILURE: invalid action specified" | tee -a ${LOG}
  exitFail
  ;;
esac

exitSuccess
