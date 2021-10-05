# Copyright (c) 2021 Contributors to the Eclipse Foundation
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
print 'Checking for paused endpoints'
endpoints = AdminControl.queryNames('*:type=J2CMessageEndpoint,*').splitlines()
for endpoint in endpoints:
    status = AdminControl.invoke(endpoint,'getStatus')
    if status != "1":
        AdminControl.invoke(endpoint,'resume')
        print 'Resuming Endpoint:'
        print endpoint
print 'Resume Endpoints Complete'
