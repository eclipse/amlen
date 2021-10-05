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
######################################################################################## 
#
# This script generates the mqttbench client list for running the CONNBURST-TLS-OAUTH
# benchmark test.
#
########################################################################################
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
from shutil import copy2
import getpass

import requests
import requests.exceptions
from requests import Request, Session, Response
from requests.auth import HTTPBasicAuth
from requests.cookies import RequestsCookieJar
from requests.exceptions import ConnectTimeout, HTTPError, Timeout,\
    ConnectionError
from requests.packages.urllib3.exceptions import InsecureRequestWarning,\
    InsecurePlatformWarning, SNIMissingWarning
from requests.packages.urllib3.request import RequestMethods
from sys import version_info
from ipaddress import ip_address
import subprocess
from multiprocessing.dummy import Pool as ThreadPool 

#################
### Constants ###
#################
RESTTIMEOUT=90
MAX_WAIT_ATTEMPTS=20

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

########################
### Global Variables ###
########################
g_logger = None
g_stopRequested = False
g_session = None
g_retryCount = 3
g_OAuthServerIP="<OAuth server IP address>"
g_OAuthServerPort=9443

# Locks for thread safety
g_writeLogLock = threading.Lock()

CLIENTLIST_FILE="CONNBURST-TLS-OAUTH.json"
NUMTHREADS = 96 
NUMCLIENTS = 1e6     # Total number of clients with some unique OAUTH tokens possibly reused if NUM_O_CLIENTS is less than this number.
NUM_O_CLIENTS = 10000  # Number of OAUTH with UNIQUE tokens 
OAUTH_TOKEN_NAME="IMA_OAUTH_ACCESS_TOKEN"
OAUTH_TOKEN_PASSWORD="test"
USERNAME_PREFIX="test"

BASEDIR="../.."
CLIENTID_PREFIX="perfclient"

import sys
import argparse
sys.path.append(BASEDIR)
from mqttbenchObjs import *

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

# Retrieve OAuth token for user uid for used by multi threading
def getOAuthTokenMT(id):
    threadName = threading.current_thread().getName()
    if (id % 10)==0:
        if threadName == "Thread-1":
    	    writeLog(LOGL.INFO, "size of token list is {0}".format(len(tokenList)))

    return getOAuthToken(USERNAME_PREFIX + str(id),OAUTH_TOKEN_PASSWORD + str(id))

# Retrieve OAuth token for user uid
def getOAuthToken(uid,password):
    global g_retryCount, g_OAuthServerIP, g_OAuthServerPort
    
    writeLog(LOGL.DEBUG, "Retrieving OAuth token for user {0}...".format(uid), LOGP.EVENT)
    
    restRequest = RestRequest()
    restResponse = None
    
    ## Retrieve OAuth token
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + g_OAuthServerIP + ":" + str(g_OAuthServerPort) + "/oauth2/endpoint/DemoProvider/token" 
    restRequest.headers = {"Content-Type": "application/x-www-form-urlencoded;charset=UTF-8"}
    restRequest.payload = "grant_type=password&client_id=LibertyRocks&client_secret=AndMakesConfigurationEasy&username=" + uid + "&password=" + password
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = restResponse.response.text;
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to retrieve OAuth token for user {0}. Exiting...".format(uid), LOGP.EVENT)
        exit(1)

    tokenList.append(responsePayload);
    
    return responsePayload

# Create a client
def createClient(id, version, dstip):
    client = MBClient(CLIENTID_PREFIX + str(id), version)
    client.dst = dstip
    client.dstPort = 8885
    client.cleanStart = True
    client.useTLS = True
    client.username = OAUTH_TOKEN_NAME
    client.password = tokenList[id%NUM_O_CLIENTS] # read from pre-populated list

    return client

# Read command line arguments
def readArgs():
    global destlist, g_OAuthServerIP, g_OAuthServerPort, g_logger, g_retryCount

    parser = argparse.ArgumentParser(description='Creates the mqttbench client list file for the CONNBURST.TLS.OAUTH benchmark', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--destlist", required=True, dest="destlist", metavar="<comma separated list of IP or DNS name>", help="list of IPv4 addresses or DNS name of MessageSight server messaging endpoints")
    parser.add_argument("--OAuthServer", required=True, dest="OAuthServer", metavar="<IP or DNS name or OAuth server>", help="IPv4 addresses or DNS name of the OAuth server")
    parser.add_argument("--OAuthServerPort", type=int, default=9443, dest="OAuthServerPort", metavar="<server port of the OAuth server>", help="server port of the OAuth server (default 443)")
    parser.add_argument('-l', "--logLevel", type=str, dest='logLevel', metavar='<logLevel>', help="The case-insensitive level of logging to use (CRITICAL,ERROR,WARNING,INFO,DEBUG,NOTSET).")
    parser.add_argument("--requestRetries", type=int, dest="retryCount", metavar="<num retries>", help="specify the number of attempts for each REST API operation before giving up")
    args = parser.parse_args()
    
    destlist.extend(args.destlist.split(","))
    g_OAuthServerIP = args.OAuthServer
    g_OAuthServerPort = args.OAuthServerPort

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

# Decrease verbosity for REST requests
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)
requests.packages.urllib3.disable_warnings(InsecurePlatformWarning)
requests.packages.urllib3.disable_warnings(SNIMissingWarning)

# Set signal handlers on the main thread
signal.signal(signal.SIGTERM, sigHandler)
signal.signal(signal.SIGINT, sigHandler)

destlist = []
clientlist = []
tokenList = []
clfile = None
try:
    clfile = open(CLIENTLIST_FILE, "w")        
except Exception as e:
    print("Failed to open {0} for writing, exception {1}".format(CLIENTLIST_FILE, e))
    exit(1)

# Process the command line arguments
readArgs()

# Pre-populate tokenList with oauth tokens
pool = ThreadPool(NUMTHREADS) 
tokenList = pool.map(getOAuthTokenMT, range(NUM_O_CLIENTS))
writeLog(LOGL.INFO, "size of token list is {0}".format(len(tokenList)))
pool.close()
pool.join()
writeLog(LOGL.INFO, "Completed generation of OAUTH tokens. size of token list is {0}".format(len(tokenList)))
writeLog(LOGL.INFO, "Start creating mqttbench client list")

# Create client entries in the mqttbench client list file
count = 0
for i in range(0, int(NUMCLIENTS)):
    idx = count % len(destlist)
    dstip = destlist[idx]
    client = createClient(i, MBCONSTANTS.MQTT_TCP5, dstip)
    clientlist.append(client)
    count += 1
    
writeLog(LOGL.INFO, "Start write mqttbench client list file to file")
MBCL.MBshuffleClients(clientlist)
MBCL.MBprintClients(clientlist, clfile)

