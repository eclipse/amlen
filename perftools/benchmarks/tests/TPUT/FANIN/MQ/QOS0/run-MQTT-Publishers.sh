#!/bin/bash

# -----------------------------------------------------------------------------------------------------
#
# This script executes the following variant of the TPUT.FANIN.MQ benchmark test.
#
# Variant:
# - QoS 0 (non-durable client sessions)
# - MQ receivers getting messages from a non-persistent local queue on the MQ QMgr
# - X MQTT publishers (each publishing on a unique topic)
# - Y MQ receivers
#
# The script takes 5 parameters:
# - the number of MQ QMgrs used in the test
# - the number of connections per MQ QMgr used in the test
# - number of MQTT publishers  (in SI units, e.g. 1, 1K, 500K, 1M, etc.)
# - number of MQ subscribers  (in SI units, e.g. 1, 5, 10, etc.)
# - the size (in bytes) of the message to receive (only used for stats purposes).
#
# This variant of the TPUT.FANIN.MQ benchmark should be executed with the following combinations.
#
# Message Sizes (bytes): 64, 256, 1024, 4096, 16384, 65536, 262144
#
# see README.md for test details
# -----------------------------------------------------------------------------------------------------

NUMQMGR=0
NUMCONN_PER_QMGR=0
NUMPUBBERS=0
NUMSUBBERS=0
MSGSIZE=0
if [ "$1" == "" -o "$2" == "" -o "$3" == "" -o "$4" == "" -o "$5" == "" ] ; then
    echo "usage: $0 <numQMgrs> <numConnPerQMgr> <number of MQTT publishers> <number of MQ subscribers> <msg size in bytes>"
    echo "  e.g. $0 1 1 1M 1 2048"
    echo "  e.g. $0 5 10 10K 10 64"
    exit 1
else
    NUMQMGR=$1
    NUMCONN_PER_QMGR=$2
    NUMPUBBERS=$3
    NUMSUBBERS=$4
    MSGSIZE=$5
fi

export GraphiteIP=<hostname of Graphite relay>
export TESTNAME=TPUT-FANIN-MQ-Q0-${NUMPUBBERS}PUB
export TESTVARIANT=${TESTNAME}-Q0-QMgrs-${NUMQMGR}-MQCONN-${NUMCONN_PER_QMGR}-${NUMPUBBERS}PUB-${NUMSUBBERS}SUB
export GraphiteMetricRoot=$TESTVARIANT-MSG-${MSGSIZE}

# publishers and subscribers on different machines so use GTOD clock
export ClockSrc=1

# delay between new connections (units in microseconds)
export DelayTime=100
export DelayCount=1

# TLS settings (v1.2 = ECDHE....   v1.3 = TLS....)
#export SSLCipher=ECDHE-RSA-AES256-GCM-SHA384
export SSLCipher=TLS_AES_256_GCM_SHA384
export SSLClientMeth=TLSv12

# Setup trust store
CERTSDIR=../../../../../certs
ROOTCA_HASH=`openssl x509 -hash -in $CERTSDIR/cert-rootCA.pem -noout`
mkdir -p certs
ln -sf ../$CERTSDIR/cert-rootCA.pem certs/${ROOTCA_HASH}.0

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

