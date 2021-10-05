#!/bin/bash  +x
#
#

source mqttCfg.sh
parse-options;
build_device_array;


#  use "-x option" declare -i MQTT_PUB_INSTANCES=20
#  use "-y option" declare -i MQTT_SUB_INSTANCES=20
#  use "-z & -Z option'   MQTT_ORG_INSTANCES and start at MQTT_INSTANCE_INDEX

ORGS_STARTED=0
DEVICES_STARTED=0


while [[ ${ORGS_STARTED} -lt ${MQTT_ORG_INSTANCES} ]] ; 
do 
   # Compute the INDEX into the DEVICES_* and APIKEYS_* arrays for this ORG
      let ORG_INDEX=( ${ORGS_STARTED}+${MQTT_INSTANCE_INDEX} )*${DEVICE_ORG_MEMBERS}
      LOG_MSG="Start ORG[${ORGS_STARTED}]=ORG #$(( ${ORG_INDEX}/${DEVICE_ORG_MEMBERS} )) of ${MQTT_ORG_INSTANCES} ORGs starting at ORG ${MQTT_INSTANCE_INDEX}"
      log_msg; 

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
   
      (( ORGS_STARTED++ ))
done

(( ARRAY_INDEX-- ))

LOG_MSG="------------------------------------------------------------------------"
log_msg;
LOG_MSG="Waiting on ${MQTT_PUB_INSTANCES} Pubs launched in ${MQTT_ORG_INSTANCES} Orgs starting at ORG #${MQTT_INSTANCE_INDEX} to complete."
log_msg;
wait


verify_pub_logs;
LOG_MSG="------------------------------------------------------------------------"
log_msg;

if [[ "${LOG_FINALE}" == "${LOG_FINALE_PASS}" ]] ; then
   LOG_MSG="${LOG_FINALE}: Test case $0 was successful! "
   log_msg;
else
   LOG_MSG="${LOG_FINALE}: Test case $0 failed with ${LOG_ERROR_COUNT} error(s), see messages above! "
   log_msg;
fi

