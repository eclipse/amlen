#!/bin/bash
COM="MessageSightInfo"
#echo Running set commands
command=$(snmpset -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 10 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStoreDiskUsageWarningThreshold.0 i 70)
echo $command

command=$(snmpset -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 10 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStoreDiskUsageAlertThreshold.0 i 80)
echo $command

command=$(snmpset -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 10 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaMemoryUsageAlertThreshold.0 i 85)
echo $command

command=$(snmpset -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 10 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStorePool1MemoryLowAlertThreshold.0 i 85)
echo $command

command=$(snmpset -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 10 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaMemoryUsageWarningEnable.0 i 1)
echo $command

command=$(snmpset -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 10 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaMemoryUsageAlertEnable.0 i 1)
echo $command

command=$(snmpset -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 10 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStoreDiskUsageAlertEnable.0 i 1)
echo $command

command=$(snmpset -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 10 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStoreDiskUsageAlertEnable.0 i 1)
echo $command

command=$(snmpset -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 10 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStorePool1MemoryLowAlertEnable.0 i 1)
echo $command

#SNMPGET - could add SNMPGET commands before and after to verify commands have not actually changed.
#Currently believed to not be necessary 
#command=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 10 -cpublic ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStoreDiskUsageWarningThreshold.0)
#echo $command

