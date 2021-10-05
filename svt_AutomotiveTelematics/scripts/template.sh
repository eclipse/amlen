#!/bin/bash


if [ "${A1_HOST}" == "" ]; then
  if [ -f ${TESTROOT}/scripts/ISMsetup.sh ]; then
    source ${TESTROOT}/scripts/ISMsetup.sh
  elif [ -f ../scripts/ISMsetup.sh ]; then
    source ../scripts/ISMsetup.sh
  else
    source /niagara/test/scripts/ISMsetup.sh
  fi
fi

GetPrimaryServer() {
    printf "A1\n"
  return
  i=0;for line in `ssh admin@${A1_HOST} status imaserver`;do list[$i]=${line};((i++)); done

  if [[ ! "${list[*]}" =~ "HARole" ]]; then
    printf "A1\n"
  else
    if [[ "${list[*]}" =~ "PRIMARY" ]]; then
      printf "A1\n"
    else
      if [ "${A2_HOST}" == "" ]; then
        printf "A1\n"
      else
        i=0; for line in `ssh admin@${A2_HOST} status imaserver`; do list2[$i]=${line}; ((i++)); done

        if [[ "${list2[*]}" =~ "PRIMARY" ]]; then
          printf "A2\n"
        else
          printf "A1\n"
        fi
        unset list2
      fi
    fi
  fi
  unset list
}

PRIMARY=`GetPrimaryServer`
AP_HOST=$(eval echo \$\{${PRIMARY}_HOST\})
AP_IPv4_1=$(eval echo \$\{${PRIMARY}_IPv4_1\})
AP_IPv4_2=$(eval echo \$\{${PRIMARY}_IPv4_2\})
unset PRIMARY


test_template_define() {

  declare testcase=""
  declare description=""
  declare -i timeout=0
  declare -i minutes=0
  declare -i qos=0
  declare order=false
  declare stats=false
  declare listen=false
  declare -i mqtt_subscribers=0
  declare -i mqtt_sslsubscribers=0
  declare -i jms_subscribers=0
  declare -i jms_sslsubscribers=0
  declare -i cmqtt_subscribers=0
  declare -i mqtt_publishers=0
  declare -i mqtt_sslpublishers=0
  declare -i paho_publishers=0
  declare -i paho_sslpublishers=0
  declare -i jms_publishers=0
  declare -i jms_sslpublishers=0
  declare -i cmqtt_publishers=0
  declare -i js_publishers=0
  declare -i mqtt_vehicles=0
  declare -i paho_vehicles=0
  declare -l jms_vehicles=0
  declare run_mqttbench=false
  declare enable_ldap=false
  declare i

  for i in "$@"; do
    if [[ $i = testcase=* ]]; then
       testcase=${i##testcase=}
    elif [[ $i = description=* ]]; then
      description=${i##description=}
    elif [[ $i = timeout=* ]]; then
      timeout=${i##*=}
    elif [[ $i = minutes=* ]]; then
      minutes=${i##*=}
    elif [[ $i = qos=* ]]; then
      qos=${i##*=}
    elif [[ $i = order=* ]]; then
      order=${i##*=}
    elif [[ $i = stats=* ]]; then
      stats=${i##*=}
    elif [[ $i = listen=* ]]; then
      listen=${i##*=}
    elif [[ $i = mqtt_subscribers=* ]]; then
      mqtt_subscribers=${i##*=}
    elif [[ $i = mqtt_sslsubscribers=* ]]; then
      mqtt_sslsubscribers=${i##*=}
    elif [[ $i = jms_subscribers=* ]]; then
      jms_subscribers=${i##*=}
    elif [[ $i = jms_sslsubscribers=* ]]; then
      jms_sslsubscribers=${i##*=}
    elif [[ $i = cmqtt_subscribers=* ]]; then
      cmqtt_subscribers=${i##*=}
    elif [[ $i = mqtt_publishers=* ]]; then
      mqtt_publishers=${i##*=}
    elif [[ $i = mqtt_sslpublishers=* ]]; then
      mqtt_sslpublishers=${i##*=}
    elif [[ $i = paho_publishers=* ]]; then
      paho_publishers=${i##*=}
    elif [[ $i = paho_sslpublishers=* ]]; then
      paho_sslpublishers=${i##*=}
    elif [[ $i = jms_publishers=* ]]; then
      jms_publishers=${i##*=}
    elif [[ $i = jms_sslpublishers=* ]]; then
      jms_sslpublishers=${i##*=}
    elif [[ $i = cmqtt_publishers=* ]]; then
      cmqtt_publishers=${i##*=}
    elif [[ $i = js_publishers=* ]]; then
      js_publishers=${i##*=}
    elif [[ $i = mqtt_vehicles=* ]]; then
      mqtt_vehicles=${i##*=}
    elif [[ $i = paho_vehicles=* ]]; then
      paho_vehicles=${i##*=}
    elif [[ $i = jms_vehicles=* ]]; then
      jms_vehicles=${i##*=}
    elif [[ $i = run_mqttbench=* ]]; then
      run_mqttbench=${i##*=}
    elif [[ $i = enable_ldap=* ]]; then
      enable_ldap=${i##*=}
    else
      echo invalid parameter "$i"
    fi
  done

 # if [[ ${enable_ldap} =~ [true] ]]; then
     declare -ir internet_port=16112
 # else
    declare -ir internet_port=16111
 # fi

  declare -lir intranet_port=16999
  declare -lir demo_port=16102

  declare -lir BATCH=200
  declare -lir BATCH2=200
  declare -lir MMAX=$(((${M_COUNT}-1)*${minutes}*60))
  declare -lir MMIN=$((${minutes}*30))
  declare -li MESSAGES=$((${minutes}*(${mqtt_vehicles}+${paho_vehicles}+${jms_vehicles})/10))
  if [ ${MMAX} -lt ${MMIN} ]; then MMIN=${MMAX}; fi
  if [ ${MESSAGES} -lt ${MMIN} ]; then MESSAGES=${MMIN}; fi
  if [ ${MESSAGES} -gt ${MMAX} ]; then MESSAGES=${MMAX}; fi

  declare -lr LDAPPASSWORD=imasvtest

  declare -li lc=0
  declare -li save_lc=0
  declare -li i=0
  declare -li id=0
  declare -l uname
  declare -l remainder=0

  declare -li msub=0
  declare -li mpub=0
  declare -lir msub_min=1
  declare -lir msub_max=1
  declare -lir mpub_min=2
  declare -lir mpub_max=${M_COUNT}

  unset component
  unset test_template_compare_string
  unset test_template_metrics_v1

  echo xml[${n}]=${testcase}
  xml[${n}]=${testcase}
  echo test_template_initialize_test "${xml[${n}]}"
  test_template_initialize_test "${xml[${n}]}"
  echo scenario[${n}]="${xml[${n}]} - "${description}
  scenario[${n}]="${xml[${n}]} - "${description}
  echo timeouts[${n}]=${timeout}
  timeouts[${n}]=${timeout}

#  component[$((++lc))]=runCommandNow,m1,./server_restart.sh
#  component[$((++lc))]=sleep,1

#  for i in $(eval echo {1..${M_COUNT}}); do
#   component[$((++lc))]=javaAppDriver,m$i,"-e|\-version,-o|-verbose"
#  done

#  Start mqttbench
#  if [ "${run_mqttbench}" = "true" ]; then
#    for i in $(eval echo {1..${M_COUNT}}); do
#       component[$((++lc))]=cAppDriver,m${i},"-e|$(eval echo \$\{M${i}_TESTROOT\})/svt_atelm/runbench.sh,-o|$(eval echo \$\{AP_IPv4_$((${i}%2+1))\})|16102|$(($timeout-${M_COUNT}))"
#       component[$((++lc))]=cAppDriver,m${i},"-e|$(eval echo \$\{M${i}_TESTROOT\})/svt_atelm/runbench.sh,-o|$(($timeout-${M_COUNT}*2))"
#       component[$((++lc))]=sleep,1
#    done
#
#    wait for mqttbench to stabilize
#    component[$((++lc))]=sleep,20
#  fi


if [ "${A1_TYPE}" == "DOCKER" ]; then
  component[$((++lc))]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.py,"
  component[$((++lc))]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.py,"
else
  component[$((++lc))]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.sh,"
  component[$((++lc))]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.sh,"
fi

  save_lc=${lc}
  # Start the Java MQTT Subscribers
  i=0
  while [ $i -lt ${mqtt_subscribers} ]; do
    msub=$((${msub} % ${msub_max} + 1))
    if [ ${msub} -lt ${msub_min} ]; then msub=${msub_min}; fi
    component[$((++lc))]=javaAppDriver,m$msub,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|0|-t|/APP/1/#|-s|tcp://${AP_IPv4_2}:${intranet_port}|-n|${MESSAGES}|-exact|-nq|${MESSAGES}:$((${MESSAGES}*2)):0:0,-s|JVM_ARGS=-Xss1024K|-Xshareclasses"
    test_template_compare_string[${lc}]="eceived ${MESSAGES} messages."
    search[$msub]=1
    ((i++))
  done

  # Start the JMS Subscribers
  i=0
  while [ $i -lt ${jms_subscribers} ]; do
    msub=$((${msub} % ${msub_max} + 1))
    if [ ${msub} -lt ${msub_min} ]; then msub=${msub_min}; fi
    component[$((++lc))]=javaAppDriver,m$msub,"-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/APP/1/#|-s|tcp://${AP_IPv4_2}:${intranet_port}|-n|${MESSAGES},-s|JVM_ARGS=-Xss1024K|-Xshareclasses"
    test_template_compare_string[${lc}]="received ${MESSAGES} messages."
    search[$msub]=1
    ((i++))
  done

  i=0
  while [ $i -lt ${jms_sslsubscribers} ]; do
    msub=$((${msub} % ${msub_max} + 1))
    if [ ${msub} -lt ${msub_min} ]; then msub=${msub_min}; fi
    component[$((++lc))]=javaAppDriver,m$msub,"-e|svt.jms.ism.JMSSample,-o|-a|subscribe|-t|/APP/1/#|-s|tcps://${AP_IPv4_2}:${demo_port}|-n|${MESSAGES},-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=${M1_TESTROOT}/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
    test_template_compare_string[${lc}]="received ${MESSAGES} messages."
    search[$msub]=1
    ((i++))
  done

  # Start the C MQTT Subscribers
  i=0
  while [ $i -lt ${cmqtt_subscribers} ]; do
    msub=$((${msub} % ${msub_max} + 1))
    if [ ${msub} -lt ${msub_min} ]; then msub=${msub_min}; fi
    component[$((++lc))]=cAppDriver,m$msub,"-e|com.ibm.ima.samples.mqttv3.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|0|-t|/APP/1/#|-s|tcp://${AP_IPv4_2}:${intranet_port}|-n|${MESSAGES},-s|JVM_ARGS=-Xss1024K"
    test_template_compare_string[${lc}]="eceived ${MESSAGES} messages."
    search[$msub]=1
    ((i++))
  done

  # Sleep 3 seconds before starting publishers
  if ([ lc != 0 ] && [ ${save_lc} != ${lc} ]); then
    component[$((++lc))]=sleep,3
  fi


#  for i in $(eval echo {1..${M_COUNT}}); do
#    component[$((++lc))]=runCommand,m$i,./getidle.sh
#  done


  # Start the Java MQTT Publishers
  i=0
  while [ $i -lt ${mqtt_publishers} ]; do
    mpub=$((${mpub} % ${mpub_max} + 1))
    if [ ${mpub} -lt ${mpub_min} ]; then mpub=${mpub_min}; fi
    component[$((++lc))]=javaAppDriver,m$mpub,"-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|0|-t|/APP/#|-s|tcp://${AP_IPv4_1}:${intranet_port}|-n|${MESSAGES},-s|JVM_ARGS=-Xss1024K|-Xshareclasses"
    test_template_compare_string[${lc}]="Success"
    search[$mpub]=1
    ((i++))
  done

  i=0
  while [ $i -lt ${mqtt_sslpublishers} ]; do
    mpub=$((${mpub} % ${mpub_max} + 1))
    if [ ${mpub} -lt ${mpub_min} ]; then mpub=${mpub_min}; fi
    component[$((++lc))]=javaAppDriver,m$mpub,"-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-q|0|-t|/APP/#|-s|ssl://${AP_IPv4_1}:${internet_port}|-n|${MESSAGES},-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=${M1_TESTROOT}/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
    test_template_compare_string[${lc}]="eceived ${MESSAGES} messages."
    search[$mpub]=1
    ((i++))
  done



  # Start MQTT SSL vehicles
  i=0
  while [ $i -lt $((${mqtt_vehicles}/${BATCH})) ]; do
    printf -v uname "c%07d" ${id}
    ((id=id+${BATCH}))
    mpub=$((${mpub} % ${mpub_max} + 1))
    if [ ${mpub} -lt ${mpub_min} ]; then mpub=${mpub_min}; fi
    component[$((++lc))]=javaAppDriver,m$mpub,"-e|svt.scale.vehicle.SVTVehicleScale,-o|-server|${AP_IPv4_1}|-port|${internet_port}|-ssl|true|-userName|${uname}|-password|${LDAPPASSWORD}|-mode|connect_once|-mqtt|${BATCH}|-minutes|${minutes}|-appid|1|-order|${order}|-qos|${qos}|-stats|${stats}|-listener|${listen}|-vverbose|true,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=${M1_TESTROOT}/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
    test_template_compare_string[${lc}]="SVTVehicleScale Success"
    test_template_metrics_v1[${lc}]="1"
    search[$mpub]=1
    ((i++))
  done

  remainder=$((${mqtt_vehicles}%${BATCH}))
  if [ ${remainder} -gt 0 ]; then
    printf -v uname "c%07d" ${id}
    ((id=id+${remainder}))
    mpub=$((${mpub} % ${mpub_max} + 1))
    if [ ${mpub} -lt ${mpub_min} ]; then mpub=${mpub_min}; fi
    component[$((++lc))]=javaAppDriver,m$mpub,"-e|svt.scale.vehicle.SVTVehicleScale,-o|-server|${AP_IPv4_1}|-port|${internet_port}|-ssl|true|-userName|${uname}|-password|${LDAPPASSWORD}|-mode|connect_once|-mqtt|${remainder}|-minutes|${minutes}|-appid|1|-order|${order}|-qos|${qos}|-stats|${stats}|-listener|${listen}|-vverbose|true,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=${M1_TESTROOT}/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
    test_template_compare_string[${lc}]="SVTVehicleScale Success"
    test_template_metrics_v1[${lc}]="1"
    search[$mpub]=1
    ((i++))
  fi


  i=0
  while [ $i -lt $((${paho_vehicles}/${BATCH})) ]; do
    printf -v uname "c%07d" ${id}
    ((id=id+${BATCH}))
    mpub=$((${mpub} % ${mpub_max} + 1))
    if [ ${mpub} -lt ${mpub_min} ]; then mpub=${mpub_min}; fi
    component[$((++lc))]=javaAppDriver,m$mpub,"-e|svt.scale.vehicle.SVTVehicleScale,-o|-server|${AP_IPv4_1}|-port|${internet_port}|-ssl|true|-userName|${uname}|-password|${LDAPPASSWORD}|-mode|connect_once|-paho|${BATCH}|-minutes|${minutes}|-appid|1|-order|${order}|-qos|${qos}|-stats|${stats}|-listener|${listen}|-vverbose|true,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=${M1_TESTROOT}/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
    test_template_compare_string[${lc}]="SVTVehicleScale Success"
    test_template_metrics_v1[${lc}]="1"
    search[$mpub]=1
    ((i++))
  done

  remainder=$((${paho_vehicles}%${BATCH}))
  if [ $((${paho_vehicles}%${BATCH})) -gt 0 ]; then
    printf -v uname "c%07d" ${id}
    ((id=id+${remainder}))
    mpub=$((${mpub} % ${mpub_max} + 1))
    if [ ${mpub} -lt ${mpub_min} ]; then mpub=${mpub_min}; fi
    component[$((++lc))]=javaAppDriver,m$mpub,"-e|svt.scale.vehicle.SVTVehicleScale,-o|-server|${AP_IPv4_1}|-port|${internet_port}|-ssl|true|-userName|${uname}|-password|${LDAPPASSWORD}|-mode|connect_once|-paho|${remainder}|-minutes|${minutes}|-appid|1|-order|${order}|-qos|${qos}|-stats|${stats}|-listener|${listen}|-vverbose|true,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=${M1_TESTROOT}/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
    test_template_compare_string[${lc}]="SVTVehicleScale Success"
    test_template_metrics_v1[${lc}]="1"
    search[$mpub]=1
    ((i++))
  fi


  i=0
  while ([ ${jms_vehicles} -gt 0 ] && [ $i -lt $((${jms_vehicles}/${BATCH2})) ]); do
    printf -v uname "c%07d" ${id}
    ((id=id+${BATCH2}))
    mpub=$((${mpub} % ${mpub_max} + 1))
    if [ ${mpub} -lt ${mpub_min} ]; then mpub=${mpub_min}; fi
    component[$((++lc))]=javaAppDriver,m$mpub,"-e|svt.scale.vehicle.SVTVehicleScale,-o|-server|${AP_IPv4_1}|-port|${internet_port}|-ssl|true|-userName|${uname}|-password|${LDAPPASSWORD}|-mode|connect_once|-jms|${BATCH2}|-minutes|${minutes}|-appid|1|-order|${order}|-qos|${qos}|-stats|${stats}|-listener|${listen}|-vverbose|true,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=${M1_TESTROOT}/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
    test_template_compare_string[${lc}]="SVTVehicleScale Success!!!"
    test_template_metrics_v1[${lc}]="1"
    search[$mpub]=1
    ((i++))
  done

  remainder=$((${jms_vehicles}%${BATCH2}))
  if [ ${remainder} -gt 0 ]; then
    printf -v uname "c%07d" ${id}
    ((id=id+${remainder}))
    mpub=$((${mpub} % ${mpub_max} + 1))
    if [ ${mpub} -lt ${mpub_min} ]; then mpub=${mpub_min}; fi
    component[$((++lc))]=javaAppDriver,m$mpub,"-e|svt.scale.vehicle.SVTVehicleScale,-o|-server|${AP_IPv4_1}|-port|${internet_port}|-ssl|true|-userName|${uname}|-password|${LDAPPASSWORD}|-mode|connect_once|-jms|${remainder}|-minutes|${minutes}|-appid|1|-order|${order}|-qos|${qos}|-stats|${stats}|-listener|${listen}|-vverbose|true,-s|JVM_ARGS=-Xss1024K|-Xshareclasses|-Djavax.net.ssl.trustStore=${M1_TESTROOT}/svt_ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
    test_template_compare_string[${lc}]="SVTVehicleScale Success!!!"
    test_template_metrics_v1[${lc}]="1"
    search[$mpub]=1
    ((i++))
  fi


  # Start C MQTT publishers
  i=0
  while [ $i -lt ${cmqtt_publishers} ]; do
    mpub=$((${mpub} % ${mpub_max} + 1))
    if [ ${mpub} -lt ${mpub_min} ]; then mpub=${mpub_min}; fi
    # component[$((++lc))]=javaAppDriver,m$mpub,"-e|svt.scale.vehicle.SVTVehicleScale,-o|-server|${AP_IPv4_1}|-port|${demo_port}|-mode|connect_once|-mqtt|${BATCH}|-minutes|${minutes}|-appid|1|-order|${order}|-qos|${qos}|-stats|${stats}|-listener|${listen},-s|JVM_ARGS=-Xss1024K"
    test_template_compare_string[${lc}]="SVTVehicleScale Success"
    test_template_metrics_v1[${lc}]="1"
    search[$mpub]=1
    ((i++))
  done

  # Start JavaScript publishers
  i=0
  while [ $i -lt ${js_publishers} ]; do
    mpub=$((${mpub} % ${mpub_max} + 1))
    if [ ${mpub} -lt ${mpub_min} ]; then mpub=${mpub_min}; fi
    component[$((++lc))]=cAppDriver,m$mpub,"-e|firefox","-o|-new-window|${URL_M1_IPv4}/${httpSubDir}/user_scale.html?&client_count=${svt}&action=$action&logfile=${xml[${n}]}-user_scale-M1-6.log&test_minutes=${minutes}&verbosity=9&start=true","-s-DISPLAY=localhost:1"
    test_template_compare_string[${lc}]="SVTVehicleScale Success"
    test_template_metrics_v1[${lc}]="1"
    search[$mpub]=1
    ((i++))
  done


  for i in ${!search[@]}; do
#      component[$((++lc))]=searchLogsEnd,m$i,${xml[$n]}_m$i.comparetest,9
      component[$((++lc))]=`test_template_format_search_component m$i`
  done

#  if [ "${run_mqttbench}" = "true" ]; then
#    for i in $(eval echo {1..${M_COUNT}}); do
#      component[$((++lc))]=cAppDriver,m${i},"-e|killall,-o|mqttbench"
#      component[$((++lc))]=runCommandEnd,m$i,$(eval echo \$\{M${i}_TESTROOT\})/svt_atelm/killbench.sh
#      component[$((++lc))]=runCommandEnd,m$i,killbench.sh
#    done
#  fi

#  component[$((++lc))]=runCommandEnd,m1,./end_report.sh

#  test_template_runorder=$(eval echo {1..${lc}})
  test_template_finalize_test



  echo
  echo ================================================================================================
  echo
  for i in ${!component[@]}; do
    echo component[$i]=${component[$i]}
  done

  echo
  for i in ${!test_template_compare_string[@]}; do
    echo test_template_compare_string[${i}]=${test_template_compare_string[${i}]}
  done

  echo
  for i in ${!test_template_metrics_v1[@]}; do
    echo test_template_metrics_v1[${i}]=${test_template_metrics_v1[${i}]}
  done

  echo
  echo test_template_runorder=${test_template_runorder}
  echo
  echo ================================================================================================
  echo

}
