#!/bin/bash

# test_template_define testcase=${tname} description=mqtt_41_311_HA timeout=600 \
#                      mqtt[0]=1 action[0]=subscribe id[0]=10 qos[0]=0 topic[0]=/svtGroup0/chat port[0]=16212 messages[0]=$((3*60000)) \
#                      mqtt[1]=1 action[1]=subscribe id[1]=11 qos[1]=1 topic[1]=/svtGroup0/chat port[1]=16212 messages[1]=$((3*60000)) \
#                      mqtt[2]=1 action[2]=subscribe id[2]=12 qos[2]=2 topic[2]=/svtGroup0/chat port[2]=16212 messages[2]=$((3*60000)) \
#                      mqtt[3]=1 action[3]=publish   id[3]=20 qos[3]=0 topic[3]=/svtGroup0/chat port[3]=16211 messages[3]=60000 \
#                      mqtt[4]=1 action[4]=publish   id[4]=21 qos[4]=1 topic[4]=/svtGroup0/chat port[4]=16211 messages[4]=60000 \
#                      mqtt[5]=1 action[5]=publish   id[5]=22 qos[5]=2 topic[5]=/svtGroup0/chat port[5]=16211 messages[5]=60000 

if [ "${A1_HOST}" == "" ]; then
  if [ -f ${TESTROOT}/scripts/ISMsetup.sh ]; then
    source ${TESTROOT}/scripts/ISMsetup.sh
  elif [ -f ../scripts/ISMsetup.sh ]; then
    source ../scripts/ISMsetup.sh
  else
    source /niagara/test/scripts/ISMsetup.sh
  fi
fi

test_template_define() {
  
  unset component
  unset test_template_compare_string
  unset test_template_metrics_v1
  unset search

  declare -al client
  declare -ali count
  declare -al action
  declare -ali id
  declare -ali qos
  declare -al topic
  declare -ali port
  declare -ali messages
  declare -al message
  declare -al ssl
  declare -ali rate
  declare -al durable
  declare -al persist
  
  echo $*

  eval $1
  eval $2
  eval $3
  eval $4

  read -ra client <<< "${5}"
  read -ra count <<< "${6}"
  read -ra action <<< "${7}"
  read -ra id <<< "${8}"
  read -ra qos <<< "${9}"
  read -ra durable <<< "${10}"
  read -ra persist <<< "${11}"
  read -ra topic <<< "${12}"
  read -ra port <<< "${13}"
  read -ra messages <<< "${14}"
  IFS='|' read -ra message <<< "${15}"
  read -ra rate <<< "${16}"
  read -ra ssl <<< "${17}"

  i=0
  while [ $i -lt ${#client[@]} ]; do
    echo client[$i]=${client[$i]}, count[$i]=${count[$i]}, action[$i]=${action[$i]}, id[$i]=${id[$i]}, qos[$i]=${qos[$i]}, durable[$i]=${durable[$i]}, topic[$i]=${topic[$i]}, persist[$i]=${persist[$i]}, port[$i]=${port[$i]}, messages[$i]=${messages[$i]}, message[$i]=${message[$i]} ssl[$i]=${ssl[$i]} rate[$i]=${rate[$i]}
    ((i++))
  done

  i=0
  while [ $i -lt ${#client[@]} ]; do
    if [ "${client[$i]}" = "mqtt" ]; then 
      echo ${count[$i]} ${client[$i]} client ${action[$i]} to ${topic[$i]} with QoS ${qos[$i]} for ${messages[$i]} messages
    else
      if [ "${action[$i]}" = "publish" ]; then 
        echo ${count[$i]} ${client[$i]} client ${action[$i]} to ${topic[$i]} with persistence ${persist[$i]} for ${messages[$i]} messages
      else
        echo ${count[$i]} ${client[$i]} client ${action[$i]} to ${topic[$i]} with durable ${durable[$i]} for ${messages[$i]} messages
      fi
    fi
    ((i++))
  done

  declare -il lc=0
  declare -il save_lc=0
  declare -il i=0
  declare -l uname
  declare -l proto
  declare -il mach=0

  xml[${n}]=${testcase}
  test_template_initialize_test "${xml[${n}]}"
  scenario[${n}]="${xml[${n}]} - "${description}
  timeouts[${n}]=${timeout}
  
  save_lc=${lc}

  component[$((++lc))]=cAppDriverLogEnd,m1,"-e|../scripts/svt-deleteAllSubscriptions.py,"
  component[$((++lc))]=cAppDriverLogEnd,m1,"-e|../scripts/svt-deleteAllMqttClients.py,"

  # Start the Java MQTT Subscribers

  x=0
  while [ $x -lt ${#client[@]} ]; do
    if [ "${action[$x]}" = "publish" ]; then 
      component[$((++lc))]=sleep,15
    fi

    if [ "${client[$x]}" = "mqtt" ]; then 
      i=0
      while [ $i -lt ${count[$x]} ]; do
        printf -v uname "u%07d" $((${id[$x]}+$i))
        if [[ (-n "${ssl[$x]}") && ("${ssl[$x]}" = "true") ]]; then proto=ssl
        else proto=tcp
        fi
    
        mach=$((${mach} % ${M_COUNT} + 1))

        component[$((++lc))]=javaAppDriver,m${mach},"-e|svt.mqtt.mq.MqttSample,-o|-a|${action[$x]}|-mqtt|3.1.1|-q|${qos[$x]}|-t|${topic[$x]}|-s|${proto}://${A1_IPv4_1}:${port[$x]}!${proto}://${A2_IPv4_1}:${port[$x]}|-i|${uname}|-u|${uname}|-p|imasvtest|-c|true"
    
        if [ "${order}" == "true" ]; then component[${lc}]=${component[${lc}]}"|-O"; fi
        if [[ (-n "${rate[$x]}") && (${rate[$x]} -gt 0) ]]; then component[${lc}]=${component[${lc}]}"|-w|${rate[$x]}"; fi
    
        if [[ (-n ${Nm[$x]}) && (${Nm[$x]} -gt 0) ]]; then component[${lc}]=${component[${lc}]}"|-Nm|${Nm[$x]}"
          else component[${lc}]=${component[${lc}]}"|-n|${messages[$x]}"; fi
    
    #    component[${lc}]=${component[${lc}]}",-s|JVM_ARGS=-Djava.util.logging.config.file=./paho/pahosub.properties"
  
        if [[ (-n "${ssl[$x]}") && ("${ssl[$x]}" = "true") ]]; then proto=ssl
          component[${lc}]=${component[${lc}]}",-s|JVM_ARGS=-Djavax.net.ssl.trustStore=${M1_TESTROOT}/common_src/ibm.jks|-Djavax.net.ssl.trustStorePassword=password"
        fi
    
        test_template_compare_string[${lc}]=${message[$x]}
        search[$mach]=1
        ((i++))
      done
    elif [ "${client[$x]}" = "jms" ]; then 
      i=0
      while [ $i -lt ${count[$x]} ]; do
        printf -v uname "u%07d" $((${id[$x]}+$i))
        if [[ (-n "${ssl[$x]}") && ("${ssl[$x]}" = "true") ]]; then proto=tcps
        else proto=tcp
        fi
    
        mach=$((${mach} % ${M_COUNT} + 1))

        component[$((++lc))]=javaAppDriver,m${mach},"-e|svt.jms.ism.JMSSample,-o|-a|${action[$x]}|-t|${topic[$x]}|-s|${proto}://${A1_IPv4_1}:${port[$x]}!${proto}://${A2_IPv4_1}:${port[$x]}|-i|${uname}|-u|${uname}|-p|imasvtest|-v"
    
        if [ "${order}" != "true" ]; then component[${lc}]=${component[${lc}]}"|-O"; fi
        if [[ (-n "${rate[$x]}") && (${rate[$x]} -gt 0) ]]; then component[${lc}]=${component[${lc}]}"|-w|${rate[$x]}"; fi
    
        if [[ (-n ${Nm[$x]}) && (${Nm[$x]} -gt 0) ]]; then component[${lc}]=${component[${lc}]}"|-Nm|${Nm[$x]}"
          else component[${lc}]=${component[${lc}]}"|-n|${messages[$x]}"; fi
    
        if [[ (-n "${durable[$x]}") && ("${durable[$x]}" = "true") ]]; then
          component[${lc}]=${component[${lc}]}"|-b"; fi

        if [[ (-n "${persist[$x]}") && ("${persist[$x]}" = "true") ]]; then
          component[${lc}]=${component[${lc}]}"|-r"; fi

        if [[ (-n "${ssl[$x]}") && ("${ssl[$x]}" = "true") ]]; then proto=ssl
          component[${lc}]=${component[${lc}]}",-s|JVM_ARGS=-Djavax.net.ssl.trustStore=${M1_TESTROOT}/common_src/ibm.jks|-Djavax.net.ssl.trustStorePassword=password"
        fi
    
        test_template_compare_string[${lc}]=${message[$x]}
        search[$mach]=1
        ((i++))
      done
    else 
      printf "else"
    fi
    ((x++))
  done

  
#-----------------------------
    component[$((++lc))]=sleep,30
#-----------------------------
#   component[$((++lc))]=cAppDriverWait,m1,"-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|stopPrimary|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=${xml[${n}]}|SVT_AT_VARIATION_QUICK_EXIT=true"
#   test_template_compare_string[${lc}]="AF_TEST_RESULT: SUCCESS"
#-----------------------------
#   component[$((++lc))]=cAppDriverWait,m1,"-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|startStandby|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=${xml[${n}]}|SVT_AT_VARIATION_QUICK_EXIT=true"
#   test_template_compare_string[${lc}]="AF_TEST_RESULT: SUCCESS"

    component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/haFunctions.py,-o|-a|restartPrimary|-t|600,"
    test_template_compare_string[${lc}]="Test result is Success!"
#-----------------------------
    component[$((++lc))]=sleep,60
#-----------------------------
#   component[$((++lc))]=cAppDriverWait,m1,"-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|stopPrimary|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=${xml[${n}]}|SVT_AT_VARIATION_QUICK_EXIT=true"
#   test_template_compare_string[${lc}]="AF_TEST_RESULT: SUCCESS"
#-----------------------------
#   component[$((++lc))]=cAppDriverWait,m1,"-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|startStandby|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=${xml[${n}]}|SVT_AT_VARIATION_QUICK_EXIT=true"
#   test_template_compare_string[${lc}]="AF_TEST_RESULT: SUCCESS"

    component[$((++lc))]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/haFunctions.py,-o|-a|restartPrimary|-t|600,"
    test_template_compare_string[${lc}]="Test result is Success!"
#-----------------------------
  
  for i in ${!search[@]}; do
    component[$((++lc))]=`test_template_format_search_component m$i`
  done

  test_template_finalize_test

  echo
  echo ================================================================================================
  echo 
  echo xml[${n}]=\"${testcase}\" > ./${testcase}.sh
  echo test_template_initialize_test \"${xml[${n}]}\"  >> ./${testcase}.sh
  echo scenario[${n}]=\"${xml[${n}]} - ${description}\" >> ./${testcase}.sh
  echo timeouts[${n}]=${timeout} >> ./${testcase}.sh

  echo >> ./${testcase}.sh
  for i in ${!component[@]}; do
    echo component[$i]=\"${component[$i]}\" >> ./${testcase}.sh
  done

  echo >> ./${testcase}.sh
  for i in ${!test_template_compare_string[@]}; do
    echo test_template_compare_string[${i}]=\"${test_template_compare_string[${i}]}\" >> ./${testcase}.sh
  done

  echo >> ./${testcase}.sh
  for i in ${!test_template_metrics_v1[@]}; do
    echo test_template_metrics_v1[${i}]=\"${test_template_metrics_v1[${i}]}\" >> ./${testcase}.sh
  done

  echo >> ./${testcase}.sh
  echo test_template_runorder=\"${test_template_runorder}\" >> ./${testcase}.sh
  echo test_template_finalize_test >> ./${testcase}.sh
  echo
  echo ================================================================================================
  echo

  unset component
  unset test_template_compare_string
  unset test_template_metrics_v1
  unset search

}



