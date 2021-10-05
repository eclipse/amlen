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
  local i
  local line
  local list
  local list2

  if [ -z ${A2_HOST} ]; then
    export svt_server=${A1_HOST}
  else

    if [[ "${A1_TYPE}" == "DOCKER" ]]; then
      list=`ssh ${A1_USER}@${A1_HOST} docker exec IMA msshell status imaserver`
    else   
      i=0;for line in `ssh admin@${A1_HOST} status imaserver`;do list[$i]=${line};((i++)); done
    fi
  
    if [[ ! "${list[*]}" =~ "HARole" ]]; then
      export svt_server=${A1_HOST}
    else
      if [[ "${list[*]}" =~ "PRIMARY" ]]; then
        export svt_server=${A1_HOST}
      else
        if [[ "${A2_TYPE}" == "DOCKER" ]]; then
          list2=`ssh ${A2_USER}@${A2_HOST} docker exec IMA msshell status imaserver`
        else
          i=0; for line in `ssh admin@${A2_HOST} status imaserver`; do list2[$i]=${line}; ((i++)); done
        fi
        if [[ "${list2[*]}" =~ "PRIMARY" ]]; then
          export svt_server=${A2_HOST}
        else
          export svt_server=${A1_HOST}
        fi
      fi
    fi
  fi
}

run
printf "${svt_server}\n"
