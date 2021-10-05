#!/bin/bash

# -----------------------------------------------------------------------------------------------------
# This script installs MessageSight Bridge and starts the MessageSight Bridge System-D service
#
# -----------------------------------------------------------------------------------------------------

BASEDIR=`dirname "$0"`

echo "Installing MessageSight Bridge"
sleep 1
sudo yum install -y IBMWIoTPMessageGatewayBridge.x86_64

echo "Enabling and starting MessageSight Bridge service in System-D"
sudo systemctl enable imabridge
sudo systemctl start imabridge

