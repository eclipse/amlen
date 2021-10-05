#!/bin/bash

# -----------------------------------------------------------------------------------------------------
#
# This script executes the SCALE.VERTICAL.CONNBURST.TLS-LDAPAUTH (TLS_AES_128_GCM_SHA256 cipher) benchmark test for a 
# MessageSight server provisioned with 24 CPUs. The following settings are calibrated for a 24 CPU
# server:
#
#   - DelayTime environment variable
#   - Reset latency command line parameter (-rl)
#   - Linger time command line parameter (-l)
#
# See README.md for test details
#
# -----------------------------------------------------------------------------------------------------

export GraphiteIP=<hostname of Graphite relay>
export TESTNAME=SCALE-VERTICAL-CONNBURST-TLS-LDAPAUTH
export TESTVARIANT=${TESTNAME}-24CPU
export GraphiteMetricRoot=$TESTVARIANT

# delay between new connections (units in microseconds)
export DelayTime=270
export DelayCount=1

# TLS settings
export SSLCipher=TLS_AES_128_GCM_SHA256
export SSLClientMeth=TLSv12

# Setup trust store
CERTSDIR=../../../../../certs
ROOTCA_HASH=`openssl x509 -hash -in $CERTSDIR/cert-rootCA.pem -noout`
mkdir -p certs
ln -sf ../$CERTSDIR/cert-rootCA.pem certs/${ROOTCA_HASH}.0

# Reset connection latency statistics every 330 seconds (-rl 330) and latency histogram units are in milliseconds (-cu 1e-3).  
# Clients remain connected for 330 seconds (-l 330)
mqttbench -cl $TESTNAME.json -T 0x20 -snap connects,connlatency 4 -cu 1e-3 -csv $TESTVARIANT.stats.csv -lcsv $TESTVARIANT.latency.csv -rl 330 -l 330 

