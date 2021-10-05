#!/bin/sh
#------------------------------------------------------------------------------
# gencrts.sh
#
# Create various ssl certificates for testing purposes.
#
#
# rootCA A - self signed root certificate
#   imaservera - server certificate signed by rootCA A - serial 1
#   imaclienta - client certificate signed by rootCA A - serial 2
#
#   imaCA A - intermediate CA signed by rootCA A - serial 3
#     imaserverb - server certificate signed by imaCA A - serial 1
#     imaclientb - client certificate signed by imaCA A - serial 2
#
#     imaCA B - intermediate CA signed by imaCA A - serial 3
#       imaserverc - server certificate signed by imaCA B - serial 1
#       imaclientc - client certificate signed by imaCA B - serial 2
#
# rootCA B - self signed root certificate
#   imaCA C - intermediate CA signed by rootCA B - serial 1
#     imaserverd - server certificate signed by imaCA C - serial 1
#     imaclientd - client certificate signed by imaCA C - serial 2
#
# The certificates checked into automation expire  September 5 14:49:27 2023 GMT
#------------------------------------------------------------------------------

#if [ "$1" == "" -o "$2" == "" ] ; then
#  echo "usage: $0 <cert size> <dir to create>"
#  echo "  e.g. $0 2048 certs2Kb"
#  exit 1
#fi

if [[ -f "../testEnv.sh" ]]
then
   # Official ISM Automation machine assumed
   source ../testEnv.sh
else
   # Manual Runs need to build Customized ISMSetup for THIS param, SSH environment file and Tag Replacement
   ../scripts/prepareTestEnvParameters.sh
   source ../scripts/ISMsetup.sh
fi

source ../scripts/commonFunctions.sh

set -x

DIR="certs"
CURRDIR=$(pwd)

cd $DIR || exit 2

for a in $(seq 1 $A_COUNT); do
    if [ ! -d "A${a}" ]; then
        mkdir "A${a}" || exit 2
    else
        rm -rf "A${a}"/*
        mkdir -p "A${a}"
    fi

    echo "Creating certs for A${a} ..."
    ./gen_dyn_certs_A${a}.sh
done

cd $CURRDIR || exit 2

if (( M_COUNT > 1 )); then
    echo "Copying certs to second client ..."
    scp -r certs ${M2_USER}@${M2_HOST}:${M2_TESTROOT}/mqtt_td_tests
fi
