#!/bin/bash

if [[ -f "../testEnv.sh" ]]
then
    source "../testEnv.sh"
else
    source ../scripts/ISMsetup.sh
fi

scp ${P1_USER}@${P1_HOST}:${P1_PROXYROOT}/*.log ./
