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
CONFIG_FILE=""
KEYSTORE_FILE=""
TRUSTSTORE_FILE=""
SERVER_IDX="B1"
JUST_START="false"
CLEAR_LOGS="false"
LOG_FILE=""
## Local Var Defaults
rc=0 
rc_curl1=0
rc_curl2=0
rc_stop=0
rc_scp=0
rc_start=0
# Number of seconds to wait between Forwarder Status attempts and retry Status 
statusSleep=7
statusRetry=10


if [[ "$#" == 0 || "${1}" == "?" || "${1}" == "-h" ]] ; then
    echo "Usage is: "
    echo "     ./startMQTTBridge.sh  -c [CONFIG_FILE] -k [KEYSTORE_FILE]  -t [TRUSTSTORE_FILE] -s [SERVER_IDX] -j [JUST_START] -l [CLEAR_LOGS] -f [logfile]"
    echo " where"
    echo "   CONFIG_FILE:     is the config file to start the bridge with."
    echo "                      if no file is specified, a file named '${CONFIG_FILE}' is used." 
    echo "   KEYSTORE_FILE:   if specified admin endpoint cert will be copied to the bridge directory  ${B1_BRIDGEROOT}/keystore."
    echo "   TRUSTSTORE_FILE: if specified server cert will be copied to the bridge directory  ${B1_BRIDGEROOT}/truststore."
    echo "   SERVER_IDX:      B1 and B2 ISMsetup/testEnv Variable prefix desired.                  Default: ${SERVER_IDX} :  B1_USER, etc."
    echo "   JUST_START:      [true|false]  Does not Alter Config, just RESTARTs after a stop.     Default: ${JUST_START}"
    echo "   CLEAR_LOGS:      [true|false]  erase the contents of $BRIDGEHOME/diag/logs for test.  Default: ${CLEAR_LOGS}"
    echo "   logfile:         IGNORED unless running under AF, output is to STDOUT"
    rc=100
else
#set -x


    while getopts "c:k:t:s:j:l:f:" option ${OPTIONS}
      do
#        echo "option=${option}  OPTARG=${OPTARG}"
        case ${option} in
          c ) CONFIG_FILE=${OPTARG}
            ;;
          k ) KEYSTORE_FILE=${OPTARG}
            ;;
          t ) TRUSTSTORE_FILE=${OPTARG}
            ;;
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
      
    # DEST_FILE=$CONFIG_FILE  NEED TO STRIP THE PATH from the input filename like when no $1 is passed like below
    DEST_FILE="${CONFIG_FILE##*/}"
      
    echo "Target Bridge ${SERVER_IDX} parameters: "
    echo "    JUST_START:\"${JUST_START}\" , CONFIG_FILE:\"${CONFIG_FILE}\" , DEST_FILE:\"${DEST_FILE}\" , KEYSTORE_FILE:\"${KEYSTORE_FILE}\" , TRUSTSTORE_FILE:\"${TRUSTSTORE_FILE}\" , CLEAR_LOGS:\"${CLEAR_LOGS}\" , LOG_FILE:\"${LOG_FILE}\" "
    

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

    if [[ ${JUST_START} == "false" ]] ; then
		#!!# HAVE TO STOP/RESTART the Bridge to actually CHANGE a Config 
		#!!#    - POSTs are a QUASI MERGE for Forwarder and Connection subObjects. If What A NEW CONFIG, remove the OLD Config here:
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
	else
	    echo "Existing Bridge Config will NOT be DELETED."
    fi

    # What to UPDATE the Config with CONFIG_FILE, uploaded here    
    if [[ -f ${CONFIG_FILE} ]] ; then
set -x
	    if [[ "${BX_TYPE}" == "DOCKER" ]] ; then
	        ssh ${BX_USER}@${BX_HOST} "docker stop ${BX_BRIDGECONTID}"
	        rc_stop=$?
	    else
	        ssh ${BX_USER}@${BX_HOST} "systemctl stop ${BX_BRIDGECONTID}"
	        rc_stop=$?
	    fi
set +x

	    echo "SCP the ${CONFIG_FILE} to Bridge, it will loaded later."

        scp -r ${CONFIG_FILE}       ${BX_USER}@${BX_HOST}:${BX_BRIDGEROOT}/
        ((rc_scp+=$?))


	else
	    echo "Cannot find config file '${CONFIG_FILE}', will restart Bridge using existing config."
	fi

#
    if [[ -f ${KEYSTORE_FILE} ]] ; then
        scp -r ${KEYSTORE_FILE}  ${BX_USER}@${BX_HOST}:${BX_BRIDGEROOT}/keystore/
        ((rc_scp+=$?))
    else
        if [[ -z ${KEYSTORE_FILE+x} ]]; then
            echo "Cannot find keystore file '${KEYSTORE_FILE}'."
            rc=98
        fi
    fi

#
    if [[ -f ${TRUSTSTORE_FILE} ]] ; then
        scp -r ${TRUSTSTORE_FILE}  ${BX_USER}@${BX_HOST}:${BX_BRIDGEROOT}/truststore/
        ((rc_scp+=$?))
    else
        if [[ -z ${TRUSTSTORE_FILE+x} ]]; then
            echo "Cannot find truststore file '${TRUSTSTORE_FILE}'."
            rc=97
        fi
    fi

#
    if [[ "${CLEAR_LOGS}" == "true" || "${CLEAR_LOGS}" == "TRUE" ]]; then
        echo "CLEARING the old Bridge logs at ${BX_BRIDGEROOT}/diag/logs/ !"
        ssh ${BX_USER}@${BX_HOST} "rm -rf  ${BX_BRIDGEROOT}/diag/logs/*"
    fi
fi


if [[ ${rc} == 0  && ${rc_curl1} == 0 && ${rc_stop} == 0 && ${rc_scp} == 0 ]] ; then
    # -u UPDATE with the desired Bridge Config and restart Bridge to get connections established
    if [[ "${DEST_FILE}" != "" ]]; then
        echo "`date` : Update Bridge at ${SERVER_IDX} (${BX_HOST}) with config ${DEST_FILE}"
        
set -x
        if [[ "${BX_TYPE}" == "DOCKER" ]] ; then
            ssh ${BX_USER}@${BX_HOST} "ulimit -c unlimited ; docker start ${BX_BRIDGECONTID}"
            rc_start=$?
            sleep 3
                ### HARDCODED /var/imabridge becuase the path in ${BRIDGEROOT} is MOUNTed path outside the container
            result=$(ssh ${BX_USER}@${BX_HOST} "docker exec imabridge ls /var/imabridge/${DEST_FILE}")
            if [[ "$?" != "0" ]] ; then
                echo "docker cp ${BX_BRIDGEROOT}/${DEST_FILE} imabridge:/var/imabridge/${DEST_FILE}"
                ssh ${BX_USER}@${BX_HOST} "docker cp ${BX_BRIDGEROOT}/${DEST_FILE} imabridge:/var/imabridge/${DEST_FILE}"
            fi
            ssh ${BX_USER}@${BX_HOST} "docker exec imabridge /bin/sh -c \"${BX_BRIDGEHOME}/imabridge -u /var/imabridge/${DEST_FILE} ; exit 0; \" "
            ## If the bridge runtime cannot pick up the new config, uncomment the following line
            ssh ${BX_USER}@${BX_HOST} "docker stop imabridge && docker start imabridge"
                ### If DEST_FILE is not found - the RC was still 0 - #TODO
            ((rc_start+=$?))

            sleep 3
            ssh ${BX_USER}@${BX_HOST} "docker ps -a | grep ${BX_BRIDGECONTID}"

        else
            ssh ${BX_USER}@${BX_HOST} "cd ${BX_BRIDGEROOT};${BX_BRIDGEHOME}/imabridge -u ${DEST_FILE}"&
            rc_start=$?

            ssh ${BX_USER}@${BX_HOST} "ulimit -c unlimited ; systemctl start ${BX_BRIDGECONTID}"
            ((rc_start+=$?))

            sleep 1
            ssh ${BX_USER}@${BX_HOST} "ps -ef | grep ${BX_BRIDGECONTID}"

        fi
set +x
    else
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
    fi
    
     
    echo "`date` : Bridge at ${SERVER_IDX} (${BX_HOST}) was restarted, checking Config and Stats."
         
    sleep ${statusSleep}
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
                        sleep ${statusSleep}
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

rc=$(( ${rc} + ${rc_stop} + ${rc_scp} + ${rc_start} + ${rc_curl1} + ${rc_curl2} ))

if [[ ${rc} == 0 ]] ; then
    echo " time: `date` : ${0}   PASSED: RC=${rc}   Test result is Success!" | tee ${LOG_FILE}
else
    echo " time: `date` : ${0}   FAILED: RC=${rc}" | tee ${LOG_FILE}
fi

#echo "TIMESTAMP AT END OF ${0}  `date`"  
