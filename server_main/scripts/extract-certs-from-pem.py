#!/usr/bin/env python3
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

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("pemfile")
args = parser.parse_args()

pemfile = args.pemfile
outfile = pemfile + ".crt"

myout = open(outfile, 'w+')

with open(pemfile, 'r') as mypem:
    current_cert = ""
    current_key = ""
    in_private_key = 0
    in_certificate = 0
    for line in mypem:
        if "BEGIN PRIVATE KEY" in line:
            in_private_key = 1
            in_certificate = 0
            current_key = current_key + line
            continue
        elif "END PRIVATE KEY" in line:
            in_private_key = 0
            current_key = current_key + line
            current_key = ""
            continue
        elif "BEGIN CERTIFICATE" in line:
            in_certificate = 1
            in_private_key = 0
            current_cert = current_cert + line
            continue
        elif "END CERTIFICATE" in line:
            in_certificate = 0
            current_cert = current_cert + line
            myout.write(current_cert)
            current_cert = ""
            continue
        elif in_private_key:
            current_key = current_key + line
            continue
        elif in_certificate:
            current_cert = current_cert + line
            continue

myout.close()
