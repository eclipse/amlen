#!/bin/bash
# -----------------------------------------------------------------------------------------------------
#
# This script executes the SCALE.VERTICAL.CONNBURST.TLS-NOAUTH (TLS_AES_128_GCM_SHA256 cipher) benchmark test for a 
# MessageSight server provisioned with 40 CPUs. The following settings are calibrated for a 40 CPU
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
export TESTNAME=SCALE-VERTICAL-CONNBURST-TLS-NOAUTH
export TESTVARIANT=${TESTNAME}-40CPU
export GraphiteMetricRoot=$TESTVARIANT

# delay between new connections (units in microseconds)
export DelayTime=150
export DelayCount=1

# TLS settings
export SSLCipher=TLS_AES_128_GCM_SHA256
export SSLClientMeth=TLSv12

# Setup trust store
CERTSDIR=../../../../../certs
ROOTCA_HASH=`openssl x509 -hash -in $CERTSDIR/cert-rootCA.pem -noout`
mkdir -p certs
ln -sf ../$CERTSDIR/cert-rootCA.pem certs/${ROOTCA_HASH}.0

# Reset connection latency statistics every 210 seconds (-rl 210) and latency histogram units are in milliseconds (-cu 1e-3).  
# Clients remain connected for 210 seconds (-l 210)
mqttbench -cl $TESTNAME.json -T 0x20 -snap connects,connlatency 4 -cu 1e-3 -csv $TESTVARIANT.stats.csv -lcsv $TESTVARIANT.latency.csv -rl 210 -l 210 

