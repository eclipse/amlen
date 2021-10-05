#!/bin/bash
#
# script to check GeneralDeleteMqttClients files
#
# $1 is either STD|HAP
#
# $2 is file of testcase stdout
#
sleep 1s
if [ "$#" -ne "2" ]; then
        echo "args required"
        echo "Syntax is [STD|HAP] [filename of testcase stdout]"
        exit 1
fi
if [[ "$1" != "STD" && "$1" != "HAP" ]]; then
        echo "Syntax is [STD|HAP] [filename of testcase stdout]"
        exit 1
fi
if [ ! -f $2 ]; then
        echo "File $2 does not exist"
        exit 1
fi

FILEOUT=$2
TESTTYPE=$1

echo "Verifing $FILEOUT for $TESTTYPE type test"
if [ "`cat $FILEOUT |grep -e "at com." -e "at java."`" ] || [ "`cat $FILEOUT |grep -e "JVMDUMP"`" ]; then
        echo "Found exception in stdout"
        exit 1
fi

#
# Here add specific verification for this testcase
#

#
# GeneralDeleteMqttClients deletes disconnected clients.
# After this test is done there is really  no extra validation
# required since it does not delete all disconnected clients. Therefore,
# within Monitoring -> 'Disconnected MQTT Clients' you will still a list.
#

#We are at end of verification. If we havent exited already then
#we are successful
echo "GeneralDeleteMqttClients Success"
exit 0
