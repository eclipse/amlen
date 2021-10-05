#!/bin/bash

echo "Must run as root or sudo"
#############################################################
# First parameter is components. msproxy|messagesight|all
# Second parameter is the BUILD ID from MessageSight build
############################################################
BUILD_COMP=$1
BUILD_ID=$2

#############################################################
# Ask for Username & Password
# This is IBM Intranet Credentials
############################################################
read -p 'Username: ' USERNAME
read -sp 'Password: ' PASSWORD

echo "username $USERNAME"
echo "build_type $BUILD_COMP"
echo "build_id $BUILD_ID"

ARTIFACTORY_URL="https://na.artifactory.swg-devops.com/artifactory/wiotp-generic-local/dependencies"

echo "Start Publishing"
##############################################################
# Update MSPROXY Files
#############################################################

if  [ $BUILD_COMP == "msproxy" ]   ||  [ $BUILD_COMP == "msproxyall" ] ||  [ $BUILD_COMP == "all" ]
then
    MSPROXY_COMP="msproxy"

    echo "MSPROXY Build Publish: $BUILD_ID"
    curl -u $USERNAME:$PASSWORD -X PUT "$ARTIFACTORY_URL/$MSPROXY_COMP/$BUILD_ID/libimaproxy.so" -T /gsacache/release/iotf/production/$BUILD_ID/proxy/server_ship/lib/libimaproxy.so
    curl -u $USERNAME:$PASSWORD -X PUT "$ARTIFACTORY_URL/$MSPROXY_COMP/$BUILD_ID/libismutil.so" -T /gsacache/release/iotf/production/$BUILD_ID/proxy/server_ship/lib/libismutil.so
    curl -u $USERNAME:$PASSWORD -X PUT "$ARTIFACTORY_URL/$MSPROXY_COMP/$BUILD_ID/imaproxy" -T /gsacache/release/iotf/production/$BUILD_ID/proxy/server_ship/bin/imaproxy
    curl -u $USERNAME:$PASSWORD -X PUT "$ARTIFACTORY_URL/$MSPROXY_COMP/$BUILD_ID/imaproxy_config.jar" -T /gsacache/release/iotf/production/$BUILD_ID/proxy/client_ship/lib/imaproxy_config.jar
fi

###################################################################
# Upload MessageSight Files
###################################################################
if [ $BUILD_COMP == "messagesight" ] ||  [ $BUILD_COMP == "all" ]
then
    VERSION=$(find /gsacache/release/iotf/production/$BUILD_ID -type f -name "IBMWIoTPMessageGatewayServer-*" -print | awk 'match($0, /-([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)\..*/,arr) {print arr[1]}')
    MSSERVER_COMP="messagesight"

    echo "MSServer Build Publish: $BUILD_ID $VERSION"
    curl -u $USERNAME:$PASSWORD -X PUT "$ARTIFACTORY_URL/$MSSERVER_COMP/$BUILD_ID/IBMWIoTPMessageGatewayServer-$VERSION.$BUILD_ID.tz" -T /gsacache/release/iotf/production/$BUILD_ID/appliance/IBMWIoTPMessageGatewayServer-$VERSION.$BUILD_ID.tz
    curl -u $USERNAME:$PASSWORD -X PUT "$ARTIFACTORY_URL/$MSSERVER_COMP/$BUILD_ID/IBMWIoTPMessageGatewayWebUI-$VERSION.$BUILD_ID.tz" -T /gsacache/release/iotf/production/$BUILD_ID/appliance/IBMWIoTPMessageGatewayWebUI-$VERSION.$BUILD_ID.tz
fi

###################################################################
# Upload Msproxy APM
###################################################################
if  [ $BUILD_COMP == "msproxyapm" ]  ||  [ $BUILD_COMP == "msproxyall" ] ||  [ $BUILD_COMP == "all" ]
then
    MSPROXY_COMP="msproxy"

    echo "MSPROXY Build Publish: $BUILD_ID"
    curl -u $USERNAME:$PASSWORD -X PUT "$ARTIFACTORY_URL/$MSPROXY_COMP/apm/$BUILD_ID/libimaproxyapm.so" -T /gsacache/release/iotf/production/$BUILD_ID/proxy/server_ship/lib/libimaproxyapm.so
    curl -u $USERNAME:$PASSWORD -X PUT "$ARTIFACTORY_URL/$MSPROXY_COMP/apm/$BUILD_ID/libismutil.so" -T /gsacache/release/iotf/production/$BUILD_ID/proxy/server_ship/lib/libismutil.so
    curl -u $USERNAME:$PASSWORD -X PUT "$ARTIFACTORY_URL/$MSPROXY_COMP/apm/$BUILD_ID/imaproxyapm" -T /gsacache/release/iotf/production/$BUILD_ID/proxy/server_ship/bin/imaproxyapm
    curl -u $USERNAME:$PASSWORD -X PUT "$ARTIFACTORY_URL/$MSPROXY_COMP/apm/$BUILD_ID/imaproxy_config.jar" -T /gsacache/release/iotf/production/$BUILD_ID/proxy/client_ship/lib/imaproxy_config.jar
fi

echo "Finished Publishing"
