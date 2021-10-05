#!/bin/bash
#------------------------------------------------------------------------------
# heuristic_xa.sh
#
# TODO: Update api calls here when we know what they will be.....
#
# Script for doing heuristic transaction related thingymabobbers
# tests.properties has test number 9000, 9001, 9002 and 9003 for stopping
# imaserver in either prepare or commit of ima.evilra.rar.
#
# Usage: ./heuristic_xa.sh -a <action> -n <test #> -f <log>
# where:
#    action: commit|rollback
#    test #: a test case number from tests.properties
#    log:    logfile name
#
# returns: 0 on success
#          1 on failure
#
# Heuristic transaction basic test procedure:
#
# 0. (JMSTestDriver) Subscribe to the reply destination and log topic
#    used for the specified test number.
#
# 1. Send request to servlet. This sends a message to an MDB.
# 2. Within the MDB, a global transaction occurs, which will invoke
#    a failure in MessageSight.
# 3. Verify that imaserver process has stopped.
# 4. Stop WAS so that it does not attempt to resolve the transaction itself.
# 5. Restart imaserver process and view subscription and transaction stats.
# 6. ERROR TEST: Attempt to forget transaction in prepared state.
# 7. SUCCESS TEST: Commit or rollback the prepared transaction.
# 8. ERROR TEST: Attempt to commit the completed transaction.
# 9. ERROR TEST: Attempt to rollback the completed transaction.
# 10. Restart WAS.
# 11. SUCCESS TEST: "Verify imaserver stat Transaction" returns no data.
#
# 12. (JMSTestDriver) - Receive or fail to receive the data depending on
#     whether the transaction was committed or rolled back.
#
# TODO: Get this working with weblogic?
#
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# In case of failure, make sure that we restart WAS and imaserver
# before exiting the script.
#------------------------------------------------------------------------------
function abort {
    # Restart imaserver if it is stopped
    cmd="curl -s -S -X GET http://${A1_HOST}:${A1_PORT}/ima/v1/service/status"
    reply=`${cmd}`
    replyRC=$?
    if [ ${replyRC} -eq 7 ] ; then
        ../scripts/serverControl.sh -a startServer -i 1
    fi

    # Restart WAS if it is stopped
    is_was_running=`ssh ${WASAddress} ps -ef | grep java | grep server1`
    if [[ "${is_was_running}" == "" ]] ; then
        start_was
    fi

    echo "FAILED: heuristic_xa.sh did not complete successfully." | tee -a ${LOG}
    exit 1
}

function stop_was {
    if [ ${cluster} -eq 0 ] ; then
        cmd="ssh ${WASAddress} ${WASPath}/bin/stopServer.sh server1 -profileName AppSrv01 -user admin -password admin"
        reply=`${cmd}`
        echo -e "${reply}\n" | tee -a ${LOG}
    else
        cmd="ssh ${WASAddresses[0]} ${WASPath}/bin/stopServer.sh server1 -profileName AppSrv01 -user admin -password admin"
        reply=`${cmd}`
        echo -e "${reply}\n" | tee -a ${LOG}
        cmd="ssh ${WASAddresses[0]} ${WASPath}/bin/stopServer.sh server2 -profileName AppSrv01 -user admin -password admin"
        reply=`${cmd}`
        echo -e "${reply}\n" | tee -a ${LOG}
        cmd="ssh ${WASAddresses[1]} ${WASPath}/bin/stopServer.sh server3 -profileName AppSrv02 -user admin -password admin"
        reply=`${cmd}`
        echo -e "${reply}\n" | tee -a ${LOG}
    fi
}

function start_was {
    if [ ${cluster} -eq 0 ] ; then
        cmd="ssh ${WASAddress} ${WASPath}/bin/startServer.sh -profileName AppSrv01 server1"
        reply=`${cmd}`
        echo -e "${reply}\n" | tee -a ${LOG}
    else
        cmd="ssh ${WASAddresses[0]} ${WASPath}/bin/startServer.sh -profileName AppSrv01 server1"
        reply=`${cmd}`
        echo -e "${reply}\n" | tee -a ${LOG}
        cmd="ssh ${WASAddresses[0]} ${WASPath}/bin/startServer.sh -profileName AppSrv01 server2"
        reply=`${cmd}`
        echo -e "${reply}\n" | tee -a ${LOG}
        cmd="ssh ${WASAddresses[1]} ${WASPath}/bin/startServer.sh -profileName AppSrv02 server3"
        reply=`${cmd}`
        echo -e "${reply}\n" | tee -a ${LOG}
    fi
}

function verify_status {
    timeout=15
    elapsed=0
    while [ $elapsed -lt $timeout ] ; do
        echo -n "*"
        sleep 1
        cmd="curl -s -S -X GET http://${A1_HOST}:${A1_PORT}/ima/v1/service/status"
        reply=`${cmd} 2>/dev/null`
        replyRC=$?
        if [ ${expectedRC} != 0 ] ; then
            if [ ${expectedRC} -eq ${replyRC} ] ; then
                break
            fi
        fi
        if [[ "$1" != "" ]] && [[ "$reply" =~ "$1" ]] ; then
            break
        fi
        elapsed=`expr $elapsed + 1`
    done

    cmd="curl -s -S -X GET http://${A1_HOST}:${A1_PORT}/ima/v1/service/status"
    reply=`${cmd} 2>/dev/null`
    if ! [[ "${reply}" =~ "$1" ]] ; then
        echo -e "FAILURE: Current status:\n\"${reply}\"\nExpected status:\n\"$1\""  | tee -a ${LOG}
        abort
    fi
}

function runRESTAPI {
  reply=`${cmd}`
  replyRC=$?
  if [ ${replyRC} -ne ${expectedRC} ] ; then
    echo "FAILURE: ${cmd}"
    abort
  fi
  echo "${reply}"
}

function print_usage {
    echo "Usage:  heuristic_xa.sh -n <test #> -a <action> -f log"
    echo ""
    echo "where:"
    echo "  test #:  corresponds to a test # in tests.properties"
    echo "  action:  commit | rollback"
    echo "  log:     logfile name"
    exit 1
}

action=""
LOG=""
testnum=""

echo "Entering heuristic_xa.sh with $# arguments: $@"

# Parse arguments
#  -a action
#  -f log
#  -n test #
while getopts "a:f:n:" opt; do
    case $opt in
    a )
        action=${OPTARG}
        if [[ "${action}" != "commit" && "${action}" != "rollback" ]] ; then
            print_usage
        fi
        ;;
    f )
        LOG=${OPTARG}
        ;;
    n )
        testnum=${OPTARG}
        if ! [[ ${testnum} =~ ^[0-9]+$ ]] ; then
            print_usage
        fi
        ;;
    * )
        exit 1
        ;;
    esac
done

if [[ "${testnum}" == "" || "${action}" == "" ]] ; then
    print_usage
fi

if [[ "${LOG}" == "" ]] ; then
    LOG="./heuristic_xa.${testnum}_${action}.log"
fi

# source the automation env variables
if [[ -f "../testEnv.sh" ]] ; then
    source "../testEnv.sh"
else
    source ../scripts/ISMsetup.sh
fi

# Figure out whether we are running against a WAS cluster or individual server
if [ -f "server.definition" ] ; then
    cluster=`grep -r -i cluster server.definition | grep -v grep`
    if [[ "${cluster}" == "" ]] ; then
        cluster=0
    else
        cluster=1
        WASAddresses[0]=`cat server.definition | cut -d';' -f 2`
        WASAddresses[1]=`cat server.definition | cut -d';' -f 3`
    fi
else
    cluster=0
fi

# Get the IP address to ssh commands to WAS server
WASAddress=`echo ${WASIP} | cut -d':' -f 1`

# Kick off the test through the servlet
testcase="JCAServlet?param=${testnum}"
testurl="http://${WASIP}/testTools_JCAtests/${testcase}"
echo "Kicking off test from servlet: ${testurl}" | tee -a ${LOG}
reply=`wget -O JCAServlet.${testnum}.${action} ${testurl}`
wgetRC=$?
echo "Servlet response:" | tee -a ${LOG}
cat "JCAServlet.${testnum}.${action}" | tee -a ${LOG}

# Verify the servlet ran correctly
response=`cat JCAServlet.${testnum}.${action} | grep "Sent message to Destination"`
if [[ "${response}" != "" && ${wgetRC} == 0 ]] ; then
    echo -e "The servlet ran successfully\n" | tee -a ${LOG}
    rm JCAServlet.${testnum}.${action}
    # Not clear why the server isn't stopping after the servlet runs. Maybe its just
    # ridiculously slow compared to running against a KVM.
    sleep 30
else
    echo "FAILURE: The servlet failed to send its message" | tee -a ${LOG}
    exit 1
fi

# The evil RA should have killed imaserver by now
# curl rc 7 == couldn't connect to server
expectedRC=7
verify_status ""

# Stop WAS so that it doesn't resolve the transaction as soon
# as imaserver is restarted
stop_was

# Restart imaserver so we can see our prepared transaction
#ssh ${A1_USER}@${A1_HOST} docker start ${A1_SRVCONTID}
../scripts/serverControl.sh -a startServer -i 1
../scripts/serverControl.sh -a checkStatus -i 1 -t 15 -s production

# Make sure imaserver is back in production mode
expectedRC=0
verify_status "Running (production)"

# Get stats of subscriptions before doing anything to the transaction
cmd="curl -s -S -X GET http://${A1_HOST}:${A1_PORT}/ima/v1/monitor/Subscription?ClientID=heur*"
reply=`${cmd}`

# Hopefully this only gets durableCMTRsub and durableBMTUTsub.
# If it gets more than that... not really concerned.
cmd="curl -s -S -X GET http://${A1_HOST}:${A1_PORT}/ima/v1/monitor/Subscription?SubName=durable*sub"
reply=`${cmd}`

# Get the XID of our Prepared transaction
echo -e "Get the XID of our prepared transaction\n" | tee -a ${LOG}

XID=`curl -XGET -sS http://${A1_HOST}:${A1_PORT}/ima/v1/monitor/Transaction?TranState=Prepared | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Transaction\"][0][\"XID\"]"`

if [[ "${XID}" == "" ]] ; then
    echo "FAILURE: No transactions were found" | tee -a ${LOG}
    abort
fi

# Try other imaserver stat Transaction arguments (XID)
echo "Monitor transaction ?XID= should return the existing XID"
cmd="curl -s -S -X GET http://${A1_HOST}:${A1_PORT}/ima/v1/monitor/Transaction?XID=${XID}"
expectedRC=0
runRESTAPI
xidFilter=`echo ${reply} | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Transaction\"][0][\"XID\"]" 2>/dev/null`
if [[ "${xidFilter}" == "" ]] ; then
  echo "FAILURE: Transaction monitor result empty for XID=${XID}"
  abort
fi

# Try other imaserver stat Transaction arguments (TranState)
echo "Monitor transaction ?TranState=Prepared should return the existing XID"
cmd="curl -s -S -X GET http://${A1_HOST}:${A1_PORT}/ima/v1/monitor/Transaction?TranState=Prepared"
expectedRC=0
runRESTAPI
preparedFilter=`echo ${reply} | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Transaction\"][0][\"XID\"]" 2>/dev/null`
if [[ "${preparedFilter}" == "" ]] ; then
  echo "FAILURE: Transaction monitor result empty for TranState=Prepared"
  abort
fi

# Try other imaserver stat Transaction arguments (StatType)
echo "Monitor transaction ?StatType=LastStateChangeTime should return the existing XID"
cmd="curl -s -S -X GET http://${A1_HOST}:${A1_PORT}/ima/v1/monitor/Transaction?StatType=LastStateChangeTime"
expectedRC=0
runRESTAPI
lastChangedFilter=`echo ${reply} | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Transaction\"][0][\"XID\"]" 2>/dev/null`
if [[ "${lastChangedFilter}" == "" ]] ; then
  echo "FAILURE: Transaction monitor result empty for StatType=LastStateChangeTime"
  abort
fi

# Try other imaserver stat Transaction arguments (ResultCount)
echo "Monitor transaction ?ResultCount=10 should return the existing XID"
cmd="curl -s -S -X GET http://${A1_HOST}:${A1_PORT}/ima/v1/monitor/Transaction?ResultCount=10"
expectedRC=0
runRESTAPI
countFilter=`echo ${reply} | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Transaction\"][0][\"XID\"]" 2>/dev/null`
if [[ "${countFilter}" == "" ]] ; then
  echo -e "FAILURE: Transaction monitor result empty for ResultCount=10\n\t"
  abort
fi

# Try other imaserver stat Transaction arguments (TranState)
echo "Monitor transaction ?StatType=Heuristic should return no XID"
cmd="curl -s -S -X GET http://${A1_HOST}:${A1_PORT}/ima/v1/monitor/Transaction?TranState=Heuristic"
# expected RC 0 because succeeds but result array is empty
expectedRC=0
runRESTAPI
emptyArr=`echo ${reply} | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Transaction\"][0]" 2>/dev/null`
if [[ "${emptyArr}" != "" ]] ; then
  echo -e "FAILURE: Transaction monitor result not empty for TranState=Heuristic\n\t${emptyArr}"
  abort
fi

# Attempt to forget transaction in prepared state. This should fail
echo "Attempting to forget a prepared global transaction. This should fail" | tee -a ${LOG}
cmd="curl -fsS -X POST -d "{\"XID\":\"${XID}\"}" http://${A1_HOST}:${A1_PORT}/ima/v1/service/forget/transaction"
expectedRC=22
runRESTAPI

# If $2 was commit, heuristically commit the transaction
# If $2 was rollback, heuristically rollback the transaction
if [[ "${action}" == "commit" ]] ; then
    echo "Attempting to heuristically committing global transaction" | tee -a ${LOG}
    cmd="curl -s -S -X POST -d "{\"XID\":\"${XID}\"}" http://${A1_HOST}:${A1_PORT}/ima/v1/service/commit/transaction"
    expectedRC=0
    runRESTAPI

    # Try other imaserver stat Transaction arguments (TranState)
    echo "Monitor transaction ?TranState=HeuristicCommit should return the existing XID"
    cmd="curl -s -S -X GET http://${A1_HOST}:${A1_PORT}/ima/v1/monitor/Transaction?TranState=HeuristicCommit"
    expectedRC=0
    runRESTAPI
    heurCommitFilter=`echo ${reply} | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Transaction\"][0][\"XID\"]" 2>/dev/null`
    if [[ "${heurCommitFilter}" == "" ]] ; then
      echo -e "FAILURE: Transaction monitor result empty for TranState=HeuristicCommit\n\t"
      abort
    fi

elif [[ "${action}" == "rollback" ]] ; then
    echo "Attempting to heuristically rollback global transaction" | tee -a ${LOG}
    cmd="curl -s -S -X POST -d "{\"XID\":\"${XID}\"}" http://${A1_HOST}:${A1_PORT}/ima/v1/service/rollback/transaction"
    expectedRC=0
    runRESTAPI

    # Try other imaserver stat Transaction arguments (TranState)
    echo "Monitor transaction ?TranState=HeuristicRollback should return the existing XID"
    cmd="curl -s -S -X GET http://${A1_HOST}:${A1_PORT}/ima/v1/monitor/Transaction?TranState=HeuristicRollback"
    expectedRC=0
    runRESTAPI
    heurRollbackFilter=`echo ${reply} | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Transaction\"][0][\"XID\"]" 2>/dev/null`
    if [[ "${heurRollbackFilter}" == "" ]] ; then
      echo -e "FAILURE: Transaction monitor result empty for TranState=HeuristicRollback\n\t"
      abort
    fi

fi

# Try other imaserver stat Transaction arguments (TranState)
echo "Monitor transaction ?TranState=Heuristic should return no XID"
cmd="curl -sS http://${A1_HOST}:${A1_PORT}/ima/v1/monitor/Transaction?TranState=Heuristic"
expectedRC=0
runRESTAPI
heuristicFilter=`echo ${reply} | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Transaction\"][0][\"XID\"]" 2>/dev/null`
if [[ "${heuristicFilter}" == "" ]] ; then
  echo -e "FAILURE: Transaction monitor result empty for TranState=Heuristic\n\t"
  abort
fi

# Try other imaserver stat Transaction arguments (TranState)
echo "Monitor transaction ?TranState=Prepared should return no XID"
cmd="curl -sS http://${A1_HOST}:${A1_PORT}/ima/v1/monitor/Transaction?TranState=Prepared"
expectedRC=0
runRESTAPI
emptyArr=`echo ${reply} | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Transaction\"][0]" 2>/dev/null`
if [[ "${emptyArr}" != "" ]] ; then
  echo -e "FAILURE: Transaction monitor result not empty for TranState=Heuristic\n\t${emptyArr}"
  abort
fi

# Attempt to commit an already completed transaction
echo "Attempting to heuristically commit global transaction. This should fail" | tee -a ${LOG}
cmd="curl -fsS -X POST -d "{\"XID\":\"${XID}\"}" http://${A1_HOST}:${A1_PORT}/ima/v1/service/commit/transaction"
expectedRC=22
runRESTAPI

# Attempt to rollback an already completed transaction
echo "Attempting to heuristically rollback global transaction. This should fail" | tee -a ${LOG}
cmd="curl -fsS -X POST -d "{\"XID\":\"${XID}\"}" http://${A1_HOST}:${A1_PORT}/ima/v1/service/rollback/transaction"
expectedRC=22
runRESTAPI

# Get stats of subscriptions after completing the transaction
cmd="curl -sS http://${A1_HOST}:${A1_PORT}/ima/v1/monitor/Subscription?ClientID=heur*"
expectedRC=0
runRESTAPI
cmd="curl -sS http://${A1_HOST}:${A1_PORT}/ima/v1/monitor/Subscription?SubName=durable*sub"
expectedRC=0
runRESTAPI

# Need to manaully forget transaction after heuristcally completing it
echo "Forgetting heuristically completed global transaction" | tee -a ${LOG}
cmd="curl -sS -XPOST -d "{\"XID\":\"${XID}\"}" http://${A1_HOST}:${A1_PORT}/ima/v1/service/forget/transaction"
expectedRC=0
runRESTAPI

# Restart WAS
start_was

# List the transactions to make sure none are left
cmd="curl -sS http://${A1_HOST}:${A1_PORT}/ima/v1/monitor/Transaction"
expectedRC=0
runRESTAPI
emptyArr=`echo ${reply} | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"Transaction\"][0]" 2>/dev/null`
if [[ "${emptyArr}" != "" ]] ; then
  echo -e "FAILURE: Transaction monitor result not empty for TranState=Heuristic\n\t${emptyArr}"
  abort
fi

# Get stats of subscriptions after restarting WAS
cmd="curl -sS http://${A1_HOST}:${A1_PORT}/ima/v1/monitor/Subscription?ClientID=heur*"
expectedRC=0
runRESTAPI
cmd="curl -sS http://${A1_HOST}:${A1_PORT}/ima/v1/monitor/Subscription?SubName=durable*sub"
expectedRC=0
runRESTAPI

echo "heuristic_xa.sh Test result is Success! The heuristically completed transaction has been resolved" | tee -a ${LOG}
exit 0
