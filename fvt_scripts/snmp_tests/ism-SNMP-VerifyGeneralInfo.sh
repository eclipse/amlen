#!/bin/bash

# This script compares the General Connection Statistics returned by 
# MessageSight via a RestAPI call vs SNMP.
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
        echo "Compare SNMP Server Stat Info - TEST SUCCESS"
        return 0
    else
        echo
        echo "Compare SNMP Server Stat Info - TEST FAILED"
        return 1
    fi
}

# Check status of snmp
RestSnmpStatusCmd="curl -f -s -S --connect-timeout 10 --max-time 15 --retry 3 --retry-max-time 45 -X GET http://$server:$port/ima/v1/service/status/SNMP"
echo "RestSnmpStatusCmd=$RestSnmpStatusCmd"
RestSnmpStatusReply=`${RestSnmpStatusCmd}`
echo "RestSnmpStatusReply=$RestSnmpStatusReply"

SnmpStatus=`echo $RestSnmpStatusReply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"SNMP\"][\"Status\"]"`
echo "SnmpStatus=$SnmpStatus"
if [[ "$SnmpStatus" != "$StatusRunning" ]]
then
    echo "Test Aborted Due to SNMP not Running"
    func_result
    exit $?
fi

declare -a SNMPServerStatContentList

# Currently only adding BadConnectionCount, Total Endpoints
StatMibList="BadConnCount TotalEndpoints"
ServerStatList=($StatMibList)

# Read in Server Connection Stat information from appliance
RestServerStatCmd="curl -f -s -S --connect-timeout 10 --max-time 15 --retry 3 --retry-max-time 45 -X GET http://$server:$port/ima/v1/monitor/Server"
echo "RestServerStatCmd=$RestServerStatCmd"
RestServerStatReply=`${RestServerStatCmd}`
echo "RestServerStatReply=$RestServerStatReply"

SNMPServerStatBadConnCount=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaBadConnections.0)
echo "SNMPServerStatBadConnCount=$SNMPServerStatBadConnCount"
SNMPServerStatTotalEndpoints=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaTotalEndpoints.0)
echo "SNMPServerStatTotalEndpoints=$SNMPServerStatTotalEndpoints"

SNMPServerStatList=("$SNMPServerStatBadConnCount" "$SNMPServerStatTotalEndpoints");
echo "SNMPServerStatList=$SNMPServerStatList"

# Strip all information from every string in the SNMP resultant array except the last token which
# contains the data we actually care about
# Example: Before:"IBM-MESSAGESIGHT-MIB::ibmImaMemoryTotalBytes.0 = Counter64: 7934193664"
#          After:7934193664
# Note this assumes there is no colon in the output data, if there is use awk 
# example in file ism-SNMP-VerifyEndpointInfo.sh

# Store result in a new array
IFS=':'
for j in "${SNMPServerStatList[@]}"
do
    set -- "$j"
    declare -a TokenizedArray=($*)
    TokenizedArrayLength=${#TokenizedArray[@]}
    Token=${TokenizedArray[${TokenizedArrayLength} - 1 ]}
    # Fancy way to strip leading spaces
    Token="${Token#"${Token%%[![:space:]]*}"}"
    # Remove Quotes if necessary ie. ("Abcd") -> (Abcd)
    Token="${Token%\"}"
    Token="${Token#\"}"
    SNMPServerStatContentList=("${SNMPServerStatContentList[@]}" "${Token}")
done
echo "SNMPServerStatContentList=$SNMPServerStatContentList"

# Now we need to parse the JSON response like our SNMP - Call our python helper script
ParsedRestCall=`python ./ism-SNMP-JSON-Parse.py "${RestServerStatReply}" "${StatMibList}" "Server"`

if [[ $? -ne 0 ]]
then
    echo "JSON Parsing Failed, Aborting Test"
    success=false
    func_result
    exit $?
fi

# Returned as a CSV string
IFS=','
declare -a ServerStatContentList=($ParsedRestCall)
ServerStatContentLength=${#ServerStatContentList[@]}
SNMPContentLength=${#SNMPServerStatContentList[@]}

if [[ $ServerStatContentLength -ne $SNMPContentLength ]]
then
    echo "ServerStatContentLength=$ServerStatContentLength"
    echo "SNMPContentLength=$SNMPContentLength" 
    echo "Contents Returned from SNMP and Rest Call Differ, Aborting Test"
    success=false
    func_result
    exit $?
fi

length=$ServerStatContentLength
counter=0
while [ $counter -lt $length ]; do
    if [[ ${ServerStatContentList[$counter]} == ${SNMPServerStatContentList[$counter]} ]]
    then
        printf "${ServerStatList[$counter]} equals SNMP Value of ${SNMPServerStatContentList[$counter]}\n"        
    else
        printf "${ServerStatList[$counter]} : ${ServerStatContentList[$counter]} does NOT equal SNMP Value of ${SNMPServerStatContentList[$counter]}\n"
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
