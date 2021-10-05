#!/bin/bash

# -----------------------------------------------------------------------------------------------------
#
# This script executes the following variant of the TPUT.FANIN.MQ benchmark test.
#
# Variant:
# - QoS 1 (durable client sessions)
# - MQ persistent queue 
# - X MQTT publishers (each publishing on a unique topic)
# - Y MQ subscribers
#
# The script takes 5 parameters:
# - the number of MQ QMgrs used in the test
# - the number of connections per MQ QMgr used in the test
# - number of MQTT publishers  (in SI units, e.g. 1, 1K, 500K, 1M, etc.)
# - number of MQ subscribers  (in SI units, e.g. 1, 5, 10, etc.)
# - the size (in bytes) of the message to receive (only used for stats purposes).
#
# This variant of the TPUT.FANIN.MQ benchmark should be executed with the following message sizes.
#
# Message Sizes (bytes): 64, 256, 1024, 4096, 16384, 65536, 262144
#
# see README.md for test details
# -----------------------------------------------------------------------------------------------------

NUMQMGR=0
NUMCONN_PER_QMGR=0
NUMPUBBERS=0
NUMSUBBERS=0
MSGSIZE=0
if [ "$1" == "" -o "$2" == "" -o "$3" == "" -o "$4" == "" -o "$5" == "" ] ; then
    echo "usage: $0 <numQMgrs> <numConnPerQMgr> <number of MQTT publishers> <number of MQ subscribers> <msg size in bytes>"
    echo "  e.g. $0 1 1 1M 1 2048"
    echo "  e.g. $0 5 10 10K 10 64"
    exit 1
else
    NUMQMGR=$1
    NUMCONN_PER_QMGR=$2
    NUMPUBBERS=$3
    NUMSUBBERS=$4
    MSGSIZE=$5
fi

export GraphiteIP=<hostname of Graphite relay>
export TESTNAME=TPUT-FANIN-MQ
export TESTVARIANT=${TESTNAME}-Q2-QMgrs-${NUMQMGR}-MQCONN-${NUMCONN_PER_QMGR}-${NUMPUBBERS}PUB-${NUMSUBBERS}SUB
export GraphiteMetricRoot=$TESTVARIANT-MSG-${MSGSIZE}
export CPH_INSTALLDIR=mqbench_cfg

# Create an instance of the MQ Receiver config (-tc Receiver)
# Open the IMA2MQ.P.QUEUE local queue object to receive from (-d IMA2MQ.P.QUEUE)
# Number of consumer worker threads (-nt $NUMSUBBERS)
# LD_LIBRARY_PATH set by perftools/benchmarks/baseenv.sh
mqbench -tc Receiver -d IMA2MQ.P.QUEUE -nt $NUMSUBBERS

