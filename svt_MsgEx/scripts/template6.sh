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

declare -li cIndex=0
declare -li mIndex=0
unset search
unset component

test_template6() {
  echo $@

  if [[ "$1" == "m?" ]]; then
    mIndex=$((${mIndex} % ${M_COUNT} + 1))
    shift 1
  elif [[ $1 == [m]* ]]; then
    mIndex=${1/m/}
    shift 1
  fi

  if [ "$1" == "init" ]; then
    cIndex=0
    mIndex=0
    unset component
    unset test_template_compare_string
    unset test_template_metrics_v1
    unset test_template_runorder
    unset search

  elif [ "$1" == "sleep" ]; then
    shift 1
    ((cIndex++))
    component[${cIndex}]="sleep,$1"
    echo component[${cIndex}]

  elif [ "$1" == "SearchString" ]; then
    shift 1
    test_template_compare_string[${cIndex}]=$*
    search[${mIndex}]=1

  elif [ "$1" == "FinalizeSearch" ]; then
    shift 1
    for i in ${!search[@]}; do
      ((cIndex++))
      component[${cIndex}]=`test_template_format_search_component m$i`
      echo component[${cIndex}]
    done

  elif [ "$1" == "ShowSummary" ]; then
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
    echo test_template_finalize_test
    echo
    echo ================================================================================================
    echo

  elif [ "$1" == "failover" ]; then
    ((cIndex++))
    component[${cIndex}]=cAppDriverLogWait,m1,"-e|../scripts/haFunctions.py,-o|-a|restartPrimary|-t|300"
    echo component[${cIndex}]

#   component[${cIndex}]="cAppDriverWait,m1,-e|/niagara/test/scripts/af_test.start.workload.sh,-o|haFunctions.sh|stopPrimary|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=${xml[${n}]}|SVT_AT_VARIATION_QUICK_EXIT=true"
#   echo component[${cIndex}]
#   ((cIndex++))
#   component[${cIndex}]="cAppDriverWait,m1,-e|/niagara/test/scripts/af_test.start.workload.sh,-o|haFunctions.sh|startStandby|1|${A1_HOST}|${A2_HOST},-s|AF_AUTOMATION_TEST=${xml[${n}]}|SVT_AT_VARIATION_QUICK_EXIT=true"
#   echo component[${cIndex}]
  elif [ "$1" == "javaAppDriver" ]; then
    shift 1
    ((cIndex++))
    component[${cIndex}]="javaAppDriver,m${mIndex},-e|$1,"
    shift 1
    if [[ "$@" != "" ]]; then
      component[${cIndex}]=${component[${cIndex}]}"-o"
      for word in $@; do
        component[${cIndex}]=${component[${cIndex}]}"|${word}"
      done
      component[${cIndex}]=${component[${cIndex}]}","
    fi
    echo component[${cIndex}]

  elif [[ "$1" == "javaAppDriverWait" ]]; then
    shift 1
    ((cIndex++))
    component[${cIndex}]="javaAppDriverWait,m${mIndex},-e|$1,"
    shift 1
    if [[ "$@" != "" ]]; then
      component[${cIndex}]=${component[${cIndex}]}"-o"
      for word in $@; do
        component[${cIndex}]=${component[${cIndex}]}"|${word}"
      done
      component[${cIndex}]=${component[${cIndex}]}","
    fi
    echo component[${cIndex}]

  elif [[ "$1" == "cAppDriverLogEnd" ]]; then
    shift 1
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
  elif [[ "$1" == "cAppDriverWait" ]]; then
    shift 1
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
  elif [[ "$1" == "cAppDriverLogWait" ]]; then
    shift 1
    ((cIndex++))
    component[${cIndex}]="cAppDriverLogWait,m1,-e|$1,"
    shift 1

    if [[ "$@" != "" ]]; then
      component[${cIndex}]=${component[${cIndex}]}"-o"
      for word in $@; do
       component[${cIndex}]=${component[${cIndex}]}"|${word}"
      done
      component[${cIndex}]=${component[${cIndex}]}","
    fi
    echo component[${cIndex}]
  fi
}

