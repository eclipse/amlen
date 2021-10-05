#!/usr/bin/python
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
import base64

password = "ima15adev"

encoded = base64.b64encode(''.join([chr(ord(c)^ord('_')) for c in password]))
print encoded

plain = ''.join([chr(ord(c)^ord('_')) for c in base64.b64decode(encoded)])
print plain

