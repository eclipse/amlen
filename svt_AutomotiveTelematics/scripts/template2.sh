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
  declare pubtopic=""
  declare subtopic=""
  declare -i pubport=-1
  declare -i subport=-1
  declare -i timeout=0
  declare -i minutes=0
  declare -i qos=0
  declare order=false
  declare stats=false
  declare listen=false
  declare -i sub_messages=-1
  declare -i pub_messages=100
  declare pub_message=""
  declare evil_submessage=""
  declare evil_pubmessage=""
  declare -i mqtt_subscribers=0
  declare -i mqtt_xsubscribers=0
  declare -i evil_subscribers=0
  declare -i mqtt_sslsubscribers=0
  declare -i mqtt_publishers=0
  declare -i evil_publishers=0
  declare -i mqtt_sslpublishers=0
  declare -i sub=0
  declare -i pub=0
  declare -i sub_Nm=0
  declare -i xsub_Nm=0
  declare -i evil_subNm=0
  declare -i pub_Nm=0
  declare -i evil_pubNm=0
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
    elif [[ $i = mqtt_xsubscribers=* ]]; then
      mqtt_xsubscribers=${i##*=}
    elif [[ $i = evil_subscribers=* ]]; then
      evil_subscribers=${i##*=}
    elif [[ $i = mqtt_sslsubscribers=* ]]; then
      mqtt_sslsubscribers=${i##*=}
    elif [[ $i = mqtt_publishers=* ]]; then
      mqtt_publishers=${i##*=}
    elif [[ $i = evil_publishers=* ]]; then
      evil_publishers=${i##*=}
    elif [[ $i = mqtt_sslpublishers=* ]]; then
      mqtt_sslpublishers=${i##*=}
    elif [[ $i = enable_ldap=* ]]; then
      enable_ldap=${i##*=}
    elif [[ $i = pubtopic=* ]]; then
      pubtopic=${i##*=}
    elif [[ $i = subtopic=* ]]; then
      subtopic=${i##*=}
    elif [[ $i = pubport=* ]]; then
      pubport=${i##*=}
    elif [[ $i = subport=* ]]; then
      subport=${i##*=}
    elif [[ $i = pub_messages=* ]]; then
      pub_messages=${i##*=}
    elif [[ $i = sub_messages=* ]]; then
      sub_messages=${i##*=}
    elif [[ $i = pub_message=* ]]; then
      pub_message=${i##*=}
    elif [[ $i = sub_message=* ]]; then
      sub_message=${i##*=}
    elif [[ $i = evil_pubmessage=* ]]; then
      evil_pubmessage=${i##*=}
    elif [[ $i = pub_Nm=* ]]; then
      pub_Nm=${i##*=}
    elif [[ $i = sub_Nm=* ]]; then
      sub_Nm=${i##*=}
    elif [[ $i = xsub_Nm=* ]]; then
      xsub_Nm=${i##*=}
    elif [[ $i = evil_pubNm=* ]]; then
      evil_pubNm=${i##*=}
    elif [[ $i = evil_subNm=* ]]; then
      evil_subNm=${i##*=}
    elif [[ $i = sub=* ]]; then
      sub=${i##*=}
    elif [[ $i = pub=* ]]; then
      pub=${i##*=}
    else
      echo invalid parameter "$i"
    fi
  done

 # if [[ ${enable_ldap} =~ [true] ]]; then
     declare -i internet_port=16112
 # else
    declare -i internet_port=16111
 # fi

  if [[ "${pubtopic}" == "" ]]; then
     pubtopic="/APP/1/#"
  fi
  if [[ "${subtopic}" == "" ]]; then
     subtopic="/APP/1/#"
  fi

  declare -lir intranet_port=16999
  declare -lir demo_port=16102

#  declare -i MESSAGES=${pub_messages}

  if [[ "${sub_messages}" == "-1" ]]; then
    sub_messages=$((${pub_messages}*(${mqtt_publishers}+${mqtt_sslpublishers})))
  fi

  if [[ "${sub_message}" == "" ]]; then
  sub_message="Received ${sub_messages} messages"
  fi

# xsub_message="Received 0 messages"
  xsub_message="Not authorized to connect"

  if [[ "${pub_message}" == "" ]]; then
     pub_message="Published ${pub_messages} messages"
  fi

  if [[ "${evil_submessage}" == "" ]]; then
     evil_submessage="Received 0 messages"
  fi

  if [[ "${evil_pubmessage}" == "" ]]; then
     evil_pubmessage=${pub_message}
  fi

  declare -lr LDAPPASSWORD=imasvtest

  declare -li lc=0
  declare -li save_lc=0
  declare -li i=0
  declare -li id=0
  declare -l uname
  declare -l remainder=0

  declare -li id=0
  declare -li msub=0
  declare -li mpub=0
  declare -lir msub_min=1
  declare -lir msub_max=${M_COUNT}
  declare -lir mpub_min=1
  declare -lir mpub_max=${M_COUNT}

  unset component
  unset test_template_compare_string
  unset test_template_metrics_v1
  unset search

  echo xml[${n}]=${testcase}
  xml[${n}]=${testcase}
  echo test_template_initialize_test "${xml[${n}]}"
  test_template_initialize_test "${xml[${n}]}"
  echo scenario[${n}]="${xml[${n}]} - "${description}
  scenario[${n}]="${xml[${n}]} - "${description}
  echo timeouts[${n}]=${timeout}
  timeouts[${n}]=${timeout}


  save_lc=${lc}

  component[$((++lc))]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllSubscriptions.py,"
  component[$((++lc))]=cAppDriverWait,m1,"-e|../scripts/svt-deleteAllMqttClients.py,"

  # Start the Java MQTT Subscribers
  i=0
  while [ $i -lt ${mqtt_subscribers} ]; do
    msub=$((${msub} % ${msub_max} + 1))
    if [ ${msub} -lt ${msub_min} ]; then msub=${msub_min}; fi
    if [ ${subport} == 0 ]; then subport=${intranet_port}; fi
    printf -v uname "u%07d" ${sub}
    ((sub=sub+1))
    component[$((++lc))]=javaAppDriverWait,m$msub,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|${subtopic}|-s|tcp://${AP_IPv4_1}:${subport}|-i|${uname}|-u|${uname}|-p|imasvtest|-v|-n|0|-c|true"
    component[$((++lc))]=javaAppDriver,m$msub,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|${subtopic}|-s|tcp://${AP_IPv4_1}:${subport}|-i|${uname}|-u|${uname}|-p|imasvtest|-v|-c|false"

    if [ ${sub_Nm} -gt 0 ]; then component[${lc}]=${component[${lc}]}"|-Nm|${sub_Nm}"
    else component[${lc}]=${component[${lc}]}"|-n|${sub_messages}"
    fi
    test_template_compare_string[${lc}]=${sub_message}
    search[$msub]=1
    ((i++))
  done

  i=0
  while [ $i -lt ${mqtt_xsubscribers} ]; do
    msub=$((${msub} % ${msub_max} + 1))
    if [ ${msub} -lt ${msub_min} ]; then msub=${msub_min}; fi
    if [ ${subport} == 0 ]; then subport=${intranet_port}; fi
    printf -v uname "x%07d" ${sub}
    ((sub=sub+1))
    component[$((++lc))]=javaAppDriver,m$msub,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|${subtopic}|-s|tcp://${AP_IPv4_1}:${subport}|-i|${uname}|-u|${uname}|-p|imasvtest|-v"

    if [ ${xsub_Nm} -gt 0 ]; then component[${lc}]=${component[${lc}]}"|-Nm|${xsub_Nm}"
    else component[${lc}]=${component[${lc}]}"|-Nm|1"
    fi
    test_template_compare_string[${lc}]=${xsub_message}
    search[$msub]=1
    ((i++))
  done

  i=0
  while [ $i -lt ${evil_subscribers} ]; do
    msub=$((${msub} % ${msub_max} + 1))
    if [ ${msub} -lt ${msub_min} ]; then msub=${msub_min}; fi
    if [ ${subport} == 0 ]; then subport=${intranet_port}; fi
    printf -v uname "u0001%03d" ${sub}
    ((sub=sub+1))
    component[$((++lc))]=javaAppDriverWait,m$msub,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|${subtopic}|-s|tcp://${AP_IPv4_1}:${subport}|-v|-i|${uname}|-u|${uname}|-p|imasvtest|-v|-n|0|-c|true"
    component[$((++lc))]=javaAppDriver,m$msub,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|${subtopic}|-s|tcp://${AP_IPv4_1}:${subport}|-v|-i|${uname}|-u|${uname}|-p|imasvtest|-v|-c|false"
    if [ ${evil_subNm} -gt 0 ]; then component[${lc}]=${component[${lc}]}"|-Nm|${evil_subNm}"
    else component[${lc}]=${component[${lc}]}"|-n|${evil_submessages}"
    fi
    test_template_compare_string[${lc}]=${evil_submessage}
    search[$msub]=1
    ((i++))
  done

  # Start the Java MQTT SSL Subscribers
  i=0
  while [ $i -lt ${mqtt_sslsubscribers} ]; do
    msub=$((${msub} % ${msub_max} + 1))
    if [ ${msub} -lt ${msub_min} ]; then msub=${msub_min}; fi
    if [ ${subport} == 0 ]; then subport=${intranet_port}; fi
    printf -v uname "u%07d" ${sub}
    ((sub=sub+1))
    component[$((++lc))]=javaAppDriverWait,m$msub,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|${subtopic}|-s|ssl://${AP_IPv4_1}:${subport}|-v|-i|${uname}|-u|${uname}|-p|imasvtest|-v|-n|0|-c|true,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=${M1_TESTROOT}/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
    component[$((++lc))]=javaAppDriver,m$msub,"-e|svt.mqtt.mq.MqttSample,-o|-a|subscribe|-mqtt|3.1.1|-q|2|-t|${subtopic}|-s|ssl://${AP_IPv4_1}:${subport}|-v|-i|${uname}|-u|${uname}|-p|imasvtest|-v|-c|false,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=${M1_TESTROOT}/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"

    if [ ${sub_Nm} -gt 0 ]; then ${component[${lc}]}=component[${lc}]+"|-Nm|${sub_Nm}"
    else component[${lc}]=${component[${lc}]}"|-n|${sub_messages}"
    fi

    component[${lc}]=${component[${lc}]}+",-s|JVM_ARGS=-Djavax.net.ssl.trustStore=${M1_TESTROOT}/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
    test_template_compare_string[${lc}]=${sub_message}
    search[$msub]=1
    ((i++))
  done

  # Sleep 3 seconds before starting publishers
  if ([ lc != 0 ] && [ ${save_lc} != ${lc} ]); then
    component[$((++lc))]=sleep,3
  fi

  # Start the Java MQTT Publishers
  i=0
  while [ $i -lt ${mqtt_publishers} ]; do
    mpub=$((${mpub} % ${mpub_max} + 1))
    if [ ${mpub} -lt ${mpub_min} ]; then mpub=${mpub_min}; fi
    if [ ${pubport} == 0 ]; then pubport=${intranet_port}; fi
    printf -v uname "u%07d" ${pub}
    ((pub=pub+1))
    component[$((++lc))]=javaAppDriver,m$mpub,"-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-w|1000|-q|2|-t|${pubtopic}|-s|tcp://${AP_IPv4_1}:${pubport}|-v|-i|${uname}|-u|${uname}|-p|imasvtest"
    if [ ${pub_Nm} -gt 0 ]; then component[${lc}]=${component[${lc}]}"|-Nm|${pub_Nm}"
    else component[${lc}]=${component[${lc}]}"|-n|${pub_messages}"
    fi
    test_template_compare_string[${lc}]=${pub_message}
    search[$mpub]=1
    ((i++))
  done

  # Start the Evil Java MQTT Publishers
  i=0
  while [ $i -lt ${evil_publishers} ]; do
    mpub=$((${mpub} % ${mpub_max} + 1))
    if [ ${mpub} -lt ${mpub_min} ]; then mpub=${mpub_min}; fi
    if [ ${pubport} == 0 ]; then pubport=${intranet_port}; fi
    printf -v uname "u0001%03d" ${pub}
    ((pub=pub+1))
    component[$((++lc))]=javaAppDriver,m$mpub,"-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-w|1000|-q|2|-t|${pubtopic}|-s|tcp://${AP_IPv4_1}:${pubport}|-v|-i|${uname}|-u|${uname}|-p|imasvtest"
    if [ ${evil_pubNm} -gt 0 ]; then component[${lc}]=${component[${lc}]}"|-Nm|${evil_pubNm}"
    else component[${lc}]=${component[${lc}]}"|-Nm|1"
    fi
    test_template_compare_string[${lc}]=${evil_pubmessage}
    search[$mpub]=1
    ((i++))
  done

  i=0
  while [ $i -lt ${mqtt_sslpublishers} ]; do
    mpub=$((${mpub} % ${mpub_max} + 1))
    if [ ${mpub} -lt ${mpub_min} ]; then mpub=${mpub_min}; fi
    if [ ${pubport} == 0 ]; then pubport=${internet_port}; fi
    printf -v uname "u%07d" ${pub}
    ((pub=pub+1))
    component[$((++lc))]=javaAppDriver,m$mpub,"-e|svt.mqtt.mq.MqttSample,-o|-a|publish|-mqtt|3.1.1|-w|1000|-q|2|-t|${pubtopic}|-s|ssl://${AP_IPv4_1}:${pubport}|-v|-i|${uname}|-u|${uname}|-p|imasvtest,-s|JVM_ARGS=-Djavax.net.ssl.trustStore=${M1_TESTROOT}/svt_jmqtt/ssl/cacerts.jks|-Djavax.net.ssl.trustStorePassword=k1ngf1sh"
    if [ ${pub_Nm} -gt 0 ]; then component[${lc}]=${component[${lc}]}"|-Nm|${pub_Nm}"
    else component[${lc}]=${component[${lc}]}"|-Nm|1"
    fi
    test_template_compare_string[${lc}]=${pub_message}
    search[$mpub]=1
    ((i++))
  done

  for i in ${!search[@]}; do
    component[$((++lc))]=`test_template_format_search_component m$i`
  done

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
  echo test_template_finalize_test
  echo
  echo ================================================================================================
  echo

}
