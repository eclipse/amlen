#!/bin/bash

# This script compares the Store Statistics returned by MessageSight via a 
# RestAPI call vs SNMP.
# The strings saved in the SNMP array and the REST API array is ordered to to 
# reflect the way the MIB is defined. 
# Changing or adding to the MIB will require changes in the order here as well 
# so please be careful.

server=${A1_IPv4_1}
server1=${A1_HOST}
port=${A1_PORT}
COM="MessageSightInfo"
StatusRunning="Active"

func_result ()
{

    if [[ $success == true ]]
    then
        echo
        echo "Compare SNMP Server Store Info - TEST SUCCESS"
        return 0
    else
        echo
        echo "Compare SNMP Server Store Info - TEST FAILED"
        return 1
    fi

}

# Check status of snmp via rest call to admin endpoint
RestSnmpStatusCmd="curl -f -s -S --connect-timeout 10 --max-time 15 --retry 3 --retry-max-time 45 -X GET http://$server:$port/ima/v1/service/status/SNMP"
echo "RestSnmpStatusCmd=$RestSnmpStatusCmd"
RestSnmpStatusReply=`${RestSnmpStatusCmd}`
echo "RestSnmpStatusReply=$RestSnmpStatusReply"

SnmpStatus=`echo $RestSnmpStatusReply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"SNMP\"][\"Status\"]"`
echo "SnmpStatus=$SnmpStatus"

if [[ "$SnmpStatus" != "$StatusRunning" ]]
then
    echo "Test Aborted Due to SNMP Not Running"
    func_result
    exit $?
fi

declare -a SNMPServerStoreContentList

StoreMibList="DiskUsedPercent MemoryUsedPercent MemoryTotalBytes Pool1TotalBytes Pool1UsedBytes Pool1UsedPercent Pool1RecordsLimitBytes Pool1RecordsUsedBytes Pool2TotalBytes Pool2UsedBytes Pool2UsedPercent" 
ServerStoreList=($StoreMibList)

#Read in Server Store information from appliance
RestServerStoreCmd="curl -f -s -S --connect-timeout 10 --max-time 15 --retry 3 --retry-max-time 45 -X GET http://$server:$port/ima/v1/monitor/Store"
echo "cmd=$RestServerStoreCmd"
RestServerStoreReply=`${RestServerStoreCmd}`
echo "reply=$RestServerStoreReply"

#Read in memory information from SNMP Get
SNMPMemoryTotalBytes=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaMemoryTotalBytes.0)
echo "SNMPMemoryTotalBytes=$SNMPMemoryTotalBytes"

#Read in store information from SNMP Get
SNMPStoreDiskUsedPercent=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStoreDiskUsedPercent.0)
echo "SNMPStoreDiskUsedPercent=$SNMPStoreDiskUsedPercent"
#SNMPStoreDiskFreeBytes=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStoreDiskFreeBytes.0)
#echo "SNMPStoreDiskFreeBytes=$SNMPStoreDiskFreeBytes"
SNMPStoreMemoryUsedPercent=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStoreMemoryUsedPercent.0)
echo "SNMPStoreMemoryUsedPercent=$SNMPStoreMemoryUsedPercent"

SNMPStoreMemoryTotalBytes=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStoreMemoryTotalBytes.0)
echo "SNMPStoreMemoryTotalBytes=$SNMPStoreMemoryTotalBytes"
SNMPStorePool1TotalBytes=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStorePool1TotalBytes.0)
echo "SNMPStorePool1TotalBytes=$SNMPStorePool1TotalBytes"
SNMPStorePool1UsedBytes=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStorePool1UsedBytes.0)
echo "SNMPStorePool1UsedBytes=$SNMPStorePool1UsedBytes"

SNMPStorePool1UsedPercent=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStorePool1UsedPercent.0)
echo "SNMPStorePool1UsedPercent=$SNMPStorePool1UsedPercent"
SNMPStorePool1RecordsLimitBytes=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStorePool1RecordsLimitBytes.0)
echo "SNMPStorePool1RecordsLimitBytes=$SNMPStorePool1RecordsLimitBytes"	
SNMPStorePool1RecordsUsedBytes=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStorePool1RecordsUsedBytes.0)
echo "SNMPStorePool1RecordsUsedBytes=$SNMPStorePool1RecordsUsedBytes"

SNMPStorePool2TotalBytes=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStorePool2TotalBytes.0)
echo "SNMPStorePool2TotalBytes=$SNMPStorePool2TotalBytes"
SNMPStorePool2UsedBytes=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStorePool2UsedBytes.0)
echo "SNMPStorePool2UsedBytes=$SNMPStorePool2UsedBytes"
SNMPStorePool2UsedPercent=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaStorePool2UsedPercent.0)
echo "SNMPStorePool2UsedPercent=$SNMPStorePool2UsedPercent"

SNMPServerStoreList=("$SNMPStoreDiskUsedPercent" "$SNMPStoreMemoryUsedPercent" "$SNMPStoreMemoryTotalBytes" "$SNMPStorePool1TotalBytes" "$SNMPStorePool1UsedBytes" "$SNMPStorePool1UsedPercent" "$SNMPStorePool1RecordsLimitBytes" "$SNMPStorePool1RecordsUsedBytes" "$SNMPStorePool2TotalBytes" "$SNMPStorePool2UsedBytes" "$SNMPStorePool2UsedPercent");
echo "SNMPServerStoreList=$SNMPServerStoreList"


# Strip all information from every string in the SNMP resultant array except the last token which
# contains the data we actually care about
# Example: Before:"IBM-MESSAGESIGHT-MIB::ibmImaMemoryTotalBytes.0 = Counter64: 7934193664"
#          After:7934193664
# Note this assumes there is no colon in the output data, if there is use awk 
# example in file ism-SNMP-VerifyEndpointInfo.sh

# Store result in a new array
IFS=':'
for j in "${SNMPServerStoreList[@]}"
do
    set -- "$j"
    declare -a TokenizedArray=($*)
    TokenizedArrayLength=${#TokenizedArray[@]}
    Token=${TokenizedArray[${TokenizedArrayLength} - 1 ]}
    # Fancy way to strip leading spaces (no idea how it works)
    Token="${Token#"${Token%%[![:space:]]*}"}"
    # Remove Quotes if necessary ie. ("Abcd") -> (Abcd)
    Token="${Token%\"}"
    Token="${Token#\"}"
    SNMPServerStoreContentList=("${SNMPServerStoreContentList[@]}" "${Token}")
done
echo "SNMPServerStoreContentList=$SNMPServerStoreContentList"

# Now we need to parse the JSON response like our SNMP - Call our python helper script
ParsedRestCall=`python ./ism-SNMP-JSON-Parse.py "${RestServerStoreReply}" "${StoreMibList}" "Store"`
echo "ParsedRestCall=$ParsedRestCall"

if [[ $? -ne 0 ]]
then
    echo "JSON Parsing Failed, Aborting Test"
    success=false
    func_result
    exit $?
fi

# Returned as a CSV String
IFS=','
declare -a ServerStoreContentList=($ParsedRestCall)
ServerStoreContentLength=${#ServerStoreContentList[@]}
SNMPContentLength=${#SNMPServerStoreContentList[@]}


if [[ $ServerStoreContentLength -ne $SNMPContentLength ]]
then
    echo "ServerStoreContentLength=$ServerStoreContentLength"
    echo "SNMPContentLength=$SNMPContentLength"
    echo "Contents Returned from SNMP and Rest Call Differ, Aborting Test"
    success=false
    func_result
    exit $?
fi

length=$ServerStoreContentLength
counter=0
while [ $counter -lt $length ]; do
    if [[ ${ServerStoreContentList[$counter]} -eq ${SNMPServerStoreContentList[$counter]} ]]
    then
        printf "${ServerStoreList[$counter]} equals SNMP Value of ${SNMPServerStoreContentList[$counter]}\n"
    else
        printf "${ServerStoreList[$counter]} : ${ServerStoreContentList[$counter]} does NOT equal SNMP Value of ${SNMPServerStoreContentList[$counter]}\n"
        success=false
    fi
    let counter+=1
done

IFS=' \t\n'
# if there was an error in syntax or non-match then success will be set to false
# if it is not set to false we can assume success is true
if [[ $success == false ]]
then
    echo "There is a mismatch"
else
    success=true		
fi
func_result
exit $?
