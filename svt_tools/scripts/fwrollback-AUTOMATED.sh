#!/bin/sh
#
#
# Syntax:
#  $0  #Loops  IMAServer_IPAddress -v | tee fn.log
#      where,
#    #Loops  :  is the number of loop to run the fill-check 
#    IMAServer_IPAddress  :  IP address of the IMA Server appliance being tested
#    -v  :  Verbose Output, by default nvcheck, fill and check output is only saved on Failures.  
#           Verbose will save the output on those commands with every execution 
#
# PreReqs:
#   Passwordless SSH setup bewtween the IMAServer and the client running this script.
#   IMA now senses if nvDIMMs are present and will set a flag to use them.   This will
#   interfere with the 'pcitools fill-check test'.   You must disable the flag with the command.
#       imaserver _memorytype "Type=0"
#     The imaserver must be stopped and restarted after making that change.  This is now automated.
#  Suggest you 'tee' the output to a log file
#  ALSO Check the SYNTAX HELP for additional assumptions
#
# Description:
#   Test the NVDIMMs with basic advanced_pd_options FILL, reboot, CHECK commands.
#   The pcitool nvcheck command is run before & after advanced_pd_options FILL/CHECK 
#   The mlnx check is run after every reboot.
#
# Return Codes:
#  1 - Initial Ping failure, IMAServer fails to start initally. MLNX or nvDIMM failure in 1st loop
#  2 - Syntax Error or Subsequent MLNX or nvDIMM failure in 2nd loop
# >2 - MLNX or nvDIMM failure in that loop number
#
# Output Files:  Strongly suggest you tee the output of this file to get complete result history:  see Syntax.
#  {SERVER.IP}.check   - results of the last adv-pd-opt _nvdimmtest check (run each loop)
#  {SERVER.IP}.fill    - results of the last adv-pd-opt _nvdimmtest fill (run each loop)
#  {SERVER.IP}.nvcheck - results of the last status nvdimm command (run multiple times per loop)
#
# Change History: 
#  add stderr with stdout on pcitool nvcheck cmd
#  add pcitool nveeprom to start of the loop  
#  REMOVED nveeprom since cmd is only in a DEV BUILD, need script to run on PROD BUILD
#  Change Syntax:  nvdimmtest is now _nvdimmtest  since it is a private command
#  Added manual Check for "advanced-pd-options _memorytype" and -v Verbose output per TSTL request to get all data all loops
#  Change manual _memorytype check to automatically reset it and restart imaserver , reset it back at the end.
#

#----------------------------------------------------------------------------------------
#  firmware_upgrade
#----------------------------------------------------------------------------------------
function firmware_upgrade {
  echo "Running \"firmware upgrade ${BUILD} \" on ${SERVER_IP}...@ "`date`
  ssh admin@${SERVER_IP} "imaserver set AcceptLicense" 
  ssh admin@${SERVER_IP} "firmware upgrade ${BUILD}" >> ${SERVER_IP}.fwup 2>&1

}

#----------------------------------------------------------------------------------------
#  firmware_rollback
#----------------------------------------------------------------------------------------
function firmware_rollback {
  echo "Running \"firmware rollback ${BUILD_1} \" on ${SERVER_IP}...@ "`date`
  ssh admin@${SERVER_IP} "imaserver set AcceptLicense" 
  ssh admin@${SERVER_IP} "firmware rollback " >> ${SERVER_IP}.fwup 2>&1

}

#----------------------------------------------------------------------------------------
#  check_fw_versions
#----------------------------------------------------------------------------------------
function check_fw_versions {
  echo "Checking \"firmware versions \" on ${SERVER_IP}...@ "`date`
  ssh admin@${SERVER_IP} "imaserver set AcceptLicense" 
  echo `ssh admin@${SERVER_IP} "show imaserver " `
  echo `ssh admin@${SERVER_IP} "show components " `
  status_nvdimm;

}


#----------------------------------------------------------------------------------------
#  ping2success
#----------------------------------------------------------------------------------------
function ping2success {
 
  SLEEPTIME=5400
#debug:  SLEEPTIME=3

  echo "Sleep for ${SLEEPTIME} seconds to allow restart to complete on ${SERVER_IP}..."
  sleep ${SLEEPTIME}
  echo "Pinging ${SERVER_IP} until it responds (takes up to 10 mins)...@ "`date`
  while [ 1 ]
  do
  ping -c1 ${SERVER_IP} &> /dev/null
  if [[ $? != 0 ]]; then
    sleep 30
  else
    break
  fi
  done

}
#----------------------------------------------------------------------------------------
#  status_nvdimm
#----------------------------------------------------------------------------------------
function status_nvdimm {
  echo "Running \"status nvdimm\" on ${SERVER_IP}...@ "`date`
  ssh admin@${SERVER_IP} "status nvdimm" > ${SERVER_IP}.nvcheck 2>&1
  grep -q 'All nvDIMMs are ready.' ${SERVER_IP}.nvcheck
  if [[ $? != 0 ]]; then
    result="FAILED"
    cat ${SERVER_IP}.nvcheck
  fi
  if [[ ${VERBOSE} == "TRUE"  && ${result} == "PASSED" ]]; then
    cat ${SERVER_IP}.nvcheck
  fi

}

#----------------------------------------------------------------------------------------
#  start_imaserver
#
# If IMAServer is not started initially, these commands are not handled appropriately:
# - status imaserver, 
# - imaserver harole, 
# - adv-pd-opt _memorytype 
#
#----------------------------------------------------------------------------------------
function start_imaserver {
#set -x
   echo "Running \"status imaserver\" on ${SERVER_IP}...@ "`date`
   IMASTATUS=`ssh admin@${SERVER_IP} "status imaserver"` 
   if [[ ${IMASTATUS} == *'Status = Running (m'*  || "${IMASTATUS}" == *'Status = Standby'* ]]; then
      restart_imaserver;

      IMASTATUS=`ssh admin@${SERVER_IP} "status imaserver" | head -n 1` 
      if [[ "${IMASTATUS}" == *'Status = Running (m'* || "${IMASTATUS}" == *'Status = Standby'* ]]; then
         result="FAILED"
         echo "IMAServer failed to start : "${IMASTATUS}
         exit 1
      fi
   fi
set +x
   if [[ ${VERBOSE} == "TRUE"  && ${result} == "PASSED" ]]; then
      echo ${IMASTATUS}
   fi

}

#----------------------------------------------------------------------------------------
#  restart_imaserver
#
#  Caveat:  If HAROLE is enabled, IMAServer may fail to restart?
#----------------------------------------------------------------------------------------
function restart_imaserver {
   echo "restarting imaserver... "

   out=`ssh admin@${SERVER_IP} "imaserver stop; status imaserver" | head -n 1`
   printf "...stopping..."
   while [ ! "${out}" == "Status = Stopped" ]; do
     sleep 5s
     printf "."
     out=`ssh admin@${SERVER_IP} status imaserver`
   done
   echo "stopped "

   out=`ssh admin@${SERVER_IP} "imaserver start; status imaserver" | head -n 1`
   printf "...starting..."
   while [[ "${out}" != 'Status = Running'* &&  "${out}" != 'Status = Standby'* ]]; do
     sleep 5s
     printf "."
     out=`ssh admin@${SERVER_IP} "status imaserver" | head -n 1`
   done
   echo "started"

}

#----------------------------------------------------------------------------------------
#  reboot_system
#
# Currently two ways to reboot the system
# - device restart is warm reboot
# - ipmitool chassis power cycle is a cold reboot
#----------------------------------------------------------------------------------------
function reboot_system  {

# AS MORE Reboot options are added in the IF below, the "${count} % #" needs to be incremented to match
   remainder=$(( ${count} % 4 ))
#echo "TEMPORARY WORK AROUND - FORCE REBOOT OPTION to Always be:  ipmitool chassiz power OFF/ON"
#remainder=4 

   if [ $remainder -eq 0 ]; then
      echo "Running \"device restart\" on ${SERVER_IP}..."
      ssh admin@${SERVER_IP} device restart
   elif [ $remainder -eq 1 ]; then
      echo "Running \"device restart force\" on ${SERVER_IP}..."
      ssh -o ServerAliveInterval=30 admin@${SERVER_IP} device restart force
   elif [ $remainder -eq 2 ]; then
      # sleep just a bit so 'status nvdimm' can erase the lockfile
      sleep 2
      echo "Running \"ipmitool chassis power cycle\" on ${SERVER_IP}..."
      ipmitool -I lanplus -H ${SERVER_IMM_IP} -U USERID -P PASSW0RD chassis power cycle
   else
      # sleep just a bit so 'status nvdimm' can erase the lockfile
      sleep 2
      echo "Running \"ipmitool chassis power off - power on\" on ${SERVER_IP}..."
      ipmitool -I lanplus -H ${SERVER_IMM_IP} -U USERID -P PASSW0RD chassis power off
      # Wait for IMM to restart
      sleep 10
      ipmitool -I lanplus -H ${SERVER_IMM_IP} -U USERID -P PASSW0RD chassis power on
   fi

}

#----------------------------------------------------------------------------------------
#  get_imm_ip_address
#
# Unfortunately, 
#  we have a few system where 9.177/9.179 address and 9.174 address do not match
#----------------------------------------------------------------------------------------
function get_imm_ip_address {

  IP4=`echo ${SERVER_IP} | cut -d . -f 4`
  if [ ${IP4} == 212 ]; then
#      IP4="217"   fixed now...
      IP4="212"
  fi
return ${IP4}
}


#----------------------------------------------------------------------------------------
#  check_mlnx
#
#----------------------------------------------------------------------------------------
function check_mlnx {
  ETH6_STATE=`ssh admin@${SERVER_IP} "show ethernet-interface eth6"`
  ETH7_STATE=`ssh admin@${SERVER_IP} "show ethernet-interface eth7"`
  HA0_STATE=`ssh admin@${SERVER_IP} "show ethernet-interface ha0"`
  HA1_STATE=`ssh admin@${SERVER_IP} "show ethernet-interface ha1"`

  if [[ "${ETH6_STATE}" != *'name "eth6"'* ]];then
     echo "THERE IS AN ERROR WITH eth6: " ${ETH6_STATE}
     exit 1
  fi
  if [[ "${ETH7_STATE}" != *'name "eth7"'* ]];then
     echo "THERE IS AN ERROR WITH eth7: " ${ETH7_STATE}
     exit 1
  fi
  if [[ "${HA0_STATE}" != *'name "ha0"'* ]];then
     echo "THERE IS AN ERROR WITH ha0: " ${HA0_STATE}
     exit 1
  fi
  if [[ "${HA1_STATE}" != *'name "ha1"'* ]];then
     echo "THERE IS AN ERROR WITH ha1: " ${HA1_STATE}
     exit 1
  fi

  return
}


#----------------------------------------------------------------------------------------
#  show_version
# Set VERSION_TYPE so you know how to get to busybox/enableshell
#----------------------------------------------------------------------------------------
function show_version {
  echo "Running \"show version\" on ${SERVER_IP}...@ "`date`
  SHOW_VERSION=`ssh admin@${SERVER_IP} "show version"`
  echo ${SHOW_VERSION}
  if [[ "${SHOW_VERSION}" == *'Firmware type: Development'* ]];then
    VERSION_TYPE="Development"
  fi

}

#----------------------------------------------------------------------------------------
#  check_nveeprom
#  Retrieve the HFPOOL value from each nvDIMM to test for the NAN Problem where chips go bad.  
#  Expected GOOD VALUES are 95 or higher.
#  Expects VERSION_TYPE set so you konw how to get to busybox/enableshell
#----------------------------------------------------------------------------------------
function check_nveeprom {
  if [[ "${VERSION_TYPE}" == "Release" ]]; then
    echo "pcitool nveeprom 2"
    echo "pcitool nveeprom 2  | grep NFPOOL" | ssh admin@${SERVER_IP} "advanced-pd-options _enableshell"
    echo "pcitool nveeprom 5"
    echo "pcitool nveeprom 5  | grep NFPOOL" | ssh admin@${SERVER_IP} "advanced-pd-options _enableshell"
    echo "pcitool nveeprom 8"
    echo "pcitool nveeprom 8  | grep NFPOOL" | ssh admin@${SERVER_IP} "advanced-pd-options _enableshell"
    echo "pcitool nveeprom 11"
    echo "pcitool nveeprom 11 | grep NFPOOL" | ssh admin@${SERVER_IP} "advanced-pd-options _enableshell"

  else
    echo "pcitool nveeprom 2"
    echo "pcitool nveeprom 2  | grep NFPOOL" | ssh admin@${SERVER_IP} busybox
    echo "pcitool nveeprom 5"
    echo "pcitool nveeprom 5  | grep NFPOOL" | ssh admin@${SERVER_IP} busybox
    echo "pcitool nveeprom 8"
    echo "pcitool nveeprom 8  | grep NFPOOL" | ssh admin@${SERVER_IP} busybox
    echo "pcitool nveeprom 11"
    echo "pcitool nveeprom 11 | grep NFPOOL" | ssh admin@${SERVER_IP} busybox
  fi

}


#----------------------------------------------------------------------------------------
#  Start of Main Routine
#----------------------------------------------------------------------------------------
#set -x

if [[ $# -lt 2 ]] || [[ $# -gt 3 ]]; then
   echo "SYNTAX Help:  "
   echo "  $0  LOOPS  IP_ADDRESS  [-v]"
   echo "where "
   echo " LOOPS      (required)  Number of FILL/CHECK tests desired"
   echo " IP_ADDRESS (required)  Target 9.net IPv4 Address of the Appliance"
   echo " -v         (optional)  VERBOSE output - puts ALL data from ALL loops into ONE (big) file "
   echo ""
   echo "Assumptions:"
   echo " - PASSWORDLESS SSH is configured between this client and the Appliance @ IP_ADDRESS"
   echo " - IMM IP Address is 9.3.174.*, if IP is 9.3.177.*, or 9.3.176.*, if IP is 9.3.179.*"
   echo " - "
   echo " - "
   exit 2
fi

declare BUILD_1="http://10.10.10.10/release/IMA110/production/20131120-1705-GM-Client-1.1/appliance/rel_bedrock.scrypt2"
declare BUILD_2="http://10.10.10.10/release/IMA110-IFIX/production/20140507-1433/appliance/rel_bedrock.scrypt2"
# set to BUILD_2 cause it will be changed to BUILD_1 on first loop
declare BUILD="${BUILD_2}"

declare -i LOOPS=$1
declare SERVER_IP=$2
declare VERBOSE=FALSE
declare HAROLE=HADISABLED
declare MEMORY=SHARED
declare VERSION_TYPE="Release"
show_version

if [[ $# -eq 3 ]]; then
   if [[ $3 == "-"[vV] ]]; then
      VERBOSE=TRUE
   else
      echo "Value for VERBOSE: '$3' is invalid and ignored. Will use VERBOSE=${VERBOSE}" 
   fi
fi

declare IP4="1"
get_imm_ip_address;
declare SERVER_IMM_IP="9.3.174.${IP4}"

echo `date` > ${SERVER_IP}.fwup
echo "BUILD_1=$BUILD_1}" >> ${SERVER_IP}.fwup
echo "BUILD_2=$BUILD_2}" >> ${SERVER_IP}.fwup

# Counts for LOOPS, Pass and Fail
declare -i count=0
declare -i pcount=0
declare -i fcount=0

ping -c1 ${SERVER_IP} &> /dev/null
if [[ $? != 0 ]]; then
  echo "Cannot PING ${SERVER_IP}... exiting"
  exit 1
fi

start_imaserver;

# Set imaserver to NOT write to the nvdimms, use pcitool cmds to write to nvdimms and don't want collisions
#   "MemoryType Type=0" will make IMAServer used shared memory, freeing nvDIMMs for this test.
echo "Checking HAROLE and whether IMAserver is set to use nvdimm storage Type=1 or shared storage Type=0."

HASTATE=`ssh admin@${SERVER_IP} "imaserver harole"`
echo "HA State: " $HASTATE
if [[ ${HASTATE} != *"Role = HADISABLED"* ]]; then
   HAROLE=ENABLED
   echo "HAROLE was ENABLED, need to reset HAROLE on exit"
   ssh admin@${SERVER_IP} "imaserver update HighAvailability EnableHA=false"
fi

# If HASTATE is STANDBY, can not change MemoryType - requires IMAServer restart to get EnableHA=false active.
if [[ ${HASTATE} == *"Role = STANDBY"* ]]; then
   restart_imaserver;
fi

MEMORYTYPE=`ssh admin@${SERVER_IP} "advanced-pd-options _memorytype" `
echo "MemoryType = " $MEMORYTYPE
# if [[ ${MEMORYTYPE} != *"MemoryType = 0"* ]]; then
#    MEMORY=NVDIMM
#    echo "IMAserver is set to use nvdimm storage. Reseting _memoryType Type=0"
#    ssh admin@${SERVER_IP} "advanced-pd-options _memorytype Type=0"
# fi

# Change to MemoryType or HAROLE require IMAServer restart to take effect.
if [ ${MEMORY} == "NVDIMM" -o ${HAROLE} == "ENABLED" ]; then
   restart_imaserver;
fi

echo "Running on BUILD:  "
ssh admin@${SERVER_IP} "nodename get; show imaserver; show version ; show components"
echo " " 

echo "--> Install BUILD_1 in partition 1:  ${BUILD_1}"
BUILD=${BUILD_1}
firmware_upgrade;
ping2success;
check_fw_versions;

#echo "--> Install BUILD_1 in partition 2:  ${BUILD_1}"
#BUILD=${BUILD_1}
#firmware_upgrade;
#ping2success;
#check_fw_versions;

BUILD=${BUILD_2}
while [ ${LOOPS} != $count ]
do 
  echo "####################################################################################"
  count=$(($count+1))
  result="PASSED"
  echo "Starting Loop $count...@ "`date`

  status_nvdimm;
  check_mlnx;
  check_nveeprom;

  firmware_upgrade;
  ping2success;
  check_fw_versions;

  firmware_rollback;
  ping2success;
  check_fw_versions;

  if [[ $result == "PASSED" ]]; then
    pcount=$(($pcount+1))
  else
    fcount=$(($fcount+1))
  fi
  echo "Loop $count of ${LOOPS} = $result (Total PASSED: $pcount - FAILED: $fcount)"
  
done
#--------------------------------------------------------------------------------------
echo " "
echo "...Post Test Cleanup..."
if [[ ${HAROLE} == "ENABLED" ]]; then
   echo "HAROLE was ENABLED, reseting HAROLE to Enabled on exit"
   ssh admin@${SERVER_IP} "imaserver update HighAvailability EnableHA=true"
fi

if [[ ${MEMORY} == "NVDIMM" ]]; then
   echo "Reset IMAserver to use nvdimm storage.  Reseting _memoryType Type=1 and Restarting IMAServer"
   ssh admin@${SERVER_IP} "advanced-pd-options _memorytype Type=1"
fi

if [ ${MEMORY} == "NVDIMM" -o ${HAROLE} == "ENABLED" ]; then
   restart_imaserver;
fi
exit $fcount
