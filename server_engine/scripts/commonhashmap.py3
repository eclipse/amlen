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
#import sys
#import gdb

def listElements(hashmap_varname):
    hashmap = gdb.parse_and_eval(hashmap_varname)

    for i in range(0, hashmap['capacity']):
        if hashmap['elements'][i]:
            print("Element "+str(i)+" exists")
