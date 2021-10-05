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
CLEAN_CONFIG="false"
KEYSTORE_FILE=""
TRUSTSTORE_FILE=""
SERVER_IDX="B1"
CLEAR_LOGS="false"
LOG_FILE=""
## Local Var Defaults
rc=0 
rc_curl1=0
rc_stop=0

if [[ $# == 0 || "${1}" == "?" || "${1}" == "-h" ]] ; then
    echo "Usage is: "
    echo "     ${0}  -c [CLEAN_CONFIG] -s [SERVER_IDX] -l [CLEAR_LOGS] -f [logfile]"
    echo " where"
    echo "   CLEAN_CONFIG: [true|false] True DELETEs existing Forwarder and Connection Objects. Default ${CLEAN_CONFIG} "
    echo "   SERVER_IDX:   B1 and B2 ISMsetup/testEnv Variable prefix desired.                  Default: ${SERVER_IDX} :  B1_USER, etc."
    echo "   CLEAR_LOGS:   [true|false]  erase the contents of $BRIDGEHOME/diag/logs for test.  Default: ${CLEAR_LOGS}"
    echo "   logfile:      IGNORED unless running under AF, output is to STDOUT"
    rc=100
else
#set -x


    while getopts "c:s:l:f:" option ${OPTIONS}
      do
#        echo "option=${option}  OPTARG=${OPTARG}"
        case ${option} in
          c ) CLEAN_CONFIG=${OPTARG}
            ;;
          s ) SERVER_IDX=${OPTARG}
            ;;
          l ) CLEAR_LOGS=${OPTARG}
            ;;
          f ) LOG_FILE=${OPTARG}
            ;;
        esac    
      done
      
    echo "Target Bridge ${SERVER_IDX} parameters: "
    echo "    CLEAN_CONFIG:\"${CLEAN_CONFIG}\" , CLEAR_LOGS:\"${CLEAR_LOGS}\" , LOG_FILE:\"${LOG_FILE}\" "
    

    # Which Bridge is the destination for the BRIDGE commands:  B1, B2, ....
    BX_TEMP=${SERVER_IDX}_USER
    BX_USER=$(eval echo \$${BX_TEMP})
    
    BX_TEMP=${SERVER_IDX}_HOST
    BX_HOST=$(eval echo \$${BX_TEMP})

    BX_TEMP=${SERVER_IDX}_BRIDGEROOT
    BX_BRIDGEROOT=$(eval echo \$${BX_TEMP})

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
    echo "    BX_USER:\"${BX_USER}\" , BX_HOST:\"${BX_HOST}\" , BX_BRIDGEPORT:\"${BX_BRIDGEPORT}\" , BX_REST_USER:\"${BX_REST_USER}\" , BX_REST_PW:\"${BX_REST_PW}\" , BX_BRIDGEROOT:\"${BX_BRIDGEROOT}\" "

    if [[ ${CLEAN_CONFIG} == "true" ]] ; then
#!!# HAVE TO STOP the Bridge to actually replace the Config - otherwise it is a QUASI MERGE for Forwarder and Connection subObjects ( new reastapi may fix)
        echo "DELETE the Forwarder and Connection Objects from Bridge Config."
        if curl --fail -X DELETE -u ""${BX_REST_USER}:${BX_REST_PW}"" http://${BX_HOST}:${BX_BRIDGEPORT}/admin/config/Forwarder/* ; then
            if curl --fail -X DELETE -u ""${BX_REST_USER}:${BX_REST_PW}"" http://${BX_HOST}:${BX_BRIDGEPORT}/admin/config/Connection/* ; then
                if curl --fail -X GET -u ""${BX_REST_USER}:${BX_REST_PW}"" http://${BX_HOST}:${BX_BRIDGEPORT}/admin/config ; then
                    rc_curl1=$?
                    # Can GET be successful and still be an error like with CURL2
                else 
                    rc_curl1=$?
                fi
            else 
                rc_curl1=$?
            fi
        else 
            rc_curl1=$?
        fi
    fi
set -x
    if [[ "${BX_TYPE}" == "DOCKER" ]] ; then
        ssh ${BX_USER}@${BX_HOST} "docker stop ${BX_BRIDGECONTID}"
        rc_stop=$?
    else
        ssh ${BX_USER}@${BX_HOST} "systemctl stop ${BX_BRIDGECONTID}"
        rc_stop=$?
    fi
set +x
    echo "`date` : Bridge at ${SERVER_IDX} (${BX_HOST}) was stopped."


    if [[ "${CLEAR_LOGS}" == "true" || "${CLEAR_LOGS}" == "TRUE" ]]; then
        echo "CLEARING the old Bridge logs at ${BX_BRIDGEROOT}/diag/logs/ !"
        ssh ${BX_USER}@${BX_HOST} "rm -rf  ${BX_BRIDGEROOT}/diag/logs/*"
    fi

fi
# KEYWORDS IN LOG for AF Success or Failure based on final cumulative RC

rc=$(( ${rc} + ${rc_stop} + ${rc_curl1} ))

if [[ ${rc} == 0 ]] ; then
    echo " time: `date` : ${0}   PASSED: RC=${rc}   Test result is Success!" | tee ${LOG_FILE}
else
    echo " time: `date` : ${0}   FAILED: RC=${rc}" | tee ${LOG_FILE}
fi

#echo "TIMESTAMP AT END OF ${0}  `date`"  c
