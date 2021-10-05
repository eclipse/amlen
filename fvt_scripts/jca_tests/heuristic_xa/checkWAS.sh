#!/bin/bash
#------------------------------------------------------------------------------
# checkWAS.sh
#
# The purpose of this script is to make sure the environment is back to
# normal in the case of any heuristic XA test failures. It would be bad for
# automation if after these tests, WAS was just not running anymore...
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
function check_running {
    # Restart imaserver if it is stopped
    run_cli_command 0 status imaserver
    if [[ "${reply}" =~ "Status = Stopped" ]] ; then
        ((errors+=1))
        echo "imaserver was not running. Starting it now." | tee -a ${LOG}
        run_cli_command 0 imaserver start
    else
        echo "imaserver is running" | tee -a ${LOG}
    fi

    # Restart WAS if it is stopped
    is_was_running=`ssh ${WASAddress} ps -ef | grep java | grep com.ibm.ws.runtime.WsServer`
    if [[ "${is_was_running}" == "" ]] ; then
        ((errors+=1))
        echo "WebSphere Application Server was not running. Starting it now." | tee -a ${LOG}
        start_was
    else
        echo "WAS is running" | tee -a ${LOG}
    fi
}

#------------------------------------------------------------------------------
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

#------------------------------------------------------------------------------
function run_cli_command {
    cmd="ssh ${A1_USER}@${A1_HOST} ${@:2}"
    reply=`${cmd} 2>&1`
    rc=$?
    echo -e "${cmd}\n${reply}\nRC=${rc}\n" | tee -a ${LOG}
    if [ ${rc} != $1 ] ; then
        echo "FAILURE: expected RC=$1 actual RC=${rc}" | tee -a ${LOG}
    fi
}

############
### MAIN ###
############

echo "Entering checkWAS.sh"

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

errors=0

# Restart IMA and WAS if they aren't running
check_running

# Clean up any heuristic transactions remaining on the server
run_cli_command 1 imaserver stat Transaction TranState=Prepared

echo -e "Looping through transactions to find prepared transaction\n" | tee -a ${LOG}

XID=""
# Loop through the reply and find any prepared transactions (State = 2)
if [[ "${reply}" =~ "No Transaction data" ]] ; then
    echo "No prepared transactions were found" | tee -a ${LOG}
else
    ((errors+=1))
    for line in ${reply} ; do
        XID=`echo ${line} | cut -d',' -f 1`
        XID="${XID%\"}"
        XID="${XID#\"}"
        if [[ "${XID}" != "" ]] ; then
            echo "Found prepared transaction:" | tee -a ${LOG}
            echo -e "XID:  ${XID}\n" | tee -a ${LOG}
            echo "Attempting to heuristically rollback global transaction" | tee -a ${LOG}
            run_cli_command 0 imaserver rollback Transaction XID=${XID}
        fi
    done
fi

run_cli_command 1 imaserver stat Transaction TranState=Heuristic

echo -e "Looping through transactions to find heuristic transaction\n" | tee -a ${LOG}

XID=""
# Loop through the reply and find any heuristically 
# prepared/committed transctions (State = 5/6)
if [[ "${reply}" =~ "No Transaction data" ]] ; then
    echo "No prepared transactions were found" | tee -a ${LOG}
else
    ((errors+=1))
    for line in ${reply} ; do
        XID=`echo ${line} | cut -d',' -f 1`
        XID="${XID%\"}"
        XID="${XID#\"}"
        if [[ "${XID}" != "" ]] ; then
            echo "Found heuristic transaction:" | tee -a ${LOG}
            echo -e "XID:  ${XID}\n" | tee -a ${LOG}
            echo "Attempting to forget global transaction" | tee -a ${LOG}
            run_cli_command 0 imaserver forget Transaction XID=${XID}
        fi
    done
fi

if [ ${errors} -eq 0 ] ; then
    echo "checkWAS.sh Test result is Success!" | tee -a ${LOG}
else
    echo "checkWAS.sh did not complete successfully!" | tee -a ${LOG}
fi    
