#
# Start Sub-Pub Pairs, does not take into account multiple PUBs msgs of same Org are received by same client

echo "Starting $0"
CUR_DIR=$( dirname $0 )
echo "CUR_DIR=${CUR_DIR}"
source ${CUR_DIR}/mqttCfg.sh

parse-options;
build_device_array;
build_apikey_array;
#set -x

#  use "-x option" declare -i MQTT_PUB_INSTANCES=20
#  use "-y option" declare -i MQTT_SUB_INSTANCES=20
#  use "-z & -Z option'   MQTT_ORG_INSTANCES and start at MQTT_INSTANCE_INDEX

ORGS_STARTED=0
DEVICES_STARTED=0
APIKEYS_STARTED=0

while [[ ${ORGS_STARTED} -lt ${MQTT_ORG_INSTANCES} ]] ; 
do 
   # Compute the INDEX into the DEVICES_* and APIKEYS_* arrays for this ORG
   let ORG_INDEX=( ${ORGS_STARTED}+${MQTT_INSTANCE_INDEX} )*${DEVICE_ORG_MEMBERS}
   LOG_MSG="Start ORG[${ORGS_STARTED}]=ORG #$(( ${ORG_INDEX}/${DEVICE_ORG_MEMBERS} )) of ${MQTT_ORG_INSTANCES} ORGs starting at ORG ${MQTT_INSTANCE_INDEX}"
   log_msg; 

   # Start the number of SUBs specifed by MQTT_SUB_INSTANCES
   while [[ ${APIKEYS_STARTED} -lt ${MQTT_SUB_INSTANCES} ]] ;
   do
      let ARRAY_INDEX=${ORG_INDEX}+${APIKEYS_STARTED} 
      LOG_MSG="start SUBSCRIBER ${APIKEYS_STARTED} in ORG ${ORGS_STARTED}, APIKEY_CLIENTID[${ARRAY_INDEX}]: ${APIKEY_CLIENTID[${ARRAY_INDEX}] } to receive ${MQTT_SUB_MSGS} msgs. "
      log_msg;

      MQTT_ACTION="${MQTT_ACTION_SUB}"
      get_apikey_credentials;   # Set MQTT_USER and MQTT_PSWD
      LOG_MSG="for ${ARRAY_INDEX} : CLIENTID=${SUB_MQTT_CLIENTID} , TOPIC=${SUB_MQTT_TOPIC} , UID=${SUB_MQTT_USER}, PSWD=${SUB_MQTT_PSWD}"
      log_debug;

      launch_sub;

      (( APIKEYS_STARTED++ ))
   done
   APIKEYS_STARTED=0


   # Verify the SUBs are connected and subscribed
   verify_sub_ready;

      
   # If the SUBs were READY, then start the PUBS
#   if [[ "$LOG_SUB_READY" == "TRUE" ]] ; then

      # Start the number of PUBs specifed by MQTT_PUB_INSTANCES
      while [[ ${DEVICES_STARTED} -lt ${MQTT_PUB_INSTANCES} ]] ;
      do
         let ARRAY_INDEX=${ORG_INDEX}+${DEVICES_STARTED} 
         LOG_MSG="start PUB member ${DEVICES_STARTED} in ORG ${ORGS_STARTED}, DEVICES_CLIENTID[${ARRAY_INDEX}]: ${DEVICE_CLIENTID[${ARRAY_INDEX}] } to send ${MQTT_PUB_MSGS} msgs. "
         log_msg;

         MQTT_ACTION="${MQTT_ACTION_PUB}"
         get_device_credentials;   # Set MQTT_USER and MQTT_PSWD
         LOG_MSG="for ${ARRAY_INDEX} : CLIENTID=${PUB_MQTT_CLIENTID} , TOPIC=${PUB_MQTT_TOPIC} , UID=${PUB_MQTT_USER}, PSWD=${PUB_MQTT_PSWD}"
         log_debug;

         MQTT_MSG="Message_sent_from_${PUB_MQTT_CLIENTID}_on_${PUB_MQTT_TOPIC}"

         launch_pub;

         (( DEVICES_STARTED++ ))
      done
      DEVICES_STARTED=0
   
#   else
#      LOG_MSG="Did not start PUBLISHER(s) when a SUBSCRIBER failed connection/subscription to ${MQTT_SRV}. "
#      log_warning;
#   fi

   (( ORGS_STARTED++ ))
done

(( ARRAY_INDEX-- ))

LOG_MSG="------------------------------------------------------------------------"
log_msg;
LOG_MSG="Waiting on ${MQTT_SUB_INSTANCES} Subs and ${MQTT_PUB_INSTANCES} Pubs launched in ${MQTT_ORG_INSTANCES} Orgs starting at ORG #${MQTT_INSTANCE_INDEX} to complete."
log_msg;
wait

verify_pub_logs;
verify_sub_logs;
LOG_MSG="------------------------------------------------------------------------"
log_msg;

if [[ "${LOG_FINALE}" == "${LOG_FINALE_PASS}" ]] ; then
   LOG_MSG="${LOG_FINALE}: Test case $0 was successful! "
   log_msg;
else
   LOG_MSG="${LOG_FINALE}: Test case $0 failed with ${LOG_ERROR_COUNT} error(s), see messages above! "
   log_msg;
fi

