#!/bin/bash
#
# MUST RUN THIS COMMAND SOURCED:   ". ../scripts/getImaVersion.sh"   or "source ../scripts/getImaVersion.sh"
# MUST HAVE SOURCED  ISMsetup.sh prior to running this script.
#   Output:
# SET Env Var "IMA_VERSION" with the version information based on the "show imaserver" command as a string in the form "Major#.minor#"
# This was needed when CLI Syntax changes were done in v1.2 and broke earlier test cases
#
# Returns:
# IMA_VERSION as an exported ENV Variable 

PRIMARY=`../scripts/getPrimaryServerHostAddr.sh`
#USER=`../scripts/getPrimaryServerHostAddr.sh`   NEED A Script for this...but this is safe today

versioncmd=`echo "show imaserver" | ssh ${A1_USER}@${PRIMARY} 2> null`

if [[ "${versioncmd}" =~ "version is 1.2" ]] ; then
    declare -rx IMA_VERSION="1.2"
elif [[ "${versioncmd}" =~ "version is 1.1" ]] ; then
    declare -rx IMA_VERSION="1.1"
elif [[ "${versioncmd}" =~ "version is 1.0" ]] ; then
    declare -rx IMA_VERSION="1.0"
else
    echo "Problem determining IMA Version - got data: `cat null`"
fi

`rm -f null`
