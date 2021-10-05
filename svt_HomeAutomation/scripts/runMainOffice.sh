#!/bin/bash

SERVER=$1
#if [[ -z ${SERVER} ]]; then
#  SERVER=`/niagara/test/scripts/getserverpriv.py`
#fi

CARS=500
VERBOSE=false
SSL=true
#PORT=5004
PORT=$2
VERBOSE_CONNECT=true
CACERTS=/mnt/pub/release/CURREL/test/latest/svt/svt_jmqtt/ssl/cacerts.jks
PASSWORD=k1ngf1sh

export CLASSPATH=/root/svt_HomeAutomation/bin:/root/svt_HomeAutomation/json-simple-1.1.1.jar:/mnt/pub/BUILD_TOOLS_FROM_RTC/applications/devworks_CandI_clients/IMA14a/SDK/clients/java/org.eclipse.paho.client.mqttv3-1.0.3-20150218.150817-5.jar:$CLASSPATH
echo $CLASSPATH

CLIENTTYPE=0

printf -v ID "c%08d" $2

export MALLOC_ARENA_MAX=2

echo java -cp $CLASSPATH -Xss128K -Xzero -Xms128M -Xmx128M -Xshareclasses -Djavax.net.debug=false -Djavax.net.ssl.trustStore=${CACERTS} -Djavax.net.ssl.trustStorePassword=${PASSWORD} svt.home.SVTHome_Engine -server "${SERVER}" -mode "connect_once" -userName ${ID} -password "svtpvtpass" -port ${PORT} -qos 0 -paho ${CARS} -messages 1000000 -vverbose ${VERBOSE} -listener false -ssl ${SSL} -cleanSession false -rate 60 -verbose_connect ${VERBOsE_CONNECT} -genClientID false -verbose_connectionLost true -reconnectDelay 60000  -keepAlive 900 -connectionTimeout 600 -connectionThrottle 5 -clientType $CLIENTTYPE
java -Xss128K -Xzero -Xms128M -Xmx128M -Xshareclasses -Djavax.net.debug=false -Djavax.net.ssl.trustStore=${CACERTS} -Djavax.net.ssl.trustStorePassword=${PASSWORD} svt.home.SVTHome_Engine -server "${SERVER}" -mode "connect_once" -userName ${ID} -password "svtpvtpass" -port ${PORT} -qos 0 -paho ${CARS} -messages 1000000 -vverbose ${VERBOSE} -listener false -ssl ${SSL} -cleanSession false -rate 60 -verbose_connect ${VERBOsE_CONNECT} -genClientID false -verbose_connectionLost true -reconnectDelay 60000 -keepAlive 900 -connectionTimeout 600 -connectionThrottle 5 -clientType CLIENTTYPE 

