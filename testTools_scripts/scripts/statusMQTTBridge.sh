#!/bin/bash +x
# started with cAppDriverLog to catch errors
# 
echo " time: `date` : running $0 with $# parameters:  [ $@ ]"


# Source the ISMsetup file to get access to information for the remote machine
if [[ -f "../testEnv.sh"  ]] ; then
  source  "../testEnv.sh"
else
  source ../scripts/ISMsetup.sh
fi

## Establish Defaults
# defect 221779/222555  BX_BRIDGECONTID="IBMWIoTPMessageGatewayBridge"
BX_BRIDGECONTID="imabridge"
## CLI DEFAULTS
SERVER_IDX="B1"
JUST_START="false"
CLEAR_LOGS="false"
LOG_FILE=""
## Local Var Defaults
rc=0 
rc_curl2=0
rc_start=0
# Number of seconds to wait between Forwarder Status attempts and retry Status 
statusSleep=7
statusRetry=10


if [[ "$#" == 0 || "${1}" == "?" || "${1}" == "-h" ]] ; then
    echo "Usage is: "
    echo "     ./startMQTTBridge.sh  -s [SERVER_IDX] -j [JUST_START] -l [CLEAR_LOGS] -f [logfile]"
    echo " where"
    echo "   SERVER_IDX:      B1 and B2 ISMsetup/testEnv Variable prefix desired.                  Default: ${SERVER_IDX} :  B1_USER, etc."
    echo "   JUST_START:      [true|false]  Does not Alter Config, just RESTARTs after a stop.     Default: ${JUST_START}"
    echo "   CLEAR_LOGS:      [true|false]  erase the contents of $BRIDGEHOME/diag/logs for test.  Default: ${CLEAR_LOGS}"
    echo "   logfile:         IGNORED unless running under AF, output is to STDOUT"
    rc=100
else
#set -x


    while getopts "s:j:l:f:" option ${OPTIONS}
      do
#        echo "option=${option}  OPTARG=${OPTARG}"
        case ${option} in
          s ) SERVER_IDX=${OPTARG}
            ;;
          j ) JUST_START=${OPTARG}
            ;;
          l ) CLEAR_LOGS=${OPTARG}
            ;;
          f ) LOG_FILE=${OPTARG}
            ;;
        esac    
      done
      
    echo "Target Bridge ${SERVER_IDX} parameters: "
    echo "    JUST_START:\"${JUST_START}\" , CLEAR_LOGS:\"${CLEAR_LOGS}\" , LOG_FILE:\"${LOG_FILE}\" "
    

    # Which Bridge is the destination for the BRIDGE commands:  B1, B2, ....
    BX_TEMP=${SERVER_IDX}_USER
    BX_USER=$(eval echo \$${BX_TEMP})
    
    BX_TEMP=${SERVER_IDX}_HOST
    BX_HOST=$(eval echo \$${BX_TEMP})

    BX_TEMP=${SERVER_IDX}_BRIDGEROOT
    BX_BRIDGEROOT=$(eval echo \$${BX_TEMP})

    BX_TEMP=${SERVER_IDX}_BRIDGEHOME
    BX_BRIDGEHOME=$(eval echo \$${BX_TEMP})

    BX_TEMP=${SERVER_IDX}_REST_USER
    BX_REST_USER=$(eval echo \$${BX_TEMP})

    BX_TEMP=${SERVER_IDX}_REST_PW
    BX_REST_PW=$(eval echo \$${BX_TEMP})

    BX_TEMP=${SERVER_IDX}_BRIDGEPORT
    BX_BRIDGEPORT=$(eval echo \$${BX_TEMP})

    BX_TEMP=${SERVER_IDX}_TYPE
    BX_TYPE=$(eval echo \$${BX_TEMP})

    BX_TEMP=${SERVER_IDX}_BRIDGECONTID
    BX_BRIDGECONTID=$(eval echo \$${BX_TEMP})

    echo "Target Bridge ${SERVER_IDX} properties:   BX_TYPE:\"${BX_TYPE}\" , BX_BRIDGECONTID:\"${BX_BRIDGECONTID}\" "
    echo "    BX_USER:\"${BX_USER}\" , BX_HOST:\"${BX_HOST}\" , BX_BRIDGEPORT:\"${BX_BRIDGEPORT}\" , BX_REST_USER:\"${BX_REST_USER}\" , BX_REST_PW:\"${BX_REST_PW}\" , BX_BRIDGEHOME:\"${BX_BRIDGEHOME}\" , BX_BRIDGEROOT:\"${BX_BRIDGEROOT}\" "

    if [[ "${CLEAR_LOGS}" == "true" || "${CLEAR_LOGS}" == "TRUE" ]]; then
        echo "CLEARING the old Bridge logs at ${BX_BRIDGEROOT}/diag/logs/ !"
        ssh ${BX_USER}@${BX_HOST} "rm -rf  ${BX_BRIDGEROOT}/diag/logs/*"
    fi

    if [[ ${JUST_START} == "true" || ${JUST_START} == "TRUE" ]] ; then
        echo "`date` : Bridge at ${SERVER_IDX} (${BX_HOST}) config NOT UPDATED, restarted using existing config"
set -x        
	    if [[ "${BX_TYPE}" == "DOCKER" ]] ; then
	        ssh ${BX_USER}@${BX_HOST} "ulimit -c unlimited ; docker start ${BX_BRIDGECONTID}"
	        rc_start=$?
	
	        sleep 1
	        ssh ${BX_USER}@${BX_HOST} "docker ps -a | grep ${BX_BRIDGECONTID}"
	    else
	        ssh ${BX_USER}@${BX_HOST} "ulimit -c unlimited ; systemctl start ${BX_BRIDGECONTID}"
	        rc_start=$?
	
	        sleep 1
	        ssh ${BX_USER}@${BX_HOST} "ps -ef | grep ${BX_BRIDGECONTID}"
	    fi
set +x

        echo "`date` : Bridge at ${SERVER_IDX} (${BX_HOST}) was restarted, checking Config and Stats."
    fi

fi


if [[ ${rc_start} == 0 && ${rc} == 0 ]] ; then
         
#    sleep ${statusSleep}
#echo "++++===================  DEBUG  =========================++++"
#echo "stop server $A1_HOST"
#set -x
#`ssh root@$A1_HOST "docker restart imaserver ; sleep 5 ; curl -X GET http://localhost:9089/ima/service/status"`
#set +x
#echo "++++===================  DEBUG  =========================++++"


    if curl --fail -X POST -u ""${BX_REST_USER}:${BX_REST_PW}"" http://${BX_HOST}:${BX_BRIDGEPORT}/admin/set/TraceLevel/5,mqtt=9,http=6,kafka=6 ; then
        if curl --fail -X GET -u ""${BX_REST_USER}:${BX_REST_PW}"" http://${BX_HOST}:${BX_BRIDGEPORT}/admin/config ; then
                # Loop for ${statusRetry} times hoping to get good results
                    i=0;
                    while [ ${i} -le ${statusRetry} ]; do

                        brstat=$( curl --fail -X GET -u ""${BX_REST_USER}:${BX_REST_PW}"" http://${BX_HOST}:${BX_BRIDGEPORT}/admin/forwarder )
                        rc_curl2=$?
                    
                        # Bridge did not Fail (yet), but IS it REALLY READY?
                        if [[ "${brstat}" == *"\"Status\": \"ConnectSrc\""*  || "${brstat}" == *"\"Status\": \"Wait\""*  ]] ; then
                            echo "`date` :   !!!   CAUTION - retry ${i} of ${statusRetry}    !!!  Bridge FORWARDER Startup is NOT COMPLETE YET:  ${brstat}"
                            rc_curl2=167
                            i=$((i+1))
                        else    
                            echo "`date` :  FINAL:  Forwarder Statistics:  ${brstat}" 
                            i=$((statusRetry+1))
                
                        fi
#set -x
                        if [[ ${i} -le ${statusRetry} ]]; then
                            sleep ${statusSleep}
                        fi
#set +x
                    done
        else 
            rc_curl2=$?
        fi
    else 
        rc_curl2=$?
    fi

#  set +x
fi

# KEYWORDS IN LOG for AF Success or Failure based on final cumulative RC

rc=$(( ${rc} +  ${rc_start} + ${rc_curl2} ))

if [[ ${rc} == 0 ]] ; then
    echo " time: `date` : ${0}   PASSED: RC=${rc}   Test result is Success!" | tee ${LOG_FILE}
else
    echo " time: `date` : ${0}   FAILED: RC=${rc}" | tee ${LOG_FILE}
fi

#echo "TIMESTAMP AT END OF ${0}  `date`"  
