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
### NOTE: passwords are automatically base64 encoded by the mqttbenchObjs.py helper script.
###       mqttbench expects the password field to be base64 encoded
###########################################################################################

from mqttbenchObjs import *
   
########################################################################################################
# Create an MQTT client with the following characteristics:
# - client id is MBCLExample1 and version is v5 client over TCP
# - connects with cleanStart = False and sessionExpiryInterval = MAX_UINT32
# - username and password are set and connects securely to "mybroker.com" on port 8883 
# - Configures a QoS 0 Will Message with a user property and is published to topic /send/last/will/here 
# - Creates a QoS 0 subscription to "/receive/these/messages" topic
########################################################################################################
def MBCLExample1():     
    x = MBClient("MBCLExample1", MBCONSTANTS.MQTT_TCP5)
    x.cleanStart = False
    x.sessionExpiryIntervalSecs = 0xFFFFFFFF
    x.username = "userx"
    x.password = "passwordx"
    x.dst = "mybroker.com"
    x.dstPort = 8883
    x.useTLS = True
    
    # Will Message
    wm = MBLastWillMsg("/send/last/will/here", "have'nt heard from this client in a while")
    wm.qos = 0
    up = MBUserProperty("prop1","value1")
    wm.userProperties.append(up)
    x.lastWillMsg = wm 
    
    # QoS 0 Subscription
    t = MBTopic("/receive/these/messages")
    t.qos = 0
    x.subscriptions.append(t)
    
    return x

########################################################################################################
# Create an MQTT client with the following characteristics:
# - client id is MBCLExample2 and version is v5 client over WebSockets
# - cleanStart = False and sessionExpiryInterval = 100
# - no username or password are specified and the client connects non-securely to "127.0.0.1" on port 80 
# - Max packet size the client will accept is 1MB and Receive Maximum is 128 inflight (i.e. unacked) messages
# - Tell message broker that this client will accept up to 10 topic aliases
# - Configure this client to send 5 topic aliases (may be adjusted by server response in CONNACK)
# - Request that message broker send reason strings in addition to reason codes, in case of failures
# - Publishes a QoS 2 retained message to topic "xyz" (messages are generated binary messages, the size specified by command line param -s <min>-<max>)
########################################################################################################
def MBCLExample2():     
    x = MBClient("MBCLExample2", MBCONSTANTS.MQTT_WS5)
    x.cleanStart = False
    x.sessionExpiryIntervalSecs = 100
    x.dst = "127.0.0.1"
    x.dstPort = 80
    x.maxPktSize = 1024*1024     # bytes
    x.recvMaximum = 128          # max unacked PUBLISH messages this client can receive
    x.topicAliasMaxIn = 10       # this client will accept up to 10 topic aliases
    x.topicAliasMaxOut = 5       # this client will send up to 5 topic aliases
    x.requestProblemInfo = True  # request message broker send reason strings in addition to RC codes
    
    t = MBTopic("xyz", qos=2)
    t.retain=True
    x.publishTopics.append(t)
    
    return x

########################################################################################################
# Create an MQTT client with the following characteristics:
# - client id is MBCLExample3 and version v3.1.1 client over TCP
# - connect securely to "mybroker.com" on port 8883
# - subscribes to "topicX" at QoS 1
########################################################################################################
def MBCLExample3():  
    x = MBClient(_id="MBCLExample3", version=MBCONSTANTS.MQTT_TCP4)
    x.dst = "mybroker.com"
    x.dstPort = 8883
    x.useTLS = True
    
    t = MBTopic("topicX", qos=1)
    x.subscriptions.append(t)
    return x

########################################################################################################
# Create an MQTT client with the following characteristics:
# - client id is MBCLExample4 and version v5 client over TCP
# - connect non-securely to "mybroker.com" on port 1883
# - subscribes to "topicY" at QoS 1 with subscription options noLocal = True, retainAsPublished = False and retainHandling = 2
########################################################################################################
def MBCLExample4(): 
    x = MBClient(_id="MBCLExample4", version=MBCONSTANTS.MQTT_TCP5)
    x.dst = "mybroker.com"
    
    t = MBTopic("topicY", qos=1)
    t.noLocal = True
    t.retainAsPublished = False
    t.retainHandling = 2
    x.subscriptions.append(t)
    return x

########################################################################################################
# Create an MQTT client with the following characteristics:
# - client id is MBCLExample5 and version v5 client over TCP
# - connect non-securely to "mybroker.com" on port 1883
# - publishes to "topicY" at QoS 1 with a response topic "replyToY" and correlation data "myId=_id"
########################################################################################################
def MBCLExample5(): 
    x = MBClient(_id="MBCLExample5", version=MBCONSTANTS.MQTT_TCP5)
    x.dst = "mybroker.com"
    
    t = MBTopic("topicY", qos=1)
    t.responseTopicStr = "replyToY"
    t.correlationData = "myId=" + x._id
    x.publishTopics.append(t)
    return x

########################################################################################################
# Create a Message object with the following characteristics:
# - the message is written to file path MSGSDIR/MBMExample1.json
# - it contains the path of the file containing the payload and an expiry interval of 3600 seconds
# - the message contains 2 user properties "a-name" and "b-name"
#
# NOTE: the path to the payloadFile is relative to the working directory when mqttbench is started, it
#       is NOT relative to the directory containing the message files.  You can also fully qualify the
#       path to the payload file.
# NOTE: All files with extension .json in the message file directory are expected to be mqttbench message
#       files.  Do not create/copy *.json files to an mqttbench message file directory which do not comply
#       with the mqttbench message file schema.
########################################################################################################
def MBMExample1(): 
    x = MBMessage(path=MSGSDIR + "/MBMExample1.json")
    x.payloadFile = "clientlists/messages/payload.dat"
    x.payloadFormat = 1
    x.contentType = MBCONSTANTS.MSG_FMT_JSON
    x.expiryIntervalSecs = 3600
    
    a = MBUserProperty("a-name", "a-value")
    x.userProperties.append(a)
    b = MBUserProperty("b-name", "b-value")
    x.userProperties.append(b)
    return x

########################################################################################################
# Create a Message object with the following characteristics:
# - the message is written to file path MSGSDIR/MBMExample2.json
# - it contains a plain text payload
# - this message specifies the topic to publish to, so it overrides the publishTopics list in the MBClient object
# - QoS is 1 and retain bit is set to True
########################################################################################################
def MBMExample2(): 
    x = MBMessage(path=MSGSDIR + "/MBMExample2.json")
    x.payload = "All work no play makes Jack a dull boy." 
    x.payloadFormat = 1
    x.contentType = MBCONSTANTS.MSG_FMT_TEXT
    
    # Optionally specify a topic destination for this particular message
    x.topic = MBTopic("my/pubtopic/tree", qos=1)
    x.topic.retain = True
    return x

########################################################################################################
# Create a Message object with the following characteristics:
# - the message is written to file path MSGSDIR/MBMExample3.json
# - it specifies a message payload size, therefore mqttbench will generate a binary message payload of the specified size (1024 bytes)
# - payloadFormat is binary(0)
########################################################################################################
def MBMExample3(): 
    x = MBMessage(path=MSGSDIR + "/MBMExample3.json")
    x.payloadSizeBytes = 1024 
    x.payloadFormat = 0
    return x


##########################
# MAIN 
##########################
clients = []            # list of clients to write to the sample mqttbench client list file
messages = []           # list of message objects which are written to individual JSON message files and processed later by mqttbench
MSGSDIR = "messages"    # directory to create, where message files will be written

##########################
# Example clients
##########################
cex1 = MBCLExample1()
clients.append(cex1)

cex2 = MBCLExample2()
clients.append(cex2)

cex3 = MBCLExample3()
clients.append(cex3)

cex4 = MBCLExample4()
clients.append(cex4)

cex5 = MBCLExample5()
clients.append(cex5)

# Print clients in the order in which they were added to the list
print("### Client List")
MBCL.MBprintClients(clients)

# Sort client list by clientID
MBCL.MBsortByClientId(clients)

# Shuffle the client list
MBCL.MBshuffleClients(clients)

##########################
# Example messages
##########################

# Create messages directory if it does not exist
try:
    os.stat(MSGSDIR)
except:    
    os.mkdir(MSGSDIR, 0o777)

mex1 = MBMExample1()
messages.append(mex1)

mex2 = MBMExample2()
messages.append(mex2)

mex3 = MBMExample3()
messages.append(mex3)

# print("\n### Message List")
# MBCL.MBprintMessages(messages)

# Write the serialized JSON message objects to the path, specified by the
# path field in each message object
MBCL.MBwriteMessageFiles(messages)


