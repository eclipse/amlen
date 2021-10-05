#!/bin/bash

# -----------------------------------------------------------------------------------------------------
#
# This script executes the CONNBURST.TLS.NOAUTH (PSK-AES256-CBC-SHA cipher) benchmark test, 
# see README.md for test details
#
# -----------------------------------------------------------------------------------------------------

export GraphiteIP=<hostname of Graphite relay>
export TESTNAME=CONNBURST-TLS-NOAUTH
export TESTVARIANT=${TESTNAME}-PSK-AES256-CBC-SHA
export GraphiteMetricRoot=$TESTVARIANT

# delay between new connections (units in microseconds)
export DelayTime=70
export DelayCount=1

# TLS settings
export SSLCipher=PSK-AES256-CBC-SHA
export SSLClientMeth=TLSv12

# Setup trust store
CERTSDIR=../../../certs
ROOTCA_HASH=`openssl x509 -hash -in $CERTSDIR/cert-rootCA.pem -noout`
mkdir -p certs
ln -sf ../$CERTSDIR/cert-rootCA.pem certs/${ROOTCA_HASH}.0

# Uncompress PSK csv file
PSK_FILE=psk.csv
if [ -e ${CERTSDIR}/${PSK_FILE}.gz ] ; then
  gunzip ${CERTSDIR}/${PSK_FILE}.gz
fi 

# Reset connection latency statistics every 180 seconds (-rl 180) and latency histogram units are in microseconds (-cu 1e-6).  
# Clients remain connected for 180 seconds (-l 180)
mqttbench -cl $TESTNAME.json -T 0x20 -snap connects,connlatency 4 -cu 1e-6 -csv $TESTVARIANT.stats.csv -lcsv $TESTVARIANT.latency.csv -rl 180 -l 180 -psk ${CERTSDIR}/${PSK_FILE}

