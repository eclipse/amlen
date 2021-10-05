#!/bin/bash

# This script compares the Server HA Status returned by MessageSight via a 
# RestAPI call vs SNMP.
# The strings saved in the SNMP array and the REST API array is ordered to to 
# reflect the way the MIB is defined. 
# Changing or adding to the MIB will require changes in the order here as well 
# so please be careful.

server=${A1_IPv4_1}
port=${A1_PORT}
COM="MessageSightInfo"
success=true
StatusRunning="Active"

func_result ()
{
    if [[ "$success" == true ]]
    then
        echo
        echo "Compare SNMP Server HA Info - TEST SUCCESS"
        return 0
    else
        echo
        echo "Compare SNMP Server HA Info - TEST FAILED"
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
    success=false
    func_result
    exit $?
fi

HAStaticMibList="Enabled"
HADynamicMibList="NewRole OldRole"
DefaultDynamicValues="UNKNOWN,UNKNOWN"

ServerHAStaticList=($HAStaticMibList)
ServerHADynamicList=($HADynamicMibList)

# Read in Server HA information from appliance
RestServerStatusCmd="curl -f -s -S --connect-timeout 10 --max-time 15 --retry 3 --retry-max-time 45 -X GET http://$server:$port/ima/v1/service/status"
echo "RestServerStatusCmd=$RestServerStatusCmd"
RestServerStatusReply=`${RestServerStatusCmd}`
echo "RestServerStatusReply=$RestServerStatusReply"

#Read in harole information from SNMP Get
SNMPEnabled=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaServerHAEnable.0)
echo "SNMPEnabled=$SNMPEnabled"
SNMPHANewRole=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaServerHANewRole.0)
echo "SNMPHANewRole=$SNMPHANewRole"
SNMPHAOldRole=$(snmpget -v2c -m ./IBM-MESSAGESIGHT-MIB.txt -t 240 -c ${COM} ${A1_IPv4_1} IBM-MESSAGESIGHT-MIB::ibmImaServerHAOldRole.0)
echo "SNMPHAOldRole=$SNMPHAOldRole"

SNMPServerHAStaticList=("$SNMPEnabled")
SNMPServerHADynamicList=("$SNMPHANewRole" "$SNMPHAOldRole")


# Strip all information from every string in the SNMP resultant array except the last token which
# contains the data we actually care about
# Example: Before:"IBM-MESSAGESIGHT-MIB::ibmImaMemoryTotalBytes.0 = Counter64: 7934193664"
#          After:7934193664
# Note this assumes there is no colon in the output data, if there is use awk 
# example in file ism-SNMP-VerifyEndpointInfo.sh

# Store result in a new array - one for static and one for dynamic
#################### Begin Static MIB Values Parsing ####################
# Check if we have only one static value controlling all dynamic output
IFS=':'
for j in "${SNMPServerHAStaticList[@]}"
do
    set -- "$j"
    declare -a TokenizedStaticArray=($*)
    TokenizedStaticArrayLength=${#TokenizedStaticArray[@]}
    TokenStatic=${TokenizedStaticArray[${TokenizedStaticArrayLength} - 1]}
    # Fancy way to strip leading spaces (no idea how it works)
    TokenStatic="${TokenStatic#"${TokenStatic%%[![:space:]]*}"}"
    # Remove Quotes if necessary ie. ("Abcd") -> (Abcd)
    TokenStatic="${TokenStatic%\"}"
    TokenStatic="${TokenStatic#\"}"
    SNMPServerHAStaticContentList=("${SNMPServerHAStaticContentList[@]}" "${TokenStatic}")
done
echo "SNMPServerHAStaticContentList=$SNMPServerHAStaticContentList"

if [[ ${#SNMPServerHAStaticList[@]} -eq 1 ]]
then
    IFS=" "
    #echo $RestServerStatusReply
    RestEnabledCmd=`echo $RestServerStatusReply | python -c "import json,sys;obj=json.load(sys.stdin);print obj[\"HighAvailability\"][\"Enabled\"]"`
    declare -a ServerHAStaticContentList=($RestEnabledCmd)
else
    # Now we need to parse the JSON response like our SNMP - Call our python helper script
    ParsedStaticRestCall=`python ./ism-SNMP-JSON-Parse.py "${RestServerStatusReply}" "${HAStaticMibList}" "HighAvailability"`
    if [[ $? -ne 0 ]]
    then
        echo "JSON Parsing Failed, Aborting Test"
        success=false
        func_result
        exit $?
    fi
    # Returned as a CSV String
    IFS=','
    declare -a ServerHAStaticContentList=($ParsedStaticRestCall)
fi

ServerHAStaticContentLength=${#ServerHAStaticContentList[@]}
SNMPStaticContentLength=${#SNMPServerHAStaticContentList[@]}

if [[ $ServerHAStaticContentLength -ne $SNMPStaticContentLength ]]
then
    echo "ServerHAStaticContentLength=$ServerHAStaticContentLength"
    echo "SNMPStaticContentLength=$SNMPStaticContentLength"
    echo "Contents Returned from SNMP and Rest Call Differ for Static Mib Values, Aborting Test"
    success=false
    func_result
    exit $?
fi

echo ${SNMPServerHAStaticContentList[0]}

#Convert Enabled to string form to compare
if [[ ${SNMPServerHAStaticContentList[0]} == "0" ]]
then
    unset SNMPServerHAStaticContentList[0]
    SNMPServerHAStaticContentList[0]="False"
else  
    unset SNMPServerHAStaticContentList[0]
    SNMPServerHAStaticContentList[0]="True"
fi

length=$ServerHAStaticContentLength
counter=0
while [ $counter -lt $length ]; do
    if [[ ${ServerHAStaticContentList[$counter]} == ${SNMPServerHAStaticContentList[$counter]} ]]
    then
        printf "${ServerHAStaticList[$counter]} equals SNMP Value of ${SNMPServerHAStaticContentList[$counter]}\n"        
    else
        printf "${ServerHAStaticList[$counter]} : ${ServerHAStaticContentList[$counter]} does NOT equal SNMP Value of ${SNMPServerHAStaticContentList[$counter]}\n"
        success=false
    fi
    let counter+=1
done

#################### End Static MIB Values Parsing ####################
if [[ "$success" == false ]]
then
    func_result
    exit $?
fi
#################### Begin Dynamic MIB Values Parsing ####################
IFS=':'
for j in "${SNMPServerHADynamicList[@]}"
do
    set -- "$j"
    declare -a TokenizedDynamicArray=($*)
    TokenizedDynamicArrayLength=${#TokenizedDynamicArray[@]}
    TokenDynamic=${TokenizedDynamicArray[${TokenizedDynamicArrayLength} - 1]}
    # Fancy way to strip leading spaces (no idea how it works)
    TokenDynamic="${TokenDynamic#"${TokenDynamic%%[![:space:]]*}"}"
    # Remove Quotes if necessary ie. ("Abcd") -> (Abcd)
    TokenDynamic="${TokenDynamic%\"}"
    TokenDynamic="${TokenDynamic#\"}"
    SNMPServerHADynamicContentList=("${SNMPServerHADynamicContentList[@]}" "${TokenDynamic}")
done
echo "SNMPServerHADynamicContentList=$SNMPServerHADynamicContentList"

if [[ ${SNMPServerHAStaticContentList[0]} == "True" ]]
then
    ParsedDynamicRestCall=`python ./ism-SNMP-JSON-Parse.py "${RestServerStatusReply}" "${HADynamicMibList}" "HighAvailability"`
    if [[ $? -ne 0 ]]
    then
        echo "JSON Parsing Failed, Aborting Test"
        success=false
        func_result
        exit $?
    fi
else
    ParsedDynamicRestCall=($DefaultDynamicValues)
fi
	
# Returned as a CSV String
IFS=','
declare -a ServerHADynamicContentList=($ParsedDynamicRestCall)
ServerHADynamicContentLength=${#ServerHADynamicContentList[@]}
SNMPDynamicContentLength=${#SNMPServerHADynamicContentList[@]}

if [[ $ServerHADynamicContentLength -ne $SNMPDynamicContentLength ]]
then
    echo "ServerHADynamicContentLength=$ServerHADynamicContentLength"
    echo "SNMPDynamicContentLength=$SNMPDynamicContentLength"
    echo "Contents Returned from SNMP and Rest Call Differ for Dynamic Mib Values, Aborting Test"
    success=false
    func_result
    exit $?
fi

length=$ServerHADynamicContentLength
counter=0
while [ $counter -lt $length ]; do
    if [[ ${ServerHADynamicContentList[$counter]} == ${SNMPServerHADynamicContentList[$counter]} ]]
    then
        printf "${ServerHADynamicList[$counter]} equals SNMP Value of ${SNMPServerHADynamicContentList[$counter]}\n"        
    else
        printf "${ServerHADynamicList[$counter]} : ${ServerHADynamicContentList[$counter]} does NOT equal SNMP Value of ${SNMPServerHADynamicContentList[$counter]}\n"
        success=false
    fi
    let counter+=1
done

#################### End Dynamic MIB Values Parsing ####################
IFS=' \t\n'
# if there was an error in syntax or non-match then success will be set to false
# if it is not set to false we can assume success is true
if [[ "$success" == false ]]
then
    echo "There is a mismatch"
fi
func_result
exit $?
