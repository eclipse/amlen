#!/bin/bash
#
# script to check GeneralTopicsRegression files
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
# GeneralTopicRegression will create producers/consumers for both
# MQTT and JMS. Some will consume messages, some are shared, some are durable,
# some are non-durable and some will buffer messages. 
#
# At this point seems the best method to verify this testcase is thru
# the testcase java code. It is difficult to determine state for post-processing
# Therefore for now no specific post validation will be done
#

#We are at end of verification. If we havent exited already then
#we are successful
echo "GeneralTopicRegression Success"
exit 0
