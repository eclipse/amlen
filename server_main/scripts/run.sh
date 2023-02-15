#!/bin/bash
# Copyright (c) 2015-2023 Contributors to the Eclipse Foundation
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

SECRETS_DIR=/secrets
DESTINATION=/tmp/userfiles/

mkdir -p -m 2770 ${DESTINATION}

cp /secrets/sslcerts/ca/ca_public_cert.pem ${DESTINATION}
cp /secrets/sslcerts/device/tls.crt ${DESTINATION}
cp /secrets/sslcerts/device/tls.key ${DESTINATION}

KEYSTORE=/var/lib/amlen/data/certificates/keystore/
mkdir -p -m 2770 ${KEYSTORE}
cp /secrets/sslcerts/device/tls.key ${KEYSTORE}
cp /secrets/sslcerts/device/tls.crt ${KEYSTORE}

TRUSTSTORE=/var/lib/amlen/data/certificates/truststore/IoTSecurityProfile
mkdir -p -m 2770 ${TRUSTSTORE}
cp /secrets/sslcerts/ca/ca_public_cert.pem ${TRUSTSTORE}

cp /secrets/sslcerts/ha/tls.key ${KEYSTORE}HA_tls.key
cp /secrets/sslcerts/ha/tls.crt ${KEYSTORE}HA_tls.crt
cp /secrets/sslcerts/ha/ca.crt /var/lib/amlen/data/certificates/truststore/HA_cafile.pem

exec /opt/ibm/imaserver/bin/startServer.sh
