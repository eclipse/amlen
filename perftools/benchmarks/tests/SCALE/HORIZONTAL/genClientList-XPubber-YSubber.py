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
# the SCALE.HORIZONTAL benchmark test.
#
# Variant:
# - QoS 0 (non-durable client sessions)
# - MQTT v5 shared subscriptions
# - X publishers  (each publishing on a unique topic)
# - Y subscribers (single wildcard subscription to receive all messages)
#
# NOTE: X and Y are based on the parameters passed to this script. Both X and Y are in 
#       SI units, e.g. 1, 500, 1K, 500K, 1M, etc.
######################################################################################
CLIENTLIST_FILE_PREFIX="SCALE-HORIZONTAL-Q0-SSUBS-NUMSRVR-"
NUMSUBAPPS = 1
PERFAPP="perfapp"

BASEDIR="../../../../.."
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
    global publicNetDestlist, privateNetDestlist, NUMPUBCLIENTS, NUMSUBCLIENTS, NUMPUBSTRING, NUMSUBSTRING, NUMSERVERS

    parser = argparse.ArgumentParser(description='Creates the mqttbench client list file for the Q0.SSUBS variant of the SCALE.HORIZONTAL benchmark', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--numPubber", required=True, dest="numPubber", metavar="<count>", help="the number of publishers to create")
    parser.add_argument("--numSubber", required=True, dest="numSubber", metavar="<count>", help="the number of subscribers to create")
    parser.add_argument("--publicNetDestList", required=True, dest="publicNetDestlist", metavar="<comma separated list of IP or DNS name>", help="list of IPv4 addresses or DNS name of MessageSight server PUBLIC messaging endpoints")
    parser.add_argument("--privateNetDestList", required=True, dest="privateNetDestlist", metavar="<comma separated list of IP or DNS name>", help="list of IPv4 addresses or DNS name of MessageSight server PRIVATE messaging endpoints")
    args = parser.parse_args()
    
    NUMPUBSTRING = args.numPubber
    NUMSUBSTRING = args.numSubber
    NUMPUBCLIENTS = fromSI(NUMPUBSTRING)
    NUMSUBCLIENTS = fromSI(NUMSUBSTRING)
        
    if NUMPUBCLIENTS == 0 or NUMSUBCLIENTS == 0:
        print("Failed to parse the number of publishers {0} and/or subscribers {1} (numbers are in SI units, e.g. 1, 100, 50K, 1M, etc.)".format(NUMPUBSTRING, NUMSUBSTRING))
        exit(1)
    
    publicNetDestlist.extend(args.publicNetDestlist.split(","))
    privateNetDestlist.extend(args.privateNetDestlist.split(","))
    
    NUMSERVERS = len(publicNetDestlist)

# Create a Publisher client
def createPUBClient(id, version, dstip):
    global NUMSERVERS
    
    client = MBClient(PUB_CLIENTID_PREFIX + str(id), version)
    client.dst = dstip
    client.dstPort = 8883
    client.cleanStart = True
    client.useTLS = True
    client.topicAliasMaxOut = 1
    
    topic = MBTopic("scalehorizontal/q0-ssubs-NUMSRVR-" + NUMSERVERS + "/" + client._id)
    topic.qos = 0
    
    client.publishTopics.append(topic)
    
    return client

# Create a Subscriber client
def createSUBClient(id, version, dstip, appid):
    global NUMSERVERS
    
    client = MBClient(SUB_CLIENTID_PREFIX + str(id), version)
    client.dst = dstip
    client.dstPort = 16901
    client.cleanStart = True
    client.useTLS = False
    client.topicAliasMaxIn = 10
    
    appname = PERFAPP + str(appid)
    sub = MBTopic("$share/" + appname + "/scalehorizontal/q0-ssubs-NUMSRVR-" + NUMSERVERS + "/#")
    sub.qos = 0
    
    client.subscriptions.append(sub)
    
    return client

NUMPUBSTRING = ""
NUMSUBSTRING = ""
NUMPUBCLIENTS = 0
NUMSUBCLIENTS = 0
NUMSERVERS = 0
publicNetDestlist = []
privateNetDestlist = []
clientlist = []

# Process the command line arguments
readArgs()

# Process num pubber and num subber SI units strings and open client list file for writing
CLIENTLIST_FILE=CLIENTLIST_FILE_PREFIX + NUMSERVERS + ".json"

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
    appid = i % NUMSUBAPPS
    client = createSUBClient(i, MBCONSTANTS.MQTT_TCP5, dstip, appid)
    clientlist.append(client)
    count += 1
    
MBCL.MBshuffleClients(clientlist)
MBCL.MBprintClients(clientlist, clfile)

