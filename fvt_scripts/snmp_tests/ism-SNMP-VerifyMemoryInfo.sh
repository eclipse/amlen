#!/bin/bash

# This script compares the Memory Statistics returned by MessageSight via a 
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
        echo "Compare SNMP Server Memory Info - TEST SUCCESS"
        return 0
    else
        echo
        echo "Compare SNMP Server Memory Info - TEST FAILED"
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

declare -a SNMPServerMemoryList

MemMibList="MemoryTotalBytes MemoryFreePercent ServerVirtualMemoryBytes ServerResidentSetBytes MessagePayloads PublishSubscribe Destinations CurrentActivity ClientStates"
ServerMemoryList=($MemMibList)

#Read in Server Memory information from appliance 
RestServerMemoryCmd="curl -f -s -S --connect-timeout 10 --max-time 15 --retry 3 --retry-max-time 45 -X GET http://$server:$port/ima/v1/monitor/Memory"
echo "RestServerMemoryCmd=$RestServerMemoryCmd"
RestServerMemoryReply=`${RestServerMemoryCmd}`
echo "RestServerMemoryReply=$RestServerMemoryReply"

#Read in memory information from SNMP Walk
SNMPMemoryTotalBytes=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaMemoryTotalBytes.0)
echo "SNMPMemoryTotalBytes=$SNMPMemoryTotalBytes"

#SNMPMemoryFreeBytes=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt  -cpublic ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaMemoryFreeBytes.0)
SNMPMemoryFreePercent=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaMemoryFreePercent.0)
echo "SNMPMemoryFreePercent=$SNMPMemoryFreePercent"
SNMPServerVirtualMemoryBytes=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaServerVirtualMemoryBytes.0)
echo "SNMPServerVirtualMemoryBytes=$SNMPServerVirtualMemoryBytes"
SNMPServerResidentSetBytes=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaServerResidentSetBytes.0)
echo "SNMPServerResidentSetBytes=$SNMPServerResidentSetBytes"
SNMPMessagePayloads=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaMessagePayloads.0)
echo "SNMPMessagePayloads=$SNMPMessagePayloads"

SNMPPublishSubscribe=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaPublishSubscribe.0)
echo "SNMPPublishSubscribe=$SNMPPublishSubscribe"
SNMPDestinations=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaDestinations.0)
echo "SNMPDestinations=$SNMPDestinations"
SNMPCurrentActivity=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaCurrentActivity.0)
echo "SNMPCurrentActivity=$SNMPCurrentActivity"

SNMPClientStates=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaClientStates.0)
echo "SNMPClientStates=$SNMPClientStates"

#Save memory info into an array - currently not including MemoryFreeBytes since the value changes constantly
SNMPServerMemoryList=("$SNMPMemoryTotalBytes" "$SNMPMemoryFreePercent" "$SNMPServerVirtualMemoryBytes" "$SNMPServerResidentSetBytes" "$SNMPMessagePayloads" "$SNMPPublishSubscribe" "$SNMPDestinations" "$SNMPCurrentActivity" "$SNMPClientStates" );
echo "SNMPServerMemoryList=$SNMPServerMemoryList"

# Strip all information from every string in the SNMP resultant array except the last token which 
# contains the data we actually care about 
# Example: Before:"IBM-MESSAGESIGHT-MIB::ibmImaMemoryTotalBytes.0 = Counter64: 7934193664"
#		   After:7934193664
# Note this assumes there is no colon in the output data, if there is use awk 
# example in file ism-SNMP-VerifyEndpointInfo.sh

# Store result in a new array
IFS=':'
for j in "${SNMPServerMemoryList[@]}"
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
    SNMPServerMemoryContentList=("${SNMPServerMemoryContentList[@]}" "${Token}")
done
echo "SNMPServerMemoryContentList=$SNMPServerMemoryContentList"

# Now we need to parse the JSON response like our SNMP - Call our python helper script
ParsedRestCall=`python ./ism-SNMP-JSON-Parse.py "${RestServerMemoryReply}" "${MemMibList}" "Memory"`

if [[ $? -ne 0 ]]
then
    echo "JSON Parsing Failed, Aborting Test"
    success=false
    func_result
    exit $?
fi

# Returned as a CSV String
IFS=','
declare -a ServerMemoryContentList=($ParsedRestCall)
ServerMemoryContentLength=${#ServerMemoryContentList[@]}
SNMPContentLength=${#SNMPServerMemoryContentList[@]}

if [[ $ServerMemoryContentLength -ne $SNMPContentLength ]];
then
    echo "ServerMemoryContentLength=$ServerMemoryContentLength"
    echo "SNMPContentLength=$SNMPContentLength"
    echo "Contents Returned from SNMP and Rest Call Differ, Aborting Test"
    success=false
    func_result
    exit $?
fi

length=$ServerMemoryContentLength
counter=0
while [ $counter -lt $length ]; do  
    if [ $counter -eq 1 ]
    then
        margin=1
        printf "Considering memory percent +/- ${margin} \n"
        minusmargin=$((${SNMPServerMemoryContentList[$counter]} - $margin))
        plusmargin=$((${SNMPServerMemoryContentList[$counter]} + $margin))
        if [[ ${ServerMemoryContentList[$counter]} -eq ${SNMPServerMemoryContentList[$counter]} ]] || [[ ${ServerMemoryContentList[$counter]} -eq ${minusmargin} ]] || [[ ${ServerMemoryContentList[$counter]} -eq ${plusmargin} ]]
        then
            printf "${ServerMemoryList[$counter]} equals(or close enough) SNMP Value of ${SNMPServerMemoryContentList[$counter]}\n"
        else
            printf "${ServerMemoryList[$counter]} : ${ServerMemoryContentList[$counter]} does NOT equal SNMP Value of ${SNMPServerMemoryContentList[$counter]}\n"
            success=false
        fi
    else
        if [[ ${ServerMemoryContentList[$counter]} -eq ${SNMPServerMemoryContentList[$counter]} ]]
        then
            printf "${ServerMemoryList[$counter]} equals SNMP Value of ${SNMPServerMemoryContentList[$counter]}\n"
        else
            printf "${ServerMemoryList[$counter]} : ${ServerMemoryContentList[$counter]} does NOT equal SNMP Value of ${SNMPServerMemoryContentList[$counter]}\n"
            success=false
        fi
    fi
    let counter+=1
done

IFS=$' \t\n'
# if there was an error in syntax or non-match then success will be set to false
# if it is not set to false we can assume success is true
if [ "$success" == false ];
then
    echo "There is a mismatch"
else
    success=true		
fi
func_result
exit $?
