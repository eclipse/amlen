#!/bin/bash

# -----------------------------------------------------------------------------------------------------
#
# This script executes the following variant of the TPUT.BROADCAST benchmark test.
#
# Variant:
# - QoS 0 (non-durable client sessions)
# - X publishers   (publishing messages to a single topic)
# - Y subscribers  (each subscribed to a single topic)
# - Z topics     
#
# The script takes  parameters:
# - number of publishers
# - number of subscribers (in SI units, e.g. 1, 500, 1K, etc.)
# - number of topics
# - the size (in bytes) of the message to publish.  
#
# This variant of the TPUT.BROADCAST benchmark should be executed with the following message sizes.
#
# Message Sizes (bytes): 64, 512, 4096
#
# see README.md for test details
# -----------------------------------------------------------------------------------------------------

MSGSIZE=0
NUMPUBBERS=0
NUMSUBBERS=0
NUMTOPICS=0
if [ "$1" == "" -o "$2" == "" -o "$3" == "" -o "$4" == "" ] ; then
    echo "usage: $0 <number of publishers (in SI units)> <number of subscribers (in SI units)> <number of topics> <msg size in bytes>"
    echo "  e.g. $0 1 1K 1 64"
    echo "  e.g. $0 1 10K 1 4096"
    exit 1
else
    NUMPUBBERS=$1
    NUMSUBBERS=$2
    NUMTOPICS=$3
    MSGSIZE=$4
fi

export GraphiteIP=<hostname of Graphite relay>
export TESTNAME=TPUT-BROADCAST
export TESTVARIANT=${TESTNAME}-Q0-${NUMTOPICS}TOPICS-${NUMPUBBERS}PUB-${NUMSUBBERS}SUB
export GraphiteMetricRoot=$TESTVARIANT-MSG-${MSGSIZE}

# delay between new connections (units in microseconds)
export DelayTime=100
export DelayCount=1

# TLS settings (v1.2 = ECDHE....   v1.3 = TLS....)
#export SSLCipher=ECDHE-RSA-AES256-GCM-SHA384
export SSLCipher=TLS_AES_256_GCM_SHA384
export SSLClientMeth=TLSv12

# Setup trust store
CERTSDIR=../../../../certs
ROOTCA_HASH=`openssl x509 -hash -in $CERTSDIR/cert-rootCA.pem -noout`
mkdir -p certs
ln -sf ../$CERTSDIR/cert-rootCA.pem certs/${ROOTCA_HASH}.0

if [ ! -e $TESTVARIANT.json ] ; then
    echo "$TESTVARIANT.json does not exist, ensure that the client list file has been created for this test variation"
    exit 1
fi

# Initial Message rate is 1 msg every second (-r 1), the message rate can be changed dynamically on the interactive mqttbench command shell with command: rate=<msgs/sec>
# Min-Max message size is based on the parameter passed to this script (-s ${MSGSIZE}-${MSGSIZE})
# Message latency histogram units are in milliseconds (-u 1e-3)  
# Latency statistics are reset every 300 seconds (-rl 300)
mqttbench -cl $TESTVARIANT.json -r 1 -s ${MSGSIZE}-${MSGSIZE} -T 0x1 -snap connects,msgrates,msgcounts,msglatency 4 -u 1e-3 -csv $TESTVARIANT.stats.csv -lcsv $TESTVARIANT.latency.csv -rl 300 

