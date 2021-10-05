#!/bin/bash

# -----------------------------------------------------------------------------------------------------
#
# This script executes the MQTTV5.PROPERTIES benchmark test.
#
# The script takes 1 parameter:
# - number of MQTTV5 user properties   
#
# This benchmark should be executed with the following number of MQTTV5 user properties.
#
# Number of MQTTV5 user properties: 1, 5, 10, 20, 30, 40, 50
# Message Sizes (bytes): 64, 4096, 262144

# see README.md for test details
# -----------------------------------------------------------------------------------------------------

MSGSIZE=0
NUMPROPERTIES=0
if [ "$1" == "" -o "$2" == "" ] ; then
    echo "usage: $0 <number of MQTTV5 user properties> <message size (bytes)>"
    echo "  e.g. $0 10 64"
    exit 1
else
    NUMPROPERTIES=$1
    MSGSIZE=$2
fi

export GraphiteIP=<hostname of Graphite relay>
export TESTNAME=MQTTV5-PROPERTIES
export TESTVARIANT=${TESTNAME}-NUMPROPS-${NUMPROPERTIES}
export GraphiteMetricRoot=$TESTVARIANT-MSG-${MSGSIZE}

# delay between new connections (units in microseconds)
export DelayTime=100
export DelayCount=1

if [ ! -e $TESTVARIANT.json ] ; then
    echo "$TESTVARIANT.json does not exist, ensure that the client list file has been created for this test variation"
    exit 1
fi

# Initial Message rate is 1 msgs/sec (-r 1), the message rate can be changed dynamically on the interactive mqttbench command shell with command: rate=<msgs/sec>
# Min-Max message size is based on the parameter passed to this script (-s ${MSGSIZE}-${MSGSIZE})
# Message latency histogram units are in microseconds (-u 1e-6)  
# Latency statistics are reset every 300 seconds (-rl 300)
# Use 3 submitter threads (-st 3)
mqttbench -cl $TESTVARIANT.json -r 1 -s ${MSGSIZE}-${MSGSIZE} -T 0x1 -snap connects,msgrates,msgcounts,msglatency 4 -u 1e-6 -csv $TESTVARIANT-MSG-${MSGSIZE}.stats.csv -lcsv $TESTVARIANT-MSG-${MSGSIZE}.latency.csv -rl 300 -st 3 

