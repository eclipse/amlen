#!/bin/bash
curDir=$(dirname ${0})
if [ -e ${curDir}/ISMsetup.sh ]; then
   source ${curDir}/ISMsetup.sh
else
   source ${curDir}/scripts/ISMsetup.sh
fi
#set -x 
theCommand="${1}"

machine=1
while [ ${machine} -le ${M_COUNT} ]
do
  m_user="M${machine}_USER"
  m_userVal=$(eval echo \$${m_user})
  m_host="M${machine}_HOST"
  m_hostVal=$(eval echo \$${m_host})
  m_testroot="M${machine}_TESTROOT"
  m_testrootVal=$(eval echo \$${m_testroot})
  m_testcaseDirVal="${m_testrootVal}/${localTestcaseDir}"

  # Run on all machines 
  echo "~-~-~-~-~  ${m_userVal} @ ${m_hostVal}  CMD:  ${theCommand}  ~-~-~-~-~"
  echo " "
  ssh ${m_userVal}@${m_hostVal} "${theCommand} "
  echo " "
  machine=`expr ${machine} + 1`
done
