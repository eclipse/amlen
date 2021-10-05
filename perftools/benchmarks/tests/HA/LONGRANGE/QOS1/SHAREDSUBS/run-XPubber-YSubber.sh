#!/bin/bash

# -----------------------------------------------------------------------------------------------------
#
# This script executes the following variant of the HA.LONGRANGE benchmark test.
#
# Variant:
# - QoS 1 (durable client sessions)
# - MQTT v5 shared subscriptions
# - X publishers  (each publishing on a unique topic)
# - Y subscribers (single wildcard subscription to receive all messages)
#
# The script takes 5 parameters:
# - number of publishers  (in SI units, e.g. 100K or 1M)
# - number of subscribers (in SI units, e.g. 1K)
# - the size (in bytes) of the message to publish.  
# - the Standby SoftLayer Data Center (dal12, wdc06, or lon06)
# - "deletestate" (an optional flag which when passed to this script will clean up client session
#                  states on the MessageSight server.  This can be useful, to quickly clean up durable
#                  client sessions/subscription from a previous test run. Alternatively, you could clean 
#                  the MessageSight store to delete client sessions/subscriptions.)  
#
# This variant of the HA.LONGRANGE benchmark should be executed with the following message sizes
# and remote data centers for the MessageSight STANDBY server.
#
# Message Sizes (bytes): 64, 1024, 4096, 65536
# Standby SoftLayer Data Centers: dal12, wdc06, lon06
#
# see README.md for test details
# -----------------------------------------------------------------------------------------------------

MSGSIZE=0
NUMPUBBERS=0
NUMSUBBERS=0
REMOTEDC=""
DELETESTATE=0
if [ "$1" == "" -o "$2" == "" -o "$3" == "" -o "$4" == "" -o "$5" == "" ] ; then
    echo "usage: $0 <number of publishers (in SI units)> <number of subscribers (in SI units)> <msg size in bytes> <remote data center> [deletestate]"
    echo "  e.g. $0 1M 1K 64 wdc06"
    echo "  e.g. $0 1M 1K 64 wdc06 deletestate"
    exit 1
else
    NUMPUBBERS=$1
    NUMSUBBERS=$2
    MSGSIZE=$3
    REMOTEDC=$4
    if [ "$5" == "deletestate" ] ; then
        DELETESTATE=1
    fi
fi

export GraphiteIP=<hostname of Graphite relay>
export TESTNAME=HA-LONGRANGE
export TESTVARIANT=${TESTNAME}-Q1-SSUBS-${NUMPUBBERS}PUB-${NUMSUBBERS}SUB
export TESTVARIANT_DS=${TESTNAME}-Q1-SSUBS-${NUMPUBBERS}PUB-${NUMSUBBERS}SUB-DELETESTATE
export GraphiteMetricRoot=$TESTVARIANT-MSG-${MSGSIZE}-DC-${REMOTEDC}

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
	# Message latency histogram units are in milliseconds (-u 1e-3)  
	# Latency statistics are reset every 300 seconds (-rl 300)
	# Use 3 submitter threads (-st 3)
	mqttbench -cl $TESTVARIANT.json -r 1 -s ${MSGSIZE}-${MSGSIZE} -T 0x1 -snap connects,msgrates,msgcounts,msglatency 4 -u 1e-3 -csv $TESTVARIANT.stats.csv -lcsv $TESTVARIANT.latency.csv -rl 300 -st 3 
fi

