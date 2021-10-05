#!/bin/bash
#
# script to check GeneralDisconnectClients files
#
# $1 is either STD|HAP
#
# $2 is file of testcase stdout
#

#
# declare remove lines array
#

declare -a REMOVELINES

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
#
# init exceptions found
#
EXCEPTIONSFOUND=0

#
# For HAP we except exceptions during failover (when failover test is running
# in conjuction). Therefore do not exit. Only signify exceptions are seen
# so that other processing can occur below
#
if [ "`cat $FILEOUT |grep -e "at com." -e "at java."`" ] || [ "`cat $FILEOUT |grep -e "JVMDUMP"`" ]; then
        echo "Found exception in stdout"
	if [[ "${TESTTYPE}" == "HAP" ]]; then
		EXCEPTIONSFOUND=1
	else
		#
		# Standalone setup should fail I believe. Will need to
		# test this scenario. But for now fail by exiting
		exit 1
	fi
fi

#
# Here add specific verification for this testcase
#

#
# GeneralDisconnectClients doesnt have much. However during HA setup
# there could be tcp connection refused errors like the following, due
# to GeneralFailoverRegression, which tests failovers. So we exepect
# connection refused during this transition:
# ---------------------------------------------------------------
#Unable to connect to server (32103) - java.net.ConnectException: Connection refused
#        at org.eclipse.paho.client.mqttv3.internal.TCPNetworkModule.start(TCPNetworkModule.java:79)
#        at org.eclipse.paho.client.mqttv3.internal.ClientComms$ConnectBG.run(ClientComms.java:590)
#        at java.lang.Thread.run(Thread.java:780)
#Caused by: java.net.ConnectException: Connection refused
#        at java.net.Socket.connect(Socket.java:590)
#        at org.eclipse.paho.client.mqttv3.internal.TCPNetworkModule.start(TCPNetworkModule.java:70)
#        ... 2 more
#---------------------------------------------------------------------
#
# Therefore check for exceptions other than above for HAP setup
echo "type ${TESTTYPE}"
echo "exceptionsfound $EXCEPTIONSFOUND"
if [[ "${TESTTYPE}" == "HAP"  &&  $EXCEPTIONSFOUND == 1 ]]; then

	echo "Parsing exceptions"
	#
	# The below command basically greps out 7 lines 
	# after "Unable to connect...". We then parse out the line numbers
	# so we can exclude them.
	#

	echo "Removing expected exceptions for HAP setup"
	REMOVELINES=`cat ${FILEOUT} |grep -A 7 -n "Unable to connect to server" |grep -v "\-\-" |tr ':' ' '|tr '-' ' ' |gawk {'print $1"d"'}`
	echo "removing lines=${REMOVELINES}"
	CLEANLINES=`sed "${REMOVELINES[@]}" ${FILEOUT}`
	echo "cleaned file=${CLEANLINES}"

	#
	# check cleaned file output
	#
	while read -r LINE; do
		echo "reading line=$LINE"
		if [[ $LINE == *"at com."* || $LINE == *"at java."* || $LINE == *"JVMDUMP"* ]]; then
			#
			# we found unexpected exception. fail and exit
			#
			echo "Found exception. Line=$LINE"
			echo "exiting"
			exit 1
		fi
	done <<< "$CLEANLINES"
fi

#We are at end of verification. If we havent exited already then
#we are successful
echo "GeneralDisconnectClients Success"
exit 0
