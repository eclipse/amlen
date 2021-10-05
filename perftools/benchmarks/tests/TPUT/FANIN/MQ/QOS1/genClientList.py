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
# This script generates the mqttbench client list for running the following variant of
# the TPUT.FANIN.MQ benchmark test.
#
# Variant:
# - QoS 1 (durable client sessions)
# - MQ receivers getting messages from a persistent local queue on the MQ QMgr
# - X MQTT publishers  (each publishing on a unique topic)
#
# NOTE: X is based on the parameters passed to this script. X is in 
#       SI units, e.g. 1, 500, 1K, 500K, 1M, etc.
######################################################################################
CLIENTLIST_FILE_PREFIX="TPUT-FANIN-MQ-Q1-"

BASEDIR="../../../.."
PUB_CLIENTID_PREFIX="pub-perfclient"

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
    global publicNetDestlist, NUMPUBCLIENTS, NUMPUBSTRING

    parser = argparse.ArgumentParser(description='Creates the mqttbench client list file for the Q1 variant of the TPUT.FANIN.MQ benchmark', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--numPubber", required=True, dest="numPubber", metavar="<count>", help="the number of publishers to create")
    parser.add_argument("--publicNetDestList", required=True, dest="publicNetDestlist", metavar="<comma separated list of IP or DNS name>", help="list of IPv4 addresses or DNS name of MessageSight server PUBLIC messaging endpoints")
    args = parser.parse_args()
    
    NUMPUBSTRING = args.numPubber
    NUMPUBCLIENTS = fromSI(NUMPUBSTRING)
        
    if NUMPUBCLIENTS == 0:
        print("Failed to parse the number of publishers {0} (numbers are in SI units, e.g. 1, 100, 50K, 1M, etc.)".format(NUMPUBSTRING))
        exit(1)
    
    publicNetDestlist.extend(args.publicNetDestlist.split(","))

# Create a Publisher client
def createPUBClient(id, version, dstip, deleteState):
    client = MBClient(PUB_CLIENTID_PREFIX + str(id), version)
    client.dst = dstip
    client.dstPort = 8883
    client.useTLS = True
    client.topicAliasMaxOut = 1
    if deleteState:
        client.cleanStart = True
        client.sessionExpiryIntervalSecs = 0
    else:
        client.cleanStart = False
        client.sessionExpiryIntervalSecs = 0x7FFFFFFF
    
    topic = MBTopic("mqconn/mqtt2mq/persist/" + client._id)
    topic.qos = 1
    
    client.publishTopics.append(topic)
    
    return client

NUMPUBSTRING = ""
NUMPUBCLIENTS = 0
publicNetDestlist = []
clientlist = []
clientDeleteStateList = []

# Process the command line arguments
readArgs()

# Process num pubber SI units strings and open client list file for writing
CLIENTLIST_FILE=CLIENTLIST_FILE_PREFIX + NUMPUBSTRING + "PUB.json"
CLIENTLIST_DELETESTATE_FILE=CLIENTLIST_FILE_PREFIX + NUMPUBSTRING + "PUB-DELETESTATE.json"

clfile = None
clFileDeleteState = None
try:
    clfile = open(CLIENTLIST_FILE, "w")        
    clFileDeleteState = open(CLIENTLIST_DELETESTATE_FILE, "w")        
except Exception as e:
    print("Failed to open {0} or {1} for writing, exception {2}".format(CLIENTLIST_FILE, CLIENTLIST_DELETESTATE_FILE, e))
    exit(1)

# Create publisher client entries in the mqttbench client list file
count = 0
for i in range(0, int(NUMPUBCLIENTS)):
    idx = count % len(publicNetDestlist)
    dstip = publicNetDestlist[idx]
    
    client = createPUBClient(i, MBCONSTANTS.MQTT_TCP5, dstip, False)
    clientlist.append(client)
    
    clientDeleteState = createPUBClient(i, MBCONSTANTS.MQTT_TCP5, dstip, True)
    clientDeleteStateList.append(clientDeleteState)
    
    count += 1

MBCL.MBshuffleClients(clientlist)
MBCL.MBprintClients(clientlist, clfile)
MBCL.MBprintClients(clientDeleteStateList, clFileDeleteState)

