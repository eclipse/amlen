#!/bin/sh

# -----------------------------------------------------------------------------------------------------
#
# This script creates and configures an MQ QMgr for the TPUT.FANIN.MQ benchmark test.
#
# Steps:
# * Add current user to mqm user group and exit script (if not already a member of mqm group)
# * Tune shared memory/semaphore kernel parameters for MQ
# * Kill/Delete any existing QMgr
# * Create new QMgr
# * Start the QMgr
# * Configure QMgr objects and start the listener
# -----------------------------------------------------------------------------------------------------

WMQ_INSTALL_DIR=/opt/mqm
NUMQ=10
QDEPTH=1000000
QM=PerfQMgr
BOND0_IP=`ip -4 addr show dev bond0 | grep -oP "(?<=inet ).*(?=/)"`

source $WMQ_INSTALL_DIR/bin/setmqenv -p $WMQ_INSTALL_DIR >/dev/null

if [ "$1" == "" ] ; then
  echo "usage: $0 <QMgr_DATA_HOME>"
  echo " e.g.: $0 ~/perfqm-data 0"
  echo "       QMgr_DATA_HOME specifies where Queue Manager data and logs are to be written"
  exit
fi

QM_DATA_HOME=$1

# Check that this user is a member of the mqm group
chkmqm=`id | grep mqm | wc -l`
if [ "$chkmqm" == "0" ] ; then
 # Make sure this user is a member of the mqm UNIX group
 echo "usermod -aG mqm $USER" | sudo sh
 echo "$USER was added to group mqm.  Logout and login back in and rerun this script to create and start MQ QMgrs."
 exit
fi

# Make sure that the Linux kernel parameters are configured appropriately for running MQ QMgrs
sudo sysctl -w kernel.shmall=5368709120 >/dev/null 2>/dev/null
sudo sysctl -w kernel.shmmax=5368709120 >/dev/null 2>/dev/null
sudo sysctl -w kernel.sem="512 524288 256 4096" >/dev/null 2>/dev/null

# Cleanup/delete existing QMgr (if it exists)
chkqm=`dspmq | grep $QM | wc -l`
if [ "$chkqm" != "0" ] ; then
   endmqm -i $QM
   pkill -9 runmqsc
   pkill -9 runmqlsr
   pkill -9 amqzxma0
   pkill -9 amqrrmfa
   dltmqm $QM
fi
rm -rf $QM_DATA_HOME

# Create the QMgr
MQDP=$QM_DATA_HOME/$QM/data
mkdir -p $MQDP ; chmod -R 777 $MQDP
mkdir -p $QM_DATA_HOME/$QM/logs ; chmod -R 777 $QM_DATA_HOME/$QM/logs
$WMQ_INSTALL_DIR/bin/crtmqm -u SYSTEM.DEAD.LETTER.QUEUE  -md $QM_DATA_HOME/$QM/data -ld $QM_DATA_HOME/$QM/logs -h 50000 -lc -lf 16384 -lp 16 $QM

# Configure and tune the Queue Manager
sed "s/LogBufferPages=0/LogBufferPages=512/" $MQDP/$QM/qm.ini > $MQDP/$QM/qm.ini.tmp
mv -f $MQDP/$QM/qm.ini.tmp $MQDP/$QM/qm.ini 2>/dev/null
QMINI=$MQDP/$QM/qm.ini

echo                                    >> $QMINI
echo "Channels:"                        >> $QMINI
echo "  MQIBindType=FASTPATH"           >> $QMINI
echo "  MaxChannels=5000"               >> $QMINI
echo "  MaxActiveChannels=5000"         >> $QMINI
echo                                    >> $QMINI
echo "TuningParameters:"                >> $QMINI
echo "  PubSubCacheSize=48"             >> $QMINI
echo "  SubscriberHashTableRows=10243"  >> $QMINI
echo "  TopicHashTableRows=10243"       >> $QMINI
echo "  SubNameHashTableRows=10243"     >> $QMINI
echo "  SubscriberHashTableRows=503"    >> $QMINI
echo "  TopicHashTableRows=10243"       >> $QMINI
echo "  SubNameHashTableRows=503"       >> $QMINI
echo "  DefaultQBufferSize=1048576"     >> $QMINI
echo "  DefaultPQBufferSize=1048576"    >> $QMINI

chown mqm:mqm $QMINI

# Start the Queue Manager
echo "Starting QMgr $QM"
echo 
sleep 2
$WMQ_INSTALL_DIR/bin/strmqm $QM

# Create base MQ objects on Queue Manager
# Wait 1 second for Queue Manager to start up
echo "Configuring base QMgr objects"
echo
sleep 1
sed "s/%ipaddr%/$BOND0_IP/g" perf.mqsc.template > perf.mqsc
$WMQ_INSTALL_DIR/bin/runmqsc $QM < perf.mqsc

# Create local queues
echo "Creating local queue"
echo
tmpmqsc=tmp.mqsc
for j in `seq 1 $NUMQ`
do
  QNAME=q$j
  echo "define qlocal($QNAME) maxdepth($QDEPTH)" >> $tmpmqsc
done

# Create local queue IMA2MQ.QUEUE for benchmark tests
echo "define qlocal(IMA2MQ.NP.QUEUE) maxdepth($QDEPTH) defpsist(no)" >> $tmpmqsc
echo "define qlocal(IMA2MQ.P.QUEUE) maxdepth($QDEPTH) defpsist(yes)" >> $tmpmqsc

$WMQ_INSTALL_DIR/bin/runmqsc $QM < $tmpmqsc
rm -f $tmpmqsc

dspmq

