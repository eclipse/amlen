#!/bin/bash

# -----------------------------------------------------------------------------------------------------
#
# This script executes the CONNBURST.TLS.CLIENTCERT benchmark test, 
# see README.md for test details
#
# -----------------------------------------------------------------------------------------------------

export GraphiteIP=<hostname of Graphite relay>
export TESTNAME=CONNBURST-TLS-CLIENTCERT
export TESTVARIANT=${TESTNAME}
export GraphiteMetricRoot=$TESTVARIANT

# delay between new connections (units in microseconds)
export DelayTime=200
export DelayCount=1

# TLS settings
export SSLCipher=TLS_AES_256_GCM_SHA384
export SSLClientMeth=TLSv12

# Setup trust store
CERTSDIR=../../../certs
ROOTCA_HASH=`openssl x509 -hash -in $CERTSDIR/cert-rootCA.pem -noout`
mkdir -p certs
ln -sf ../$CERTSDIR/cert-rootCA.pem certs/${ROOTCA_HASH}.0

# Reset connection latency statistics every 260 seconds (-rl 260) and latency histogram units are in milliseconds (-cu 1e-3).  
# Clients remain connected for 260 seconds (-l 260)
mqttbench -cl $TESTNAME.json -T 0x20 -snap connects,connlatency 4 -cu 1e-3 -csv $TESTVARIANT.stats.csv -lcsv $TESTVARIANT.latency.csv -rl 260 -l 260 

