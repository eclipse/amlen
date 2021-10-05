#!/bin/bash
#
# script to check GeneralDiscardMessages files
#
# $1 is either STD|HAP
#
# $2 is file of testcase stdout
#

# setup var for error file
ERRORFILE=./errors/SendAndDiscardMessages.txt

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
# GeneralDiscardMessages will create 'SendAndDiscardMessages.txt'. if exists
# testcase fails
# 

if [ -f $ERRORFILE ]; then
	echo "Found SendAndDiscardMessages file. Printing out errors"
	cat $ERRORFILE
	exit 1
fi

#We are at end of verification. If we havent exited already then
#we are successful
echo "GeneralDiscardMessages Success"
exit 0
