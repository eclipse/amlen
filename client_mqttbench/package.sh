#!/bin/bash

WKSP=$(realpath ..)
SRVSHIP=$WKSP/server_ship
OPENSSLDIR=/usr/local/openssl-1.1.1g/openssl-1.1.1g
ICUDIR=/usr/local/icu
SHIPDIR=ship

rm -rf $SHIPDIR
mkdir -p $SHIPDIR/mqttbench/lib
mkdir -p $SHIPDIR/mqttbench/bin
mkdir -p $SHIPDIR/mqttbench/debug/lib
mkdir -p $SHIPDIR/mqttbench/debug/bin
mkdir -p $SHIPDIR/clientlists
mkdir -p $SHIPDIR/grafana

# copy libs
cp $OPENSSLDIR/lib/libssl.so.1.1 $SHIPDIR/mqttbench/lib
cp $OPENSSLDIR/lib/libcrypto.so.1.1 $SHIPDIR/mqttbench/lib
cp $ICUDIR/lib/libicui18n.so.60 $SHIPDIR/mqttbench/lib
cp $ICUDIR/lib/libicudata.so.60 $SHIPDIR/mqttbench/lib
cp $ICUDIR/lib/libicuuc.so.60 $SHIPDIR/mqttbench/lib
cp $SRVSHIP/lib/libismutil.so $SHIPDIR/mqttbench/lib

# copy debug libs
cp $SRVSHIP/debug/lib/libismutil.so $SHIPDIR/mqttbench/debug/lib

# copy binary
cp $SRVSHIP/bin/mqttbench $SHIPDIR/mqttbench/bin

# copy debug binary
cp $SRVSHIP/debug/bin/mqttbench $SHIPDIR/mqttbench/debug/bin

# copy docs and other helper scripts
cp $WKSP/client_mqttbench/docs/*.docx $SHIPDIR/
cp -rf $WKSP/client_mqttbench/clientlists/*.py $SHIPDIR/clientlists
cp -rf $WKSP/client_mqttbench/clientlists/*.json $SHIPDIR/clientlists
cp -rf $WKSP/client_mqttbench/clientlists/messages $SHIPDIR/clientlists
cp -rf $WKSP/client_mqttbench/clientlists/legacyList.cl $SHIPDIR/clientlists

cd $SHIPDIR
BUILDID=$(date +%Y%m%d-%H%M%S)
tar zcvf mqttbench_$BUILDID.tgz *
mv mqttbench_$BUILDID.tgz ..
cd .. 
#rm -rf $SHIPDIR 








