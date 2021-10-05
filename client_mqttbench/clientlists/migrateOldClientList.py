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

#
# Script for migrating old mqttbench client list files to the new v2 format
#

import argparse
import inspect
import threading
import logging
import traceback
from mqttbenchObjs import *


########################
### Global Variables ###
########################
g_args = {}
g_logger = None

# Locks for thread safety
g_writeLogLock = threading.Lock()

# CONSTANTS
INPUTFILE="INFILE"
OUTPUTFILE="OUTFILE"
TARGETVER="TARGETVERSION"

#####################
### Inner Classes ###
#####################
class LOGP:                                 # Logging prefixes
    DEFAULT = "DEFAULT"                     # when no log prefix is provided
    EVENT = "EVENT"                         # log messages indicating some event (success or failure)
    DATA = "DATA"                           # debug messages containing helpful data

class LOGL:
    INFO = "info"
    ERROR = "error"
    DEBUG = "debug"
    WARN = "warn"

#################
### Functions ###
#################

# Write a log message
def writeLog(level, message, prefix=LOGP.DEFAULT):
    global g_logger
    
    threadName = threading.current_thread().getName()
    invokerFrame = inspect.currentframe().f_back
    invokerCode = invokerFrame.f_code
    invokerFuncName = invokerCode.co_name
    logAsInt = getattr(logging, level.upper())
    with g_writeLogLock: # writeLog() can be called from multiple threads in parallel, synchronize writes to the log file
        g_logger.log(logAsInt, "{0:<7} {1:<10} {2:<20} {3}:{4:<6} {5}".format(prefix.upper(), threadName, invokerFuncName, os.path.basename(invokerCode.co_filename), invokerFrame.f_lineno, message))

# Process command line arguments passed to BAP
def readArgs(): 
    global g_args, g_logger
    
    # Defaults 
    g_args[OUTPUTFILE] = "mqttbenchv5-cl.json"
    g_args[TARGETVER] = 4

    parser = argparse.ArgumentParser(description='Migrate an old mqttbench client list file to the new JSON format.', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--logLevel", type=str, dest='logLevel', metavar='logLevel', help="The case-insensitive level of logging to use (CRITICAL,ERROR,WARNING,INFO,DEBUG).")
    parser.add_argument("--oldcl", required=True, type=str, dest="oldClientList", metavar="file path", help="path to the old mqttbench client list file to be migrated")
    parser.add_argument("--newcl", type=str, dest="newClientList", metavar="file path", help="path to the new mqttbench client list file to be created")
    parser.add_argument("--targetVersion", type=str, dest="targetVersion", metavar="3, 4, or 5", help="optionally specify the protocol version to use for the migrated clients, default is 4")
    args = parser.parse_args()
    
    if not args.oldClientList is None:
        g_args[INPUTFILE] = args.oldClientList

    if not args.targetVersion is None:
        g_args[TARGETVER] = int(args.targetVersion)
        
    if not args.newClientList is None:
        g_args[OUTPUTFILE] = args.newClientList
    
    if args.logLevel is None: 
        logAsString = LOGL.INFO.upper()
    else:
        logAsString = args.logLevel.upper()

    logAsInt = getattr(logging, logAsString)
    logging.basicConfig(level=logAsInt, format='%(asctime)-15s  %(levelname)-5s  %(message)s', datefmt='%Y-%m-%dT%H:%M:%S')
    g_logger = logging.getLogger()
    g_logger.setLevel(logAsInt)

# Main routine
def main(): 
    global g_args, g_logger
    
    readArgs()
    
    count=1
    
    try:
        # Open legacy client list input file (infile) for read, and new client list output file (outfile) for write
        with open(g_args[INPUTFILE]) as infile, open(g_args[OUTPUTFILE], mode='w') as outfile:
            clientList = []
            for line in infile:
                fields = line.split("|")                            # legacy client list file used vertical bars as field separators
                if len(fields) < 10:
                    raise Exception("line was found with fewer than 10 columns, this is not a valid mqttbench client list file: " + line)
                
                id = fields[0]                                      # client Id
                version = "TCP" + str(g_args[TARGETVER])
                if len(fields) >= 10 and int(fields[9]) == 1:       # WebSockets flag
                    version = "WS" + str(g_args[TARGETVER])
                
                client = MBClient(id,version)                       # Allocate an MBClient object for this line in the legacy client list file
                
                if g_args[TARGETVER] < 5:
                    client.cleanSession = bool(int(fields[1]))      # clean session flag (for v3 and v4 clients)
                else:
                    if bool(int(fields[1])):
                        client.cleanStart = True                    # clean session = 1 equivalent for v5 clients
                        client.sessionExpiryInterval = 0
                    else:
                        client.cleanStart = False
                        client.sessionExpiryInterval = 0xFFFFFFFF   # clean session 0 equivalent for v5 clients
                if len(fields[2]) != 0:
                    client.username = fields[2]                     # client username
                if len(fields[3]) != 0:
                    client.password = fields[3]                     # client password
                if len(fields[4]) != 0:                             # Publish topics
                    pubTopics = fields[4].split(",")                # comma separated list of pub topics
                    for topic in pubTopics:
                        topicFields = topic.split(":",2)            # triplet QoS:Retain:TopicString
                        qos = int(topicFields[0])
                        retain = bool(int(topicFields[1]))
                        topicString = topicFields[2]
                        t = MBTopic(topicString, qos)
                        t.retain = retain
                        client.publishTopics.append(t)
                if len(fields[5]) != 0:                             # Subscriptions 
                    subTopics = fields[5].split(",")                # comma separated list of subscriptions
                    for topic in subTopics:
                        topicFields = topic.split(":",2)            # triplet QoS:Retain:TopicString (ignore retain field for subs)
                        qos = int(topicFields[0])
                        topicString = topicFields[2]
                        t = MBTopic(topicString, qos)
                        client.subscriptions.append(t)
                client.dst = fields[6]                              # msg broker address or DNS name
                client.dstPort = int(fields[7])                     # msg broker port
                client.useTLS = bool(int(fields[8]))                # TLS flag
                if len(fields) >= 11 and len(fields[10].strip()) != 0:      # linger time and reconnect delay (linger,reconnect)
                    lingerFields = fields[10].split(",")
                    client.lingerTimeSecs = int(lingerFields[0])
                    client.reconnectDelayUSecs = int(lingerFields[1])
                if len(fields) >= 12 and len(fields[11].strip()) != 0:
                    client.clientCertPath = fields[11]              # client cert path
                if len(fields) >= 13 and len(fields[12].strip()) != 0:
                    client.clientKeyPath = fields[12]               # client key path
                if len(fields) >= 14 and len(fields[13].strip()) != 0:
                    client.messageDirPath = fields[13]              # message dir path
                
                clientList.append(client) 
                count+=1
            MBCL.MBprintClients(clientList, outfile)
    except Exception as e:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        exceptionString = traceback.format_exception(exc_type, exc_value, exc_traceback)
        writeLog(LOGL.INFO, "Failed to migrate the legacy mqttbench client list file {0} at line {1}, exception {2}".format(g_args[INPUTFILE], count, exceptionString), LOGP.EVENT)
    
#############
### Main  ###
#############
main()

