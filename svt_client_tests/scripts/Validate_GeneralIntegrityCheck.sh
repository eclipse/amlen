#!/bin/bash
# script to check GeneralIntegrityCheck files
#
# $1 is either STD|HAP
#
# $2 is file of testcase stdout
#

#
# GeneralIntegrityCheck will create SendAndCheckMessages.txt if errors occur
#
CHECKFILE=./errors/SendAndCheckMessages.txt

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
#  Here add specific verification for this testcase
#

#
# Check if error file exists
#
if [ -f $CHECKFILE ]; then
	#
	# if file exist this means it should fail. However, the following
	# error always seems to occur:
	#---------------------------------
	#Multi JMS Consumer:
	#Clientid:NOT IN USE
	#Clientid:NOT IN USE
	#Destination:SVTINT-TOPIC8
	#Durable:false
	#SingleConnection:false
	#SharedSub:true SubID:SVTINT-JmsSub
	#Selector:Food = 'Chips'
	#Transacted:false AckMod: 1 Number:30
	#Retained:false
	#FailMessage:Client NOT IN USE has been configured to check for 
	#messages every 500000milliseconds.The last message arrived at 
	#1406231526723 but the time is 1406231526832
	#----------------------------------------

	#
	# lets cat the file and exit as a failure for now. 
	#
	cat $CHECKFILE
	exit 1
fi

#We are at end of verification. If we havent exited already then
#we are successful
echo "GeneralIntegrityCheck Success"
exit 0
