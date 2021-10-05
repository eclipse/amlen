#!/bin/bash

export WSHOME=~/workspace
export SRVRSHIP=${WSHOME}/application/server_ship
export CLNTSHIP=${WSHOME}/application/client_ship
ln -sf ${SRVRSHIP} ${WSHOME}/server_ship
ln -sf ${CLNTSHIP} ${WSHOME}/client_ship

export SRVRSHIPBIN=${SRVRSHIP}/bin
export JAVA_HOME=/opt/ibm/java-x86_64-80
export PERFTOOLSHOME=$WSHOME/perftools
export SLHOME=$PERFTOOLSHOME/softlayer
export TESTHOME=$WSHOME/test
export MQ_CLIENT_HOME=/opt/mqm
export SSL_HOME=/usr/local/openssl

if [ ! -e  /etc/redhat-release ] ; then
  if [ "1" == "`grep 'release.*7' /etc/redhat-release | wc -l`" ] ; then TUNINGHOME=$SLHOME/tuning/centos7 ; fi
fi

export PATH=$JAVA_HOME/bin:$SLHOME:$TUNINGHOME:${SRVRSHIPBIN}:$PATH
export LD_LIBRARY_PATH=${SSL_HOME}/lib:${SRVRSHIP}/${DBG}/lib:${CLNTSHIP}/${DBG}/lib64:${MQ_CLIENT_HOME}/lib64:${LD_LIBRARY_PATH}
export CLASSPATH=${CLNTSHIP}/lib/imaclientjms.jar:${CLNTSHIP}/lib/jms.jar:${CLNTSHIP}/test/client_jms:${CLASSPATH}
