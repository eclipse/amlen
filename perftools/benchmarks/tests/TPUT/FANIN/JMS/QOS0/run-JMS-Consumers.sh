#!/bin/bash

# -----------------------------------------------------------------------------------------------------
#
# This script executes the following variant of the TPUT.FANIN.JMS benchmark test.
#
# Variant:
# - QoS 0 (non-durable client sessions)
# - JMS non-durable, shared subscriptions with client ACK
# - X MQTT publishers (each publishing on a unique topic)
# - Y JMS subscribers (single wildcard subscription to receive all messages)
#
# The script takes 4 parameters:
# - the IPv4 address of the MessageSight server to connect to
# - number of MQTT publishers  (in SI units, e.g. 1, 1K, 500K, 1M, etc.)
# - number of JMS subscribers  (in SI units, e.g. 1, 5, 10, etc.)
# - the size (in bytes) of the message to receive (only used for stats purposes).
#
# This variant of the TPUT.FANIN.JMS benchmark should be executed with the following message sizes.
#
# Message Sizes (bytes): 64, 256, 1024, 4096, 16384, 65536, 262144
#
# see README.md for test details
# -----------------------------------------------------------------------------------------------------

DST=""
NUMSUBBERS=0
NUMPUBBERS=0
MSGSIZE=0
if [ "$1" == "" -o "$2" == "" -o "$3" == "" -o "$4" == "" ] ; then
    echo "usage: $0 <IPv4/DNS name of MessageSight server)> <number of MQTT publishers> <number of JMS subscribers> <msg size in bytes>"
    echo "  e.g. $0 A.B.C.D 1M 10 64"
    exit 1
else
    DST=$1
    NUMPUBBERS=$2
    NUMSUBBERS=$3
    MSGSIZE=$4
fi

export GraphiteIP=<hostname of Graphite relay>
export TESTNAME=TPUT-FANIN-JMS
export TESTVARIANT=${TESTNAME}-Q0-SSUBS-${NUMPUBBERS}PUB-${NUMSUBBERS}SUB
export GraphiteMetricRoot=$TESTVARIANT-MSG-${MSGSIZE}

export IMAServer=$DST
export IMAPort=16901
export ACKInterval=100

#JVM_IBM_ARGS="-Xgcthreads5 -XX:+UseParallelGC  -Xjit:optLevel=hot,count=1000"
JVM_COMMON_ARGS="-Xms20G -Xmx20G -Xss256K"
#JVM_DEBUG_ARGS="-Xdebug -Xrunjdwp:transport=dt_socket,server=y,suspend=y,address=8000"

# Subscriber parameters (-rx ...):
#  * -1, no cpu affinity
#  *  0, non-transacted session
#  *  3, receive mode (message listener)
#  *  2, ACK mode = client ACK
#  *  0, non-durable subscription
#  *  mqttfanin/q0-jms/# , topic string
#  *  1, 1 subscription per session
#  *  1, 1 session per connection
#  *  1, 1 connection per thread
#  *  $NUMSUBBERS, number of connections per thread
#  *  0, starting client ID number
# Use shared subscriptions (-mt ss)
# Let test run indefinitely (-d 0)
# CLASSPATH set by perftools/benchmarks/baseenv.sh
java $JVM_COMMON_ARGS $JVM_IBM_ARGS $JVM_DEBUG_ARGS -cp $CLASSPATH JMSBenchTest -d 0 -mt ss -rx -1:0:3:2:0:jmsfanin/q0-jms/#:1:1:1:$NUMSUBBERS:0 -T 0x1 -u 1e-6 -snap 4 -lcsv ${TESTVARIANT}-MSG-${MSGSIZE}-jmsbench.csv

