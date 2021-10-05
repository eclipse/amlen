#!/usr/bin/python
# -*- coding: utf-8 -*-
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

#--------------------------------------
# Simple script to compare 2 json files
#--------------------------------------

import os, sys, time
import json
import time

def ordered(obj):
    if isinstance(obj, dict):
        return sorted((k, ordered(v)) for k, v in obj.items())
    if isinstance(obj, list):
        return sorted(ordered(x) for x in obj)
    else:
        return obj

##-------
## "MAIN"
##-------

if (len(sys.argv) < 2 or len(sys.argv) > 3):
    print 'Usage: compjsonfile.py file1 file2 [debug]'
    print 'debug: 1 = on, default = off'
    sys.exit(1)
else:
    file1 = sys.argv[1]
    file2 = sys.argv[2]

print "Comparing %s with %s." % (file1, file2)
json1 = json.loads(open(file1).read())
json2 = json.loads(open(file2).read())

if ordered(json1) == ordered(json2):
    print "SUCCESS! Expected and obtained JSON are equal."
else:
    print "Expected and obtained JSON are NOT equal." 

# sometimes we were completing too fast and start.sh was not able to get the PID	
time.sleep( 1 )
