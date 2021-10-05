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

# Restart openldap
${IMA_SVR_INSTALL_PATH}/bin/ldapSync.sh > /dev/null 2>&1

# Restart mqconnectivity
/sbin/initctl stop mqconnectivity > /dev/null 2>&1
/sbin/initctl start mqconnectivity > /dev/null 2>&1

exit 0
