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
###################################################################################### 
#
# This script generates the mqttbench client list for running the MQTTV5.PROPERTIES 
# benchmark test.
#
######################################################################################
CLIENTLIST_FILE_PREFIX="MQTTV5-PROPERTIES-NUMPROPS-"

BASEDIR="../.."
PUB_CLIENTID_PREFIX="pub-perfclient"
SUB_CLIENTID_PREFIX="sub-perfclient"

import sys
import argparse
import math
import re
sys.path.append(BASEDIR)
from mqttbenchObjs import *

def fromSI(value):
    siMap = {'K': 1e3, 'M': 1e6}
    regexPattern = "^\d+[KM]?"
    pattern = re.compile(regexPattern)
    match = pattern.match(value)
    if not match is None and match.end() == len(value):
        noUnits = int(re.sub("[^0-9]", "", value))
        if value.find('K') >= 0:
            return int(noUnits * int(siMap['K']))
        elif value.find('M') >= 0:
            return int(noUnits * int(siMap['M']))
        else:
            return int(value)
    else:
        return 0
    
# Read command line arguments
def readArgs():
    global publicNetDestlist, privateNetDestlist, NUMUSERPROPERTIES

    parser = argparse.ArgumentParser(description='Creates the mqttbench client list file for the POLICIES benchmark', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--numProperties", type=int, required=True, dest="numProperties", metavar="<number>", help="the number of MQTTv5 user properties to attach to published messages")
    parser.add_argument("--publicNetDestList", required=True, dest="publicNetDestlist", metavar="<comma separated list of IP or DNS name>", help="list of IPv4 addresses or DNS name of MessageSight server PUBLIC messaging endpoints")
    parser.add_argument("--privateNetDestList", required=True, dest="privateNetDestlist", metavar="<comma separated list of IP or DNS name>", help="list of IPv4 addresses or DNS name of MessageSight server PRIVATE messaging endpoints")
    args = parser.parse_args()
    
    NUMUSERPROPERTIES = args.numProperties
    publicNetDestlist.extend(args.publicNetDestlist.split(","))
    privateNetDestlist.extend(args.privateNetDestlist.split(","))

# Create a Publisher client
def createPUBClient(id, version, dstip):
    global NUMUSERPROPERTIES
    
    client = MBClient(PUB_CLIENTID_PREFIX + str(id), version)
    client.dst = dstip
    client.dstPort = 1885
    client.cleanStart = True
    client.useTLS = False
    client.topicAliasMaxOut = 1
    
    topic = MBTopic("mqttv5/properties/q0/topic" + str(id))
    topic.qos = 0
    
    userProperties = []
    for i in range(0, NUMUSERPROPERTIES):
        userProperty = MBUserProperty("mypropertyname" + str(i), "mypropertyvalue" + str(i))
        userProperties.append(userProperty)
    topic.userProperties = userProperties
    
    client.publishTopics.append(topic)
    
    return client

# Create a Subscriber client
def createSUBClient(id, version, dstip):
    client = MBClient(SUB_CLIENTID_PREFIX + str(id), version)
    client.dst = dstip
    client.dstPort = 16901
    client.cleanStart = True
    client.useTLS = False
    client.topicAliasMaxIn = 10
    
    sub = MBTopic("mqttv5/properties/q0/topic" + str(id))
    sub.qos = 0
    
    client.subscriptions.append(sub)
    
    return client

NUMUSERPROPERTIES = 0
NUMPUBCLIENTS = 1e3
NUMSUBCLIENTS = 1e3
publicNetDestlist = []
privateNetDestlist = []
clientlist = []

# Process the command line arguments
readArgs()

CLIENTLIST_FILE=CLIENTLIST_FILE_PREFIX + str(NUMUSERPROPERTIES) + ".json"
clfile = None
try:
    clfile = open(CLIENTLIST_FILE, "w")        
except Exception as e:
    print("Failed to open {0} for writing, exception {1}".format(CLIENTLIST_FILE, e))
    exit(1) 

# Create publisher client entries in the mqttbench client list file
count = 0
for i in range(0, int(NUMPUBCLIENTS)):
    idx = count % len(publicNetDestlist)
    dstip = publicNetDestlist[idx]
    client = createPUBClient(i, MBCONSTANTS.MQTT_TCP5, dstip)
    clientlist.append(client)
    count += 1

# Create subscriber client entries in the mqttbench client list file
count = 0
for i in range(0, int(NUMSUBCLIENTS)):
    idx = count % len(privateNetDestlist)
    dstip = privateNetDestlist[idx]
    client = createSUBClient(i, MBCONSTANTS.MQTT_TCP5, dstip)
    clientlist.append(client)
    count += 1
    
MBCL.MBshuffleClients(clientlist)
MBCL.MBprintClients(clientlist, clfile)

