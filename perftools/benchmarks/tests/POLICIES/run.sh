#!/bin/bash

# -----------------------------------------------------------------------------------------------------
#
# This script executes the POLICIES benchmark test.
#
# The script takes 1 parameter:
# - number of policies   
#
# This benchmark should be executed with the following number of policies.
#
# Number of messaging policies: 1, 5, 10, 15, 20, 25, 30, 35, 40
#
# see README.md for test details
# -----------------------------------------------------------------------------------------------------

MSGSIZE=64
NUMPOLICIES=0
if [ "$1" == "" ] ; then
    echo "usage: $0 <number of policies>"
    echo "  e.g. $0 10"
    exit 1
else
    NUMPOLICIES=$1
fi

export GraphiteIP=<hostname of Graphite relay>
export TESTNAME=POLICIES
export TESTVARIANT=${TESTNAME}-POLICIES-${NUMPOLICIES}
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

