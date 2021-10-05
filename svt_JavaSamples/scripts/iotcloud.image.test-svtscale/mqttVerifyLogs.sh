#!/bin/bash +x
source mqttCfg.sh



##############################################################################
## MAIN
##############################################################################

parse-options ;

build_device_array;
build_apikey_array;

LOG_MSG="------------------------------------------------------------------------"
log_msg;
verify_pub_logs;
verify_sub_logs; 
LOG_MSG="------------------------------------------------------------------------"
log_msg;

if [[ "${LOG_FINALE}" == "${LOG_FINALE_PASS}" ]] ; then
   echo "${LOG_FINALE}: Test case $0 was successful! "
else
   echo "${LOG_FINALE}: Test case $0 failed with ${LOG_ERROR_COUNT} error(s), see messages above! "

fi
