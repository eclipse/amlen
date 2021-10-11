#!/bin/bash

# This script compares the Endpoint Statistics returned by MessageSight via a RestAPI call vs SNMP
# The strings saved in the SNMP array and the REST API array is ordered to to reflect the way the MIB
# is defined. Changing or adding to the MIB will require changes in the order here as well so please
# be careful.

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
        echo "Compare SNMP Server Endpoint Info - TEST SUCCESS"
        return 0
    else
        echo
        echo "Compare SNMP Server Endpoint Info - TEST FAILED"
        return 1
    fi

}

# Check status of snmp via rest call to admin endpoint
RestSnmpStatusCmd="curl -f -s -S --connect-timeout 10 --max-time 15 --retry 3 --retry-max-time 45 -X GET http://$server:$port/ima/v1/service/status/SNMP"
echo "RestSnmpStatusCmd=$RestSnmpStatusCmd"
RestSnmpStatusReply=`${RestSnmpStatusCmd}`
echo "RestSnmpStatusReply=$RestSnmpStatusReply"

SnmpStatus=`echo $RestSnmpStatusReply | python3 -c "import json,sys;obj=json.load(sys.stdin);print(obj[\"SNMP\"][\"Status\"])"`
echo "SnmpStatus=$SnmpStatus"
if [[ "$SnmpStatus" != "$StatusRunning" ]]
then
    echo "Test Aborted Due to SNMP Not Running"
    func_result
    exit $?
fi

declare -a SNMPEndpointContentList

#FullEndpointMibList="Name IPAddr Enabled Total Active Messages Bytes LastErrorCode ConfigTime ResetTime BadConnections"
EndpointMibList="Name Enabled"
ServerEndpointList=($EndpointMibList)

# Get Server Endpoint Info from Appliance
RestServerEndpointCmd="curl -f -s -S --connect-timeout 10 --max-time 15 --retry 3 --retry-max-time 45 -X GET http://$server:$port/ima/v1/monitor/Endpoint"
echo "RestServerEndpointCmd=$RestServerEndpointCmd"
RestServerEndpointReply=`$RestServerEndpointCmd`
echo "RestServerEndpointReply=$RestServerEndpointReply"

#Determine index of Endpoint
snmptableEndpoint=$(snmptable -v2c -m ./IBM-MESSAGESIGHT-MIB.txt  -t 240 -c ${COM} ${A1_IPv4_1} -OX Endpoint | grep SNMP1EP)
set -- "$snmptableEndpoint"
declare -a Array=($*)
TableIndex=${Array[0]}

#Read in store information from SNMP Get
SNMPServerEndpointNameSNMP1EP=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaEndpointName.$TableIndex)
echo "SNMPServerEndpointNameSNMP1EP=$SNMPServerEndpointNameSNMP1EP"
SNMPServerEndpointEnabledSNMP1EP=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaEndpointEnabled.$TableIndex)
echo "SNMPServerEndpointEnabledSNMP1EP=$SNMPServerEndpointEnabledSNMP1EP"

SNMPServerEndpointList=("$SNMPServerEndpointNameSNMP1EP" "$SNMPServerEndpointEnabledSNMP1EP");

# Strip all information from every string in the SNMP resultant array except the last token which
# contains the data we actually care about
# Example: Before:"IBM-MESSAGESIGHT-MIB::ibmImaMemoryTotalBytes.0 = Counter64: 7934193664"
#          After:7934193664

# Store result in a new array
for j in "${SNMPServerEndpointList[@]}"
do
    set -- "$j"
    Token=`echo "${j}" | awk 'BEGIN { FS=": " } { print $NF }'`
    # Fancy way to strip leading spaces
    Token="${Token#"${Token%%[![:space:]]*}"}"
    # Remove Quotes if necessary ie. ("Abcd") -> (Abcd)
    Token="${Token%\"}"
    Token="${Token#\"}"
    SNMPServerEndpointContentList=("${SNMPServerEndpointContentList[@]}" "${Token}")
done
echo "SNMPServerEndpointContentList=$SNMPServerEndpointContentList"

# Now we need to parse the JSON response like our SNMP - Call our python helper script
ParsedRestCall=`python ./ism-SNMP-JSON-Parse.py -a "${RestServerEndpointReply}" "${EndpointMibList}" "Endpoint"`
if [[ $? -ne 0 ]]
then
    echo "JSON Parsing failed, Aborting Test"
    success=false
    func_result
    exit $?
fi

# Tables are returned as oid1,oid2 oid1,oid2 ... from the JSON Parser
IFS=' '
declare -a ServerEndpointContentList=($ParsedRestCall)
EndpointEntry=${ServerEndpointContentList[${TableIndex}-1]}
IFS=','
declare -a EntryContentList=($EndpointEntry)
EntryContentLength=${#EntryContentList[@]}
SNMPContentLength=${#SNMPServerEndpointContentList[@]}

if [[ $EntryContentLength -ne $SNMPContentLength ]]
then
    echo "EntryContentLength=$EntryContentLength"
    echo "SNMPContentLength=$SNMPContentLength "
    echo "Contents Returned from SNMP and Rest Call Differ, Aborting Test"
    success=false
    func_result
    exit $?
fi

# Convert second value of SNMP Endpoint Content List to string form to compare
if [ ${SNMPServerEndpointContentList[1]} == "0" ]
then
    unset SNMPServerEndpointContentList[1]
    SNMPServerEndpointContentList=("${SNMPServerEndpointContentList[@]}" "False")
else
    unset SNMPServerEndpointContentList[1]
    SNMPServerEndpointContentList=("${SNMPServerEndpointContentList[@]}" "True")
fi

length=$EntryContentLength
counter=0

while [ $counter -lt $length ]; do
    if [[ ${EntryContentList[$counter]} == ${SNMPServerEndpointContentList[$counter]} ]]
    then
        printf "${ServerEndpointList[$counter]} equals SNMP Value of ${SNMPServerEndpointContentList[$counter]}\n"
    else
        printf "${ServerEndpointList[$counter]} : ${EntryContentList[$counter]} does NOT equal SNMP Value of ${SNMPServerEndpointContentList[$counter]}\n"
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
