#!/bin/sh
# Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

export WORKSPACE=$1

SERVERSHIPDIR=${WORKSPACE}/server_ship
MESSAGEDIR=${SERVERSHIPDIR}/messages
PATHPROPSCRIPT=${WORKSPACE}/server_build/path_parser.py

echo SERVERSHIPDIR=${SERVERSHIPDIR}
echo MESSAGEDIR=${MESSAGEDIR}
echo PATHPROPSCRIPT=${PATHPROPSCRIPT} 

# Unzip messages.zip
cd ${MESSAGEDIR}
unzip messages.zip
cd ${SERVERSHIPDIR}

#Replace variables in messages. ChkPii currently gets upset by them
python3 ${PATHPROPSCRIPT} -mdirreplace -i "${MESSAGEDIR}" -o "${MESSAGEDIR}"

# Run chkpii word count. CHKPII with option /WC will not detect errors.
CHKPII "${MESSAGEDIR}/*" /WC /UTF8 /EN /O ${SERVERSHIPDIR}/CHKPII.SUMWC /S

# Run chkpii to detect errors
CHKPII "${MESSAGEDIR}/*" /UTF8 /EN /O ${SERVERSHIPDIR}/CHKPII.SUMERR /S

