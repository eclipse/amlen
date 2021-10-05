#!/bin/bash
# Copyright (c) 2018-2021 Contributors to the Eclipse Foundation
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
INITLOG=/var/imaproxy/diag/logs/imaproxy_init.log
export INITLOG
mkdir -p -m 770 $(dirname $INITLOG)
CURDIR=`pwd`
export CURDIR

touch ${INITLOG}

echo "-------------------------------------------------------------------"  >> ${INITLOG}
echo "Initialize IBM IoT MessageSight Proxy " >> ${INITLOG}
echo "Date: `date` " >> ${INITLOG}


isDocker=0
if [ -f /.dockerenv ]
then
    isDocker=1
fi


if [ $isDocker -eq 1 ]
then
    # Find container UUID
    UUID=`cat /proc/self/cgroup | grep -o  -e "docker-.*.scope" | head -n 1 | sed "s/docker-\(.*\).scope/\\1/"`
    # Alternative form of information containing container ID
    if [ "${UUID}" == "" ]
    then
        UUID=`cat /proc/self/cgroup | grep -o -e ".*:/docker/.*" | head -n 1 | sed "s/.*:\/docker\/\(.*\)/\\1/"`
    fi
    SHORT_UUID=`echo ${UUID} | cut -c1-12`
    export SHORT_UUID
    echo "Start MessageSight Proxy container: ${SHORT_UUID}" >> ${INITLOG}
else
    SHORT_UUID="imaproxy"
    export SHORT_UUID
    echo "Start MessageSight Proxy instance: ${SHORT_UUID}" >> ${INITLOG}
fi

# Predefined directory locations in the container
IMADATADIR=/var/imaproxy/data
IMALOGDIR=/var/imaproxy/diag/logs
IMACOREDIR=/var/imaproxy/diag/cores
IMACFGDIR=/var/imaproxy
IMASERVERCFG=/opt/ibm/imaproxy/bin/imaproxy.cfg
IMADYNPROXYCFG=/var/imaproxy/proxy.cfg

# Initialize instance if required
if [ ! -f ${IMACFGDIR}/MessageSightInstance.inited ]
then
    /opt/ibm/imaproxy/bin/initImaserverInstance.sh >> ${INITLOG}
fi

# Initialize container specific data
if [ ! -f ${IMACFGDIR}/.serverCFGUpdated ]
then

    if [ ! -f ${IMADYNPROXYCFG} ]
    then

        if [ ! -z ${IMAPROXY_ADMINPORT+x} ]  # Check if IMAPROXY_ADMINPORT has been set in environment
        then
            # IMAPROXY_ADMINPORT=${IMAPROXY_ADMINPORT:-9082}
            IMAPROXY_ADMINIFACE=${IMAPROXY_ADMINIFACE:-localhost}
            IMAPROXY_ADMINSECURE=${IMAPROXY_ADMINSECURE:-true}
            IMAPROXY_ADMINUSER=${IMAPROXY_ADMINUSER:-adminUser}
            IMAPROXY_ADMINPW=${IMAPROXY_ADMINPW:-adminPassword}

            re='^[0-9]+$'
            if [[ ! $IMAPROXY_ADMINPORT =~ $re ]] ||  [[ $IMAPROXY_ADMINPORT -lt 1 ]] ||  [[ $IMAPROXY_ADMINPORT -gt 65535 ]]
            then
                echo "IMAPROXY_ADMINPORT is set as $IMAPROXY_ADMINPORT .  A number between 1 and 65535 is expected."
                exit 1
            fi

            if [[ $IMAPROXY_ADMINSECURE != "true" ]] && [[ $IMAPROXY_ADMINSECURE != "false" ]]
            then
                echo "IMAPROXY_ADMINSECURE is set as $IMAPROXY_ADMINSECURE . true is set now."
                IMAPROXY_ADMINSECURE="true"
            fi

            cat > $IMADYNPROXYCFG <<EOF
{
    "Endpoint": {
        "admin": {
            "Port": $IMAPROXY_ADMINPORT,
            "Interface": "$IMAPROXY_ADMINIFACE",
            "Secure": $IMAPROXY_ADMINSECURE,
            "Protocol": "Admin",
            "Method": "TLSv1.2",
            "Certificate": "imaproxy_default_cert.pem",
            "Key": "imaproxy_default_key.pem",
            "EnableAbout": true,
            "Authentication": "basic"
        }
    },
    "User": {
        "$IMAPROXY_ADMINUSER": { "Password": "$IMAPROXY_ADMINPW" }
    }
}
EOF

        fi #if [ ! -z ${IMAPROXY_ADMINPORT+x} ]
        touch ${IMADYNPROXYCFG}
        touch ${IMACFGDIR}/.serverCFGUpdated
    fi
fi

if [ ! -f ${IMACFGDIR}/.defaultAdminCerts ]
then
    mkdir -p -m 770 /var/imaproxy/keystore

    if [[ ! -f /var/imaproxy/keystore/imaproxy_default_key.pem ]]  ||  [[ ! -f /var/imaproxy/keystore/imaproxy_default_cert.pem ]]
    then
        cp /opt/ibm/imaproxy/certificates/keystore/imaproxy_default_key.pem /var/imaproxy/keystore/imaproxy_default_key.pem
        cp /opt/ibm/imaproxy/certificates/keystore/imaproxy_default_cert.pem /var/imaproxy/keystore/imaproxy_default_cert.pem
    fi
    touch ${IMACFGDIR}/.defaultAdminCerts
fi


echo "IBM IoT MessageSight Proxy instance is initialized. " >> ${INITLOG}
echo "-------------------------------------------------------------------"  >> ${INITLOG}
echo  >> ${INITLOG}


