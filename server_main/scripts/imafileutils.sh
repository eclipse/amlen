#!/bin/bash
# Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#

FORMAT=JSON
function validate_url
{
    # check URL format.
    #Currently, only scp://user@ip:/filepath/filename is supported
    if [[ $URL != scp://* ]] ; then
        echo "URL has wrong format. Only scp is supported."
        exit 1
    fi
}

function validate_json
{
    # suffix checking is currently disabled
    SUFFIX=$(echo $FILENAME | /usr/bin/awk -F'.' '{print $NF}')
    # check filename
    if [ `echo $SUFFIX | /usr/bin/tr [:upper:] [:lower:]` !=  `echo $FORMAT | /usr/bin/tr [:upper:] [:lower:]` ] ; then
        echo "The file to be imported/exported has to be named with .json suffix."
        exit 1;
    fi
}

ACTION=$1

if [ ${ACTION} == "remove" ]
then
    if [ $# -eq 2 ] ; then
        URL=$2
    else
        echo "File name is needed"
        exit 1
    fi

    FILENAME=$(echo $URL | /usr/bin/awk -F'/' '{print $NF}')
    /bin/rm -rf ${USERFILESDIR}/$FILENAME
    error=$?
    if [ $error -ne 0 ] ; then
        exit 1
    fi
    exit 0
fi

if [ ${ACTION} == "convert" ]
then
    FILE=$2

    # convert files using perl
    perl -pi -e 's/\r\n$/\n/g' $FILE >/dev/null 2>&1 3>&1
    rc=$?
    if [ $rc != 0 ] ; then
        exit 1
    fi
    exit 0
fi

if [ ${ACTION} == "deleteProfileDir" ]
then
    tdir=$2
    profile=$3

    if [ -d $tdir ]
    then
        cd $tdir
        echo "Delete ${profile}" > /dev/null 2>&1 3>&1
        rm -rf ${profile} > /dev/null 2>&1 3>&1
        echo "Delete ${profile}_allowedClientCerts" > /dev/null 2>&1 3>&1
        rm -rf ${profile}_allowedClientCerts > /dev/null 2>&1 3>&1
        rm -rf ${profile}_capath > /dev/null 2>&1 3>&1
        rm -f ${profile}_cafile.pem > /dev/null 2>&1 3>&1
        rm -rf ${profile}_crl > /dev/null 2>&1 3>&1
    fi
    exit 0
fi

echo "Unknown ACTION : $ACTION"
exit 1
