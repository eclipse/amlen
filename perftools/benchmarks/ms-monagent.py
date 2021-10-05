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
# This script is a monitoring agent which will periodically send MessageSight server statistics
# to a Graphite server
#
######################################################################################
import argparse
import base64
import collections
import copy
import glob
import csv
from datetime import datetime
import inspect
import json
import logging
import os
from pprint import pformat
import random
import sys
import threading
from time import sleep
import time
import urllib
import signal
import re
import gzip
import socket

import requests
import requests.exceptions
from requests import Request, Session, Response
from requests.auth import HTTPBasicAuth
from requests.cookies import RequestsCookieJar
from requests.exceptions import ConnectTimeout, HTTPError, Timeout, ConnectionError
from requests.packages.urllib3.exceptions import InsecureRequestWarning,\
    InsecurePlatformWarning, SNIMissingWarning, ConnectTimeoutError
from requests.packages.urllib3.request import RequestMethods
from sys import version_info
from ipaddress import ip_address

#################
### Constants ###
#################
CARBON_SERVER="<hostname of Graphite relay>"
CARBON_PORT=2003
STAT_INTERVAL_SECS=4
METRIC_ROOT="msstats"

RESTTIMEOUT=90
MAX_WAIT_ATTEMPTS=10

DEFAULT_ADMIN_USERNAME="admin"
DEFAULT_ADMIN_PASSWORD="admin"
DEFAULT_ADMIN_PORT=9089

IMASERVICE="Server"
MS_NAME="Name"
MS_ACTIVE="Active"
MS_BADCONN="BadConnections"
MS_BYTES="Bytes"
MS_MESSAGES="Messages"
MS_TOTAL="Total"

# Stats labels
MS_STATE="State"
MS_MEMORY="Memory"
MS_ENDPOINT="Endpoint"
MS_SERVER="Server"
MS_STORE="Store"
MS_SUBSCRIPTION="Subscription"
MS_SUB_ST_PH="BufferedPercentHighest"
MS_SUB_ST_HWM_PH="BufferedHWMPercentHighest"
MS_TOPICSTRING="TopicString"
MS_SUBNAME="SubName"
MS_HWM_PERCENT="BufferedHWMPercent"
MS_MSGS_PERCENT="BufferedPercent"
MS_MSGS_BUFF="BufferedMsgs"
MS_CONSUMERS="Consumers"
MS_DISCARDED="DiscardedMsgs"
MS_REJECTED="RejectedMsgs"
MS_EXPIRED="ExpiredMsgs"
MS_IS_DUR="IsDurable"
MS_IS_SHARED="IsShared"
MS_MAX_MSGS="MaxMessages"
MS_PUB_MSGS="PublishedMsgs"

########################
### Global Variables ###
########################
g_logger = None
g_stopRequested = False
g_session = None
g_retryCount = 3
g_severName = None
g_skipStats = False

# Locks for thread safety
g_writeLogLock = threading.Lock()

#####################
### Inner Classes ###
#####################
class LOGP:                                 # Logging prefixes
    DEFAULT = "DEFAULT"                     # when no log prefix is provided
    EVENT = "EVENT"                         # log messages indicating some event (success or failure)
    DATA = "DATA"                           # debug messages containing helpful data
    REST = "REST"                           # log messages related to REST API invocations
    NOTE = "NOTE"

class LOGL:
    INFO = "info"
    ERROR = "error"
    DEBUG = "debug"
    WARN = "warn"

class MSConfig:
    adminEndpoint = None
    adminPort = DEFAULT_ADMIN_PORT
    
class RestMethod:
    POST = "POST"
    GET = "GET"
    PUT = "PUT"
    DELETE = "DELETE"
    PATCH = "PATCH"

class RestRequest:
    method = None               # REST method to invoke
    URL = None                  # The target URL to invoke
    urlparams = None            # URL parameters
    headers = None              # HTTP headers
    auth = None                 # Auth headers
    cookies = None              # HTTP cookies
    payload = None              # HTTP request payload
    files = None                # File upload
    session = None              # The session to invoke the request on
    verify = False              # Verify request
    
    def __repr__(self):
        return pformat(vars(self), indent=2, width=1)

class RestResponse:
    success = False             # Pass/Fail flag indicating whether the request was successful
    response = None             # The response object from the rest request
    exception = None            # Any exception that was thrown

    def __repr__(self):
        return pformat(vars(self), indent=2, width=1)

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

# Create signal handler to handle signals from user
def sigHandler(signum, frame):
    global g_stopRequested

    writeLog(LOGL.INFO, "Handling signal: {0}. Setting global stop flag and exiting.\n".format(signum), LOGP.EVENT)
    g_stopRequested = True 
    exit(2)

# Helper function to invoke REST requests for various REST API calls
def invokeRESTCall(restRequest, maxRetries, acceptableStatusCode=0):
    global RESTTIMEOUT
    
    restResponse = RestResponse()
    response = None         # response object returned from requests API
    preppedRequest = None   # Prepared request object

    # Prepare the request
    request = Request(restRequest.method,
                      restRequest.URL,
                      headers=restRequest.headers,
                      auth=restRequest.auth,
                      params=restRequest.urlparams,
                      cookies=restRequest.cookies,
                      data=restRequest.payload,
                      files=restRequest.files
                      )
    if(restRequest.session is None):
        restRequest.session = Session()  # a new HTTP session from which the request will be invoked

    retryCount = 0
    preppedRequest = None
    statusCode = None
    while (not restResponse.success and retryCount < maxRetries):
        restResponse.exception = None  # reset the exception on each attempt, return last exception
        statusCode = None # reset the status code on each attempt
        try:
            writeLog(LOGL.DEBUG, "Attempt {0} at invoking UN-prepped REST request {1}".format(retryCount, pformat(vars(request))), LOGP.REST)
            preppedRequest = restRequest.session.prepare_request(request)
            writeLog(LOGL.DEBUG, "Attempt {0} at invoking prepped REST request {1}".format(retryCount, pformat(vars(preppedRequest))), LOGP.REST)
            response = restRequest.session.send(preppedRequest,
                                                timeout=RESTTIMEOUT,
                                                verify=restRequest.verify)
            response.raise_for_status() # raise HTTPError exception if HTTP response code >= 400
            
            # In Debug mode log the HTTP response history
            writeLog(LOGL.DEBUG, "response history is {0}".format(response.history), LOGP.REST)
            if len(response.history) > 0: 
                for resp in response.history :
                    writeLog(LOGL.DEBUG, "History line is {0}".format(resp), LOGP.REST)
                    writeLog(LOGL.DEBUG, "response in bytes was:", LOGP.REST)
                    for chunk in resp.iter_content(100):
                        writeLog(LOGL.DEBUG, str(chunk), LOGP.REST)

            restResponse.response = response
            restResponse.success = True
            break
        except HTTPError as e:  # special handling for HTTPError exception (we maintain per response code stats)
            restResponse.exception = e
            statusCode = e.response.status_code

        except Exception as e:  # catch all other exceptions and process them in the same way
            restResponse.exception = e
            if (isinstance(e, ConnectTimeout) or isinstance(e, ConnectionError)) and acceptableStatusCode == 0xDEAD:
                statusCode = acceptableStatusCode

        # Exception path
        if(restResponse.exception is not None):
            if(statusCode != acceptableStatusCode):
                writeLog(LOGL.ERROR, "An exception of type {0} occurred while processing the REST request {1}, statusCode={2} acceptableStatusCode={3}\nException Args: {4}" \
                                     .format(type(restResponse.exception).__name__,preppedRequest, statusCode, acceptableStatusCode, restResponse.exception.args), LOGP.REST)
            if(hasattr(restResponse.exception, 'response') and restResponse.exception.response is not None):
                restResponse.response = restResponse.exception.response
            else:
                restResponse.response = Response() # create an empty response

            if(statusCode in [400, 401, 403, 404, 405, 409, 413]):
                if(statusCode == acceptableStatusCode):
                    restResponse.success = True
                    
                break # might as well not even retry if response code is one of these

        retryCount+=1

    if(restResponse.success == False):
        if(statusCode != acceptableStatusCode):
            writeLog(LOGL.ERROR, "REST request object {0}.\nPrepped HTTP request object {1}".format(restRequest,pformat(vars(preppedRequest))), LOGP.REST)
            writeLog(LOGL.ERROR, "REST response object {0}.\nHTTP response object {1}".format(restResponse,pformat(vars(restResponse.response))), LOGP.REST)
    else:
        writeLog(LOGL.DEBUG, "REST response object {0}.\nHTTP response object {1}".format(restResponse,pformat(vars(restResponse.response))), LOGP.REST)

    return restResponse

# Get MessageSight subscription stats
def getSubscriptionStats(msconfig, endpoint, statType):
    global g_retryCount
    statsBuf = ""
    
    restRequest = RestRequest()
    restResponse = None
    
    ## Get the server state
    restRequest.method = RestMethod.GET
    restRequest.URL = "https://" + endpoint + ":" + str(msconfig.adminPort) + "/ima/monitor/Subscription?ResultCount=10&StatType=" + statType
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)

    restResponse = invokeRESTCall(restRequest, g_retryCount)

    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to get the subscription stats for {0}, statType {1}".format(endpoint, statType), LOGP.EVENT)
    else:
        responsePayload = json.loads(restResponse.response.text)
        subscriptionData = responsePayload[MS_SUBSCRIPTION]
        count = 0
        for sub in subscriptionData:
            topicName = sub[MS_TOPICSTRING].replace(".","_").replace("/","_").replace("+","PLUS").replace("#","HASH").replace("$","")
            subName = sub[MS_SUBNAME].replace(".","_").replace("/","_").replace("+","PLUS").replace("#","HASH").replace("$","")
            if len(subName) == 0:
                subName = "None"
            
            statsBuf += makeMetric(msconfig, MS_SUBSCRIPTION + "." + topicName + "." + subName + "." + MS_HWM_PERCENT, sub[MS_HWM_PERCENT])
            statsBuf += makeMetric(msconfig, MS_SUBSCRIPTION + "." + topicName + "." + subName + "." + MS_MSGS_PERCENT, sub[MS_MSGS_PERCENT])
            statsBuf += makeMetric(msconfig, MS_SUBSCRIPTION + "." + topicName + "." + subName + "." + MS_MSGS_BUFF, sub[MS_MSGS_BUFF])
            statsBuf += makeMetric(msconfig, MS_SUBSCRIPTION + "." + topicName + "." + subName + "." + MS_DISCARDED, sub[MS_DISCARDED])
            statsBuf += makeMetric(msconfig, MS_SUBSCRIPTION + "." + topicName + "." + subName + "." + MS_REJECTED, sub[MS_REJECTED])
            statsBuf += makeMetric(msconfig, MS_SUBSCRIPTION + "." + topicName + "." + subName + "." + MS_EXPIRED, sub[MS_EXPIRED])
            statsBuf += makeMetric(msconfig, MS_SUBSCRIPTION + "." + topicName + "." + subName + "." + MS_PUB_MSGS, sub[MS_PUB_MSGS])
            statsBuf += makeMetric(msconfig, MS_SUBSCRIPTION + "." + topicName + "." + subName + "." + MS_IS_DUR, sub[MS_IS_DUR])
            statsBuf += makeMetric(msconfig, MS_SUBSCRIPTION + "." + topicName + "." + subName + "." + MS_IS_SHARED, sub[MS_IS_SHARED])
            statsBuf += makeMetric(msconfig, MS_SUBSCRIPTION + "." + topicName + "." + subName + "." + MS_CONSUMERS, sub[MS_CONSUMERS])
    
            count += 1
            if count > 5:
                break
            
    return statsBuf 

# Get the MessageSight store stats
def getStoreStats(msconfig, endpoint):
    global g_retryCount
    statsBuf = ""
    
    restRequest = RestRequest()
    restResponse = None
    
    ## Get the server state
    restRequest.method = RestMethod.GET
    restRequest.URL = "https://" + endpoint + ":" + str(msconfig.adminPort) + "/ima/monitor/Store"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)

    restResponse = invokeRESTCall(restRequest, g_retryCount)

    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to get the store stats for {0}".format(endpoint), LOGP.EVENT)
    else:
        responsePayload = json.loads(restResponse.response.text)
        storeData = responsePayload[MS_STORE]
        for key in storeData:
            statsBuf += makeMetric(msconfig, MS_STORE + "." + key, storeData[key])
    
    return statsBuf

# Get the MessageSight server stats
def getServerStats(msconfig, endpoint):
    global g_retryCount
    statsBuf = ""
    
    restRequest = RestRequest()
    restResponse = None
    
    ## Get the server state
    restRequest.method = RestMethod.GET
    restRequest.URL = "https://" + endpoint + ":" + str(msconfig.adminPort) + "/ima/monitor/Server"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)

    restResponse = invokeRESTCall(restRequest, g_retryCount)

    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to get the server stats for {0}".format(endpoint), LOGP.EVENT)
    else:
        responsePayload = json.loads(restResponse.response.text)
        serverData = responsePayload[MS_SERVER]
        for key in serverData:
            statsBuf += makeMetric(msconfig, MS_SERVER + "." + key, serverData[key])
    
    return statsBuf   
    
# Get the MessageSight endpoint stats
def getEndpointStats(msconfig, endpoint):
    global g_retryCount
    statsBuf = ""
    
    restRequest = RestRequest()
    restResponse = None
    
    ## Get the server state
    restRequest.method = RestMethod.GET
    restRequest.URL = "https://" + endpoint + ":" + str(msconfig.adminPort) + "/ima/monitor/Endpoint"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)

    restResponse = invokeRESTCall(restRequest, g_retryCount)

    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to get the endpoint stats for {0}".format(endpoint), LOGP.EVENT)
    else:
        responsePayload = json.loads(restResponse.response.text)
        endpointData = responsePayload[MS_ENDPOINT]
        for endpoint in endpointData:
            endpointName = endpoint[MS_NAME].replace("!","")
            statsBuf += makeMetric(msconfig, MS_ENDPOINT + "." + endpointName + "." + MS_ACTIVE, endpoint[MS_ACTIVE])
            statsBuf += makeMetric(msconfig, MS_ENDPOINT + "." + endpointName + "." + MS_MESSAGES, endpoint[MS_MESSAGES])
            statsBuf += makeMetric(msconfig, MS_ENDPOINT + "." + endpointName + "." + MS_BYTES, endpoint[MS_BYTES])
    
    return statsBuf        

# Get the MessageSight server state
def getServerMemory(msconfig, endpoint):
    global g_retryCount
    statsBuf = ""
    
    restRequest = RestRequest()
    restResponse = None
    
    ## Get the server state
    restRequest.method = RestMethod.GET
    restRequest.URL = "https://" + endpoint + ":" + str(msconfig.adminPort) + "/ima/monitor/Memory"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)

    restResponse = invokeRESTCall(restRequest, g_retryCount)

    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to get the server memory stats for {0}".format(endpoint), LOGP.EVENT)
    else:
        responsePayload = json.loads(restResponse.response.text)
        memData = responsePayload[MS_MEMORY]
        for key in memData:
            statsBuf += makeMetric(msconfig, MS_MEMORY + "." + key, memData[key])
    
    return statsBuf

# Get the MessageSight server state
def getServerState(msconfig, endpoint):
    global g_retryCount, g_skipStats
    statsBuf = ""
    
    restRequest = RestRequest()
    restResponse = None
    
    ## Get the server state
    restRequest.method = RestMethod.GET
    restRequest.URL = "https://" + endpoint + ":" + str(msconfig.adminPort) + "/ima/service/status"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)

    restResponse = invokeRESTCall(restRequest, g_retryCount)

    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to get the server state for {0}".format(endpoint), LOGP.EVENT)
        g_skipStats = True
    else:
        responsePayload = json.loads(restResponse.response.text)
        statsBuf = makeMetric(msconfig, MS_STATE, responsePayload[IMASERVICE][MS_STATE])
        if responsePayload[IMASERVICE][MS_STATE] != 1:
            g_skipStats = True
        else:
            g_skipStats = False
    
    return statsBuf

# Get the MessageSight server name
def getServerName(msconfig, endpoint):
    global g_retryCount
    servername = endpoint.replace(".", "-")
    
    restRequest = RestRequest()
    restResponse = None
    
    ## Get the server state
    count=0
    while count < MAX_WAIT_ATTEMPTS:
        restRequest.method = RestMethod.GET
        restRequest.URL = "https://" + endpoint + ":" + str(msconfig.adminPort) + "/ima/service/status"
        restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    
        restResponse = invokeRESTCall(restRequest, g_retryCount)
    
        if restResponse.success == False:
            writeLog(LOGL.ERROR, "Failed to get the server name for {0}".format(endpoint), LOGP.EVENT)
        else:
            responsePayload = json.loads(restResponse.response.text)
            servername = responsePayload[IMASERVICE][MS_NAME].split(".")[0]
            break
    
        count += 1
    
    return servername

# Send metrics buffer to Graphite
def sendMetrics(msconfig, statsBuff):
    try:
        sock = socket.socket()
        sock.connect((CARBON_SERVER, CARBON_PORT))
        sock.sendall(statsBuff)
        sock.close()
    except Exception as e:
        writeLog(LOGL.ERROR, "Exception occurred while sending metrics to Graphite, {0}".format(e), LOGP.EVENT)

# Make a metric triplet
def makeMetric(msconfig, label, value):
    global g_severName
    
    mpath = METRIC_ROOT + "." + g_severName + "." + label
    timestamp = int(time.time())
    return "{0} {1} {2:d}\n".format(mpath, value, timestamp)

# Process command line arguments
def readArgs(msconfig): 
    global g_logger, g_retryCount
    
    parser = argparse.ArgumentParser(description='MessageSight server monitoring agent', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-l', "--logLevel", type=str, dest='logLevel', metavar='<logLevel>', help="The case-insensitive level of logging to use (CRITICAL,ERROR,WARNING,INFO,DEBUG,NOTSET).")
    parser.add_argument("--requestRetries", type=int, dest="retryCount", metavar="<num retries>", help="specify the number of attempts for each REST API operation before giving up")
    parser.add_argument("--adminEndpoint", required=True, dest="adminEndpoint", metavar="<IPv4 address>", help="IPv4 address of the admin endpoint of the primary MessageSight server to send REST requests to")
    parser.add_argument("--adminPort", type=int, dest="adminPort", metavar="<TCP port>", help="TCP port number of the admin endpoint to send REST requests to")
    
    args = parser.parse_args()
    
    ## Configure the logger
    for key in logging.Logger.manager.loggerDict:
        logging.getLogger(key).setLevel(logging.WARNING)

    if args.logLevel is None: 
        logAsString = LOGL.INFO.upper()
    else:
        logAsString = args.logLevel.upper()

    logAsInt = getattr(logging, logAsString)
    logging.basicConfig(level=logAsInt, format='%(asctime)-15s  %(levelname)-5s  %(message)s', datefmt='%Y-%m-%dT%H:%M:%S')
    g_logger = logging.getLogger()
    g_logger.setLevel(logAsInt)

    ## Check params
    if not args.retryCount is None:
        g_retryCount = args.retryCount
    
    if not args.adminPort is None:
        msconfig.adminPort = args.adminPort
        
    msconfig.adminEndpoint = args.adminEndpoint

    writeLog(LOGL.DEBUG, "adminEndpoint = {0}".format(msconfig.adminEndpoint), LOGP.DATA)
    writeLog(LOGL.DEBUG, "API retry count = {0}".format(g_retryCount), LOGP.DATA)  

# Main loop through the list of orgs specified in the BAP input file
def main(): 
    global g_severName, g_skipStats
    
    msconfig = MSConfig()
    
    # Process command line arguments and store them in g_args[], also setup global logger in readArgs()
    readArgs(msconfig)
    
    # Get the server name (used in the metric path)
    g_severName = getServerName(msconfig, msconfig.adminEndpoint)
    
    # Monitoring Loop
    count=0
    while(True):
        statsBuff = ""
        
        if (count % 6) == 0:
            writeLog(LOGL.INFO, "Monitor loop {0}".format(count), LOGP.EVENT)
        
        # Get server state
        statsBuff += getServerState(msconfig, msconfig.adminEndpoint)
        
        if not g_skipStats:
            # Get server memory stats
            statsBuff += getServerMemory(msconfig, msconfig.adminEndpoint)
            
            # Get endpoint stats
            statsBuff += getEndpointStats(msconfig, msconfig.adminEndpoint)
    
            # Get server stats
            statsBuff += getServerStats(msconfig, msconfig.adminEndpoint)
            
            # Get store stats
            statsBuff += getStoreStats(msconfig, msconfig.adminEndpoint)
            
            # Get subscription stats
            statsBuff += getSubscriptionStats(msconfig, msconfig.adminEndpoint, MS_SUB_ST_PH)
            statsBuff += getSubscriptionStats(msconfig, msconfig.adminEndpoint, MS_SUB_ST_HWM_PH)
            
        # Sending MessageSight stats to Graphite
        sendMetrics(msconfig, statsBuff)
        
        count += 1
        sleep(STAT_INTERVAL_SECS)

    
### START OF MAIN ROUTINE ###
# Decrease verbosity for REST requests
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)
requests.packages.urllib3.disable_warnings(InsecurePlatformWarning)
requests.packages.urllib3.disable_warnings(SNIMissingWarning)

# Set signal handlers on the main thread
signal.signal(signal.SIGTERM, sigHandler)
signal.signal(signal.SIGINT, sigHandler)

# Invoke Main function
main()


