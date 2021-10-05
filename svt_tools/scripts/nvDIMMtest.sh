#!/bin/sh
#
# Version[dd-mm-yyyy]:  06-02-2015
# Syntax:
#  nvDIMMtest.sh  #Loops  IMAServer_IPAddress -v | tee fn.log
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
#     The imaserver must be stopped and restarted after making that change.
#  Suggest you 'tee' the output to a log file
#
# Description:
#   Test the NVDIMMs with basic advanced_pd_options FILL, reboot, CHECK commands.
#   The pcitool nvcheck command is run before & after advanced_pd_options FILL/CHECK 
#
# Change History: 
#  add stderr with stdout on pcitool nvcheck cmd
#  add pcitool nveeprom to start of the loop  
#  REMOVED nveeprom since cmd is only in a DEV BUILD, need script to run on PROD BUILD
#  Change Syntax:  nvdimmtest is now _nvdimmtest  since it is a private command
#  Added Check for "advanced-pd-options _memorytype" and -v Verbose output per TSTL request to get all data all loops
#  Added pcitool stats for each dimm/each loop  (THIS Version is not run at CSC)
#----------------------------------------------------------------------------------------
#  status_nvdimm
#----------------------------------------------------------------------------------------
function status_nvdimm {
  echo "Running \"status nvdimm\" on $SERVER_IP...@ "`date`
  ssh admin@$SERVER_IP "status nvdimm" > $SERVER_IP.nvcheck 2>&1
  grep -q 'All nvDIMMs are ready.' $SERVER_IP.nvcheck
  if [[ $? != 0 ]]; then
    result="FAILED"
    cat $SERVER_IP.nvcheck
  fi
  if [[ ${VERBOSE} == "TRUE"  && ${result} == "PASSED" ]]; then
    cat $SERVER_IP.nvcheck
  fi

}

#----------------------------------------------------------------------------------------
#  reboot_system
#
# Currently fout ways to reboot the system
# - device restarts are warm reboot
# - ipmitool chassis power cycle are a cold reboot
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
      ssh admin@${SERVER_IP} device restart force
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
#      IP4="217"  This got fixed  :-) yay!
      IP4="212"
  fi
return ${IP4}
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
   echo "Assumes PASSWORDLESS SSH is configured between this client and the Appliance @ IP_ADDRESS"
   exit 99
fi

declare -i LOOPS=$1
declare SERVER_IP=$2
declare VERBOSE=FALSE

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

declare -i count=0
declare -i pcount=0
declare -i fcount=0

ping -c1 ${SERVER_IP} &> /dev/null
if [[ $? != 0 ]]; then
  echo "Cannot PING ${SERVER_IP}... exiting"
  exit 1
fi

# Check whether imaserver is writing to the nvdimms on startup.  This will
# cause the test to fail.  "MemoryType = 1"
echo "Checking whether IMAserver is set to use nvdimm storage."
ssh admin@${SERVER_IP} advanced-pd-options _memorytype > ${SERVER_IP}.memtype
grep -q MemoryType.=.0 ${SERVER_IP}.memtype
if [[ $? != 0 ]]; then
    echo "IMAserver is set to use nvdimm storage.  Expect fill/check data to be corrupted by IMA concurrent access."
    echo " THIS IS NOT THE EXPECTED ENVIRONMENT:  Please Read 'PreReqs:' section of this script!!!s"
    echo " IF YOU ARE SURE THIS IS WHAT YOU WANT TO DO: Press any key to continue or press CTRL+C to Cancel"
    read y
else
    echo "IMAserver is set to use shared storage.  Expect fill/check nvDIMM data to NOT be corrupted by IMA."
fi
cat ${SERVER_IP}.memtype

echo "Running on BUILD:  "
ssh admin@${SERVER_IP} "nodename get; show imaserver; show version ; show components"
echo " " 

while [ ${LOOPS} != $count ]
do 
  echo "####################################################################################"
  count=$(($count+1))
  result="PASSED"
  echo "Starting Loop $count...@ "`date`

  status_nvdimm;

  echo "Running \"advanced-pd-options _nvdimmtest fill\" on ${SERVER_IP}..."
  ssh admin@${SERVER_IP} advanced-pd-options _nvdimmtest fill > ${SERVER_IP}.fill
  for j in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
  do 
    grep -q Wrote.*$j.*GB ${SERVER_IP}.fill
    if [[ $? != 0 ]]; then
      result="FAILED"
      cat ${SERVER_IP}.fill
      break
    fi
  done  
  if [[ ${VERBOSE} == "TRUE"  && ${result} == "PASSED" ]]; then
      cat ${SERVER_IP}.fill
  fi
  
  status_nvdimm;

  reboot_system;

  echo "Sleep for 300 seconds to allow restart to complete on ${SERVER_IP}..."
  sleep 300
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

  status_nvdimm;

  echo "Running \"advanced-pd-options _nvdimmtest check\" on ${SERVER_IP}..."
  ssh admin@${SERVER_IP} advanced-pd-options _nvdimmtest check > ${SERVER_IP}.check
  for j in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
  do 
    grep -q Checked.*$j.*GB ${SERVER_IP}.check
    if [[ $? != 0 ]]; then
      result="FAILED"
      cat ${SERVER_IP}.check
      break
    fi
  done  
  if [[ ${VERBOSE} == "TRUE"  && ${result} == "PASSED" ]]; then
      cat ${SERVER_IP}.check
  fi


  if [[ ${VERBOSE} == "TRUE" ]]; then
      for k in 2 5 8 11
      do
          echo " "
          echo "STATS for nvDIMM slot ${k}"
          `echo "1" | ssh admin@${SERVER_IP} "advanced-pd-options _pcitool nvregs ${k}" > ${SERVER_IP}.nvregs.${k} `
          cat ${SERVER_IP}.nvregs.${k}
          `echo "1" | ssh admin@${SERVER_IP} "advanced-pd-options _pcitool smbhd ${k} 2" > ${SERVER_IP}.smbhd.${k}-2 `
          cat ${SERVER_IP}.smbhd.${k}-2
          `echo "1" | ssh admin@${SERVER_IP} "advanced-pd-options _pcitool smbhd ${k} 0xa" > ${SERVER_IP}.smbhd.${k}-0xa `
          cat ${SERVER_IP}.smbhd.${k}-0xa
          `echo "1" | ssh admin@${SERVER_IP} "advanced-pd-options _pcitool nveeprom ${k} " > ${SERVER_IP}.nveeprom.${k} `
          cat ${SERVER_IP}.nveeprom.${k} 
      done
  fi

  
  status_nvdimm;

  if [[ $result == "PASSED" ]]; then
    pcount=$(($pcount+1))
  else
    fcount=$(($fcount+1))
  fi
  echo "Loop $count of ${LOOPS} = $result (Total PASSED: $pcount FAILED: $fcount)"

done
