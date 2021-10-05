#!/bin/bash +x
##########################
## Env. Variable Defaults
##########################
#---  CLI Default Values when not passed with invocation
declare app="$0"
declare params="$@"
declare MQTT_STAND="lon02-5"			# CLI -s option to compute the MQTT_SRV value dynamically.
							# MQTT_SRV derived from messaging.${MQTT_STAND}.test.internetofthings.ibm.cloud.com, LIVE has no "$MQTT_STAND.test"
declare MQTT_SRV="tcp://159.8.138.100:1883"	# CLI -s option: MQTT Server IP as a URI address (lon02-5). This is in the format of tcp://<ipaddress>:<port>. 
declare MQTT_PUB_INSTANCES=1			# myCLI -x option: Number of MQTT PUB Clients to start
declare MQTT_SUB_INSTANCES=1			# myCLI -y option: Number of MQTT SUB Clients to start
declare MQTT_ORG_INSTANCES=1			# myCLI -z option: Number of ORG to start Pub/Sub Clients
declare MQTT_INSTANCE_INDEX=0			# myCLI -Z option: ORG Index in the DEVICE/APIKEYS Arrays to start credential retrieval, can not exceed ORG_MEMBERS

declare MQTT_PORT="1883"
declare MQTT_SSL_PORT="8883"
declare MQTT_DURABLE="FALSE"
declare DEBUG="FALSE"

declare DEBUG="TRUE"

#---  MQTT CLIENT INVOCATION PARAMETERS (not exposed in CLI)
if [ "${TEST_DIR}" == "" ] ; then
   declare TEST_DIR="/test/niagara/setup"	 # when not running in a Docker Container.
fi
declare  MQTT_CLASSPATH="${TEST_DIR}/javasample.jar:${TEST_DIR}/org.eclipse.paho.client.mqttv3.jar"

declare -i MQTT_QOS=0			# CLI -q option:  Quality of Service
						# CLI -m option:  MQTT Message Text (default text)
#declare    MQTT_MSG="Message sent from ${PUB_MQTT_CLIENTID} on ${PUB_MQTT_TOPIC}"
declare    MQTT_MSG="{\"d\": {\"mCount\":1000}}"
declare -i MQTT_PUB_MSGS=1000		# CLI -n option:  Number of message text is to be sent.
declare -i MQTT_SUB_MSGS=${MQTT_PUB_MSGS}	# RUNTIME RECOMPUTED:  Number of message text is to be sent TIMES Number of Pub clients
declare -i MQTT_PUB_RATE=10			# CLI -w option:  throttle for number of messages per second.
declare -i MQTT_PUB_SECONDS=0               # CLI -N option:  Hours the publisher will send the specified message
						# CLI -Nm option:  Minutes the publisher will send the specified message
                                          # both -N and -Nm are converted to 'seconds' as svt/mqtt/mq/MqttSample expects.
declare    MQTT_VERSION=3.1.1		# CLI -mqtt option:  MQTT Client code version
declare -i MQTT_KEEPALIVE=0			# CLI -ka option:  KeepAlive Interval
declare -i MQTT_CONNECTION_TIMEOUT=0	# CLI -ct option:  Connection Timeout (Subscriber gives up waiting to receive?)
declare -i MQTT_CONNECTION_RETRY=15	# CLI -cr option:  Connection Retry (Subscriber gives up trying to connect)

declare MQTT_ACTION_PUB="publish" 		# Pub MQTT_Action value
declare MQTT_ACTION_SUB="subscribe"	# Sub MQTT_Action value 	
declare MQTT_ACTION="${MQTT_ACTION_PUB}"	# CLI -a option:  publish or subscribe action

declare MQTT_CLEAN_SESSION="static-true"	# CLI -c option:  Session data to be removed on client disconnect. Defaults value is static-true
declare MQTT_UNSUBSCRIBE="TRUE"           # CLI -e option:  Does client unsubscribe at disconnect. Defaults value is TRUE
declare MQTT_LOGFILE=""			# CLI -o option:  STDOUT saved to file
declare MQTT_PERSIST_DIR=""			# CLI -pm option: enable persistence and specify datastore directory. The default persistence is FALSE.
declare MQTT_RETAIN_MSG="FALSE"		# CLI -rm option: enable retained messages. The default retained is FALSE.


#--- Credential Variables used with DEVICES and APIKEYS arrays
declare -i ARRAY_INDEX=0
declare    PUB_MQTT_CLIENTID=""	# DEVICES_CLIENTID[#] shortcut set by get_device_credentials()
declare    PUB_MQTT_TOPIC=""	# DEVICES_TOPIC[#] shortcut set by get_device_credentials()
declare    PUB_MQTT_USER=""		# DEVICES_USER[#] shortcut set by get_device_credentials()
declare    PUB_MQTT_PSWD=""		# DEVICES_PSWD[#] shortcut set by get_device_credentials()

declare    SUB_MQTT_CLIENTID=""	# APIKEY_CLIENTID[#] shortcut set by get_apikey_credentials()
declare    SUB_MQTT_TOPIC=""	# APIKEY_TOPIC[#] shortcut set by get_apikey_credentials()
declare    SUB_MQTT_USER=""		# APIKEY_USER[#] shortcut set by get_apikey_credentials()
declare    SUB_MQTT_PSWD=""		# APIKEY_PSWD[#] shortcut set by get_apikey_credentials()

#---  Credential File related variables
###  EXPECT these two files to PREEXIST in the Docker Container from test-integration Docker RUN Cmds
###  RECALCULATE the values if MQTT_STAND is passed.
declare    LOG_DIR="/home"
declare    CREDENTIALS_DIR="/var/log"
declare    DEVICE_FILE="${CREDENTIALS_DIR}/${MQTT_STAND}_devices_master_10000"
declare    APIKEY_FILE="${CREDENTIALS_DIR}/${MQTT_STAND}_apikeys_master_10000"
#  These 3 Field must match the constraints used to create DEVICE_FILE and APIKEY_FILE
declare -i ORG_MEMBERS=`awk -F '\:' '{print $2}' ${APIKEY_FILE} | uniq | wc -l`	# Number of ORG defined in CREDENTIALS

declare -i DEVICE_ORG_MEMBERS=10			# Number of Device ClientIds per Org
declare -i APIKEY_ORG_MEMBERS=10			# Number of APIKEY ClientIds per Org

declare -i DEVICE_LENGTH=0
declare -a DEVICE_CLIENTID[]
declare -a DEVICE_TOPIC[]
declare -a DEVICE_USER[]
declare -a DEVICE_PSWD[]

declare -i APIKEY_LENGTH=0
declare -a APIKEY_CLIENTID[]
declare -a APIKEY_TOPIC[]
declare -a APIKEY_USER[]
declare -a APIKEY_PSWD[]

# used for the mqttScale.sh - balancing the Sub msg count expectations when multiple Pubs are started.
declare -i ORGS_STARTED=0
declare -i DEVICES_STARTED=0
declare -i APIKEYS_STARTED=0


#---  LOGGING Related Parameters.
declare    LOG_SUB_READY_1="subscribed to topic: "
declare    LOG_SUB_READY="FALSE"
declare    LOG_SUB_VERIFY_1="Received first message on topic"
declare    LOG_SUB_VERIFY_2="Received ${MQTT_SUB_MSGS} messages."		# RUNTIME RECOMPUTED: 
declare    LOG_PUB_VERIFY_1="Published ${MQTT_PUB_MSGS} messages to topic"	# RUNTIME RECOMPUTED:

declare -i LOG_ERROR_COUNT=0	# Count of Errors occurances

declare    LOG_FINALE="PASS"	# Assume LOG_FINALE_PASS unless the LOG_ERROR_COUNT is incremented, set to LOG_FINALE_FAIL.
declare    LOG_FINALE_PASS="PASS"	# VALUE STRING of LOG_FINALE when PASS
declare    LOG_FINALE_FAIL="--- !!! FAIL !!! ---"  # VALUE STRING of LOG_FINALE when FAIL

declare    LOG_MSG=""		# LogMessage Text - set before the call to log_error()

#---

#---

#---

##  AMS01-3 
# -- DEVICE NAME -------------------------	-- PUBLIC --	-- PRIVATE --
# datapower-0.ams01-3.iotcloud.ibm.com		37.58.109.235	10.104.58.199
# monagent-0.ams01-3.iotcloud.ibm.com		5.153.46.206	10.104.58.248
# monitor-0.ams01-3.iotcloud.ibm.com				10.104.58.210
# messagesight-0.ams01-3.iotcloud.ibm.com messagesight-0	10.70.107.110   
# msproxy-0.ams01-3.iotcloud.ibm.com		5.153.46.201	10.104.58.232
# msproxy-quickstart-0.ams01-3.iotcloud.ibm.com	46.16.188.199 10.104.58.233
# portal-0.ams01-3.iotcloud.ibm.com		46.16.188.195	10.104.58.245
# portal-quickstart-0.ams01-3.iotcloud.ibm.com	46.16.188.200	10.104.58.242
# seed.ams01-3.iotcloud.ibm.com			159.8.4.53	10.104.58.234
#
##  LON02-1
# -- DEVICE NAME -------------------------	-- PUBLIC --	-- PRIVATE --
# datapower-0.lon02-1.iotcloud.ibm.com		5.10.103.155	10.112.20.79
# messagesight-0.lon02-1.iotcloud.ibm.com		5.10.103.156	10.112.20.82
# monagent-0.lon02-1.iotcloud.ibm.com		5.10.103.154	10.112.20.78
# monitor-0.lon02-1.iotcloud.ibm.com				10.112.20.81	
# msproxy-0.lon02-1.iotcloud.ibm.com		5.10.103.151	10.112.20.75
# msproxy-quickstart-0.lon02-1.iotcloud.ibm.com	5.10.103.152	10.112.20.76
# portal-0.lon02-1.iotcloud.ibm.com		5.10.103.150	10.112.20.74
# portal-quickstart-0.lon02-1.iotcloud.ibm.com	5.10.103.153	10.112.20.77
# seed.lon02-1.iotcloud.ibm.com			5.10.103.157	10.112.20.72
#

##########################
##   FUNCTIONS  
##########################

#---------------------------------------------------------------------------
#  fcn()
#
#---------------------------------------------------------------------------
fcn() {
   LOG_MSG="Enter:  sample function template"
   log_debug;


}

#----------------------------------------------------------------------------
#  usage-options()  
#
#----------------------------------------------------------------------------
usage-options() {
set +x
  printf "\n"
  printf "usage:  %s [-S|-s MQTT_SRV] -x -y -z -Z -n  \n" $app
  printf "where: \n"
  printf "MOST COMMON parameters are: \n"
  printf "	-S <STAND>		IoT Softlayer STAND NAME\n"
  printf "				  Default:  ${MQTT_STAND}, will compute SERVER_URI\n"
  printf "	-s <SERVER_URI>		MQTT Server IP Address (maybe DNS NAME?)\n"
  printf "				  Default: ${MQTT_SRV}\n"
  printf "  DO -S or -s, but DON'T do both, unpredictable results\n"
  printf "	-x <PUBs>		Number of Org Publishers  (0-${DEVICE_ORG_MEMBERS})\n"
  printf "				  Default: ${MQTT_PUB_INSTANCES}\n"
  printf "	-y <SUBs>		Number of Org Subscribers (0-${APIKEY_ORG_MEMBERS})\n"
  printf "				  Default: ${MQTT_SUB_INSTANCES}\n"
  printf "	-z <ORGs>		Number of Orgs to start   (0-${ORG_MEMBERS})\n"
  printf "				  Default: ${MQTT_ORG_INSTANCES}\n"
  printf "	-Z <ORG_INDEX>		ORG index to start at     (0-${ORG_MEMBERS})\n"
  printf "				  Default:  ${MQTT_INSTANCE_INDEX}\n"
  printf "	-n <MSGS>		Number of messages to send	\n"
  printf "				  Default:  ${MQTT_PUB_MSGS}\n"
  printf "\n"
  printf "ADDITIONAL: \n"
  printf "\n"
  printf "	-a <ACTION>	Action:  ${MQTT_ACTION_PUB} or ${MQTT_ACTION_SUB} \n"
  printf "			  Default:  ${MQTT_ACTION}\n"
  printf "	-d <DEBUG>	TRUE or FALSE\n"
  printf "			  Default:  ${DEBUG}\n"
  printf "	-c <>		Clean Session [TRUE|FALSE]		Currently PERSISTENCE is not supported.\n"
  printf "			  Default: ${MQTT_CLEAN_SESSION} \n"
  printf "	-e <>		Client unsubscribe before Disconnect? [TRUE|FALSE]\n"
  printf "			  Default: ${MQTT_UNSUBSCRIBE} \n"
  printf "	-T <>		How long Subscriber waits to receive after a connection is made.\n"
  printf "			  Default: ${MQTT_CONNECTION_TIMEOUT} \n"
  printf "	-C <>		Connection Retry before termination.\n"
  printf "			  Default: ${MQTT_CONNECTION_RETRY} \n"
  printf "	-K <>		KeepAlive Interval \n"
  printf "			  Default: ${MQTT_KEEPALIVE} \n"
  printf "	-l <>		MQTT Client Log Level\n"
  printf "			  Default: ${LOG_LEVEL}            CURRENTLY NOT IMPLEMENTED\n"
  printf "	-m <>		MQTT Message Text\n"
  printf "			  Default: ${MQTT_MSG} \n"
  printf "	-M <>		MQTT Client Version			Currently only support 3.1.1\n"
  printf "			  Default: ${MQTT_VERSION} \n"
  printf "	-n <>		Number of Messages to Publish (or receive if Subscriber)\n"
  printf "			  Default: ${MQTT_PUB_MSGS} \n"
  printf "	-N <>		Number of Hours the Publisher will SEND before terminating. (0 is unlimited) \n"
  printf "			  Default: ${MQTT_PUB_SECONDS} \n"
  printf "	-q <>		Quality of Service			Currently only QoS 1 supported.\n"
  printf "			  Default: ${MQTT_QOS} \n"
  printf "	-i <>		MQTT Client ID		(computed from Credentials)\n"
  printf "			  Default: ${MQTT_CLIENTID} \n"
  printf "	-t <>		MQTT Topic		(computed from Credentials)\n"
  printf "			  Default: ${MQTT_TOPIC} \n"
  printf "	-u <>		MQTT User ID		(computed from Credentials)\n"
  printf "			  Default: ${MQTT_USER} \n"
  printf "	-p <>		MQTT Password		(computed from Credentials)\n"
  printf "			  Default: ${MQTT_PSWD} \n"
  printf "	-w <>		Throttle for Number of Messages/sec \n"
  printf "			  Default: ${MQTT_PUB_RATE} \n"
  printf "	-o <>		STDOUT save to LOGFILE.\n"
  printf "			  Default: ${MQTT_LOGFILE} \n"
  printf "	-P <>		Enable Persistence and Datastore Directory\n"
  printf "			  Default: ${MQTT_PERSIST_DIR} \n"
  printf "	-R <>		Enable Retained Messages\n"
  printf "			  Default: ${MQTT_RETAIN_MSG} \n"
  printf "	-h		This HELP!\n"
  printf "\n"
  printf "\n"
  printf "Example:  %s -S %s -x %s -y %s -z %s -Z %s -n %s\n" "$0" ${MQTT_STAND} ${MQTT_PUB_INSTANCES} ${MQTT_SUB_INSTANCES} ${MQTT_ORG_INSTANCES} ${MQTT_INSTANCE_INDEX} ${MQTT_PUB_MSGS}
  printf "Example:  %s -s %s -x %s -y %s -z %s -Z %s -n %s\n" "$0" ${MQTT_SRV} 10 10 ${ORG_MEMBERS} ${MQTT_INSTANCE_INDEX} 100000

  exit 99
}

#----------------------------------------------------------------------------
# parse-options()
#----------------------------------------------------------------------------
parse-options(){
   LOG_MSG="Enter:  parse-options : ${params}"
   log_debug;

#   set -x
	while getopts "a:C:T:c:d:e:i:K:l:M:m:n:N:o:P:p:R:q:s:S:t:u:w:x:y:z:Z:h" option ${params}
	do
		case ${option} in
		s )
                     MQTT_SRV=${OPTARG} ;;
		S )
                     MQTT_STAND=${OPTARG} 
                     #MQTT_SRV will be set as tcp://[dns_ip]:${MQTT_PORT} if nslookup_proxy() is successful
                     nslookup_proxy;
                     ;;
		x )
			MQTT_PUB_INSTANCES=${OPTARG} ;;
		y )
			MQTT_SUB_INSTANCES=${OPTARG} ;;
		Z )
			MQTT_INSTANCE_INDEX=${OPTARG} ;;
		z )
			MQTT_ORG_INSTANCES=${OPTARG} ;;
		a )
			MQTT_ACTION=${OPTARG} ;;
		d )
			DEBUG=${OPTARG} ;;	
		c )
			MQTT_CLEAN_SESSION=${OPTARG}
# NOT TRUE			MQTT_CLEAN_SESSION="static-true"		#IoT-C only supports cleansession=TRUE in v1.0
			;;
		e )
			MQTT_UNSUBSCRIBE=${OPTARG} ;;
		T )
			MQTT_CONNECTION_TIMEOUT=${OPTARG} ;;
		C )
			MQTT_CONNECTION_RETRY=${OPTARG} ;;
		K )
			MQTT_KEEPALIVE=${OPTARG} ;;
		l )
			LOG_LEVEL=${OPTARG} ;;
 		m )
			MQTT_MSG=${OPTARG} ;;
 		M )
			MQTT_VERSION=${OPTARG} ;;
 		n )
			MQTT_PUB_MSGS=${OPTARG} ;;
              # svt/mqtt/mq/MqttSample expects this time in seconds -N and -Nm, not going to both with -Ns parameter
 		N )
			let MQTT_PUB_SECONDS=${OPTARG}*60*60 ;;
		q )
			MQTT_QOS=${OPTARG} ;;
		i )
			MQTT_CLIENTID=${OPTARG} ;;
		t )
			MQTT_TOPIC=${OPTARG} ;;
		u )
			MQTT_USER=${OPTARG} ;;
		w )
			MQTT_PUB_RATE=${OPTARG} ;;
		p )
			MQTT_PSWD=${OPTARG} ;;
		o )
			MQTT_LOGFILE="${OPTARG}" ;;
		P )
			MQTT_PERSIST_DIR="${OPTARG}" ;;
		R )
			MQTT_RETAIN_MSG="${OPTARG}" ;;
		h )
                     printf "%s %s     --> HELP REQUESTED! <--\n" "$0" "${params}"
			usage-options;
			;;
		* )
			LOG_MSG="Unknown option: ${option}"
                     log_fail;
                     printf "%s %s     --> invalid parameters <--\n" "$0" "${params}"
			usage-options;
			;;
		esac	
	done
   set +x

   # Validation of Input constraints and RECALC files and values based on inputs
   DEVICE_FILE="${CREDENTIALS_DIR}/${MQTT_STAND}_devices_master_10000"
   APIKEY_FILE="${CREDENTIALS_DIR}/${MQTT_STAND}_apikeys_master_10000"
   ORG_MEMBERS=`awk -F '\:' '{print $2}' ${APIKEY_FILE} | uniq | wc -l`	# Number of ORG defined in CREDENTIALS

   
   if [[ ${MQTT_PUB_INSTANCES} -gt ${DEVICE_ORG_MEMBERS} ]] ; then
      LOG_MSG="The value for MQTT_PUB_INSTANCES was greater than the maximum, ${DEVICE_ORG_MEMBERS}.  The value will be reset to ${DEVICE_ORG_MEMBERS}"
      log_warning;
      MQTT_PUB_INSTANCES=${DEVICE_ORG_MEMBERS}
   fi

   if [[ ${MQTT_SUB_INSTANCES} -gt ${APIKEY_ORG_MEMBERS} ]] ; then
      LOG_MSG="The value for MQTT_SUB_INSTANCES was greater than the maximum, ${APIKEY_ORG_MEMBERS}.  The value will be reset to ${APIKEY_ORG_MEMBERS}"
      log_warning;
      MQTT_SUB_INSTANCES=${APIKEY_ORG_MEMBERS}
   fi

   let ORGS_ALL_IN=${MQTT_ORG_INSTANCES}+${MQTT_INSTANCE_INDEX}
   if [[ ${ORGS_ALL_IN} -gt ${ORG_MEMBERS} ]] ; then
      LOG_MSG="The combination for MQTT_ORG_INSTANCES, ${MQTT_ORG_INSTANCES} to create starting at MQTT_INSTANCE_INDEX, ${MQTT_INSTANCE_INDEX} was greater than the maximum, ${ORG_MEMBERS}.  The starting Index value will be reset to $(( ${ORG_MEMBERS}-${MQTT_ORG_INSTANCES} ))"
      log_warning;
      let MQTT_INSTANCE_INDEX=${ORG_MEMBERS}-${MQTT_ORG_INSTANCES}
   fi

   # Compute the Expect Sub Msgs based on number of Publisher started per ORG
   let MQTT_SUB_MSGS=${MQTT_PUB_MSGS}*${MQTT_PUB_INSTANCES}
   LOG_SUB_VERIFY_2="Received ${MQTT_SUB_MSGS} messages."
   LOG_PUB_VERIFY_1="Published ${MQTT_PUB_MSGS} messages to topic"


   LOG_MSG="Starting ${MQTT_PUB_INSTANCES} MQTT Pub and ${MQTT_SUB_INSTANCES} Sub Clients for ${MQTT_ORG_INSTANCES} ORGs starting at ORG Index ${MQTT_INSTANCE_INDEX} on ${HOSTNAME}."
   log_msg;

}

#---------------------------------------------------------------------------
#  nslookup_proxy()
#
#---------------------------------------------------------------------------
nslookup_proxy() {
   LOG_MSG="Enter:  nslookup_proxy"
   log_debug;
   
   MQTT_PROXY="messaging.${MQTT_STAND}.test.internetofthings.ibmcloud.com"
   temp=`nslookup ${MQTT_PROXY}`
   temp_dns=`echo ${temp} | cut -d : -f 5`
   temp_ip=`echo ${temp} | cut -d : -f 6 | tr -d ' '`
   temp_err=`echo ${temp} | cut -d : -f 8
`

   if [[ "${temp_dns}" == *"${MQTT_PROXY} Address"* ]] ; then		# Found the hostname
      MQTT_SRV="tcp://${temp_ip}:${MQTT_PORT}"
      echo "MQTT_SRV=${MQTT_SRV} from dns lookup for ${MQTT_PROXY}"
   else
      LOG_MSG="Proxy in STAND with name:  \'messaging.${MQTT_STAND}.test.internetofthings.ibmcloud.com\'  was not found!"
      log_error;
      exit 69;
   fi

}


#---------------------------------------------------------------------------
#  build_device_array()
# PreReqs:
#   1. SOURCED mqttCFG.sh
#   2. The 'device_master' CREDENTIALS FILE is in the current directory (same dir as this file)
# INPUT:
#   none
# OUTPUT:
#   1. Set the FOLLOWING Env Vars:
#   -  DEVICE_LENGTH : length of the DEVICES arrays, MUST "MINUS 1" when used on 0-based array index
#   -  DEVICE_CLIENTID{[*]} : CLIENTID : Array of clientIds
#   -  DEVICE_TOPIC{[*]}    : TOPIC : Array of topic names
#   -  DEVICE_USER{[*]}     : UID: Array of UIDs
#   -  DEVICE_PSWD{[*]}     : PSWD : Array of PSWDs
#
#---------------------------------------------------------------------------
build_device_array() {
   LOG_MSG="Enter:  build_device_array with: ${DEVICE_FILE}"
   log_debug;
#set -x
   awk '{print }'  ${DEVICE_FILE} | sort > ${DEVICE_FILE}.sort
   i=0
   while read line ; 
   do
      DEVICE_CLIENTID[${i}]=`echo ${line} | cut -d "|" -f 1`	# ClientId
      DEVICE_TOPIC[${i}]=`echo ${line} | cut -d "|" -f 2`		# Topic
      DEVICE_USER[${i}]=`echo ${line} | cut -d "|" -f 3`		# UID
      DEVICE_PSWD[${i}]=`echo ${line} | cut -d "|" -f 4`		# PSWD

      x=$(( i%1000 ))
	  if [ "${x}" == 0 ]; then
		LOG_MSG=" @ ${i}: `echo ${DEVICE_CLIENTID[${i}]}, ${DEVICE_TOPIC[${i}]}, ${DEVICE_USER[${i}]}, ${DEVICE_PSWD[${i}]}` "
		log_debug;
	  fi
      
      (( i++ ))
   done < ${DEVICE_FILE}.sort

#set -x

   DEVICE_LENGTH=${#DEVICE_CLIENTID[@]}
   LOG_MSG="DEVICE ARRAY LENGTH:${DEVICE_LENGTH}"
   log_debug;

set +x
}

#---------------------------------------------------------------------------
#  build_apikey_array()
# PreReqs:
#   1. SOURCED mqttCFG.sh
#   2. The 'apikeys_master' CREDENTIALS FILE is in the current directory (same dir as this file)
# INPUT:
#   none
# OUTPUT:
#   1. Set the FOLLOWING Env Vars:
#   -  APIKEY_LENGTH : length of the APIKEYS arrays, MUST "MINUS 1" when used on 0-based array index
#   -  APIKEY_CLIENTID{[*]} : CLIENTID : Array of clientIds
#   -  APIKEY_TOPIC{[*]}    : TOPIC : Array of topic names
#   -  APIKEY_USER{[*]}     : UID: Array of UIDs
#   -  APIKEY_PSWD{[*]}     : PSWD : Array of PSWDs
#
#---------------------------------------------------------------------------
build_apikey_array() {
   LOG_MSG="Enter:  build_api_array with:  ${APIKEY_FILE}"
   log_debug;
#set -x
   awk '{print }'  ${APIKEY_FILE} | sort > ${APIKEY_FILE}.sort

   i=0
   while read line ; 
   do
      APIKEY_CLIENTID[${i}]=`echo ${line} | cut -d "|" -f 1`	# ClientId
      A_TOPIC[${i}]=`echo ${line} | cut -d "|" -f 2`	# Topic
      APIKEY_TOPIC[${i}]=`echo ${A_TOPIC//[*]/+}`		# Topic with correct MQTT Wildcard
      APIKEY_USER[${i}]=`echo ${line} | cut -d "|" -f 3`	# UID
      APIKEY_PSWD[${i}]=`echo ${line} | cut -d "|" -f 4`	# PSWD

      x=$(( i%1000 ))
	  if [ "${x}" == 0 ]; then
		LOG_MSG=" @ ${i}: `echo ${APIKEY_CLIENTID[${i}]}, ${APIKEY_TOPIC[${i}]}, ${APIKEY_USER[${i}]}, ${APIKEY_PSWD[${i}]}` "
		log_debug;
	  fi
      
      (( i++ ))
   done < ${APIKEY_FILE}.sort
#set -x

   APIKEY_LENGTH=${#APIKEY_CLIENTID[@]}
   LOG_MSG="APIKEY ARRAY LENGTH:${APIKEY_LENGTH}"
   log_debug;

}


#---------------------------------------------------------------------------
#  get_device_credentials()
#  PreReqs:
#  - ARRAY_INDEX is set with the array offset to the desired entry in DEVICES_CREDENTIALS[]
#  INPUT:
#  - none
#  OUTPUTS:
#  -  PUB_MQTT_CLIENTID contain CLIENTID  at DEVICE_CLIENTID[${ARRAY_INDEX}]}
#  -  PUB_MQTT_TOPIC contains the TOPIC   at DEVICE_TOPIC[${ARRAY_INDEX}]}
#  -  PUB_MQTT_USER contains the  USERID  at DEVICE_USER[${ARRAY_INDEX}]
#  -  PUB_MQTT_PSWD contains the PASSWORD at DEVICE_PSWD[${ARRAY_INDEX}]
#---------------------------------------------------------------------------

get_device_credentials() {
   LOG_MSG="Get DEVICE Credentials for device at index: '${ARRAY_INDEX}'"
   log_debug;
#set -x
   PUB_MQTT_CLIENTID=`echo ${DEVICE_CLIENTID[${ARRAY_INDEX}]}`
   PUB_MQTT_TOPIC=`echo ${DEVICE_TOPIC[${ARRAY_INDEX}]}`
   PUB_MQTT_USER=`echo ${DEVICE_USER[${ARRAY_INDEX}]}`
   PUB_MQTT_PSWD=`echo ${DEVICE_PSWD[${ARRAY_INDEX}]}`
set +x
}


#---------------------------------------------------------------------------
#  get_apikey_credentials()
#  PreReqs:
#  - ARRAY_INDEX is set with the array offset to the desired entry in APIKEYS_CREDENTIALS[]
#  INPUT:
#  - none
#  OUTPUTS:
#  -  SUB_MQTT_CLIENTID conatin CLIENTID at APIKEY_CLIENTID[${ARRAY_INDEX}]
#  -  SUB_MQTT_TOPIC contain the TOPIC   at APIKEY_TOPIC[${ARRAY_INDEX}]
#  -  SUB_MQTT_USER contain the  USERID  at APIKEYS_USER[${ARRAY_INDEX}]
#  -  SUB_MQTT_PSWD contain the PASSWORD at APIKEYS_PSWD[${ARRAY_INDEX}]
#---------------------------------------------------------------------------

get_apikey_credentials() {
#set -x
   LOG_MSG="Get APIKEY Credentials for apikey at index: '${ARRAY_INDEX}'"
   log_debug;

   SUB_MQTT_CLIENTID=`echo ${APIKEY_CLIENTID[${ARRAY_INDEX}]}`
   SUB_MQTT_TOPIC=`echo ${APIKEY_TOPIC[${ARRAY_INDEX}]}`
   SUB_MQTT_USER=`echo ${APIKEY_USER[${ARRAY_INDEX}]}`
   SUB_MQTT_PSWD=`echo ${APIKEY_PSWD[${ARRAY_INDEX}]}`

}


#---------------------------------------------------------------------------
#  verify_sub_ready()
#     Verify the LOG_SUB_READY_1 text is in the log file - check all logs per Organization
#  PreReqs:  
#     RUN:  build_apikey_arrays has built APIKEY_* structures
#     ORG_STARTED : Set with the Organization offset whose logs will be queried.
#  Input:
#     none
#  Output:
#    LOG_SUB_READY : "TRUE" or "FALSE" status for Successful For ALL MQTT Connection
#---------------------------------------------------------------------------
verify_sub_ready() {
   LOG_MSG="Enter:  verify_sub_ready "
   log_debug;

   MQTT_ACTION="${MQTT_ACTION_SUB}"
   LOG_CHECK_ITERATIONS=15
   LOG_CHECK_SLEEP=1
   LOG_SUB_READY=FALSE
   LOG_SUB_READY_COUNT=0
###   ORGS_STARTED=0
   APIKEYS_STARTED=0
   LOG_FILE=""

###   while [[ ${ORGS_STARTED} -lt ${MQTT_ORG_INSTANCES} ]] ; 
###   do

      # Compute the INDEX into the DEVICES_* and APIKEYS_* arrays for this ORG
      let ORG_INDEX=( ${ORGS_STARTED}+${MQTT_INSTANCE_INDEX} )*${DEVICE_ORG_MEMBERS}
      # Start log compare for the number of SUBs specifed by MQTT_SUB_INSTANCES
      while [[ ${APIKEYS_STARTED} -lt ${MQTT_SUB_INSTANCES} ]] ;
      do
         let ARRAY_INDEX=${ORG_INDEX}+${APIKEYS_STARTED} 
         LOG_MSG="Verify SUBSCRIBER ${APIKEYS_STARTED} in ORG ${ORGS_STARTED}, APIKEY_CLIENTID[${ARRAY_INDEX}]: ${APIKEY_CLIENTID[${ARRAY_INDEX}] } has subscribed for ${MQTT_SUB_MSGS} msgs.  "
         log_msg;

         for (( i=0 ; i<=${LOG_CHECK_ITERATIONS} ; i++ ))
         do 
            LOG_FILE="${LOG_DIR}/${MQTT_ACTION}.${APIKEY_CLIENTID[${ARRAY_INDEX}] }.log"
            theGrep=`grep "${LOG_SUB_READY_1}" ${LOG_FILE}`
            RC=$?
            if [[ $RC -ne 0 ]] ; then
               sleep ${LOG_CHECK_SLEEP} 
         
            else 
               LOG_SUB_READY="TRUE"
               (( LOG_SUB_READY_COUNT++ ))
               break
            fi

         done

            if [[ "${LOG_SUB_READY}" != "TRUE" ]] ; then
               LOG_MSG="Unable to find 'log ready text': \"${LOG_SUB_READY_1}\" in file: ${LOG_FILE} - may be slow consumer."
#               log_error;
               log_warning;
            fi

         LOG_SUB_READY="FALSE"
         (( APIKEYS_STARTED++ ))
      done

      APIKEYS_STARTED=0
###      (( ORGS_STARTED++ ))

###   done

###   let LOG_SUB_READY_EXPECT=${MQTT_ORG_INSTANCES}*${MQTT_SUB_INSTANCES}
###   if [[ "${LOG_SUB_READY_COUNT}" -eq "${LOG_SUB_READY_EXPECT}" ]] ; then
   if [[ "${LOG_SUB_READY_COUNT}" -eq "${MQTT_SUB_INSTANCES}" ]] ; then
      LOG_SUB_READY="TRUE"
   fi

}



#---------------------------------------------------------------------------
#  verify_sub_logs()
#  PreReqs:  
#     build_apikey_arrays has built APIKEY_* structures
#  Input:
#     none
#  Output:
#    LOG_ERROR_COUNT  : number of failures in logs
#    LOG_FINALE : "PASS" or "FAIL" status overall test case
#
#---------------------------------------------------------------------------
verify_sub_logs() {
   LOG_MSG="Enter:  verify_sub_logs"
   log_debug;

   MQTT_ACTION="${MQTT_ACTION_SUB}"
#   LOG_SUB_VERIFY_2="Received ${MQTT_SUB_MSGS} messages."
   ORGS_STARTED=0
   APIKEYS_STARTED=0
   LOG_FILE=""

   while [[ ${ORGS_STARTED} -lt ${MQTT_ORG_INSTANCES} ]] ; 
   do

      # Compute the INDEX into the DEVICES_* and APIKEYS_* arrays for this ORG
      let ORG_INDEX=( ${ORGS_STARTED}+${MQTT_INSTANCE_INDEX} )*${DEVICE_ORG_MEMBERS}
      # Start log compare for the number of SUBs specifed by MQTT_SUB_INSTANCES
      while [[ ${APIKEYS_STARTED} -lt ${MQTT_SUB_INSTANCES} ]] ;
      do
         let ARRAY_INDEX=${ORG_INDEX}+${APIKEYS_STARTED} 

         LOG_FILE="${LOG_DIR}/${MQTT_ACTION}.${APIKEY_CLIENTID[${ARRAY_INDEX}] }.log"
         theGrep=`grep "${LOG_SUB_VERIFY_1}" ${LOG_FILE} `
         RC=$?
         if [[ $RC -ne 0 ]] ; then
            LOG_MSG="Unable to find 'sub verify 1 text': \"${LOG_SUB_VERIFY_1}\" in file: ${LOG_FILE}. "
            log_error;
         fi
         theGrep=`grep "${LOG_SUB_VERIFY_2}" ${LOG_FILE} `
         RC=$?
         if [[ $RC -ne 0 ]] ; then
            LOG_MSG="Unable to find 'sub verify 2 text': \"${LOG_SUB_VERIFY_2}\" in file: ${LOG_FILE}. "
            log_error;
         fi
         (( APIKEYS_STARTED++ ))
      done

      APIKEYS_STARTED=0
      (( ORGS_STARTED++ ))

   done
   LOG_MSG="====== Subscriber Synopsis  ====================================="
   log_msg;
   let TOTAL_SUBS=${MQTT_ORG_INSTANCES}*${MQTT_SUB_INSTANCES}
   LOG_MSG="%==>  More stats:  Started ${TOTAL_SUBS} Subscribers."
   log_msg;
   TOTAL_MSG=`grep "${LOG_SUB_VERIFY_1}" ${LOG_DIR}/${MQTT_ACTION}.*.log |wc -l `
   LOG_MSG="%==>  ${TOTAL_MSG} Subscribers received the 'LOG_SUB_VERIFY_1' msg : \"${LOG_SUB_VERIFY_1}\"."
   log_msg;
   TOTAL_MSG=`grep "${LOG_SUB_VERIFY_2}" ${LOG_DIR}/${MQTT_ACTION}.*.log |wc -l `
   LOG_MSG="%==>  ${TOTAL_MSG} Subscribers received the 'LOG_SUB_VERIFY_2' msg : \"${LOG_SUB_VERIFY_2}\"."
   log_msg;
   LOG_MSG="==============================+================================="
   log_msg;
}




#---------------------------------------------------------------------------
#  verify_pub_logs()
#  PreReqs:  
#     build_device_arrays has built DEVICE_* structures
#  Input:
#     none
#  Output:
#    LOG_ERROR_COUNT  : number of failures in logs
#    LOG_FINALE : "PASS" or "FAIL" status overall test case
#
#---------------------------------------------------------------------------
verify_pub_logs() {
   LOG_MSG="Enter:  verify_pub_logs"
   log_debug;

   MQTT_ACTION="${MQTT_ACTION_PUB}"
#   LOG_PUB_VERIFY_1="Published ${MQTT_PUB_MSGS} messages to topic"
   ORGS_STARTED=0
   DEVICES_STARTED=0
   LOG_FILE=""
 
   while [[ ${ORGS_STARTED} -lt ${MQTT_ORG_INSTANCES} ]] ; 
   do

      # Compute the INDEX into the DEVICES_* and APIKEYS_* arrays for this ORG
      let ORG_INDEX=( ${ORGS_STARTED}+${MQTT_INSTANCE_INDEX} )*${DEVICE_ORG_MEMBERS}
      # Start log compare for the number of PUBs specifed by MQTT_PUB_INSTANCES
      while [[ ${DEVICES_STARTED} -lt ${MQTT_PUB_INSTANCES} ]] ;
      do
         let ARRAY_INDEX=${ORG_INDEX}+${DEVICES_STARTED} 

         LOG_FILE="${LOG_DIR}/${MQTT_ACTION}.${DEVICE_CLIENTID[${ARRAY_INDEX}] }.log"
         theGrep=`grep "${LOG_PUB_VERIFY_1}" ${LOG_FILE} `
         RC=$?
         if [[ $RC -ne 0 ]] ; then
            LOG_MSG="Unable to find 'pub verify 1 text': \"${LOG_PUB_VERIFY_1}\" in file: ${LOG_FILE}. "
            log_error;
         fi
         (( DEVICES_STARTED++ ))
      done

      DEVICES_STARTED=0
      (( ORGS_STARTED++ ))

   done
   LOG_MSG="====== Publisher Synopsis  ======================================"
   log_msg;
   let TOTAL_PUBS=${MQTT_ORG_INSTANCES}*${MQTT_PUB_INSTANCES}
   LOG_MSG="%==>  More stats:  Started ${TOTAL_PUBS} Publishers."
   log_msg;
   TOTAL_MSG=`grep "${LOG_PUB_VERIFY_1}" ${LOG_DIR}/${MQTT_ACTION}.*.log |wc -l `
   LOG_MSG="%==>  ${TOTAL_MSG} Publishers logged the 'LOG_PUB_VERIFY_1' msg : \"${LOG_PUB_VERIFY_1}\"."
   log_msg;
   LOG_MSG="================================================================"
   log_msg;
}


#---------------------------------------------------------------------------
#  log_error()
#     Print Error Messages and increment Error counter 
#  PreReqs:  
#     LOG_MSG set with the text to print
#  Input:
#     none
#  Output:
#    LOG_ERROR_COUNT  : number of failures in logs
#    LOG_FINALE : "PASS" or "FAIL" status overall test case
#    STDOUT will have LOG_MSG echoed.
#---------------------------------------------------------------------------
log_error() {
   (( LOG_ERROR_COUNT++ ))
   LOG_FINALE="${LOG_FINALE_FAIL}"
   echo "`date` : ERROR: ${LOG_FINALE} : ${LOG_MSG} "
}

#---------------------------------------------------------------------------
#  log_warning()
#     Print WARNING Messages (no LOG_ERROR_COUNT increment)
#  PreReqs:  
#     LOG_MSG set with the text to print
#  Input:
#     none
#  Output:
#    STDOUT will have LOG_MSG echoed.
#---------------------------------------------------------------------------
log_warning() {
   echo "`date` : WARNING : ${LOG_MSG} "
}

#---------------------------------------------------------------------------
#  log_debug()
#     Print DEBUG Messages (no LOG_ERROR_COUNT increment)
#  PreReqs:  
#     LOG_MSG set with the text to print
#  Input:
#     none
#  Output:
#    STDOUT will have LOG_MSG echoed.
#---------------------------------------------------------------------------
log_debug() {
   if [[ "${DEBUG}" == "TRUE" ]] ; then
      echo "`date` : ${LOG_MSG} "
   fi
}

#---------------------------------------------------------------------------
#  log_msg()
#     Print WARNING Messages (no LOG_ERROR_COUNT increment)
#  PreReqs:  
#     LOG_MSG set with the text to print
#  Input:
#     none
#  Output:
#    STDOUT will have LOG_MSG echoed.
#---------------------------------------------------------------------------
log_msg() {
   echo "`date` : ${LOG_MSG} "
}

#---------------------------------------------------------------------------
#  launch_pub()
#     Start an MQTT Publisher
#  PreReqs:  
#     ALL CLI PARAMETERS must be set
#  Input:
#     none
#  Output:
#    MQTT PUB Client started in background
#---------------------------------------------------------------------------
launch_pub() {
   if [[ "${DEBUG}" == "TRUE" ]] ; then
      LOG_MSG="Enter:  launch_pub"
      log_debug;
#      CMD="java  -cp ${MQTT_CLASSPATH}  svt/mqtt/mq/MqttSample -s "${MQTT_SRV}" -mqtt ${MQTT_VERSION} -a ${MQTT_ACTION} -cr ${MQTT_CONNECTION_RETRY} -q ${MQTT_QOS} -t ${PUB_MQTT_TOPIC} -m \"${MQTT_MSG}\" -n ${MQTT_PUB_MSGS} -w ${MQTT_PUB_RATE} -i ${PUB_MQTT_CLIENTID} -u ${PUB_MQTT_USER}  -p "${PUB_MQTT_PSWD}"  -v -debug "
      CMD="java  -cp ${MQTT_CLASSPATH}  svt/mqtt/mq/MqttSample -s "${MQTT_SRV}" -mqtt ${MQTT_VERSION} -a ${MQTT_ACTION} -cr ${MQTT_CONNECTION_RETRY} -q ${MQTT_QOS} -t ${PUB_MQTT_TOPIC} -m \"${MQTT_MSG}\" -n ${MQTT_PUB_MSGS} -w ${MQTT_PUB_RATE} -i ${PUB_MQTT_CLIENTID} -u ${PUB_MQTT_USER}  -p "${PUB_MQTT_PSWD}"  -v -debug   -c ${MQTT_CLEAN_SESSION} -OJSON "

      # INPUT: -T
      if [ "${MQTT_CONNECTION_TIMEOUT}" != 0 ]; then
         CMD="${CMD}  -ct ${MQTT_CONNECTION_TIMEOUT}"
      fi
      # INPUT: -K
      if [ "${MQTT_KEEPALIVE}" != 0 ]; then
         CMD="${CMD}  -ka ${MQTT_KEEPALIVE}"
      fi
      # INPUT: -N
      if [ "${MQTT_PUB_SECONDS}" != 0 ]; then
         CMD="${CMD}  -N ${MQTT_PUB_SECONDS}"
      fi
      # INPUT: -P
      if [ "${MQTT_PERSIST_DIR}" != "" ]; then
         CMD="${CMD}  -pm \"${MQTT_PERSIST_DIR}\" "
      fi
      # INPUT: -R
      if [ "${MQTT_RETAIN_MSG}" == "TRUE" ]; then
         CMD="${CMD}  -rm"
      fi
      # INPUT: -o
      if [ "${MQTT_LOGFILE}" != "" ]; then
         CMD="${CMD}  -o ${MQTT_LOGFILE}"
      fi
   else
#      CMD="java  -cp ${MQTT_CLASSPATH}  svt/mqtt/mq/MqttSample -s "${MQTT_SRV}" -mqtt ${MQTT_VERSION} -a ${MQTT_ACTION} -cr ${MQTT_CONNECTION_RETRY} -q ${MQTT_QOS} -t ${PUB_MQTT_TOPIC} -m \"${MQTT_MSG}\" -n ${MQTT_PUB_MSGS} -w ${MQTT_PUB_RATE} -i ${PUB_MQTT_CLIENTID} -u ${PUB_MQTT_USER}  -p "${PUB_MQTT_PSWD}"  "
      CMD="java  -cp ${MQTT_CLASSPATH}  svt/mqtt/mq/MqttSample -s "${MQTT_SRV}" -mqtt ${MQTT_VERSION} -a ${MQTT_ACTION} -cr ${MQTT_CONNECTION_RETRY} -q ${MQTT_QOS} -t ${PUB_MQTT_TOPIC} -m \"${MQTT_MSG}\" -n ${MQTT_PUB_MSGS} -w ${MQTT_PUB_RATE} -i ${PUB_MQTT_CLIENTID} -u ${PUB_MQTT_USER}  -p "${PUB_MQTT_PSWD}"   -c ${MQTT_CLEAN_SESSION} -OJSON "

      # INPUT: -T
      if [ "${MQTT_CONNECTION_TIMEOUT}" != 0 ]; then
         CMD="${CMD}  -ct ${MQTT_CONNECTION_TIMEOUT}"
      fi
      # INPUT: -K
      if [ "${MQTT_KEEPALIVE}" != 0 ]; then
         CMD="${CMD}  -ka ${MQTT_KEEPALIVE}"
      fi
      # INPUT: -N
      if [ "${MQTT_PUB_SECONDS}" != 0 ]; then
         CMD="${CMD}  -N ${MQTT_PUB_SECONDS}"
      fi
      # INPUT: -P
      if [ "${MQTT_PERSIST_DIR}" != "" ]; then
         CMD="${CMD}  -pm \"${MQTT_PERSIST_DIR}\" "
      fi
      # INPUT: -R
      if [ "${MQTT_RETAIN_MSG}" == "TRUE" ]; then
         CMD="${CMD}  -rm"
      fi
      # INPUT: -o
      if [ "${MQTT_LOGFILE}" != "" ]; then
         CMD="${CMD}  -o ${MQTT_LOGFILE}"
      fi
   fi
   LOG_MSG="${CMD}"
   log_debug;
   ${CMD}  >${LOG_DIR}/${MQTT_ACTION}.${PUB_MQTT_CLIENTID}.log  2>&1 & 

}

#---------------------------------------------------------------------------
#  launch_sub()
#     Start an MQTT Subscriber
#  PreReqs:  
#     ALL CLI PARAMETERS must be set
#  Input:
#     none
#  Output:
#    MQTT SUB Client started in background
#---------------------------------------------------------------------------
launch_sub() {
   if [[ "${DEBUG}" == "TRUE" ]] ; then
      LOG_MSG="Enter:  launch_sub"
      log_debug;
#      CMD="java  -cp ${MQTT_CLASSPATH}  svt/mqtt/mq/MqttSample -s "${MQTT_SRV}"              -mqtt ${MQTT_VERSION} -a ${MQTT_ACTION} -cr ${MQTT_CONNECTION_RETRY} -q ${MQTT_QOS} -t ${SUB_MQTT_TOPIC} -n ${MQTT_SUB_MSGS} -i ${SUB_MQTT_CLIENTID} -u ${SUB_MQTT_USER} -p "${SUB_MQTT_PSWD}" -v -debug"
      CMD="java  -cp ${MQTT_CLASSPATH}  svt/mqtt/mq/MqttSample -s "${MQTT_SRV}"              -mqtt ${MQTT_VERSION} -a ${MQTT_ACTION} -cr ${MQTT_CONNECTION_RETRY} -q ${MQTT_QOS} -t ${SUB_MQTT_TOPIC} -n ${MQTT_SUB_MSGS} -i ${SUB_MQTT_CLIENTID} -u ${SUB_MQTT_USER} -p "${SUB_MQTT_PSWD}" -v -debug -c ${MQTT_CLEAN_SESSION} -e ${MQTT_UNSUBSCRIBE} -OJSON "
      #CMD="java  -cp ${MQTT_CLASSPATH}  svt/mqtt/mq/MqttSample -s ssl://${MQTT_SRV}:8883  -mqtt ${MQTT_VERSION} -a ${MQTT_ACTION} -cr ${MQTT_CONNECTION_RETRY} -q ${MQTT_QOS} -t ${SUB_MQTT_TOPIC} -n ${MQTT_SUB_MSGS} -i ${SUB_MQTT_CLIENTID} -u ${SUB_MQTT_USER} -p "${SUB_MQTT_PSWD}" -v -debug "

      # INPUT: -T
      if [ "${MQTT_CONNECTION_TIMEOUT}" != 0 ]; then
         CMD="${CMD}  -ct ${MQTT_CONNECTION_TIMEOUT}"
      fi
      # INPUT: -K
      if [ "${MQTT_KEEPALIVE}" != 0 ]; then
         CMD="${CMD}  -ka ${MQTT_KEEPALIVE}"
      fi
      # INPUT: -N
      if [ "${MQTT_PUB_SECONDS}" != 0 ]; then
         CMD="${CMD}  -N ${MQTT_PUB_SECONDS}"
      fi
      # INPUT: -P
      if [ "${MQTT_PERSIST_DIR}" != "" ]; then
         CMD="${CMD}  -pm \"${MQTT_PERSIST_DIR}\" "
      fi
      # INPUT: -R
      if [ "${MQTT_RETAIN_MSG}" == "TRUE" ]; then
         CMD="${CMD}  -rm"
      fi
      # INPUT: -o
      if [ "${MQTT_LOGFILE}" != "" ]; then
         CMD="${CMD}  -o ${MQTT_LOGFILE}"
      fi
   else
#      CMD="java  -cp ${MQTT_CLASSPATH}  svt/mqtt/mq/MqttSample -s "${MQTT_SRV}"              -mqtt ${MQTT_VERSION} -a ${MQTT_ACTION} -cr ${MQTT_CONNECTION_RETRY} -q ${MQTT_QOS} -t ${SUB_MQTT_TOPIC} -n ${MQTT_SUB_MSGS} -i ${SUB_MQTT_CLIENTID} -u ${SUB_MQTT_USER} -p "${SUB_MQTT_PSWD}"  "
      CMD="java  -cp ${MQTT_CLASSPATH}  svt/mqtt/mq/MqttSample -s "${MQTT_SRV}"              -mqtt ${MQTT_VERSION} -a ${MQTT_ACTION} -cr ${MQTT_CONNECTION_RETRY} -q ${MQTT_QOS} -t ${SUB_MQTT_TOPIC} -n ${MQTT_SUB_MSGS} -i ${SUB_MQTT_CLIENTID} -u ${SUB_MQTT_USER} -p "${SUB_MQTT_PSWD}"  -c ${MQTT_CLEAN_SESSION} -e ${MQTT_UNSUBSCRIBE}  -OJSON "
      #CMD="java  -cp ${MQTT_CLASSPATH}  svt/mqtt/mq/MqttSample -s ssl://${MQTT_SRV}:8883  -mqtt ${MQTT_VERSION} -a ${MQTT_ACTION} -cr ${MQTT_CONNECTION_RETRY} -q ${MQTT_QOS} -t ${SUB_MQTT_TOPIC} -n ${MQTT_SUB_MSGS} -i ${SUB_MQTT_CLIENTID} -u ${SUB_MQTT_USER} -p "${SUB_MQTT_PSWD}"  "

      # INPUT: -T
      if [ "${MQTT_CONNECTION_TIMEOUT}" != 0 ]; then
         CMD="${CMD}  -ct ${MQTT_CONNECTION_TIMEOUT}"
      fi
      # INPUT: -K
      if [ "${MQTT_KEEPALIVE}" != 0 ]; then
         CMD="${CMD}  -ka ${MQTT_KEEPALIVE}"
      fi
      # INPUT: -N
      if [ "${MQTT_PUB_SECONDS}" != 0 ]; then
         CMD="${CMD}  -N ${MQTT_PUB_SECONDS}"
      fi
      # INPUT: -P
      if [ "${MQTT_PERSIST_DIR}" != "" ]; then
         CMD="${CMD}  -pm \"${MQTT_PERSIST_DIR}\" "
      fi
      # INPUT: -R
      if [ "${MQTT_RETAIN_MSG}" == "TRUE" ]; then
         CMD="${CMD}  -rm"
      fi
      # INPUT: -o
      if [ "${MQTT_LOGFILE}" != "" ]; then
         CMD="${CMD}  -o ${MQTT_LOGFILE}"
      fi
   fi
   LOG_MSG="${CMD}"
   log_debug;
   ${CMD} >${LOG_DIR}/${MQTT_ACTION}.${SUB_MQTT_CLIENTID}.log  2>&1 &   

}

