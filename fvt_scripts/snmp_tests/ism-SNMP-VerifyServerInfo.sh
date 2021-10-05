#!/bin/bash

# This script compares the Server Status returned by MessageSight via a 
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
        echo "Compare SNMP Server Status Info - TEST SUCCESS"
        return 0
    else
        echo
        echo "Compare SNMP Server Status Info - TEST FAILED"
        return 1
    fi
}

# Check status of snmp
RestSnmpStatusCmd="curl -f -s -S --connect-timeout 10 --max-time 15 --retry 3 --retry-max-time 45 -X GET http://$server:$port/ima/v1/service/status/SNMP"
echo "RestSnmpStatusReply=$RestSnmpStatusReply"
RestSnmpStatusReply=`${RestSnmpStatusCmd}`
echo "RestSnmpStatusReply=$RestSnmpStatusReply"

SnmpStatus=`echo $RestSnmpStatusReply| python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"SNMP\"][\"Status\"]"`
echo "SnmpStatus=$SnmpStatus"
if [[ "$SnmpStatus" != "$StatusRunning" ]]
then
    echo "Test Aborted Due to SNMP not Running"
    func_result
    exit $?
fi

declare -a SNMPServerStatusList

StatusMibList="State StateDescription"
ServerStatusList=($StatusMibList)

# Read in Server status information from appliance 
RestServerStatusCmd="curl -f -s -S --connect-timeout 10 --max-time 15 --retry 3 --retry-max-time 45 -X GET http://$server:$port/ima/v1/service/status/Server"
echo "RestServerStatusCmd=$RestServerStatusCmd"
RestServerStatusReply=`${RestServerStatusCmd}`
echo "RestServerStatusReply=$RestServerStatusReply"

# Read in server information from SNMP Get
SNMPServerState=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaServerState.0)
echo "SNMPServerState=$SNMPServerState"
SNMPServerStateDescription=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaServerStateDesc.0)
echo "SNMPServerStateDescription=$SNMPServerStateDescription"
SNMPServerStatusList=("$SNMPServerState" "$SNMPServerStateDescription");
echo "SNMPServerStatusList=$SNMPServerStatusList"

# Strip all information from every string in the SNMP resultant array except the last token which
# contains the data we actually care about
# Example: Before:"IBM-MESSAGESIGHT-MIB::ibmImaMemoryTotalBytes.0 = Counter64: 7934193664"
#          After:7934193664
# Note this assumes there is no colon in the output data, if there is use awk 
# example in file ism-SNMP-VerifyEndpointInfo.sh

# Store result in a new array
IFS=':'
for j in "${SNMPServerStatusList[@]}"
do
    set -- "$j"
    declare -a TokenizedArray=($*)
    TokenizedArrayLength=${#TokenizedArray[@]}
    Token=${TokenizedArray[${TokenizedArrayLength} - 1 ]}
    #Fancy way to strip leading spaces
    Token="${Token#"${Token%%[![:space:]]*}"}"
    # Remove Quotes if necessary ie. ("Abcd") -> (Abcd)
    Token="${Token%\"}"
    Token="${Token#\"}"
    SNMPServerStatusContentList=("${SNMPServerStatusContentList[@]}" "${Token}")
done
echo "SNMPServerStatusContentList=$SNMPServerStatusContentList"

# Now we need to parse the JSON response like our SNMP - Call our python helper script
ParsedRestCall=`python ./ism-SNMP-JSON-Parse.py "${RestServerStatusReply}" "${StatusMibList}" "Server"`

if [[ $? -ne 0 ]]
then
    echo "JSON Parsing Failed, Aborting Test"
    success=false
    func_result
    exit $?
fi

# Returned as a CSV String
IFS=','
declare -a ServerStatusContentList=($ParsedRestCall)
ServerStatusContentLength=${#ServerStatusContentList[@]}
SNMPContentLength=${#SNMPServerStatusContentList[@]}

if [[ $ServerStatusContentLength -ne $SNMPContentLength ]]
then
    echo "ServerStatusContentLength=$ServerStatusContentLength"
    echo "SNMPContentLength=$SNMPContentLength"
    echo "Contents Returned from SNMP and Rest Call Differ, Aborting Test"
    success=false
    func_result
    exit $?
fi                                             
# Compare Server info given by SNMP with REST API call - only nonvariable mibs
length=$ServerStatusContentLength
counter=0
while [ $counter -lt $length ]; do
    if [[ ${ServerStatusContentList[$counter]} = ${SNMPServerStatusContentList[$counter]} ]]
    then
        printf "${ServerStatusList[$counter]} equals SNMP Value of ${SNMPServerStatusContentList[$counter]}\n"
    else
        printf "${ServerStatusList[$counter]} : ${ServerStatusContentList[$counter]} does NOT equal to SNMP Value of ${SNMPServerStatusContentList[$counter]}\n"
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
