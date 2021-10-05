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
import os
import sys
import json
import logging
import requests
import csv

#csv_file = os.environ.get("CSV_DEVICE_FILE")
#json_file = os.environ.get("JSON_DEVICE_FILE")
json_file=None
dev_prefix = "c"
dev_start = 40000
dev_end = 100000
dev_type="car"
dev_pw = "svtfortest"
csv_file="next20kdevices.csv"

rowcount =0

if csv_file is not None :
    with open(csv_file, mode='w',newline='') as devicesFile:
        writer = csv.writer(devicesFile)
        for num in range(dev_start,dev_end):
            newId = dev_prefix+"%07d"%(num)
            writer.writerow([newId,dev_type,dev_pw])
            rowcount +=1

if json_file is not None :
    with open(json_file, mode='w',newline='') as devicesFile:
        for num in range(dev_start,dev_end):
            newId = dev_prefix+"%07d"%(num)
            json.dump({"deviceId":newId,"deviceType":dev_type,"devicePw":dev_pw})
            rowcount +=1
            
print ("created {0} rows".format(rowcount))
