#!/bin/bash

#-------------------------------------------------------------------------------
function print_cmderror {
  echo " "
  echo "COMMAND FAILED:  \"${@}\"  "
  echo "  Terminating Processing..."
  echo " "
  exit 1
}

#-------------------------------------------------------------------------------
function sshit {
  cmdHead=`echo ${@} | cut -d " " -f 1`
  cmdTail=`echo ${@} | cut -d " " -f 2-`

  ssh ${cmdHead} "${cmdTail}" > /dev/null  &
  if [[ $? -ne 0 ]] ; then
    print_cmderror "ssh ${cmdHead} \"${cmdTail}\""
  fi
}


#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# B E G I N   M A I N   P A T H 
#-------------------------------------------------------------------------------
# Set some variables needed for the script
# set -x
THECMD=$0
curDir=$(dirname ${0})

FILE="${curDir}/ISMsetup.sh"

if [[ -f "../testEnv.sh"  ]]
then
  FILE="../testEnv.sh"
fi 

# Source the Test Env. Parameter File
set +x
source ${FILE}


i=1
while  [ ${i} -le ${M_COUNT} ]
do
    tagHead=M${i}
    m_user="${tagHead}_USER"
    m_userVal=$(eval echo \$${m_user})
    m_host="${tagHead}_HOST"
    m_hostVal=$(eval echo \$${m_host})
    m_tcroot="${tagHead}_TESTROOT"
    m_tcrootVal=$(eval echo \$${m_tcroot})
    m_testrootScriptVal="${m_tcrootVal}/scripts"
    tagHead=M${i}

    sshit ${m_userVal}@${m_hostVal} "cp -fr ${m_tcrootVal}/common_src/*  ${m_tcrootVal}/common/ " 

i=$[${i}+1]
done
