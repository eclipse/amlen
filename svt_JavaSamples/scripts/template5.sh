#!/bin/bash

declare -li cIndex=0
declare -li mIndex=0
unset search
unset component

test_template5_init() {
  cIndex=0
  mIndex=0
  unset search
  unset component
}

test_template5_MQTTClientWait() {
    echo java svt.mqtt.mq.MqttSample $@
    mIndex=$((${mIndex} % ${M_COUNT} + 1))
    ((cIndex++))
    component[${cIndex}]="javaAppDriverWait,m${mIndex},-e|svt.mqtt.mq.MqttSample,-o"
    for word in $@; do 
      component[${cIndex}]=${component[${cIndex}]}"|${word}"
    done
    echo component[${cIndex}]
}
test_template5_MQTTClient() {
    echo java svt.mqtt.mq.MqttSample $@
    mIndex=$((${mIndex} % ${M_COUNT} + 1))
    ((cIndex++))
    component[${cIndex}]="javaAppDriver,m${mIndex},-e|svt.mqtt.mq.MqttSample,-o"
    for word in $@; do 
      component[${cIndex}]=${component[${cIndex}]}"|${word}"
    done
    echo component[${cIndex}]
}

test_template5_JMSClientWait() {
    echo java svt.jms.ism.JMSSample $@
    mIndex=$((${mIndex} % ${M_COUNT} + 1))
    ((cIndex++))
    component[${cIndex}]="javaAppDriverWait,m${mIndex},-e|svt.jms.ism.JMSSample,-o"
    for word in $@; do 
      component[${cIndex}]=${component[${cIndex}]}"|${word}"
    done
    echo component[${cIndex}]
}

test_template5_JMSClient() {
    echo java svt.jms.ism.JMSSample $@
    mIndex=$((${mIndex} % ${M_COUNT} + 1))
    ((cIndex++))
    component[${cIndex}]="javaAppDriver,m${mIndex},-e|svt.jms.ism.JMSSample,-o"
    for word in $@; do 
      component[${cIndex}]=${component[${cIndex}]}"|${word}"
    done
    echo component[${cIndex}]
}

test_template5_sleep() {
   ((cIndex++))
   component[${cIndex}]="sleep,$1"
    echo component[${cIndex}]
}

test_template5_SearchString() {
   test_template_compare_string[${cIndex}]=$*
   search[${mIndex}]=1
}

test_template5_FinalizeSearch() {
  for i in ${!search[@]}; do
    ((cIndex++))
    component[${cIndex}]=`test_template_format_search_component m$i`
    echo component[${cIndex}]
  done
}


test_template5_Failover() {
  ((cIndex++))
#  component[${cIndex}]="cAppDriverWait,m1,-e|/niagara/test/scripts/af_test.start.workload.sh,-o|haFunctions.sh|stopPrimary|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=jmqtt_04_01|SVT_AT_VARIATION_QUICK_EXIT=true"
#  echo component[${cIndex}]
#  ((cIndex++))
#  component[${cIndex}]="cAppDriverWait,m1,-e|/niagara/test/scripts/af_test.start.workload.sh,-o|haFunctions.sh|startStandyby|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=jmqtt_04_01|SVT_AT_VARIATION_QUICK_EXIT=true"
#  echo component[${cIndex}]
  component[${cIndex}]="cAppDriverWait,m1,-e|${M1_TESTROOT}/scripts/haFunctions.py,-o|-a|restartPrimary|-t|600,"
  echo component[${cIndex}]
}

test_template5_BashScriptWait() {
  ((cIndex++))
  component[${cIndex}]="cAppDriverWait,m1,-e|$1,"

  shift 1
  if [[ "$@" != "" ]]; then
    component[${cIndex}]=${component[${cIndex}]}"-o"
    for word in $@; do 
     component[${cIndex}]=${component[${cIndex}]}"|${word}"
    done
    component[${cIndex}]=${component[${cIndex}]}","
  fi 
  echo component[${cIndex}]
}

test_template5_BashScriptEnd() {
  ((cIndex++))
  component[${cIndex}]="cAppDriverLogEnd,m1,-e|$1,"

  shift 1
  if [[ "$@" != "" ]]; then
    component[${cIndex}]=${component[${cIndex}]}"-o"
    for word in $@; do 
     component[${cIndex}]=${component[${cIndex}]}"|${word}"
    done
    component[${cIndex}]=${component[${cIndex}]}","
  fi 
  echo component[${cIndex}]
}

test_template5_ShowSummary() {
  echo
  echo ================================================================================================
  echo
  echo xml[${n}]=\"${xml[${n}]}\" 
  echo test_template_initialize_test \"${xml[${n}]}\"  
  echo scenario[${n}]=\"${scenario[${n}]}\" 
  echo timeouts[${n}]=${timeouts[${n}]}

  echo 
  for i in ${!component[@]}; do
    echo component[$i]=\"${component[$i]}\" 
  done

  echo 
  for i in ${!test_template_compare_string[@]}; do
    echo test_template_compare_string[${i}]=\"${test_template_compare_string[${i}]}\" 
  done

  echo 
  for i in ${!test_template_metrics_v1[@]}; do
    echo test_template_metrics_v1[${i}]=\"${test_template_metrics_v1[${i}]}\" 
  done

  echo 
  echo test_template_runorder=\"${test_template_runorder}\" 
  echo test_template_finalize_test 
  echo
  echo ================================================================================================
  echo

  unset component
  unset test_template_compare_string
  unset test_template_metrics_v1
  unset search
}


