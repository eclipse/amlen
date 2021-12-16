#!/bin/bash

# Set environment for running ad hoc mqttbench testing

BASEDIR=$(dirname $(realpath $BASH_SOURCE))
WKSPDIR=$(dirname $BASEDIR)
#DEBUG=debug

export SSL_HOME=/usr/local/openssl
export ICU_HOME=/usr/local/icu4c

export PATH=${BASEDIR}/$DEBUG/bin:$PATH
export LD_LIBRARY_PATH=$WKSPDIR/server_ship/$DEBUG/lib:$SSL_HOME/lib:$ICU_HOME/lib:$LD_LIBRARY_PATH
echo "PATH: $PATH"
echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
