#!/bin/bash
# Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

PLATFORM_TYPE=0

PLATFORM_DATA=${IMA_SVR_DATA_PATH}/data/config/platform.dat
export PLATFORM_DATA

# If dat file is already created before, get PLATFORM_LICE type
LIC=Development
if [ -f $PLATFORM_DATA ]
then
    LIC=`grep "PLATFORM_LICE" $PLATFORM_DATA | cut -d":" -f2`
    if [ "${LIC}" == "" ]
    then
        LIC=Development
    else
        exit 0
    fi
fi

# Generate internal Serial number
SNO=`uuidgen | cut -d "-" -f1`
if [ "${SNO}" == "" ]
then
    SNO="11111111"
fi

# Take the last 7 chars from serial number
nc=$((${#SNO}-7))
if [ $nc -le 0 ]
then
    PLATFORM_SNUM=${SNO}
else
    PLATFORM_SNUM=`echo ${SNO:$nc:7}`
fi
export PLATFORM_SNUM

   
# check Hypervisor vendor
HYPERVISOR=`/usr/bin/lscpu | grep "Hypervisor vendor" | cut -d":" -f2 | sed 's/ //g'`

# Check for XEN
if [ "${HYPERVISOR}" == "Xen" ]
then
    echo "PLATFORM_ISVM:1" > $PLATFORM_DATA
    echo "PLATFORM_TYPE:5" >> $PLATFORM_DATA
    echo "PLATFORM_VEND:XEN" >> $PLATFORM_DATA
    echo "PLATFORM_LICE:$LIC" >> $PLATFORM_DATA
    echo "PLATFORM_SNUM:${PLATFORM_SNUM}" >> $PLATFORM_DATA
    exit 0
fi

# Check for VMware
if [ "${HYPERVISOR}" == "VMware" ]
then
    echo "PLATFORM_ISVM:1" > $PLATFORM_DATA
    echo "PLATFORM_TYPE:4" >> $PLATFORM_DATA
    echo "PLATFORM_VEND:VMware" >> $PLATFORM_DATA
    echo "PLATFORM_LICE:$LIC" >> $PLATFORM_DATA
    echo "PLATFORM_SNUM:${PLATFORM_SNUM}" >> $PLATFORM_DATA
    exit 0
fi

# Check for VirtualBox
if [ "${PRODUCTNAME}" == "VirtualBox" ]
then
    echo "PLATFORM_ISVM:1" > $PLATFORM_DATA
    echo "PLATFORM_TYPE:6" >> $PLATFORM_DATA
    echo "PLATFORM_VEND:Oracle" >> $PLATFORM_DATA
    echo "PLATFORM_LICE:$LIC" >> $PLATFORM_DATA
    echo "PLATFORM_SNUM:${PLATFORM_SNUM}" >> $PLATFORM_DATA
    exit 0
fi

# Check for KVM
isKVM=0
if [ "${PRODUCTNAME}" == "KVM" ]
then
    isKVM=1
fi

# on KVMs, dmidecode may not be in exec PATH
if [ $isKVM -eq 0 ]
then
    /usr/sbin/dmidecode | grep -v grep | grep "KVM" > /dev/null 2>&1 3>&1
    if [ $? -eq 0 ]
    then
        isKVM=1
    fi
fi

if [ $isKVM -eq 1 ]
then
    echo "PLATFORM_ISVM:1" > $PLATFORM_DATA
    echo "PLATFORM_TYPE:7" >> $PLATFORM_DATA
    echo "PLATFORM_VEND:Linux" >> $PLATFORM_DATA
    echo "PLATFORM_LICE:$LIC" >> $PLATFORM_DATA
    echo "PLATFORM_SNUM:${PLATFORM_SNUM}" >> $PLATFORM_DATA
    exit 0
fi

   
# Check for AZURE
if [ "${PRODUCTNAME}" == "Virtual Machine" ]
then
    dmidecode -s system-manufacturer | grep "Microsoft" > /dev/null 2>&1
    if [ $? -eq 0 ]
    then
        echo "PLATFORM_ISVM:1" > $PLATFORM_DATA
        echo "PLATFORM_TYPE:8" >> $PLATFORM_DATA
        echo "PLATFORM_VEND:Microsoft" >> $PLATFORM_DATA
        echo "PLATFORM_LICE:$LIC" >> $PLATFORM_DATA
        echo "PLATFORM_SNUM:${PLATFORM_SNUM}" >> $PLATFORM_DATA
        exit 0
    fi
fi

# Check for docker
df -k | grep "mapper/docker" > /dev/null 2>&1
if [ $? -eq 0 ]
then
    echo "PLATFORM_ISVM:1" > $PLATFORM_DATA
    echo "PLATFORM_TYPE:10" >> $PLATFORM_DATA
    echo "PLATFORM_VEND:Docker" >> $PLATFORM_DATA
    echo "PLATFORM_LICE:$LIC" >> $PLATFORM_DATA
    echo "PLATFORM_SNUM:${PLATFORM_SNUM}" >> $PLATFORM_DATA
    exit 0
fi

# Unknown system
echo "PLATFORM_ISVM:0" > $PLATFORM_DATA
echo "PLATFORM_TYPE:11" >> $PLATFORM_DATA
echo "PLATFORM_VEND:Unknown" >> $PLATFORM_DATA
echo "PLATFORM_LICE:$LIC" >> $PLATFORM_DATA
echo "PLATFORM_SNUM:${PLATFORM_SNUM}" >> $PLATFORM_DATA
exit 0
