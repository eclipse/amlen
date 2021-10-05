#!/bin/bash

# -----------------------------------------------------------------------------------------------------
#
# This script executes the CONNBURST.NONSECURE benchmark test, see README.md for test details
#
# -----------------------------------------------------------------------------------------------------

export GraphiteIP=<hostname of Graphite relay>
export TESTNAME=CONNBURST-NONSECURE
export GraphiteMetricRoot=$TESTNAME

# delay between new connections (units in microseconds)
export DelayTime=60
export DelayCount=1

# Reset connection latency statistics every 180 seconds (-rl 180) and latency histogram units are in microseconds (-cu 1e-6)
# Client remain connected for 180 seconds (-l 180) 
mqttbench -cl $TESTNAME.json -T 0x20 -snap connects,connlatency 4 -cu 1e-6 -csv $TESTNAME.stats.csv -lcsv $TESTNAME.latency.csv -rl 180 -l 180
