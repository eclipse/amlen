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
# GeneralConnections simply connect/disconnect to appliance.
# Therefore, there is no extra validation required once test is stopped/exited.
# To review status during runtime use 'Monitor'->'Connection Monitor' which
# should be ~500 connections with all testcases running at same time.
#

#We are at end of verification. If we havent exited already then
#we are successful
echo "GeneralConnections Success"
exit 0
