#!/bin/bash
#
# script to check GeneralDeleteSubscriptions files
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
# GeneralDeleteSubscriptions will delete subscriptions base on input
# topic argument. Currently automation scripts use 'SVTSUB-TOPIC'.
# After testcase ends/exits, there is no data to validate. Could determine if
# some 'SVTSUB-TOPIC' subscriptions exist but  may depend on rate of creation
# and deleting which occurs at same time. Therefore, the end result of this
# testcase is indeterminate.
#


#We are at end of verification. If we havent exited already then
#we are successful
echo "GeneralDeleteSubscriptions Success"
exit 0
