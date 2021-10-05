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

###########################################################################################
### A detailed description of the classes used in the example below can be found in the
### mqttbenchObjs.py script
###
### This script will generate a client list file for mqttbench v5, based on the number of
### clients specified by the -n command line parameter  
###
### NOTE: passwords are automatically base64 encoded by the mqttbenchObjs.py helper script.
###       mqttbench expects the password field to be base64 encoded
###########################################################################################

from mqttbenchObjs import *
import argparse
import base64

def clientTypeX(i):
    client = MBClient("cid" + str(i), MBCONSTANTS.MQTT_TCP5, serverDst, serverPort)
    client.cleanStart = True
    client.useTLS = bool(useTLS)
    
    # Create a publish topic
    t = MBTopic("topic" + str(i), qos=1)
    t.retain=True
    client.publishTopics.append(t)

    # Create a subscription
    s = MBTopic("topic" + str(i), qos=1)
    client.subscriptions.append(s)
    
    return client

###########################################################################################
# MAIN
#
# NOTE: passwords are automatically base64 encoded by the mqttbenchObjs.py helper script
#       mqttbench expects the password field to be base64 encoded 
###########################################################################################

numClients = 1;          # number of clients to create
serverDst = "127.0.0.1"  # IPv4 address / DNS name of the MQTT message broker(s) to connect to
serverPort = 1883        # TCP port number of the MQTT message broker(s) to connect to
useTLS = 0               # default is non-TLS connections
clients = []             # list of clients to write to the sample mqttbench client list file
messages = []            # list of message objects which are written to individual JSON message files and processed later by mqttbench
MSGSDIR = "messages"     # directory to create, where message files will be written

parser = argparse.ArgumentParser(description='Create an mqttbench v2 client list file.', formatter_class=argparse.RawTextHelpFormatter)
parser.add_argument('-n', "--numClients", type=str, dest='numClients', metavar='num', help="Specify the number of clients to create")
parser.add_argument('-d', "--dst", type=str, dest='serverDst', metavar='<ip> or <dns name>', help="IP or DNS name of the MQTT message broker(s)")
parser.add_argument('-p', "--dstPort", type=str, dest='serverPort', metavar='port number', help="TCP port number of the MQTT message broker(s) to connect to")
parser.add_argument('-t', "--useTLS", type=str, dest='useTLS', metavar='0|1', help="Indicate whether to use TLS connections or not")
args = parser.parse_args()

if not args.numClients == None:
    numClients = args.numClients    
if not args.serverDst == None:
    serverDst = args.serverDst 
if not args.serverPort == None:
    serverPort = args.serverPort 
if not args.useTLS == None:
    useTLS = args.useTLS

# Loop to create the clients
for i in range(0, int(numClients)):
    client = clientTypeX(i)
    clients.append(client) # add this client to the list of clients

# Print clients in the order in which they were added to the list
MBCL.MBprintClients(clients)
