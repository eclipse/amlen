#! /bin/bash
#
# Inputs:  NONE
#
# Dependencies:
#  ISM Test Directory Structure:  ${TESTROOT}.. /scripts, /tc, /common_src
#  The Tag Replacement directory ${TESTROOT}/common will be created
#  NOTICE:
#  If any remote machine has a different number of files in /common_src
#  they will not get replaced if extra, or errors will be generate if missing.
#
# Return Code:
#   0 - All machine received have processed tag replacement successfully.
#   1 - One or more machines failed to complete tag replacement process.
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
function printUsage {
   echo " "
   echo "Usage:  ${THECMD}  "
   echo " "
   echo " "
}

#-------------------------------------------------------------------------------
function print_cmderror {
  echo " "
  echo "COMMAND FAILED:  \"$1\"  "
  echo "  Terminating Processing..."
  echo " "
  exit 1
}

#-------------------------------------------------------------------------------
function echodebug {
  if [ ${DEBUG} == "ON" ]
  then
    echo ${@}
  fi
}

#-------------------------------------------------------------------------------
function scpit {
  scp ${@} > /dev/null
  if [[ $? -ne 0 ]] ; then
    print_cmderror "scp ${@}"
  fi
}

#-------------------------------------------------------------------------------
function sshit {
  ssh ${@} > /dev/null  &
  if [[ $? -ne 0 ]] ; then
    print_cmderror "ssh ${@}"
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
baseName=$(basename ${0})
OUTFILE=${baseName%%.*}
#OUTFILE=`echo $(basename ${0}) | cut -d '.' -f 1`
FILE="${curDir}/ISMsetup.sh"
DEBUG=OFF
TIDYUP=ON

if [[ -f "../testEnv.sh"  ]]
then
  FILE="../testEnv.sh"
fi 

# Source the Test Env. Parameter File
source ${FILE}

#set -x
# Verify the output file can be written   
ERR=0
touch ${OUTFILE}
if [[ ! -w ${OUTFILE} ]]; then
   print_error "ERROR: Unable to write to log file ${OUTFILE}."
   echo "Error in Input: ${THECMD} $@ "
   printUsage
   exit 1
fi


#-------------------------------------------------------------------------------
# Iterate through the Input Test Env. parameter file 
# Build the SED replacement lists

sedStem="#!/bin/bash"
declare sedParams=""
sedCmdLinux="sed -i "
sedCmdApple="sed -i '' "
sedTail=" ../common/*"

# LAW: Added some comments, made some changes here to improve efficiency, and cleaned up a bit
# Build the sed replacement expressions from the setup file
while read line
do
  # If the line is not a comment, a blank line, or a THIS statement, then it must be processed.
  if [[  ! ${line} =~ ^[#].*  &&  ! ${line} == "" && ! ${line} == *THIS=* ]]
  then
      # remove "export"
      noexport=${line#* }
      # isolate the tag name
      tag=${noexport%%=*}
      #tag=`echo ${line} | cut -d "=" -f 1 | cut -d " " -f 2`   
      # isolate the value 
      tagValue=${noexport#*=}
      #tagValue=`echo ${line} | cut -d "=" -f 2-` 
      
      # If the tag we are replacing is IPv6 and the tagvalue is an IPv4 address, this special case is to remove the brackets around the tag as we do the subsitution
      # We do this replace first followed by the regular one below for the cases without square brackets	  
      if [[ ${tag} =~ "IPv6" && !( ${tagValue} =~ .*:.* ) ]] ; then
      # Use the default '/' as the sed delimeter, unless the tagValue contains a '/'. In that case, use a '|' delimeter for sed
      if [[ ! ${tagValue} =~ .*/.* ]]
      then
        sedParams="${sedParams} -e 's/\[${tag}\]/$tagValue/g' "
      else
        sedParams="${sedParams} -e 's|\[${tag}\]|$tagValue|g' "
      fi	  

      fi
	  
      # Use the default '/' as the sed delimeter, unless the tagValue contains a '/'. In that case, use a '|' delimeter for sed
      if [[ ! ${tagValue} =~ .*/.* ]]
      then
        sedParams="${sedParams} -e 's/${tag}/$tagValue/g' "
      else
        sedParams="${sedParams} -e 's|${tag}|$tagValue|g' "
      fi
  fi
done <${FILE}

echodebug Sed COMMAND SO FAR:  ${sedStem} ${sedParams} ${sedThis} ${sedTail} 

#--------------------------------------------------------------------------------------------------------
#  Build the THIS_* variables from the env sourcing of ${FILE}, conctruct a SED command for each machine
#  Then send SED command to each machine to execute.
#--------------------------------------------------------------------------------------------------------
# LAW: Added some comments, made some changes here to improve efficiency, and cleaned up a bit
i=1
while [ ${i} -le ${M_COUNT} ]
do
  # Establish the THIS replacement statements
  tagHead=M${i}
  sedThis=""
  ALLTAGS=` env | grep ${tagHead}`
  
  for aTag in ${ALLTAGS}
  do
    if [[  ! ${aTag} =~ THIS=.* ]]
    then
      # Isolate the tag
      tag=${aTag%%=*}
      # Remove the tagHead
      tagTail=${tag#*_}
      #tagTail=`echo ${aTag} | cut -d '=' -f 1 | cut -d "_" -f 2-`
      # Isolate the value
      tagValue=${aTag#*=}
      #tagValue=`echo ${aTag} | cut -d "=" -f 2-`
      # Use the default '/' as the sed delimeter, unless the tagValue contains a '/'. In that case, use a '|' delimeter for sed
      if [[ ! ${tagValue} =~ .*/.* ]]
      then
        sedThis="${sedThis}  -e 's/THIS_${tagTail}/${tagValue}/g'"
      else
        sedThis="${sedThis}  -e 's|THIS_${tagTail}|${tagValue}|g'"
      fi
    fi
  done
  # Construct the Sed command for the target machine
  #LAW: I don't think this line is necessary, it gets overwritten below #echo ${sedStem} ${sedParams} ${sedThis} ${sedTail} >  ${OUTFILE}.${tagHead}.sh

  # Send and Execute the Sed Command on the target machine
  m_user="${tagHead}_USER"
  m_userVal=$(eval echo \$${m_user})
  m_host="${tagHead}_HOST"
  m_hostVal=$(eval echo \$${m_host})
  m_tcroot="${tagHead}_TESTROOT"
  m_tcrootVal=$(eval echo \$${m_tcroot})
  m_testrootScriptVal="${m_tcrootVal}/scripts"

  # Construct the Sed command for the target machine
  echo ${sedStem}  >  ${OUTFILE}.${tagHead}.sh
  echo "if [[ ! -d "${m_tcrootVal}/common/" ]]; then mkdir -p ${m_tcrootVal}/common/; fi;" >>  ${OUTFILE}.${tagHead}.sh
  echo "cp ${m_tcrootVal}/common_src/*  ${m_tcrootVal}/common/; chmod +rw ${m_tcrootVal}/common/*;" >>  ${OUTFILE}.${tagHead}.sh

  echo 'if [[ `uname -a`  == *"Darwin"* ]]; then' >>  ${OUTFILE}.${tagHead}.sh
  echo "    " ${sedCmdApple} ${sedParams} ${sedThis} ${sedTail} >>  ${OUTFILE}.${tagHead}.sh
  echo "else" >>  ${OUTFILE}.${tagHead}.sh
  echo "    " ${sedCmdLinux} ${sedParams} ${sedThis} ${sedTail} >>  ${OUTFILE}.${tagHead}.sh
  echo "fi" >>  ${OUTFILE}.${tagHead}.sh

  echodebug `cat ${OUTFILE}.${tagHead}.sh`
  echodebug "scp ${OUTFILE}.${tagHead}.sh to ${tagHead} @${m_hostVal}"
  scpit ${OUTFILE}.${tagHead}.sh  ${m_userVal}@${m_hostVal}:${m_testrootScriptVal}/

  echodebug "ssh to execute ${OUTFILE}.${tagHead}.sh on ${tagHead} @${m_hostVal}"
  sshit ${m_userVal}@${m_hostVal} "chmod 777 ${m_testrootScriptVal}/${OUTFILE}.${tagHead}.sh; cd ${m_testrootScriptVal}; ${m_testrootScriptVal}/${OUTFILE}.${tagHead}.sh " 

i=$[${i}+1]
done

# Wait for the remote SSH's to complete
wait

# Locally and Remotely ERASE ALL  ${OUTFILE}.${tagHead}.sh  files
rm -f ${OUTFILE}
if [ ${TIDYUP} == "ON" ]
then
  i=1
  while  [ ${i} -le ${M_COUNT} ]
  do
    m_user="${tagHead}_USER"
    m_userVal=$(eval echo \$${m_user})
    m_host="${tagHead}_HOST"
    m_hostVal=$(eval echo \$${m_host})
    m_tcroot="${tagHead}_TESTROOT"
    m_tcrootVal=$(eval echo \$${m_tcroot})
    m_testrootScriptVal="${m_tcrootVal}/scripts"
    tagHead=M${i}

    rm -f ${OUTFILE}.${tagHead}.sh
    sshit ${m_userVal}@${m_hostVal} "rm -f ${m_testrootScriptVal}/${OUTFILE}.${tagHead}.sh " 

  i=$[${i}+1]
  done
fi
