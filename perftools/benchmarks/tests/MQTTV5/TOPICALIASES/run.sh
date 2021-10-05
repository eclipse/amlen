#!/bin/bash

# -----------------------------------------------------------------------------------------------------
#
# This script executes the MQTTV5.TOPICALIASES benchmark test.
#
# The script takes 3 parameters:
# - length of topic string (in bytes)
# - size of message payload (in bytes)
# - a flag to indicate whether topic aliases are enabled (True|False)   
#
# This benchmark should be executed with the following:
#
# Topic string lengths (bytes):
# * 10, 20, 30, 40, 50, 100
#
# Message Sizes (bytes):
# * 64, 1024, 2048
#
# Topic Aliases Enabled:
# * True, False
#
# see README.md for test details
# -----------------------------------------------------------------------------------------------------

TOPICSTRINGLEN=0
MSGSIZE=0
ENABLETOPICALIASES=""
if [ "$1" == "" -o "$2" == "" -o "$3" == "" ] ; then
    echo "usage: $0 <topic string length(bytes)> <msg size(bytes)> <True|False>"
    echo "  e.g. $0 10 1024 True"
    echo "  e.g. $0 10 1024 False"
    exit 1
else
    TOPICSTRINGLEN=$1
    MSGSIZE=$2
    ENABLETOPICALIASES=$3
    
    if [ "$3" != "True" -a "$3" != "False" ] ; then
        echo "Allowed values for third parameter (enable topic aliases) are True or False, provided value was $3"
        exit 2    
    fi
fi

export GraphiteIP=<hostname of Graphite relay>
export TESTNAME=MQTTV5-TOPICALIASES-TOPICLEN-${TOPICSTRINGLEN}-TOPICALIASENABLED-${ENABLETOPICALIASES}
export TESTVARIANT=${TESTNAME}-MSGSIZE-${MSGSIZE}
export GraphiteMetricRoot=$TESTVARIANT

# delay between new connections (units in microseconds)
export DelayTime=100
export DelayCount=1

if [ ! -e $TESTNAME.json ] ; then
    echo "$TESTNAME.json does not exist, ensure that the client list file has been created for this test variation"
    exit 1
fi

# Initial Message rate is 1 msgs/sec (-r 1), the message rate can be changed dynamically on the interactive mqttbench command shell with command: rate=<msgs/sec>
# Min-Max message size is based on the parameter passed to this script (-s ${MSGSIZE}-${MSGSIZE})
# Message latency histogram units are in microseconds (-u 1e-6)  
# Latency statistics are reset every 300 seconds (-rl 300)
# Use 3 submitter threads (-st 3)
mqttbench -cl $TESTNAME.json -r 1 -s ${MSGSIZE}-${MSGSIZE} -T 0x1 -snap connects,msgrates,msgcounts,msglatency 4 -u 1e-6 -csv $TESTVARIANT.stats.csv -lcsv $TESTVARIANT.latency.csv -rl 300 -st 3 

