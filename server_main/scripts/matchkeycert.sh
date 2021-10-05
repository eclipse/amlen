#!/bin/sh
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

CERTNAME=$1
KEYNAME=$2

CERTMD5=$(/usr/bin/openssl x509 -noout -modulus -in "$CERTNAME" | openssl md5)
#echo $CERTMD5
KEYMD5=$(/usr/bin/openssl rsa -noout -modulus -in "$KEYNAME" | openssl md5)
#echo $KEYMD5

if [[ $CERTMD5 = $KEYMD5 ]] ; then
#echo "it is a match"
exit 0
fi
#echo "it is NOT a match"
exit 1
