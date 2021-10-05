#!/bin/bash

# -----------------------------------------------------------------------------------------------------
#
# This script executes the following variant of the TPUT.FANIN.MQTT benchmark test.
#
# Variant:
# - QoS 2 (durable client sessions)
# - Topic tree is partitioned into non-overlapping segments (1 shared subscription per partition
#   and multiple subscribers per shared subscription)
# - X publishers  (each publishing on a unique topic)
# - Y subscribers (single shared subscription per partition)
#
# The script takes 4 parameters:
# - number of publishers  (in SI units, e.g. 1, 1K, 500K, 1M, etc.)
# - number of subscribers (in SI units, e.g. 1, 500, 1K, etc.)
# - the size (in bytes) of the message to publish.  
# - "deletestate" (an optional flag which when passed to this script will clean up client session
#                  states on the MessageSight server.  This can be useful, to quickly clean up durable
#                  client sessions/subscription from a previous test run. Alternatively, you could clean 
#                  the MessageSight store to delete client sessions/subscriptions.)  
#
# This variant of the TPUT.FANIN.MQTT benchmark should be executed with the following message sizes.
#
# Message Sizes (bytes): 64, 256, 1024, 4096, 16384, 65536, 262144
#
# see README.md for test details
# -----------------------------------------------------------------------------------------------------

MSGSIZE=0
NUMPUBBERS=0
NUMSUBBERS=0
DELETESTATE=0
if [ "$1" == "" -o "$2" == "" -o "$3" == "" ] ; then
    echo "usage: $0 <number of publishers (in SI units)> <number of subscribers (in SI units)> <msg size in bytes> [deletestate]"
    echo "  e.g. $0 1M 1 64"
    echo "  e.g. $0 1M 1 64 deletestate"
    exit 1
else
    NUMPUBBERS=$1
    NUMSUBBERS=$2
    MSGSIZE=$3
    if [ "$4" == "deletestate" ] ; then
        DELETESTATE=1
	fi
fi

export GraphiteIP=<hostname of Graphite relay>
export TESTNAME=TPUT-FANIN-MQTT
export TESTVARIANT=${TESTNAME}-Q2-HYBRID-${NUMPUBBERS}PUB-${NUMSUBBERS}SUB
export TESTVARIANT_DS=${TESTNAME}-Q2-HYBRID-${NUMPUBBERS}PUB-${NUMSUBBERS}SUB-DELETESTATE
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

if [ "$DELETESTATE" == "1" ] ; then
    # Clean up the client sessions
    
    echo "Cleaning up client sessions, CTRL+C or quit mqttbench once the last client has connected, at this point the client state should be gone after disconnect."
    mqttbench -cl $TESTVARIANT_DS.json -r 1 -s ${MSGSIZE}-${MSGSIZE}
else
    # Run the test

	# Initial Message rate is 1 msgs/sec (-r 1), the message rate can be changed dynamically on the interactive mqttbench command shell with command: rate=<msgs/sec>
	# Min-Max message size is based on the parameter passed to this script (-s ${MSGSIZE}-${MSGSIZE})
	# Message latency histogram units are in microseconds (-u 1e-6)  
	# Latency statistics are reset every 300 seconds (-rl 300)
	# Use 3 submitter threads (-st 3)
	mqttbench -cl $TESTVARIANT.json -r 1 -s ${MSGSIZE}-${MSGSIZE} -T 0x1 -snap connects,msgrates,msgcounts,msglatency 4 -u 1e-6 -csv $TESTVARIANT-MSG-${MSGSIZE}.stats.csv -lcsv $TESTVARIANT-MSG-${MSGSIZE}.latency.csv -rl 300 -st 3 
fi

