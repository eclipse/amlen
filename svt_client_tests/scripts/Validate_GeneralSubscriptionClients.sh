#!/bin/bash
#
# script to check GeneralSubcriptionClients files
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
if [ "`cat $FILEOUT |grep -e "at com." -e "at java."`" ] || [ "`cat $FILEOUT |grep -e "JVMDUMP"`" ]; then
        echo "Found exception in stdout"
        exit 1
fi

#
# Here add specific verification for this testcase
#


#
# GeneralSubscriptionClients will create new mqtt subscriber clients, connect,
# subscribe and then disconnect. Base on code it doesnt look to receive 
# messages
#
# At this point there is does not seem to be a certain known state for post
# processing after testcase is stopped (or ended). Therefore, for now
# will not perform specific post validation
#

#We are at end of verification. If we havent exited already then
#we are successful
echo "GeneralSubscriptionClients Success"
exit 0
