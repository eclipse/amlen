#!/bin/bash +x

source mqttCfg.sh
parse-options;

build_apikey_array;

#set -x

#  use "-x option" declare -i MQTT_PUB_INSTANCES=20
#  use "-y option" declare -i MQTT_SUB_INSTANCES=20
#  use "-z & -Z option'   MQTT_ORG_INSTANCES and start at MQTT_INSTANCE_INDEX

ORGS_STARTED=0
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

   (( ORGS_STARTED++ ))
done

(( ARRAY_INDEX-- ))

LOG_MSG="------------------------------------------------------------------------"
log_msg;
LOG_MSG="Waiting on ${MQTT_SUB_INSTANCES} Subs launched in ${MQTT_ORG_INSTANCES} Orgs starting at ORG #${MQTT_INSTANCE_INDEX} to complete."
log_msg;
wait

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

