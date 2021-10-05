#!/bin/bash

# Start the trap daemon to capture traps from MessageSight
# The trapdaemon will run in the background through the entire ismAutoSNMP
# 
# 
#

#Get log file parameter
while getopts "f:" option ${OPTIONS}
  do
    case ${option} in
    f )
        LOG_FILE=${OPTARG}
        ;;
    esac	
done

#Initiate snmptrapd, run in background 
echo "lsof | grep snmptrapd | grep UDP" >> $LOG_FILE
reply=$(lsof | grep snmptrapd | grep UDP)
RC=$?
echo "reply=$reply" >> $LOG_FILE
echo "RC=$RC" >> $LOG_FILE
if [ $RC -ne 0 ] ; then
    exit 1
fi

set -- "$reply"
IFS=" "; declare -a Array=($*)
#echo ${Array[@]}  >> $LOG_FILE
echo "kill -9 ${Array[1]}" >> $LOG_FILE
kill -9 ${Array[1]}
RC=$?
echo "RC=$RC" >> $LOG_FILE

#Remove snmptraps.out file
#rm -rf snmptraps.out

echo "ssh ${A1_USER}@${A1_HOST} ps -ef | grep snmpd | grep -v grep | awk {print $2}"  >> $LOG_FILE
reply=`ssh ${A1_USER}@${A1_HOST} "ps -ef | grep snmpd | grep -v grep | awk '{print $2}'"`
RC=$?
echo "reply=$reply" >> $LOG_FILE
echo "RC=$RC" >> $LOG_FILE
if [ $RC -ne 0 ] ; then
    exit 1
fi

set -- "$reply"
IFS=" "; declare -a Array=($*)
echo ${Array[@]}  >> $LOG_FILE
echo "ssh ${A1_USER}@${A1_HOST} kill -9 ${Array[1]}" >> $LOG_FILE
ssh ${A1_USER}@${A1_HOST} "kill -9 ${Array[1]}"
RC=$?
echo "RC=$RC" >> $LOG_FILE
if [ $RC -ne 0 ] ; then
    exit 1
else
    echo -e "\nTest result is Success!" >> $LOG_FILE
fi
