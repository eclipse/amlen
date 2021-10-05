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
# the TPUT.BROADCAST benchmark test.
#
# Variant:
# - QoS 2 (durable client sessions)
# - X publishers  (publishing messages to a single topic)
# - Y subscribers (each subscribed to a single topic)
# - Z topics
#
# NOTE: X and Y are based on the parameters passed to this script. Both X and Y are in 
#       SI units, e.g. 1, 500, 1K, 500K, 1M, etc.
######################################################################################
CLIENTLIST_FILE_PREFIX="TPUT-BROADCAST-Q2-"

BASEDIR="../../.."
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
    global publicNetDestlist, privateNetDestlist, NUMPUBCLIENTS, NUMSUBCLIENTS, NUMTOPICS, NUMPUBSTRING, NUMSUBSTRING, NUMTOPICSTRING

    parser = argparse.ArgumentParser(description='Creates the mqttbench client list file for the Q1 variant of the TPUT.BROADCAST benchmark', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--numPubber", required=True, dest="numPubber", metavar="<count>", help="the number of publishers to create")
    parser.add_argument("--numSubber", required=True, dest="numSubber", metavar="<count>", help="the number of subscribers to create")
    parser.add_argument("--numTopics", required=True, dest="numTopics", metavar="<count>", help="the number of topics to publish to")
    parser.add_argument("--publicNetDestList", required=True, dest="publicNetDestlist", metavar="<comma separated list of IP or DNS name>", help="list of IPv4 addresses or DNS name of MessageSight server PUBLIC messaging endpoints")
    parser.add_argument("--privateNetDestList", required=True, dest="privateNetDestlist", metavar="<comma separated list of IP or DNS name>", help="list of IPv4 addresses or DNS name of MessageSight server PRIVATE messaging endpoints")
    args = parser.parse_args()
    
    NUMPUBSTRING = args.numPubber
    NUMSUBSTRING = args.numSubber
    NUMTOPICSTRING = args.numTopics
    NUMPUBCLIENTS = fromSI(NUMPUBSTRING)
    NUMSUBCLIENTS = fromSI(NUMSUBSTRING)
    NUMTOPICS = int(NUMTOPICSTRING)
        
    if NUMPUBCLIENTS == 0 or NUMSUBCLIENTS == 0:
        print("Failed to parse the number of publishers {0} and/or subscribers {1} (numbers are in SI units, e.g. 1, 100, 50K, 1M, etc.)".format(NUMPUBSTRING, NUMSUBSTRING))
        exit(1)
    
    if NUMTOPICS < 1:
        print("The number of topics in this test should be greater than 0, (--numTopics {0} was specified)".format(NUMTOPICSTRING))
        exit(1)
    
    publicNetDestlist.extend(args.publicNetDestlist.split(","))
    privateNetDestlist.extend(args.privateNetDestlist.split(","))

# Create a Publisher client
def createPUBClient(id, version, dstip, topicNum, deleteState):
    client = MBClient(PUB_CLIENTID_PREFIX + str(id), version)
    client.dst = dstip
    client.dstPort = 16901
    client.useTLS = False
    client.topicAliasMaxOut = 1
    if deleteState:
        client.cleanStart = True
        client.sessionExpiryIntervalSecs = 0
    else:
        client.cleanStart = False
        client.sessionExpiryIntervalSecs = 0x7FFFFFFF    
    
    topic = MBTopic("broadcast/q2-" + NUMTOPICSTRING + "TOPICS-" + NUMPUBSTRING + "PUB-" + NUMSUBSTRING + "SUB/broadcast" + str(topicNum))
    topic.qos = 2
    
    client.publishTopics.append(topic)
    
    return client

# Create a Subscriber client
def createSUBClient(id, version, dstip, topicNum, deleteState):
    client = MBClient(SUB_CLIENTID_PREFIX + str(id), version)
    client.dst = dstip
    client.dstPort = 8883
    client.useTLS = True
    client.topicAliasMaxIn = 1
    if deleteState:
        client.cleanStart = True
        client.sessionExpiryIntervalSecs = 0
    else:
        client.cleanStart = False
        client.sessionExpiryIntervalSecs = 0x7FFFFFFF

    sub = MBTopic("broadcast/q2-" + NUMTOPICSTRING + "TOPICS-" + NUMPUBSTRING + "PUB-" + NUMSUBSTRING + "SUB/broadcast" + str(topicNum))
    sub.qos = 2
    
    client.subscriptions.append(sub)
    
    return client

NUMPUBSTRING = ""
NUMSUBSTRING = ""
NUMTOPICSTRING = ""
NUMPUBCLIENTS = 0
NUMSUBCLIENTS = 0
NUMTOPICS = 0
publicNetDestlist = []
privateNetDestlist = []
clientlist = []
clientDeleteStateList = []

# Process the command line arguments
readArgs()

# Process num pubber and num subber SI units strings and open client list file for writing
CLIENTLIST_FILE=CLIENTLIST_FILE_PREFIX + NUMTOPICSTRING + "TOPICS-" + NUMPUBSTRING + "PUB-" + NUMSUBSTRING + "SUB.json"
CLIENTLIST_DELETESTATE_FILE=CLIENTLIST_FILE_PREFIX + NUMTOPICSTRING + "TOPICS-" + NUMPUBSTRING + "PUB-" + NUMSUBSTRING + "SUB-DELETESTATE.json"

clfile = None
clFileDeleteState = None
try:
    clfile = open(CLIENTLIST_FILE, "w")
    clFileDeleteState = open(CLIENTLIST_DELETESTATE_FILE, "w")            
except Exception as e:
    print("Failed to open {0} or {1} for writing, exception {2}".format(CLIENTLIST_FILE, CLIENTLIST_DELETESTATE_FILE, e))
    exit(1)

# Create publisher client entries in the mqttbench client list file and the "delete client state" client list file
count = 0
for i in range(0, int(NUMPUBCLIENTS)):
    idx = count % len(publicNetDestlist)
    dstip = privateNetDestlist[idx]
    topicNum = count % NUMTOPICS
    
    client = createPUBClient(i, MBCONSTANTS.MQTT_TCP5, dstip, topicNum, False)
    clientlist.append(client)
    
    clientDeleteState = createPUBClient(i, MBCONSTANTS.MQTT_TCP5, dstip, topicNum, True)
    clientDeleteStateList.append(clientDeleteState)
    
    count += 1

# Create subscriber client entries in the mqttbench client list file and the "delete client state" client list file
count = 0
for i in range(0, int(NUMSUBCLIENTS)):
    idx = count % len(privateNetDestlist)
    dstip = publicNetDestlist[idx]
    topicNum = count % NUMTOPICS
    
    client = createSUBClient(i, MBCONSTANTS.MQTT_TCP5, dstip, topicNum, False)
    clientlist.append(client)
    
    clientDeleteState = createSUBClient(i, MBCONSTANTS.MQTT_TCP5, dstip, topicNum, True)
    clientDeleteStateList.append(clientDeleteState)

    count += 1
    
MBCL.MBshuffleClients(clientlist)
MBCL.MBprintClients(clientlist, clfile)
MBCL.MBprintClients(clientDeleteStateList, clFileDeleteState)

