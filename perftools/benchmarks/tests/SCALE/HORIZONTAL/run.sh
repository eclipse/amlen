#!/bin/bash

# -----------------------------------------------------------------------------------------------------
#
# This script executes the following variant of the SCALE.HORIZONTAL benchmark test.
#
# Variant:
# - QoS 0 (non-durable client sessions)
# - MQTT v5 shared subscriptions
#
#
# The script takes 3 parameters:
# - number of MessageSight servers in the test (e.g. 1, 2, 4, 6, etc.)
# - mqttbench instance id (e.g. 1, 2, 3, 4, etc. this uniquely identifies this instance of mqttbench)
# - the size (in bytes) of the message to publish.  
#
# This variant of the SCALE.HORIZONTAL benchmark should be executed with the following message sizes.
#
# Message Sizes (bytes): 64, 4096, 262144
#
# see README.md for test details
# -----------------------------------------------------------------------------------------------------

NUMSERVERS=0
MBINSTID=0
MSGSIZE=0
if [ "$1" == "" -o "$2" == "" -o "$3" == "" ] ; then
    echo "usage: $0 <number of MessageSight servers> <ID of this mqttbench instance> <msg size in bytes>"
    echo "  e.g. $0 10 1 64"
    echo "  e.g. $0 6 5 1024"
    exit 1
else
    NUMSERVERS=$1
    MBINSTID=$2
    MSGSIZE=$3
fi

export GraphiteIP=<hostname of Graphite relay>
export TESTNAME=SCALE-HORIZONTAL
export TESTVARIANT=${TESTNAME}-Q0-SSUBS-NUMSRVR-${NUMSERVERS}
export GraphiteMetricRoot=$TESTVARIANT-MSG-${MSGSIZE}

# delay between new connections (units in microseconds)
export DelayTime=100
export DelayCount=1

# TLS settings (v1.2 = ECDHE....   v1.3 = TLS....)
#export SSLCipher=ECDHE-RSA-AES256-GCM-SHA384
export SSLCipher=TLS_AES_256_GCM_SHA384
export SSLClientMeth=TLSv12

# Setup trust store
CERTSDIR=../../../../../../certs
ROOTCA_HASH=`openssl x509 -hash -in $CERTSDIR/cert-rootCA.pem -noout`
mkdir -p certs
ln -sf ../$CERTSDIR/cert-rootCA.pem certs/${ROOTCA_HASH}.0

if [ ! -e $TESTVARIANT.json ] ; then
    echo "$TESTVARIANT.json does not exist, ensure that the client list file has been created for this test variation"
    exit 1
fi

# Initial Message rate is 1 msgs/sec (-r 1), the message rate can be changed dynamically on the interactive mqttbench command shell with command: rate=<msgs/sec>
# Min-Max message size is based on the parameter passed to this script (-s ${MSGSIZE}-${MSGSIZE})
# Message latency histogram units are in microseconds (-u 1e-6)  
# Latency statistics are reset every 300 seconds (-rl 300)
# mqttbench instance ID (-i ${MBINSTID})
mqttbench -i ${MBINSTID} -cl $TESTVARIANT.json -r 1 -s ${MSGSIZE}-${MSGSIZE} -T 0x1 -snap connects,msgrates,msgcounts,msglatency 4 -u 1e-6 -csv $TESTVARIANT.stats.csv -lcsv $TESTVARIANT.latency.csv -rl 300 

