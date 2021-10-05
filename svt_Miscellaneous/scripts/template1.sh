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


GetServer() {
# i=0;for line in `ssh admin@${A1_HOST} status imaserver`;do list[$i]=${line};((i++)); done

# if [[ ! "${list[*]}" =~ "HARole" ]]; then
#   printf "A1\n"
# else
#   if [[ "${list[*]}" =~ "$1" ]]; then
#     printf "A1\n"
#   else
#     if [ "${A2_HOST}" == "" ]; then
#       printf "A1\n"
#     else
#       i=0; for line in `ssh admin@${A2_HOST} status imaserver`; do list2[$i]=${line}; ((i++)); done

#       if [[ "${list2[*]}" =~ "$1" ]]; then
#         printf "A2\n"
#       else
#         printf "A1\n"
#       fi
#       unset list2
#     fi
#   fi
# fi
# unset list
  printf "A1\n"
}



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
  eval $5
  eval $6
  eval $7
  eval $8
  eval $9

  read -ra client <<< "${10}"
  read -ra count <<< "${11}"
  read -ra action <<< "${12}"
  read -ra id <<< "${13}"
  read -ra qos <<< "${14}"
  read -ra durable <<< "${15}"
  read -ra persist <<< "${16}"
  read -ra topic <<< "${17}"
  read -ra port <<< "${18}"
  read -ra messages <<< "${19}"
  IFS='|' read -ra message <<< "${20}"
  read -ra rate <<< "${21}"
  read -ra ssl <<< "${22}"

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
  declare -il mod=0

  xml[${n}]=${testcase}
  test_template_initialize_test "${xml[${n}]}"
  scenario[${n}]="${xml[${n}]} - "${description}
  timeouts[${n}]=${timeout}

  save_lc=${lc}

  PRIMARY=`GetServer PRIMARY`
  AP_HOST=$(eval echo \$\{${PRIMARY}_HOST\})
  AP_IPv4_1=$(eval echo \$\{${PRIMARY}_IPv4_1\})
  AP_IPv4_2=$(eval echo \$\{${PRIMARY}_IPv4_2\})
  unset PRIMARY

  STANDBY=`GetServer STANDBY`
  AS_HOST=$(eval echo \$\{${STANDBY}_HOST\})
  AS_IPv4_1=$(eval echo \$\{${STANDBY}_IPv4_1\})
  AS_IPv4_2=$(eval echo \$\{${STANDBY}_IPv4_2\})
  unset STANDBY

  if [ "${fill}" == "true" ]; then

  x=0
  while [ $x -lt ${#client[@]} ]; do
      i=0
      while [ $i -lt ${count[$x]} ]; do
        printf -v uname "u%07d" $((${id[$x]}+$i))

        mach=$((${mach} % ${M_COUNT} + 1))

        if [ "${action[$x]}" == "subscribe" ]; then
          component[$((++lc))]=javaAppDriverWait
        else
          component[$((++lc))]=javaAppDriver
        fi

        if [ "${client[$x]}" == "jmqtt" ]; then
          if [[ (-n "${ssl[$x]}") && ("${ssl[$x]}" = "true") ]]; then proto=ssl
            else proto=tcp; fi

          component[${lc}]+=,m${mach},"-e|svt.mqtt.mq.MqttSample,-o|-a|${action[$x]}|-mqtt|3.1.1|-t|${topic[$x]}|-s|${proto}://${A1_IPv4_1}:${port[$x]}+${proto}://${A2_IPv4_1}:${port[$x]}|-i|${uname}|-v"

          if [ -n "${qos[$x]}" ]; then component[${lc}]=${component[${lc}]}"|-q|"${qos[$x]}; fi
          if [ "${action[$x]}" == "subscribe" ]; then component[${lc}]=${component[${lc}]}"|-c|true"; fi

        else
          if [ "${client[$x]}" == "jms" ]; then
            if [[ (-n "${ssl[$x]}") && ("${ssl[$x]}" = "true") ]]; then proto=tcps
              else proto=tcp; fi

            component[${lc}]+=,m${mach},"-e|svt.jms.ism.JMSSample,-o|-a|${action[$x]}|-t|${topic[$x]}|-s|${proto}://${A1_IPv4_1}:${port[$x]}+${proto}://${A2_IPv4_1}:${port[$x]}|-i|${uname}|-v"

            if [[ (-n "${durable[$x]}") && ("${durable[$x]}" = "true") ]]; then component[${lc}]=${component[${lc}]}"|-b"; fi
            if [[ (-n "${persist[$x]}") && ("${persist[$x]}" = "true") ]]; then component[${lc}]=${component[${lc}]}"|-r"; fi
          fi
        fi

        if [ "${userid}" == "true" ]; then component[${lc}]=${component[${lc}]}"|-u|${uname}|-p|imasvtest"; fi
        if [[ (-n "${rate[$x]}") && (${rate[$x]} -gt 0) ]]; then component[${lc}]=${component[${lc}]}"|-w|${rate[$x]}"; fi

        if [ "${action[$x]}" == "subscribe" ]; then
          component[${lc}]=${component[${lc}]}"|-n|0";
        else
          component[${lc}]=${component[${lc}]}"|-n|${messages[$x]}";
          if [ "${order}" == "true" ]; then component[${lc}]=${component[${lc}]}"|-O"; fi
        fi

        if [[ (-n "${ssl[$x]}") && ("${ssl[$x]}" = "true") ]]; then
          component[${lc}]=${component[${lc}]}",-s|JVM_ARGS=-Djavax.net.ssl.trustStore=${M1_TESTROOT}/common_src/ibm.jks|-Djavax.net.ssl.trustStorePassword=password"
        fi

        if [ "${action[$x]}" != "subscribe" ]; then
          test_template_compare_string[${lc}]=${message[$x]}
          search[$mach]=1
        fi
        ((i++))
      done
    ((x++))
  done
  fi

  if [[ ("${wait}" == "before") && (${failover} -gt 0) ]]; then
#   component[$((++lc))]=cAppDriverWait,m1,"-e|${M1_TESTROOT}/svt_misc/wait.sh,-o|${topic[0]}|${messages[0]}"
    component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-t|${topic[0]}|-k|BufferedMsgs|-c|lt|-v|${messages[0]}|-w|10|-m|300|-r|true"

  elif [[ ("${wait}" == "after") && (${failover} -gt 0) ]]; then
#   component[$((++lc))]=cAppDriverWait,m1,"-e|${M1_TESTROOT}/svt_misc/wait.sh,-o|${topic[0]}|$((${messages[0]} / 2))"
    component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_HOST}:${A1_PORT}|-t|${topic[0]}|-k|BufferedMsgs|-c|lt|-v|$((${messages[0]}/2))|-w|10|-m|300|-r|true"
  fi

  i=0; while [ $i -lt ${failover} ]; do
#   component[$((++lc))]=cAppDriverWait,m1,"-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|stopPrimary|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=${xml[${n}]}|SVT_AT_VARIATION_QUICK_EXIT=true"
#   test_template_compare_string[${lc}]="AF_TEST_RESULT: SUCCESS"

#   component[$((++lc))]=cAppDriverWait,m1,"-e|${M1_TESTROOT}/scripts/af_test.start.workload.sh,-o|haFunctions.sh|startStandby|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=${xml[${n}]}|SVT_AT_VARIATION_QUICK_EXIT=true"
#   test_template_compare_string[${lc}]="AF_TEST_RESULT: SUCCESS"


    component[$((++lc))]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|restartPrimary|-t|300"


    ((i++))
  done

  if [[ ("${wait}" == "after") && (${failover} -gt 0) ]]; then
#   component[$((++lc))]=cAppDriverWait,m1,"-e|${M1_TESTROOT}/svt_misc/wait.sh,-o|${topic[0]}|${messages[0]}"
    component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A2_HOST}:${A2_PORT}|-t|${topic[0]}|-k|BufferedMsgs|-c|lt|-v|${messages[0]}|-w|10|-m|300|-r|true"
  fi

  if [ "${verify}" == "true" ]; then

  mod=$((${failover} % 2))
  if [ ${mod} -eq 1 ]; then
    AV_HOST=${AS_HOST}
    AV_IPv4_1=${AS_IPv4_1}
    AV_IPv4_2=${AS_IPv4_2}
  else
    AV_HOST=${AP_HOST}
    AV_IPv4_1=${AP_IPv4_1}
    AV_IPv4_2=${AP_IPv4_2}
  fi
  echo "failover:  "${failover}
  echo "mod:  "${mod}
  echo "AV_IPv4_1:  "${AV_IPv4_1}
  unset mod

  x=0
  while [ $x -lt ${#client[@]} ]; do
    i=0
    while [ $i -lt ${count[$x]} ]; do
      printf -v uname "u%07d" $((${id[$x]}+$i))

      if [ "${action[$x]}" == "subscribe" ]; then
        mach=$((${mach} % ${M_COUNT} + 1))

        if [ "${client[$x]}" == "jmqtt" ]; then
          if [[ (-n "${ssl[$x]}") && ("${ssl[$x]}" = "true") ]]; then proto=ssl
            else proto=tcp; fi

          component[$((++lc))]=javaAppDriver,m${mach},"-e|svt.mqtt.mq.MqttSample,-o|-a|${action[$x]}|-mqtt|3.1.1|-t|${topic[$x]}|-s|${proto}://${A1_IPv4_1}:${port[$x]}+${proto}://${A2_IPv4_1}:${port[$x]}|-i|${uname}|-v|-c|false"

          if [ -n "${qos[$x]}" ]; then component[${lc}]=${component[${lc}]}"|-q|"${qos[$x]}; fi

        else
          if [ "${client[$x]}" == "jms" ]; then
            if [[ (-n "${ssl[$x]}") && ("${ssl[$x]}" = "true") ]]; then proto=tcps
              else proto=tcp; fi

            component[$((++lc))]=javaAppDriver,m${mach},"-e|svt.jms.ism.JMSSample,-o|-a|${action[$x]}|-t|${topic[$x]}|-s|${proto}://${A1_IPv4_1}:${port[$x]}+${proto}://${A2_IPv4_1}:${port[$x]}|-i|${uname}|-v"

            if [[ (-n "${durable[$x]}") && ("${durable[$x]}" = "true") ]]; then component[${lc}]=${component[${lc}]}"|-b"; fi
            if [[ (-n "${persist[$x]}") && ("${persist[$x]}" = "true") ]]; then component[${lc}]=${component[${lc}]}"|-r"; fi
          fi
        fi

        if [ "${order}" == "true" ]; then component[${lc}]=${component[${lc}]}"|-O"; fi
        if [ "${userid}" == "true" ]; then component[${lc}]=${component[${lc}]}"|-u|${uname}|-p|imasvtest"; fi
        if [[ (-n "${rate[$x]}") && (${rate[$x]} -gt 0) ]]; then component[${lc}]=${component[${lc}]}"|-w|${rate[$x]}"; fi

        component[${lc}]=${component[${lc}]}"|-n|${messages[$x]}";

        if [[ (-n "${ssl[$x]}") && ("${ssl[$x]}" = "true") ]]; then
          component[${lc}]=${component[${lc}]}",-s|JVM_ARGS=-Djavax.net.ssl.trustStore=${M1_TESTROOT}/common_src/ibm.jks|-Djavax.net.ssl.trustStorePassword=password|-Djavax.net.ssl.KeyStore=${M1_TESTROOT}/common_src/ibm.jks|-Djavax.net.ssl.keyStorePassword=password"
        fi

        test_template_compare_string[${lc}]=${message[$x]}
        search[$mach]=1
      fi
      ((i++))
    done
    ((x++))
  done
  fi

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
