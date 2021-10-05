#!/bin/bash
#
# script to check GeneralConnections files
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
# Check for exceptions
if [ "`cat $FILEOUT |grep -e "at com." -e "at java."`" ] || [ "`cat $FILEOUT |grep -e "JVMDUMP"`" ]; then
	echo "Found exception in stdout"
	exit 1
fi

#
# Here add specific verification for this testcase
#

#
# Check Queues that were created. There are 8 queues:
#----------------------------------
#In GENERAL:
#Queue 1 has a mix of messages being consumed and buffered
#Queue 2 will have no messages consumed
#Queue 3 will have messages consumed 
#(small buffering as consumers try to keep up with producers)
#Queue 4 will have a mix of messages being consumer and buffered
#Queue 5 will have messages consumed 
#(small buffering as consumers try to keep up with producers)
#Queue 6 will have messages consumed 
#(small buffering as consumers try to keep up with producers)
#Queue 7 will have messages consumed 
#(small buffering as consumers try to keep up with producers)
#Queue 8 will have messages consumed 
#(small buffering as consumers try to keep up with producers)
#-----------------------------------
# Queues are name 'SVTREG-QUEUE[1-8]'
#
# We need to look at state of Queues after test exits.
#


#We are at end of verification. If we havent exited already then
#we are successful
echo "GeneralQueueRegression Success"
exit 0
