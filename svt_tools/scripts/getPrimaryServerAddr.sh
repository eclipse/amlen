#!/bin/bash

if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  if [[ -f "../scripts/ISMsetup.sh"  ]]
  then
    source ../scripts/ISMsetup.sh
  else
    source /niagara/test/scripts/ISMsetup.sh
  fi
fi

run() {
  local one
  local i
  local t
  local t2
  local line
  local list
  local list2

  if [ ! -z $1 ] ; then
    one=$1
  else 
    one=10
  fi
  
  t=`echo ${A1_HOST} | grep -e "^$one"`
  if [ -z ${t} ]; then
    t=`echo ${A1_IPv4_1} | grep -e "^$one"`
  fi
  if [ -z ${t} ]; then
    t=`echo ${A1_IPv4_2} | grep -e "^$one"`
  fi
  if [ -z ${t} ]; then
    t=${A1_HOST}
  fi
  
  if [ -z ${A2_HOST} ]; then
    export svt_server=${t}
  else
    i=0;for line in `ssh admin@${A1_HOST} status imaserver`;do list[$i]=${line};((i++)); done
  
    if [[ ! "${list[*]}" =~ "HARole" ]]; then
      export svt_server=${t}
    else
      if [[ "${list[*]}" =~ "PRIMARY" ]]; then
        export svt_server=${t}
      else
        t2=`echo ${A2_HOST} | grep -e "^$one"`
        if [ -z ${t2} ]; then
          t2=`echo ${A2_IPv4_1} | grep -e "^$one"`
        fi
        if [ -z ${t2} ]; then
          t2=`echo ${A2_IPv4_2} | grep -e "^$one"`
        fi
        if [ -z ${t2} ]; then
          t2=${A2_HOST}
        fi
        i=0; for line in `ssh admin@${A2_HOST} status imaserver`; do list2[$i]=${line}; ((i++)); done
        if [[ "${list2[*]}" =~ "PRIMARY" ]]; then
          export svt_server=${t2}
        else
          export svt_server=${t}
        fi
      fi
    fi
  fi
}

run $@
printf "${svt_server}\n"
