#!/bin/bash

# -----------------------------------------------------------------------------------------------------
#
# This script executes the CONNBURST.TLS.NOAUTH (ECDHE-RSA-AES256-GCM-SHA384 cipher) benchmark test, 
# see README.md for test details
#
# -----------------------------------------------------------------------------------------------------

export GraphiteIP=<hostname of Graphite relay>
export TESTNAME=CONNBURST-TLS-NOAUTH
export TESTVARIANT=${TESTNAME}-TLS_AES_256_GCM_SHA384
export GraphiteMetricRoot=$TESTVARIANT

# delay between new connections (units in microseconds)
export DelayTime=110
export DelayCount=1

# TLS settings
export SSLCipher=TLS_AES_256_GCM_SHA384
export SSLClientMeth=TLSv12

# Setup trust store
CERTSDIR=../../../certs
ROOTCA_HASH=`openssl x509 -hash -in $CERTSDIR/cert-rootCA.pem -noout`
mkdir -p certs
ln -sf ../$CERTSDIR/cert-rootCA.pem certs/${ROOTCA_HASH}.0

# Reset connection latency statistics every 170 seconds (-rl 170) and latency histogram units are in milliseconds (-cu 1e-3).  
# Clients remain connected for 170 seconds (-l 170)
mqttbench -cl $TESTNAME.json -T 0x20 -snap connects,connlatency 4 -cu 1e-3 -csv $TESTVARIANT.stats.csv -lcsv $TESTVARIANT.latency.csv -rl 170 -l 170 

