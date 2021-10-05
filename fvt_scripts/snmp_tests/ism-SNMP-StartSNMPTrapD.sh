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

# Copy MessageSight Mib to server system, create .snmp/mibs path if necessary
echo "ssh ${A1_USER}@${A1_HOST} mkdir -p /root/.snmp/mibs" >> $LOG_FILE
ssh ${A1_USER}@${A1_HOST} mkdir -p /root/.snmp/mibs
RC=$?
echo "RC=$RC" >> $LOG_FILE
if [ $RC -ne 0 ] ; then
    exit 1
fi
echo "scp ./IBM-MESSAGESIGHT-MIB.txt ${A1_USER}@${A1_HOST}:/${A1_USER}/.snmp/mibs" >> $LOG_FILE
scp ./IBM-MESSAGESIGHT-MIB.txt ${A1_USER}@${A1_HOST}:/${A1_USER}/.snmp/mibs
RC=$?
echo "RC=$RC" >> $LOG_FILE
if [ $RC -ne 0 ] ; then
    exit 1
fi

# Configure snmpd.conf on host enivronment of server system
ssh_snmpd_conf=`echo "master			agentx 
agentaddress	tcp:161,udp:161,tcp6:161,udp6:161
agentXSocket	tcp:705,udp:705,tcp6:705,udp6:705
trap2sink	${M1_IPv4_1}	MessageSightInfo
rwcommunity	MessageSightInfo	${M1_IPv4_1}
rwcommunity6	MessageSightInfo	${M1_IPv4_1}
" | ssh ${A1_USER}@${A1_HOST} "cat > /${A1_USER}/.snmp/snmpd.conf"`

echo "We just ran a complicated ssh command to generate the /root/.snmp/snmpd.conf file" >> $LOG_FILE
echo "Now echo-ing the output of that command in case we fail." >> $LOG_FILE
echo "---------------------" >> $LOG_FILE
echo $ssh_snmpd_conf >> $LOG_FILE


# Start snmpd daemon on host environment of server system 
echo "ssh ${A1_USER}@${A1_HOST} snmpd -C -c /root/.snmp/snmpd.conf" >> $LOG_FILE 
ssh_snmpd=`ssh ${A1_USER}@${A1_HOST} "snmpd -C -c /root/.snmp/snmpd.conf"`
RC=$?
echo "Echoing ssh_snmpd variable which contains output of command to start snmpd." >> $LOG_FILE
echo "$ssh_snmpd" >> $LOG_FILE
echo "RC=$RC" >> $LOG_FILE
if [ $RC -ne 0 ] ; then
    exit 1
fi


#Initiate snmptrapd, run in background, output traps to snmptraps.out file
echo "snmptrapd -L f snmptraps.out -O q -c ./snmptrapd.conf -x tcp:${M1_IPv4_1}:705"  >> $LOG_FILE
snmptrapd -L f snmptraps.out -O q -c ./snmptrapd.conf -x tcp:${M1_IPv4_1}:705
RC=$?
echo "RC=$RC" >> $LOG_FILE
if [ $RC -ne 0 ] ; then
    exit 1
else
    echo -e "\nTest result is Success!" >> $LOG_FILE
fi
