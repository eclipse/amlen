#!/bin/bash

# -----------------------------------------------------------------------------------------------------
#
# This script executes the CONNBURST.TLS.OAUTH benchmark test, see README.md for test details
#
# -----------------------------------------------------------------------------------------------------

export GraphiteIP=<hostname of Graphite relay>
export TESTNAME=CONNBURST-TLS-OAUTH
export GraphiteMetricRoot=$TESTNAME

# delay between new connections (units in microseconds)
export DelayTime=240
export DelayCount=1

# TLS settings
export SSLCipher=TLS_AES_256_GCM_SHA384
export SSLClientMeth=TLSv12

# Setup trust store
CERTSDIR=../../../certs
ROOTCA_HASH=`openssl x509 -hash -in $CERTSDIR/cert-rootCA.pem -noout`
mkdir -p certs
ln -sf ../$CERTSDIR/cert-rootCA.pem certs/${ROOTCA_HASH}.0

# Reset connection latency statistics every 300 seconds (-rl 300) and latency histogram units are in microseconds (-cu 1e-3)
# Client remain connected for 300 seconds (-l 300) 
mqttbench -cl $TESTNAME.json -T 0x20 -snap connects,connlatency 4 -cu 1e-3 -csv $TESTNAME.stats.csv -lcsv $TESTNAME.latency.csv -rl 300 -l 300  

