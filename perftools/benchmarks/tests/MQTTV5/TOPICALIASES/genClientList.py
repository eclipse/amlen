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
# This script generates the mqttbench client list for running the MQTTV5.TOPICALIASES 
# benchmark test.
#
######################################################################################
CLIENTLIST_FILE_PREFIX="MQTTV5-TOPICALIASES-TOPICLEN-"

BASEDIR="../.."
PUB_CLIENTID_PREFIX="pub-perfclient"
SUB_CLIENTID_PREFIX="sub-perfclient"

import sys
import argparse
import math
import re
import distutils.util
from random import choice
from string import ascii_uppercase
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
    global publicNetDestlist, privateNetDestlist, TOPICLENGTH, ALIASESENABLED

    parser = argparse.ArgumentParser(description='Creates the mqttbench client list file for the POLICIES benchmark', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--enableAliases", type=distutils.util.strtobool, required=True, dest="enableAliases", metavar="<True|False>", help="a boolean flag indicating whether or not to enable topic aliases on the topics to publish messages on")
    parser.add_argument("--topicLength", type=int, required=True, dest="topicLength", metavar="<bytes>", help="the length of the topic string (in bytes) to publish messages on")
    parser.add_argument("--publicNetDestList", required=True, dest="publicNetDestlist", metavar="<comma separated list of IP or DNS name>", help="list of IPv4 addresses or DNS name of MessageSight server PUBLIC messaging endpoints")
    parser.add_argument("--privateNetDestList", required=True, dest="privateNetDestlist", metavar="<comma separated list of IP or DNS name>", help="list of IPv4 addresses or DNS name of MessageSight server PRIVATE messaging endpoints")
    args = parser.parse_args()
    
    TOPICLENGTH = args.topicLength
    if TOPICLENGTH < STATICTOPICLEN:
        print("--topicLength must be greater than or equal to 3")
        exit(1)
    
    ALIASESENABLED = bool(args.enableAliases)
    publicNetDestlist.extend(args.publicNetDestlist.split(","))
    privateNetDestlist.extend(args.privateNetDestlist.split(","))

# Create a Publisher client
def createPUBClient(id, version, dstip, topicString):
    global ALIASESENABLED
    
    client = MBClient(PUB_CLIENTID_PREFIX + str(id), version)
    client.dst = dstip
    client.dstPort = 1885
    client.cleanStart = True
    client.useTLS = False
    if ALIASESENABLED:
        client.topicAliasMaxOut = 1
    
    topic = MBTopic(topicString)
    topic.qos = 0
    
    client.publishTopics.append(topic)
    
    return client

# Create a Subscriber client
def createSUBClient(id, version, dstip, topicString):
    global ALIASESENABLED
    
    client = MBClient(SUB_CLIENTID_PREFIX + str(id), version)
    client.dst = dstip
    client.dstPort = 16901
    client.cleanStart = True
    client.useTLS = False
    if ALIASESENABLED:
        client.topicAliasMaxIn = 1
    
    sub = MBTopic(topicString)
    sub.qos = 0
    
    client.subscriptions.append(sub)
    
    return client

STATICTOPICLEN = 3    # 3 bytes for client IDs 0 - 999 
TOPICLENGTH = 0
ALIASESENABLED = False
NUMCLIENTS = 1e3 
publicNetDestlist = []
privateNetDestlist = []
clientlist = []

# Process the command line arguments
readArgs()

CLIENTLIST_FILE=CLIENTLIST_FILE_PREFIX + str(TOPICLENGTH) + "-TOPICALIASENABLED-" + str(ALIASESENABLED) + ".json"
clfile = None
try:
    clfile = open(CLIENTLIST_FILE, "w")        
except Exception as e:
    print("Failed to open {0} for writing, exception {1}".format(CLIENTLIST_FILE, e))
    exit(1) 

# Generate topic string
randTopicLen = TOPICLENGTH - STATICTOPICLEN

# Create publisher and subscriber client entries in the mqttbench client list file
count = 0
for i in range(0, int(NUMCLIENTS)):
    pubIdx = count % len(publicNetDestlist)
    pubDstIP = publicNetDestlist[pubIdx]
    privIdx = count % len(privateNetDestlist)
    privDstIP = privateNetDestlist[privIdx]
    
    randString = "".join(choice(ascii_uppercase) for j in range(randTopicLen))
    iterString = str(i).zfill(3)
    topic = iterString + randString
    
    pubClient = createPUBClient(i, MBCONSTANTS.MQTT_TCP5, pubDstIP, topic)
    subClient = createSUBClient(i, MBCONSTANTS.MQTT_TCP5, privDstIP, topic)
    clientlist.append(pubClient)
    clientlist.append(subClient)
    count += 1
    
MBCL.MBshuffleClients(clientlist)
MBCL.MBprintClients(clientlist, clfile)

