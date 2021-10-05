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
# This script will setup a MessageSight server for performance benchmarking. Deletes
# Demo objects, creates endpoints, messaging policies, connection policies, etc.
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

#################
### Constants ###
#################
RESTTIMEOUT=90
MAX_WAIT_ATTEMPTS=20
IMASERVICE="Server"
MQCONNSERVICE="MQConnectivity"
STATUS_RUNNING="Running"
STATUS_ACTIVE="Active"
STATUS_STANDBY="Standby"

DEFAULT_ADMIN_USERNAME="admin"
DEFAULT_ADMIN_PASSWORD="admin"
DEFAULT_ADMIN_PORT=9089
SERVER_CERT_PATH="certs/server-trust-chain.pem"
SERVER_CERT_NAME="server-trust-chain.pem"
SERVER_KEY_PATH="certs/key-server.pem"
SERVER_KEY_NAME="key-server.pem"
TRUSTED_CERT_INT_PATH="certs/cert-client-intCA.pem"
TRUSTED_CERT_INT_NAME="cert-client-intCA.pem"
TRUSTED_CERT_ROOT_PATH="certs/cert-rootCA.pem"
TRUSTED_CERT_ROOT_NAME="cert-rootCA.pem"
PSK_FILE_PATH="certs/psk.csv.gz"
PSK_TMP_PATH="/tmp/psk.csv"
PSK_FILE_NAME="psk.csv"
CRL_CERT_PATH="certs/cert-client-intCA-crl.pem.gz"
CRL_CERT_NAME="cert-client-intCA-crl.pem"
CRL_PROF_NAME="PerfCRLProfile"
OAUTH_SERVER_DNS="<hostname of OAuth server>"
OAUTH_PROF="PerfOAuthProfile"
OAUTH_CERT_PATH="certs/certandkey-oauth.pem"
OAUTH_CERT_NAME="certandkey-oauth.pem"
LDAP_SERVER_DNS="<hostname of ldap server>"
LDAP_CERT_PATH="certs/cert-ldap.pem"
LDAP_CERT_NAME="cert-ldap.pem"
MQ_KDB_PATH="certs/mq.kdb"
MQ_KDB_NAME="mq.kdb"
MQ_KDB_STASH_PATH="certs/mq.sth"
MQ_KDB_STASH_NAME="mq.sth"

CERT_PROF_SERVER="PerfServerCertProfile"
CERT_PROF_TRUSTEDCA="PerfTrustedCACertProfile"
SEC_PROF_NOAUTH="PerfNoAuthSecurityProfile"
SEC_PROF_CLNTCERTAUTH="PerfClntCertSecurityProfile"
SEC_PROF_OAUTH="PerfOAuthSecurityProfile"
SEC_PROF_LDAPAUTH="PerfLDAPSecurityProfile"

MHUB_PERF="PerfMessageHub"
CONN_POLICY_INTERNET="InternetConnectionPolicy"
CONN_POLICY_INTRANET="IntranetConnectionPolicy"
TOPIC_POLICY_INTERNET="InternetTopicPolicy"
TOPIC_POLICY_INTRANET="IntranetTopicPolicy"
SUB_POLICY_INTRANET="IntranetSharedSubscriptionPolicy"
QUEUE_POLICY_INTRANET="IntranetQueuePolicy"

ENDP_NONSECURE_INTERNET="InternetNonSecure"
ENDP_NONSECURE_INTERNET_LARGE_POLICIES="InternetNonSecureLargePolicies"
ENDP_SECURE_NOAUTH_INTERNET="InternetSecureNoAuth"
ENDP_SECURE_CLNTCERTAUTH_INTERNET="InternetSecureClntCertAuth"
ENDP_SECURE_OAUTH_INTERNET="InternetSecureOAuth"
ENDP_SECURE_LDAPAUTH_INTERNET="InternetSecureLDAPAuth"
ENDP_NONSECURE_INTRANET="IntranetNonSecure"
ENDP_SECURE_NOAUTH_INTRANET="IntranetSecureNoAuth"

NUM_MQ_QMGR=5
NUM_CONN_PER_QMGR=10
MQ_PORT=1414
MQCONN_QM_CONN="PerfQMgr"
MQCONN_QM="QM"
MQCONN_CHANNEL_USER="imauser"
MQCONN_CHANNEL_PASS="password"
MQCONN_MS2MQ_NP_RULE="Perf-MS-to-MQ-NonPersist-rule"
MQCONN_MS2MQ_P_RULE="Perf-MS-to-MQ-Persist-rule"
MQCONN_MS2MQ_RULE_NP_SRC="mqconn/mqtt2mq/nonpersist"
MQCONN_MS2MQ_RULE_P_SRC="mqconn/mqtt2mq/persist"
MQCONN_MS2MQ_RULE_NONPERSIST_DST="IMA2MQ.NP.QUEUE"
MQCONN_MS2MQ_RULE_PERSIST_DST="IMA2MQ.P.QUEUE"
MQCONN_MQ2MS_RULE="Perf-MQ-to-MS-rule"
MQCONN_MQ2MS_RULE_SRC="MQ2IMA/TOPICTREE"
MQCONN_MQ2MS_RULE_DST="mqconn/mq2mqtt/perf"

NUMADMINSUBS=10
ADMINSUB_PART1="/perfapp"
ADMINSUB_PART2="/mqttfanin/"
ADMINSUB_PART3="-adminsubs/part"

NUMJMSQUEUES=100
JMSQUEUE_NAME="Queue"
TRACEMAX="400MB"

HAGROUP="PERFHA"
HA_ROLE_PRIMARY="PRIMARY"
HA_ROLE_STANDBY="STANDBY"

EXPORT_FILE="msperfconfig"

########################
### Global Variables ###
########################
g_logger = None
g_stopRequested = False
g_session = None
g_retryCount = 3

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
    adminEndpointStandby = None
    adminPort = DEFAULT_ADMIN_PORT
    primaryReplicaIntf = None
    primaryDiscoveryIntf = None
    standbyReplicaIntf = None
    standbyDiscoveryIntf = None
    configMQConn = False
    serverCert = None
    serverKey = None
    trustedIntCACert = None
    trustedRootCACert = None
    crlCert = None
    pskData = None
    oauthCert = None
    ldapCert = None
    mqKDB = None
    mqSTH = None
    
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

# Wait for the server to be in running state
def waitReady(msconfig, endpoint, service, status, retries=MAX_WAIT_ATTEMPTS):
    global RESTTIMEOUT
    
    ready = False
    attempts = 0
    URLSUFFIX = "/MQConnectivity" if service == MQCONNSERVICE else ""
    
    RESTTIMEOUT = 10 # decrease timeout for the wait loop
       
    restRequest = RestRequest()
    restResponse = None
    
    while attempts < retries:
        ## Check server status
        restRequest.method = RestMethod.GET
        restRequest.URL = "https://" + endpoint + ":" + str(msconfig.adminPort) + "/ima/service/status" + URLSUFFIX
        restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    
        responsePayload = None
        restResponse = invokeRESTCall(restRequest, g_retryCount, 0xDEAD)
        if restResponse.success == True:
            responsePayload = json.loads(restResponse.response.text)

        if restResponse.success == False and \
           not (isinstance(restResponse.exception,ConnectTimeout) or isinstance(restResponse.exception, ConnectionError)):
            writeLog(LOGL.ERROR, "Failed to get the server status. Exiting...", LOGP.EVENT)
            exit(1)
        else:
            if not responsePayload is None and responsePayload[service]["Status"] == status:
                ready = True
                break
        
        sleep(5)
        attempts += 1
        writeLog(LOGL.INFO, "Waiting for service to be ready...", LOGP.EVENT)
     
    RESTTIMEOUT = 90 # restore the default timeout before exiting the wait function
    
    return ready
    
# Setup security artifacts
def exportConfig(msconfig):
    global g_retryCount
    
    restRequest = RestRequest()
    restResponse = None

    ## Create the server certificate profile
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/service/export"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"FileName": EXPORT_FILE, 
               "Password": "admin"}
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to export the server configuration. Exiting...", LOGP.EVENT)
        exit(1) 
        
# Verify that the server is in the expected HA role
def checkHARole(msconfig, endpoint, expectedRole, retries=MAX_WAIT_ATTEMPTS):
    global g_retryCount, RESTTIMEOUT
    
    RESTTIMEOUT = 10 # decrease timeout for the wait loop
    ready = False
    attempts = 0
       
    restRequest = RestRequest()
    restResponse = None
    
    while attempts < retries:
        ## Check server status
        restRequest.method = RestMethod.GET
        restRequest.URL = "https://" + endpoint + ":" + str(msconfig.adminPort) + "/ima/service/status"
        restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    
        restResponse = invokeRESTCall(restRequest, g_retryCount, 0xDEAD)
        responsePayload = json.loads(restResponse.response.text)
    
        if restResponse.success == False and restResponse.exception != ConnectTimeout:
            writeLog(LOGL.ERROR, "Failed to get the server status. Exiting...", LOGP.EVENT)
            exit(1)
        else:
            if responsePayload["HighAvailability"]["NewRole"] == expectedRole:
                ready = True
                break
        
        sleep(3)
        attempts += 1
     
    RESTTIMEOUT = 90 # restore the default timeout before exiting the wait function
    
    return ready
    

# Configure HA
def configureHA(msconfig):
    global g_retryCount
    
    restRequest = RestRequest()
    restResponse = None
    
    # Check that the server is in Running state
    if not waitReady(msconfig, msconfig.adminEndpointStandby, IMASERVICE, STATUS_RUNNING):
        writeLog(LOGL.INFO, "MessageSight standby server at {0} is not in {1} state, exiting...".format(msconfig.adminEndpointStandby, STATUS_RUNNING), LOGP.EVENT)
        exit(1)

    # Accept License (pre-reset accept license)
    writeLog(LOGL.INFO, "Accepting MessageSight license (pre-reset) on Standby MessageSight server...", LOGP.EVENT)
    acceptLicense(msconfig, msconfig.adminEndpointStandby)
    
    # Check that the server is in Running state
    if not waitReady(msconfig, msconfig.adminEndpointStandby, IMASERVICE, STATUS_RUNNING):
        writeLog(LOGL.INFO, "MessageSight standby server at {0} is not in {1} state, exiting...".format(msconfig.adminEndpointStandby, STATUS_RUNNING), LOGP.EVENT)
        exit(1)
    
    # Perform factory reset of Standby server
    writeLog(LOGL.INFO, "Factory reset of standby server configuration (this can take several seconds)...", LOGP.EVENT)
    factoryReset(msconfig, msconfig.adminEndpointStandby)
    
    sleep(8)  # add delay to account for slow stop
    
    # Check that the standby server is in Running state
    if not waitReady(msconfig, msconfig.adminEndpointStandby, IMASERVICE, STATUS_RUNNING):
        writeLog(LOGL.INFO, "MessageSight standby server at {0} is not in {1} state, exiting...".format(msconfig.adminEndpointStandby, STATUS_RUNNING), LOGP.EVENT)
        exit(1)
    
    writeLog(LOGL.INFO, "Locking down admin endpoint on standby...", LOGP.EVENT)
    lockDownAdminEndpoint(msconfig, msconfig.adminEndpointStandby)
    
    ## Enable HA on the primary
    writeLog(LOGL.INFO, "Enabling HA on primary", LOGP.EVENT)
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"HighAvailability": 
                 {"Group": HAGROUP,
                  "EnableHA": True,
                  "StartupMode": "AutoDetect",
                  "RemoteDiscoveryNIC": msconfig.standbyDiscoveryIntf,
                  "LocalReplicationNIC": msconfig.primaryReplicaIntf,
                  "LocalDiscoveryNIC": msconfig.primaryDiscoveryIntf,
                  "DiscoveryTimeout": 600,
                  "HeartbeatTimeout": 10,
                  "PreferredPrimary": True,
                  "UseSecuredConnections": True}
              } 
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to enable MessageSight HA on the primary. Exiting...", LOGP.EVENT)
        exit(1)
        
    ## Enable HA on the standby
    writeLog(LOGL.INFO, "Enabling HA on standby", LOGP.EVENT)
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpointStandby + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"HighAvailability": 
                 {"Group": HAGROUP,
                  "EnableHA": True,
                  "StartupMode": "AutoDetect",
                  "RemoteDiscoveryNIC": msconfig.primaryDiscoveryIntf,
                  "LocalReplicationNIC": msconfig.standbyReplicaIntf,
                  "LocalDiscoveryNIC": msconfig.standbyDiscoveryIntf,
                  "DiscoveryTimeout": 600,
                  "HeartbeatTimeout": 10,
                  "PreferredPrimary": False,
                  "UseSecuredConnections": True}
              } 
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to enable MessageSight HA on the standby. Exiting...", LOGP.EVENT)
        exit(1)
        
    # Restart the standby
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpointStandby + ":" + str(msconfig.adminPort) + "/ima/service/restart"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"Service": "Server"} 
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to restart the MessageSight server on the standby. Exiting...", LOGP.EVENT)
        exit(1)
        
    # Restart the primary
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/service/restart"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"Service": "Server"} 
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to restart the MessageSight server on the primary. Exiting...", LOGP.EVENT)
        exit(1)

    # Check that the primary server is in Running state
    if not waitReady(msconfig, msconfig.adminEndpoint, IMASERVICE, STATUS_RUNNING, 20):
        writeLog(LOGL.INFO, "MessageSight server at {0} is not in {1} state, exiting...".format(msconfig.adminEndpoint, STATUS_RUNNING), LOGP.EVENT)
        exit(1)

    # Check that the standby server is in Running state
    if not waitReady(msconfig, msconfig.adminEndpointStandby, IMASERVICE, STATUS_RUNNING):
        writeLog(LOGL.INFO, "MessageSight standby server at {0} is not in {1} state, exiting...".format(msconfig.adminEndpointStandby, STATUS_RUNNING), LOGP.EVENT)
        exit(1)
    
    # Check that the primary server has assumed the PRIMARY HA role    
    if not checkHARole(msconfig, msconfig.adminEndpoint, HA_ROLE_PRIMARY):
        writeLog(LOGL.INFO, "MessageSight server at {0} is not in HA role {1}, exiting...".format(msconfig.adminEndpoint, HA_ROLE_PRIMARY), LOGP.EVENT)
        exit(1)

    # Check that the standby server has assumed the STANDBY HA role
    if not checkHARole(msconfig, msconfig.adminEndpointStandby, HA_ROLE_STANDBY):
        writeLog(LOGL.INFO, "MessageSight standby server at {0} is not in HA role {1}, exiting...".format(msconfig.adminEndpointStandby, HA_ROLE_STANDBY), LOGP.EVENT)
        exit(1)
        
    writeLog(LOGL.INFO, "Completed MessageSight HA configuration", LOGP.EVENT)
        
# Configure MQ QMgr connections
def configureMQConn(msconfig):
    global g_retryCount
    
    restRequest = RestRequest()
    restResponse = None
    
    ## Enable MQConnectivity
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"MQConnectivityEnabled": True} 
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to enable MessageSight MQ Connectivity. Exiting...", LOGP.EVENT)
        exit(1)
        
    ## Restart MQConnectivity service
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/service/restart"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"Service": "MQConnectivity"} 
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to restart MessageSight MQ Connectivity. Exiting...", LOGP.EVENT)
        exit(1)

    sleep(3) # MQC<->IMAServer comms is a little flaky

    # Check that the MQConnectivity service is in Active state
    if not waitReady(msconfig, msconfig.adminEndpoint, MQCONNSERVICE, STATUS_ACTIVE):
        writeLog(LOGL.INFO, "MessageSight MQConnectivity service at {0} is not in {1} state, exiting...".format(msconfig.adminEndpoint, STATUS_ACTIVE), LOGP.EVENT)
        exit(1)

    ## Create a number of MQ QMgr connections
    qmgrConnsStr = ""
    count = 0
    for i in range(0, NUM_MQ_QMGR):
        for j in range(0, NUM_CONN_PER_QMGR):
            restRequest.method = RestMethod.POST
            restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
            restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
            restRequest.headers = {"Content-Type": "application/json"}
            payload = {"QueueManagerConnection": 
                         {MQCONN_QM_CONN + str(i) + "-" + str(j):
                            {"Verify": False,
                             "QueueManagerName": MQCONN_QM_CONN,
                             "ConnectionName": "<hostname of qmgr server>(" + str(MQ_PORT) + ")",
                             "ChannelName": "SYSTEM.IMA.SVRCONN",
                             "ChannelUserName": MQCONN_CHANNEL_USER,
                             "ChannelUserPassword": MQCONN_CHANNEL_PASS,
                             "Force": True}
                         }
                      }
            restRequest.payload = json.dumps(payload)
            
            restResponse = invokeRESTCall(restRequest, g_retryCount)
            responsePayload = json.loads(restResponse.response.text)
            
            if restResponse.success == False:
                writeLog(LOGL.ERROR, "Failed to configure MQ QMgr connection {0}. Exiting...".format(MQCONN_QM_CONN + str(i) + "-" + str(j)), LOGP.EVENT)
                exit(1)
            
            count += 1
            
            if count < NUM_MQ_QMGR * NUM_CONN_PER_QMGR:
                qmgrConnsStr += MQCONN_QM_CONN + str(i) + "-" + str(j) + ","
            else:
                qmgrConnsStr += MQCONN_QM_CONN + str(i) + "-" + str(j)
                
    ## Create the MessageSight to MQ destination mapping rule for QoS0 / Non-Persistent Queue
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"DestinationMappingRule": 
                 {MQCONN_MS2MQ_NP_RULE:
                    {"Enabled": True,
                     "QueueManagerConnection": qmgrConnsStr,
                     "RuleType": 5,
                     "Source": MQCONN_MS2MQ_RULE_NP_SRC,
                     "Destination": MQCONN_MS2MQ_RULE_NONPERSIST_DST,
                     "MaxMessages": 2000000}
                 }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to configure MQ Connectivity destination mapping rule {0}. Exiting...".format(MQCONN_MS2MQ_RULE), LOGP.EVENT)
        exit(1)

    ## Create the MessageSight to MQ destination mapping rule for QoS 1 and 2 / Persistent Queue
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"DestinationMappingRule": 
                 {MQCONN_MS2MQ_P_RULE:
                    {"Enabled": True,
                     "QueueManagerConnection": qmgrConnsStr,
                     "RuleType": 5,
                     "Source": MQCONN_MS2MQ_RULE_P_SRC,
                     "Destination": MQCONN_MS2MQ_RULE_PERSIST_DST,
                     "MaxMessages": 2000000}
                 }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to configure MQ Connectivity destination mapping rule {0}. Exiting...".format(MQCONN_MS2MQ_RULE), LOGP.EVENT)
        exit(1)
        
    ## Create the MQ to MessageSight destination mapping rule
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"DestinationMappingRule": 
                 {MQCONN_MQ2MS_RULE:
                    {"Enabled": True,
                     "QueueManagerConnection": qmgrConnsStr,
                     "RuleType": 9,
                     "Source": MQCONN_MQ2MS_RULE_SRC,
                     "Destination": MQCONN_MQ2MS_RULE_DST,
                     "MaxMessages": 2000000}
                 }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to configure MQ Connectivity destination mapping rule {0}. Exiting...".format(MQCONN_MQ2MS_RULE), LOGP.EVENT)
        exit(1)

# Configure the MQ certificate for secure communications
def configureMQCert(msconfig):
    global g_retryCount
    
    restRequest = RestRequest()
    restResponse = None

    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"MQCertificate":
                {"MQSSLKey": MQ_KDB_NAME,
                 "MQStashPassword": MQ_KDB_STASH_NAME,
                 "Overwrite": True}
               }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to configure the MQ certificate. Exiting...", LOGP.EVENT)
        exit(1) 

# Create Administrative subscriptions
def createAdminSubs(msconfig):
    global g_retryCount
    
    restRequest = RestRequest()
    restResponse = None
    
    ## Create a number of Administrative Subscriptions for the QoS 0 benchmark tests
    for i in range(0, NUMADMINSUBS):
        adminSubName=ADMINSUB_PART1 + str(i) + ADMINSUB_PART2 + "q0" + ADMINSUB_PART3 + str(i) + "/#"
        
        restRequest.method = RestMethod.POST
        restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
        restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
        restRequest.headers = {"Content-Type": "application/json"}
        payload = {"AdminSubscription": 
                    {adminSubName: 
                      {"Name": "AdminSub-Q0-Part" + str(i),
                       "SubscriptionPolicy": SUB_POLICY_INTRANET,
                       "MaxQualityOfService": 0}
                    }
                  }
        restRequest.payload = json.dumps(payload)
        
        restResponse = invokeRESTCall(restRequest, g_retryCount)
        responsePayload = json.loads(restResponse.response.text)
        
        if restResponse.success == False:
            writeLog(LOGL.ERROR, "Failed to create admin sub '{0}'. Exiting...".format(adminSubName), LOGP.EVENT)
            exit(1)
    
    ## Create a number of Administrative Subscriptions for the QoS 1 benchmark tests
    for i in range(0, NUMADMINSUBS):
        adminSubName=ADMINSUB_PART1 + str(i) + ADMINSUB_PART2 + "q1" + ADMINSUB_PART3 + str(i) + "/#"
        
        restRequest.method = RestMethod.POST
        restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
        restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
        restRequest.headers = {"Content-Type": "application/json"}
        payload = {"AdminSubscription": 
                    {adminSubName: 
                      {"Name": "AdminSub-Q1-Part" + str(i),
                       "SubscriptionPolicy": SUB_POLICY_INTRANET,
                       "MaxQualityOfService": 1}
                    }
                  }
        restRequest.payload = json.dumps(payload)
        
        restResponse = invokeRESTCall(restRequest, g_retryCount)
        responsePayload = json.loads(restResponse.response.text)
        
        if restResponse.success == False:
            writeLog(LOGL.ERROR, "Failed to create admin sub '{0}'. Exiting...".format(adminSubName), LOGP.EVENT)
            exit(1)
            
    ## Create a number of Administrative Subscriptions for the QoS 2 benchmark tests
    for i in range(0, NUMADMINSUBS):
        adminSubName=ADMINSUB_PART1 + str(i) + ADMINSUB_PART2 + "q2" + ADMINSUB_PART3 + str(i) + "/#"
        
        restRequest.method = RestMethod.POST
        restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
        restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
        restRequest.headers = {"Content-Type": "application/json"}
        payload = {"AdminSubscription": 
                    {adminSubName: 
                      {"Name": "AdminSub-Q2-Part" + str(i),
                       "SubscriptionPolicy": SUB_POLICY_INTRANET,
                       "MaxQualityOfService": 2}
                    }
                  }
        restRequest.payload = json.dumps(payload)
        
        restResponse = invokeRESTCall(restRequest, g_retryCount)
        responsePayload = json.loads(restResponse.response.text)
        
        if restResponse.success == False:
            writeLog(LOGL.ERROR, "Failed to create admin sub '{0}'. Exiting...".format(adminSubName), LOGP.EVENT)
            exit(1)

# Create Queue (for JMS clients)
def createQueues(msconfig):
    global g_retryCount
    
    restRequest = RestRequest()
    restResponse = None
    
    ## Create a number of JMS Queues
    for i in range(0, NUMJMSQUEUES):
        restRequest.method = RestMethod.POST
        restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
        restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
        restRequest.headers = {"Content-Type": "application/json"}
        payload = {"Queue": 
                    {JMSQUEUE_NAME + str(i): 
                      {"Description": "Queue used for JMS client performance benchmarks",
                       "AllowSend": True,
                       "ConcurrentConsumers": True,
                       "MaxMessages": 2000000}
                    }
                  }
        restRequest.payload = json.dumps(payload)
        
        restResponse = invokeRESTCall(restRequest, g_retryCount)
        responsePayload = json.loads(restResponse.response.text)
        
        if restResponse.success == False:
            writeLog(LOGL.ERROR, "Failed to create Queue '{0}'. Exiting...".format(JMSQUEUE_NAME + str(i)), LOGP.EVENT)
            exit(1)

# Create messaging endpoints
def createEndpoints(msconfig):
    global g_retryCount
    
    restRequest = RestRequest()
    restResponse = None

    ## Create the public Internet facing non-secure endpoint
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"Endpoint": 
                {ENDP_NONSECURE_INTERNET: 
                  {"Description": "Public Internet facing non-secure endpoint",
                   "Enabled": True,
                   "MessageHub": MHUB_PERF,
                   "Port": 1884,
                   "Interface": "All",
                   "Protocol": "All",
                   "ConnectionPolicies": CONN_POLICY_INTERNET,
                   "TopicPolicies": TOPIC_POLICY_INTERNET
                   }
                }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create endpoint '{0}'. Exiting...".format(ENDP_NONSECURE_INTERNET), LOGP.EVENT)
        exit(1)
    
    ## Create the public Internet facing non-secure large policies endpoint
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"Endpoint": 
                {ENDP_NONSECURE_INTERNET_LARGE_POLICIES: 
                  {"Description": "Public Internet facing non-secure large number of messaging policies endpoint",
                   "Enabled": True,
                   "MessageHub": MHUB_PERF,
                   "Port": 1885,
                   "Interface": "All",
                   "Protocol": "All",
                   "ConnectionPolicies": CONN_POLICY_INTERNET,
                   "TopicPolicies": TOPIC_POLICY_INTERNET
                   }
                }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create endpoint '{0}'. Exiting...".format(ENDP_NONSECURE_INTERNET_LARGE_POLICIES), LOGP.EVENT)
        exit(1)
        
    ## Create the public Internet facing secure NoAuth endpoint
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"Endpoint": 
                {ENDP_SECURE_NOAUTH_INTERNET: 
                  {"Description": "Public Internet facing secure NoAuth endpoint",
                   "Enabled": True,
                   "MessageHub": MHUB_PERF,
                   "Port": 8883,
                   "Interface": "All",
                   "Protocol": "All",
                   "SecurityProfile": SEC_PROF_NOAUTH,
                   "ConnectionPolicies": CONN_POLICY_INTERNET,
                   "TopicPolicies": TOPIC_POLICY_INTERNET
                   }
                }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create endpoint '{0}'. Exiting...".format(ENDP_SECURE_NOAUTH_INTERNET), LOGP.EVENT)
        exit(1)
        
    ## Create the public Internet facing secure ClientCertAuth endpoint
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"Endpoint": 
                {ENDP_SECURE_CLNTCERTAUTH_INTERNET: 
                  {"Description": "Public Internet facing secure ClientCertAuth endpoint",
                   "Enabled": True,
                   "MessageHub": MHUB_PERF,
                   "Port": 8884,
                   "Interface": "All",
                   "Protocol": "All",
                   "SecurityProfile": SEC_PROF_CLNTCERTAUTH,
                   "ConnectionPolicies": CONN_POLICY_INTERNET,
                   "TopicPolicies": TOPIC_POLICY_INTERNET
                   }
                }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create endpoint '{0}'. Exiting...".format(ENDP_SECURE_CLNTCERTAUTH_INTERNET), LOGP.EVENT)
        exit(1)
        
    ## Create the public Internet facing secure OAuth endpoint
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"Endpoint": 
                {ENDP_SECURE_OAUTH_INTERNET: 
                  {"Description": "Public Internet facing secure OAuth endpoint",
                   "Enabled": True,
                   "MessageHub": MHUB_PERF,
                   "Port": 8885,
                   "Interface": "All",
                   "Protocol": "All",
                   "SecurityProfile": SEC_PROF_OAUTH,
                   "ConnectionPolicies": CONN_POLICY_INTERNET,
                   "TopicPolicies": TOPIC_POLICY_INTERNET
                   }
                }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create endpoint '{0}'. Exiting...".format(ENDP_SECURE_OAUTH_INTERNET), LOGP.EVENT)
        exit(1)
        
    ## Create the public Internet facing secure LDAPAuth endpoint
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"Endpoint": 
                {ENDP_SECURE_LDAPAUTH_INTERNET: 
                  {"Description": "Public Internet facing secure LDAPAuth endpoint",
                   "Enabled": True,
                   "MessageHub": MHUB_PERF,
                   "Port": 8886,
                   "Interface": "All",
                   "Protocol": "All",
                   "SecurityProfile": SEC_PROF_LDAPAUTH,
                   "ConnectionPolicies": CONN_POLICY_INTERNET,
                   "TopicPolicies": TOPIC_POLICY_INTERNET
                   }
                }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create endpoint '{0}'. Exiting...".format(ENDP_SECURE_LDAPAUTH_INTERNET), LOGP.EVENT)
        exit(1)
    
    ## Create the private Intranet facing non-secure endpoint
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"Endpoint": 
                {ENDP_NONSECURE_INTRANET: 
                  {"Description": "Private Intranet facing non-secure endpoint",
                   "Enabled": True,
                   "MessageHub": MHUB_PERF,
                   "Port": 16901,
                   "Interface": msconfig.adminEndpoint,  # private network only
                   "Protocol": "All",
                   "ConnectionPolicies": CONN_POLICY_INTRANET,
                   "TopicPolicies": TOPIC_POLICY_INTRANET,
                   "SubscriptionPolicies": SUB_POLICY_INTRANET,
                   "QueuePolicies": QUEUE_POLICY_INTRANET
                   }
                }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create endpoint '{0}'. Exiting...".format(ENDP_NONSECURE_INTRANET), LOGP.EVENT)
        exit(1)
        
    ## Create the private Intranet facing secure NoAuth endpoint
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"Endpoint": 
                {ENDP_SECURE_NOAUTH_INTRANET: 
                  {"Description": "Private Intranet facing secure NoAuth endpoint",
                   "Enabled": True,
                   "MessageHub": MHUB_PERF,
                   "Port": 16902,
                   "Interface": msconfig.adminEndpoint,
                   "Protocol": "All",
                   "SecurityProfile": SEC_PROF_NOAUTH,
                   "ConnectionPolicies": CONN_POLICY_INTRANET,
                   "TopicPolicies": TOPIC_POLICY_INTRANET,
                   "SubscriptionPolicies": SUB_POLICY_INTRANET,
                   "QueuePolicies": QUEUE_POLICY_INTRANET
                   }
                }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create endpoint '{0}'. Exiting...".format(ENDP_SECURE_NOAUTH_INTRANET), LOGP.EVENT)
        exit(1)
    
# Create the connection and messaging policies
def createPolicies(msconfig):
    global g_retryCount
    
    restRequest = RestRequest()
    restResponse = None

    ## Create the public Internet facing connection policy
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"ConnectionPolicy": 
                {CONN_POLICY_INTERNET: 
                  {"Description": "Public Internet facing connection policy",
                   "ClientID": "*",
                   "AllowDurable": True,
                   "AllowPersistentMessages": True,
                   "ExpectedMessageRate": "Low",
                   "MaxSessionExpiryInterval": 0x7FFFFFFF}
                }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create connection policy '{0}'. Exiting...".format(CONN_POLICY_INTERNET), LOGP.EVENT)
        exit(1)
        
    ## Create the private Intranet facing connection policy
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"ConnectionPolicy": 
                {CONN_POLICY_INTRANET: 
                  {"Description": "Private Intranet facing connection policy",
                   "ClientID": "*",
                   "AllowDurable": True,
                   "AllowPersistentMessages": True,
                   "ExpectedMessageRate": "Max"}
                }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create connection policy '{0}'. Exiting...".format(CONN_POLICY_INTERNET), LOGP.EVENT)
        exit(1)
    
    ## Create the public Internet facing topic policy
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"TopicPolicy": 
                {TOPIC_POLICY_INTERNET: 
                  {"Description": "Public Internet facing topic policy",
                   "ClientID": "*",
                   "Topic": "*",
                   "ActionList": "Publish,Subscribe",
                   "MaxMessages": 5000,
                   "MaxMessagesBehavior": "DiscardOldMessages"}
#                   "MaxMessageTimeToLive": "1000000"}        # cannot re-enable until defect 217467 (msg expiry lock contention) is resolved
                }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create topic policy '{0}'. Exiting...".format(TOPIC_POLICY_INTERNET), LOGP.EVENT)
        exit(1)

    ## Create the private Intranet facing topic policy
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"TopicPolicy": 
                {TOPIC_POLICY_INTRANET: 
                  {"Description": "Private Intranet facing topic policy",
                   "ClientID": "*",
                   "Topic": "*",
                   "ActionList": "Publish,Subscribe",
                   "MaxMessages": 2000000,
                   "MaxMessagesBehavior": "DiscardOldMessages"}
                }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create topic policy '{0}'. Exiting...".format(TOPIC_POLICY_INTRANET), LOGP.EVENT)
        exit(1)
    
    ## Create the private Intranet facing shared subscription policy
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"SubscriptionPolicy": 
                {SUB_POLICY_INTRANET: 
                  {"Description": "Private Intranet facing shared subscription policy",
                   "ClientID": "*",
                   "Subscription": "*",
                   "ActionList": "Receive,Control",
                   "MaxMessages": 2000000,
                   "MaxMessagesBehavior": "DiscardOldMessages"}
                }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create shared subscription policy '{0}'. Exiting...".format(SUB_POLICY_INTRANET), LOGP.EVENT)
        exit(1)

    ## Create the private Intranet facing Queue policy
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"QueuePolicy": 
                {QUEUE_POLICY_INTRANET: 
                  {"Description": "Private Intranet facing Queue policy",
                   "ClientID": "*",
                   "Queue": "*",
                   "ActionList": "Send,Receive,Browse"}
                }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create Queue policy '{0}'. Exiting...".format(QUEUE_POLICY_INTRANET), LOGP.EVENT)
        exit(1)

# Create the Performance message hub
def createMessageHub(msconfig):
    global g_retryCount
    
    restRequest = RestRequest()
    restResponse = None

    ## Create the server certificate profile
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"MessageHub": 
                {MHUB_PERF: 
                  {"Description": "Message Hub for Performance benchmarking"}
                }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create Message Hub '{0}'. Exiting...".format(MHUB_PERF), LOGP.EVENT)
        exit(1)
        
# Setup security artifacts
def setupSecurity(msconfig):
    global g_retryCount
    
    restRequest = RestRequest()
    restResponse = None

    ## Create the server certificate profile
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"CertificateProfile": 
                {CERT_PROF_SERVER: 
                  {"Certificate": SERVER_CERT_NAME, 
                   "Key": SERVER_KEY_NAME, 
                   "Overwrite": True}
                 }
               }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create certificate profile '{0}'. Exiting...".format(CERT_PROF_SERVER), LOGP.EVENT)
        exit(1) 

    ## Create the NoAuth server security profile
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"SecurityProfile": 
                {SEC_PROF_NOAUTH: 
                    {"TLSEnabled": True, 
                     "MinimumProtocolMethod": "TLSv1.2", 
                     "UsePasswordAuthentication": False, 
                     "Ciphers": "Best",
                     "CertificateProfile": CERT_PROF_SERVER}
                 }
               }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create security profile '{0}'. Exiting...".format(SEC_PROF_NOAUTH), LOGP.EVENT)
        exit(1)
    
    ## Create a CRL Profile for the ClientCertAuth security profile
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"CRLProfile": 
                {CRL_PROF_NAME: 
                   {"CRLSource": CRL_CERT_NAME,
                    "Overwrite": True}  
                 }
               }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount) 
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create CRL profile {0}. Exiting...".format(CRL_CERT_PATH), LOGP.EVENT)
        exit(1)
        
    ## Create the ClientCertAuth server security profile
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"SecurityProfile": 
                {SEC_PROF_CLNTCERTAUTH: 
                    {"TLSEnabled": True, 
                     "MinimumProtocolMethod": "TLSv1.2", 
                     "UsePasswordAuthentication": False, 
                     "Ciphers": "Best",
                     "CertificateProfile": CERT_PROF_SERVER,
                     "CRLProfile": CRL_PROF_NAME,
                     "UseClientCertificate": True}
                 }
               }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create security profile '{0}'. Exiting...".format(SEC_PROF_CLNTCERTAUTH), LOGP.EVENT)
        exit(1)
        
    ## Associate Trusted CA cert with the ClientCertAuth server security profile
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"TrustedCertificate": 
                [{"TrustedCertificate": TRUSTED_CERT_INT_NAME,
                  "SecurityProfileName": SEC_PROF_CLNTCERTAUTH,
                  "Overwrite": True  
                 },
                 {"TrustedCertificate": TRUSTED_CERT_ROOT_NAME,
                  "SecurityProfileName": SEC_PROF_CLNTCERTAUTH,
                  "Overwrite": True  
                 }]
               }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to associate trusted CA certs {0} and {1} with security profile '{1}'. Exiting...".format(TRUSTED_CERT_INT_PATH, TRUSTED_CERT_ROOT_PATH, SEC_PROF_CLNTCERTAUTH), LOGP.EVENT)
        exit(1)
    
    ## Apply the PSK file
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"PreSharedKey": PSK_FILE_NAME}
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount) 
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to apply PSK file {0}. Exiting...".format(PSK_FILE_PATH), LOGP.EVENT)
        exit(1)
        
    ## Create the OAuth security profile
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"SecurityProfile": 
                {SEC_PROF_OAUTH: 
                    {"TLSEnabled": True, 
                     "MinimumProtocolMethod": "TLSv1.2", 
                     "UsePasswordAuthentication": True, 
                     "Ciphers": "Best",
                     "CertificateProfile": CERT_PROF_SERVER,
                     "OAuthProfile": OAUTH_PROF}
                 }
               }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create security profile '{0}'. Exiting...".format(SEC_PROF_CLNTCERTAUTH), LOGP.EVENT)
        exit(1)
        
    ## Create the LDAP Auth security profile
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"SecurityProfile": 
                {SEC_PROF_LDAPAUTH: 
                    {"TLSEnabled": True, 
                     "MinimumProtocolMethod": "TLSv1.2", 
                     "UsePasswordAuthentication": True, 
                     "Ciphers": "Best",
                     "CertificateProfile": CERT_PROF_SERVER}
                 }
               }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create security profile '{0}'. Exiting...".format(SEC_PROF_CLNTCERTAUTH), LOGP.EVENT)
        exit(1)

# Delete default demo artifacts from pristine install
def deleteDemoArtifacts(msconfig):
    global g_retryCount
    
    restRequest = RestRequest()
    restResponse = None
          
    ## Delete demo endpoint
    restRequest.method = RestMethod.DELETE
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration/Endpoint/DemoEndpoint"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount, 404)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to delete demo endpoint. Exiting...", LOGP.EVENT)
        exit(1)

    ## Delete demo MQTT endpoint
    restRequest.method = RestMethod.DELETE
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration/Endpoint/DemoMqttEndpoint"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount, 404)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to delete demo MQTT endpoint. Exiting...", LOGP.EVENT)
        exit(1)
        
    ## Delete demo messaging policy
    restRequest.method = RestMethod.DELETE
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration/SubscriptionPolicy/DemoSubscriptionPolicy"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount, 404)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to delete demo subscription policy. Exiting...", LOGP.EVENT)
        exit(1)
        
    ## Delete demo topic policy
    restRequest.method = RestMethod.DELETE
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration/TopicPolicy/DemoTopicPolicy"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount, 404)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to delete demo topic policy. Exiting...", LOGP.EVENT)
        exit(1)
        
    ## Delete demo connection policy
    restRequest.method = RestMethod.DELETE
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration/ConnectionPolicy/DemoConnectionPolicy"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)

    restResponse = invokeRESTCall(restRequest, g_retryCount, 404)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to delete demo connection policy. Exiting...", LOGP.EVENT)
        exit(1)   
        
    ## Delete demo message hub
    restRequest.method = RestMethod.DELETE
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration/MessageHub/DemoHub"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)

    restResponse = invokeRESTCall(restRequest, g_retryCount, 404)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to delete demo message hub. Exiting...", LOGP.EVENT)
        exit(1) 

# Locks down the admin endpoint to the interface that was used to connect to
def lockDownAdminEndpoint(msconfig, endpoint):
    global g_retryCount
    
    restRequest = RestRequest()
    restResponse = None
          
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + endpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = { "AdminEndpoint": {"Interface": endpoint}}
    restRequest.payload = json.dumps(payload)

    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to lock down the Admin Endpoint. Exiting...", LOGP.EVENT)
        exit(1)

# Configure the LDAP connnection
def configureLDAP(msconfig):
    global g_retryCount
    
    restRequest = RestRequest()
    restResponse = None

    ## Configure the LDAP connection
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"LDAP":
                  {"Enabled": False,
                   "Verify": False,
                   "URL": "ldaps://" + LDAP_SERVER_DNS + ":" + str(689),
                   "Certificate": LDAP_CERT_NAME,
                   "BindDN": "cn=manager,o=imadev,dc=ibm,dc=com",
                   "BindPassword": "secret",
                   "BaseDN": "ou=PERF,o=imadev,dc=ibm,dc=com",
                   "UserSuffix": "ou=users,ou=PERF,o=imadev,dc=ibm,dc=com",
                   "UserIdMap": "uid",
                   "GroupSuffix": "ou=groups,ou=PERF,o=imadev,dc=ibm,dc=com",
                   "GroupIdMap": "cn",
                   "GroupMemberIdMap": "member",
                   "EnableCache": True,
                   "CacheTimeout": 10,
                   "GroupCacheTimeout": 600,
                   "IgnoreCase": False,
                   "Timeout": 30,
                   "MaxConnections": 100,
                   "Overwrite": True}
               }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to configure LDAP. Exiting...", LOGP.EVENT)
        exit(1)

# Create an OAuth Profile
def createOAuthProfile(msconfig):
    global g_retryCount
    
    restRequest = RestRequest()
    restResponse = None

    ## Configure the OAuth server connection
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"OAuthProfile": 
                {OAUTH_PROF: 
                  {"KeyFileName": OAUTH_CERT_NAME,
                   "AuthKey": "access_token",
                   "ResourceURL": "https://" + OAUTH_SERVER_DNS + ":" + str(9443) + "/oauth2/endpoint/DemoProvider/authorize",
                   #"UserInfoURL": "",                  # for connection test not required. Makes test setup easier
                   #"UserInfoKey": "username",          # for connection test not required. Makes test setup easier
                   "Overwrite": True}
                 }
               }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to create OAuth profile '{0}'. Exiting...".format(OAUTH_PROF), LOGP.EVENT)
        exit(1) 

# Set TraceMax size
def setTraceMax(msconfig):
    global g_retryCount
    
    restRequest = RestRequest()
    restResponse = None

    ## Set TraceMax
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"TraceMax": TRACEMAX} 
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to set TraceMax to '{0}'. Exiting...".format(TRACEMAX), LOGP.EVENT)
        exit(1) 
        
# Upload certificates/keys
def uploadCerts(msconfig):
    global g_retryCount, RESTTIMEOUT
    
    restRequest = RestRequest()
    restResponse = None
          
    ## Upload the server certificate file
    restRequest.method = RestMethod.PUT
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/file/" + SERVER_CERT_NAME
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.payload = msconfig.serverCert

    restResponse = invokeRESTCall(restRequest, g_retryCount)
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to upload the server certificate file {0}. Exiting...".format(SERVER_CERT_PATH), LOGP.EVENT)
        exit(1)

    ## Upload the server key file
    restRequest.method = RestMethod.PUT
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/file/" + SERVER_KEY_NAME
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.payload = msconfig.serverKey

    restResponse = invokeRESTCall(restRequest, g_retryCount)
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to upload the server key file {0}. Exiting...".format(SERVER_KEY_PATH), LOGP.EVENT)
        exit(1)
        
    ## Upload the trusted intermediate CA cert file
    restRequest.method = RestMethod.PUT
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/file/" + TRUSTED_CERT_INT_NAME
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.payload = msconfig.trustedIntCACert

    restResponse = invokeRESTCall(restRequest, g_retryCount)
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to upload the trusted CA certificate file {0}. Exiting...".format(TRUSTED_CERT_INT_PATH), LOGP.EVENT)
        exit(1)

    ## Upload the trusted root CA cert file
    restRequest.method = RestMethod.PUT
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/file/" + TRUSTED_CERT_ROOT_NAME
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.payload = msconfig.trustedRootCACert

    restResponse = invokeRESTCall(restRequest, g_retryCount)
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to upload the trusted CA certificate file {0}. Exiting...".format(TRUSTED_CERT_ROOT_PATH), LOGP.EVENT)
        exit(1)

    ## Upload the CRL cert file
    restRequest.method = RestMethod.PUT
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/file/" + CRL_CERT_NAME
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.payload = msconfig.crlCert

    restResponse = invokeRESTCall(restRequest, g_retryCount)
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to upload the CRL certificate file {0}. Exiting...".format(CRL_CERT_PATH), LOGP.EVENT)
        exit(1)

    ## Upload the PSK file
    writeLog(LOGL.INFO, "Uploading large pre-shared key (PSK) file (via scp)", LOGP.EVENT)
    tmpSrcPath = PSK_TMP_PATH
    tmpDstPath = "/tmp/userfiles"
    try:
        os.system("/usr/bin/scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -i ~/.ssh/id_rsa  " + tmpSrcPath + " root@" + msconfig.adminEndpoint + ":" + tmpDstPath + " &>/dev/null")
        os.remove(tmpSrcPath) # cleanup tmp file after upload
    except Exception as e:
        writeLog(LOGL.INFO, "Failed with exception {0}, while attempting to upload large PSK file {1}".format(e, PSK_TMP_PATH), LOGP.EVENT)
    
    ## Upload the OAuth cert file
    restRequest.method = RestMethod.PUT
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/file/" + OAUTH_CERT_NAME
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.payload = msconfig.oauthCert

    restResponse = invokeRESTCall(restRequest, g_retryCount)
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to upload the OAuth server cert file {0}. Exiting...".format(OAUTH_CERT_PATH), LOGP.EVENT)
        exit(1)
        
    ## Upload the LDAP cert file
    restRequest.method = RestMethod.PUT
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/file/" + LDAP_CERT_NAME
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.payload = msconfig.ldapCert

    restResponse = invokeRESTCall(restRequest, g_retryCount)
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to upload the LDAP server cert file {0}. Exiting...".format(LDAP_CERT_PATH), LOGP.EVENT)
        exit(1)

    ## Upload the MQ Key Database file
    restRequest.method = RestMethod.PUT
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/file/" + MQ_KDB_NAME
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.payload = msconfig.mqKDB

    restResponse = invokeRESTCall(restRequest, g_retryCount)
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to upload the MQ key database file {0}. Exiting...".format(MQ_KDB_PATH), LOGP.EVENT)
        exit(1)
        
    ## Upload the MQ Key Database Stash file
    restRequest.method = RestMethod.PUT
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/file/" + MQ_KDB_STASH_NAME
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.payload = msconfig.mqSTH

    restResponse = invokeRESTCall(restRequest, g_retryCount)
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to upload the MQ key database stash file {0}. Exiting...".format(MQ_KDB_STASH_PATH), LOGP.EVENT)
        exit(1)

# Accept MessageSight license
def acceptLicense(msconfig, endpoint):
    global g_retryCount
    
    restRequest = RestRequest()
    restResponse = None
          
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + endpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"LicensedUsage": "Production",
               "Accept": True}
    restRequest.payload = json.dumps(payload)

    restResponse = invokeRESTCall(restRequest, g_retryCount)
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to accept the MessageSight license. Exiting...", LOGP.EVENT)
        exit(1)
        
# Factory reset of server configuration
def factoryReset(msconfig, endpoint):
    global g_retryCount
    
    restRequest = RestRequest()
    restResponse = None
          
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + endpoint + ":" + str(msconfig.adminPort) + "/ima/service/restart"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"Service": "Server",
               "Reset": "config"}
    restRequest.payload = json.dumps(payload)

    restResponse = invokeRESTCall(restRequest, g_retryCount)
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to reset configuration for MessageSight server at {0}. Exiting...".format(endpoint), LOGP.EVENT)
        exit(1)

# Load certificates/keys from files
def loadCerts(msconfig):
    ## Load server certificate
    try:
        with open(SERVER_CERT_PATH, 'r') as f:
            msconfig.serverCert = f.read()
    except IOError as e:
        writeLog(LOGL.ERROR, "An error {0} occurred while reading server certificate file {1}".format(e, SERVER_CERT_PATH), LOGP.EVENT)
        exit(1)
    
    ## Load server key
    try:
        with open(SERVER_KEY_PATH, 'r') as f:
            msconfig.serverKey = f.read()
    except IOError as e:
        writeLog(LOGL.ERROR, "An error {0} occurred while reading server key file {1}".format(e, SERVER_KEY_PATH), LOGP.EVENT)
        exit(1)
        
    ## Load trusted CA certificates
    try:
        with open(TRUSTED_CERT_INT_PATH, 'r') as f:
            msconfig.trustedIntCACert = f.read()
    except IOError as e:
        writeLog(LOGL.ERROR, "An error {0} occurred while reading trusted CA cert file {1}".format(e, TRUSTED_CERT_INT_PATH), LOGP.EVENT)
        exit(1)

    try:
        with open(TRUSTED_CERT_ROOT_PATH, 'r') as f:
            msconfig.trustedRootCACert = f.read()
    except IOError as e:
        writeLog(LOGL.ERROR, "An error {0} occurred while reading trusted CA cert file {1}".format(e, TRUSTED_CERT_ROOT_PATH), LOGP.EVENT)
        exit(1)
  
    ## Load CRL cert
    try:
        with gzip.open(CRL_CERT_PATH, 'rb') as f:
            msconfig.crlCert = f.read()
    except IOError as e:
        writeLog(LOGL.ERROR, "An error {0} occurred while reading CRL cert file {1}".format(e, CRL_CERT_PATH), LOGP.EVENT)
        exit(1)
        
    ## Load PSK file
    try:
        with gzip.open(PSK_FILE_PATH, 'rb') as f, open(PSK_TMP_PATH, 'w') as t:
            msconfig.pskData = f.read()
            t.write(msconfig.pskData)
    except IOError as e:
        writeLog(LOGL.ERROR, "An error {0} occurred while reading PSK file {1} and writing tmp file {1}".format(e, PSK_FILE_PATH, PSK_TMP_PATH), LOGP.EVENT)
        exit(1)
        
    ## Load OAuth cert file
    try:
        with open(OAUTH_CERT_PATH, 'r') as f:
            msconfig.oauthCert = f.read()
    except IOError as e:
        writeLog(LOGL.ERROR, "An error {0} occurred while reading OAuth server cert file {1}".format(e, OAUTH_CERT_PATH), LOGP.EVENT)
        exit(1)

    ## Load LDAP cert file
    try:
        with open(LDAP_CERT_PATH, 'r') as f:
            msconfig.ldapCert = f.read()
    except IOError as e:
        writeLog(LOGL.ERROR, "An error {0} occurred while reading LDAP server cert file {1}".format(e, LDAP_CERT_PATH), LOGP.EVENT)
        exit(1)
        
    ## Load MQ Key Database file
    try:
        with open(MQ_KDB_PATH, 'r') as f:
            msconfig.mqKDB = f.read()
    except IOError as e:
        writeLog(LOGL.ERROR, "An error {0} occurred while reading MQ key database file {1}".format(e, MQ_KDB_PATH), LOGP.EVENT)
        exit(1)
        
    ## Load MQ KDB stash file
    try:
        with open(MQ_KDB_STASH_PATH, 'r') as f:
            msconfig.mqSTH = f.read()
    except IOError as e:
        writeLog(LOGL.ERROR, "An error {0} occurred while reading MQ key database stash file {1}".format(e, MQ_KDB_STASH_PATH), LOGP.EVENT)
        exit(1)
    

# Process command line arguments
def readArgs(msconfig): 
    global g_logger, g_retryCount, NUM_MQ_QMGR, NUM_CONN_PER_QMGR
    
    parser = argparse.ArgumentParser(description='Perform initial setup of MessageSight server for benchmarking', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-l', "--logLevel", type=str, dest='logLevel', metavar='<logLevel>', help="The case-insensitive level of logging to use (CRITICAL,ERROR,WARNING,INFO,DEBUG,NOTSET).")
    parser.add_argument("--requestRetries", type=int, dest="retryCount", metavar="<num retries>", help="specify the number of attempts for each REST API operation before giving up")
    parser.add_argument("--adminEndpoint", required=True, dest="adminEndpoint", metavar="<IPv4 address>", help="IPv4 address of the admin endpoint of the primary MessageSight server to send REST requests to")
    parser.add_argument("--adminEndpointStandby", dest="adminEndpointStandby", metavar="<IPv4 address>", help="IPv4 address of the admin endpoint of the standby MessageSight server to send REST requests to")
    parser.add_argument("--adminPort", type=int, dest="adminPort", metavar="<TCP port>", help="TCP port number of the admin endpoint to send REST requests to")
    parser.add_argument("--primaryReplicaIntf", dest="primaryReplicaIntf", metavar="<IPv4 address>", help="IPv4 address of the primary HA replication interface")
    parser.add_argument("--primaryDiscoveryIntf", dest="primaryDiscoveryIntf", metavar="<IPv4 address>", help="IPv4 address of the primary HA discovery interface")
    parser.add_argument("--standbyReplicaIntf", dest="standbyReplicaIntf", metavar="<IPv4 address>", help="IPv4 address of the standby HA replication interface")
    parser.add_argument("--standbyDiscoveryIntf", dest="standbyDiscoveryIntf", metavar="<IPv4 address>", help="IPv4 address of the standby HA discovery interface")
    parser.add_argument("--configMQConn", dest="configMQConn", action='store_true', help="Flag to indicate whether to configure MessageSight MQConnectivity")
    parser.add_argument("--numQMgr", type=int, dest="numQMgr", metavar="<positive integer>", help="Number of MQ QMgr systems used in the testing")
    parser.add_argument("--numConnPerQMgr", type=int, dest="numConnPerQMgr", metavar="<positive integer>", help="Number of connections per MQ QMgr used in the testing")
    
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

    if args.configMQConn:
        msconfig.configMQConn = True
        if not args.numQMgr is None and args.numQMgr > 0:
            NUM_MQ_QMGR = args.numQMgr
        if not args.numConnPerQMgr is None and args.numConnPerQMgr > 0:
            NUM_CONN_PER_QMGR = args.numConnPerQMgr
    
    haparams=0
    if not args.primaryReplicaIntf is None:
        msconfig.primaryReplicaIntf = args.primaryReplicaIntf
        haparams += 1

    if not args.primaryDiscoveryIntf is None:
        msconfig.primaryDiscoveryIntf = args.primaryDiscoveryIntf
        haparams += 1

    if not args.standbyReplicaIntf is None:
        msconfig.standbyReplicaIntf = args.standbyReplicaIntf
        haparams += 1

    if not args.standbyDiscoveryIntf is None:
        msconfig.standbyDiscoveryIntf = args.standbyDiscoveryIntf
        haparams += 1

    if not args.adminEndpointStandby is None:
        msconfig.adminEndpointStandby = args.adminEndpointStandby
        haparams += 1
        
    if haparams > 0 and haparams < 5:
        writeLog(LOGL.ERROR, "When configuring MessageSight HA you must provide all the following arguments: --primaryReplicaIntf, --primaryDiscoveryIntf, " \
                             "--standbyReplicaIntf, --standbyDiscoveryIntf, --adminEndpointStandby" , LOGP.EVENT)
        exit(1)
        
    msconfig.adminEndpoint = args.adminEndpoint

    writeLog(LOGL.DEBUG, "adminEndpoint = {0}".format(msconfig.adminEndpoint), LOGP.DATA)
    writeLog(LOGL.DEBUG, "API retry count = {0}".format(g_retryCount), LOGP.DATA)  

# Main loop through the list of orgs specified in the BAP input file
def setupMSServer(msconfig):     
   
    # Load certificates
    writeLog(LOGL.INFO, "Reading certificate files...", LOGP.EVENT)
    loadCerts(msconfig)
    
    # Check that the server is in Running state
    if not waitReady(msconfig, msconfig.adminEndpoint, IMASERVICE, STATUS_RUNNING):
        writeLog(LOGL.INFO, "MessageSight server at {0} is not in {1} state, exiting...".format(msconfig.adminEndpoint, STATUS_RUNNING), LOGP.EVENT)
        exit(1)

    # Accept License (pre-reset accept license)
    writeLog(LOGL.INFO, "Accepting MessageSight license (pre-reset)...", LOGP.EVENT)
    acceptLicense(msconfig, msconfig.adminEndpoint)
    
    # Check that the server is in Running state
    if not waitReady(msconfig, msconfig.adminEndpoint, IMASERVICE, STATUS_RUNNING):
        writeLog(LOGL.INFO, "MessageSight server at {0} is not in {1} state, exiting...".format(msconfig.adminEndpoint, STATUS_RUNNING), LOGP.EVENT)
        exit(1)

    # Reset the server configuration
    writeLog(LOGL.INFO, "Factory reset of server configuration (this can take a few minutes)...", LOGP.EVENT)
    factoryReset(msconfig, msconfig.adminEndpoint)
    
    sleep(8)  # add delay to account for slow stop
    
    # Check that the server is in Running state
    if not waitReady(msconfig, msconfig.adminEndpoint, IMASERVICE, STATUS_RUNNING):
        writeLog(LOGL.INFO, "MessageSight server at {0} is not in {1} state, exiting...".format(msconfig.adminEndpoint, STATUS_RUNNING), LOGP.EVENT)
        exit(1)
        
    # Accept License
    writeLog(LOGL.INFO, "Accepting MessageSight license (post-reset)...", LOGP.EVENT)
    acceptLicense(msconfig, msconfig.adminEndpoint)

    # Lock down admin endpoint
    writeLog(LOGL.INFO, "Locking down admin endpoint...", LOGP.EVENT)
    lockDownAdminEndpoint(msconfig, msconfig.adminEndpoint)

    # Delete Demo artifacts
    writeLog(LOGL.INFO, "Deleting demo artifacts...", LOGP.EVENT)
    deleteDemoArtifacts(msconfig)
    
    # Upload certificates
    writeLog(LOGL.INFO, "Upload certificates...", LOGP.EVENT)
    uploadCerts(msconfig)

    # Set TraceMax
    writeLog(LOGL.INFO, "Set TraceMax...", LOGP.EVENT)
    setTraceMax(msconfig)

    # Configure LDAP connection
    writeLog(LOGL.INFO, "Setting up LDAP connection (disabled by default)...", LOGP.EVENT)
    configureLDAP(msconfig)
    
    # Create an OAuth Profile
    writeLog(LOGL.INFO, "Creating OAuth Profile...", LOGP.EVENT)
    createOAuthProfile(msconfig)
    
    # Setup security 
    writeLog(LOGL.INFO, "Setting up security profile(s)...", LOGP.EVENT)
    setupSecurity(msconfig)
    
    # Create a MessageHub 
    writeLog(LOGL.INFO, "Creating the Perf Message Hub...", LOGP.EVENT)
    createMessageHub(msconfig)

    # Create connection and messaging policies
    writeLog(LOGL.INFO, "Creating connection and messaging policies...", LOGP.EVENT)
    createPolicies(msconfig)

    # Create endpoints
    writeLog(LOGL.INFO, "Creating messaging endpoints...", LOGP.EVENT)
    createEndpoints(msconfig)
    
    # Configure MQConnectivity if indicated by user
    if msconfig.configMQConn:
        # Set the MQ certificate
        writeLog(LOGL.INFO, "Configure MQ certificate ...", LOGP.EVENT)
        configureMQCert(msconfig)
        
        # Configure MQ QM Connections
        writeLog(LOGL.INFO, "Configure MQ QMgr connections ...", LOGP.EVENT)
        configureMQConn(msconfig)
    
    # Create Queues
    writeLog(LOGL.INFO, "Creating Queues (this can take several seconds) ...", LOGP.EVENT)
    createQueues(msconfig)

    # Create Administrative subscriptions
    writeLog(LOGL.INFO, "Creating Administrative Subscriptions ...", LOGP.EVENT)
    createAdminSubs(msconfig)
    
     # Configure HA
    if not msconfig.primaryDiscoveryIntf is None and \
       not msconfig.primaryReplicaIntf is None and \
       not msconfig.standbyDiscoveryIntf is None and \
       not msconfig.standbyReplicaIntf is None:
        writeLog(LOGL.INFO, "Configuring High Availability pair PRIMARY {0} / STANDBY {1}".format(msconfig.primaryReplicaIntf, msconfig.standbyReplicaIntf), LOGP.EVENT)
        configureHA(msconfig)
    
    # Export config 
    writeLog(LOGL.INFO, "Exporting configuration...", LOGP.EVENT)
    exportConfig(msconfig)   

# Check invoker of this script
def startedAsRoot():
    return subprocess.check_output('whoami').strip() == 'root'

# Check to see if user is root and rerun this script as root, if not
def main():
    msconfig = MSConfig()
    
    # Process command line arguments and store them in g_args[], also setup global logger in readArgs()
    readArgs(msconfig)
    
    if startedAsRoot():
        writeLog(LOGL.INFO, "Running {0} as {1}".format(sys.argv[0], subprocess.check_output('whoami').strip()), LOGP.EVENT)
        setupMSServer(msconfig)
        
        endTime = int(time.time())
        writeLog(LOGL.INFO, "Setup time: {0} seconds".format(endTime - startTime), LOGP.EVENT)
    else:
        cmd = "sudo python {0} {1}".format(os.path.realpath(sys.argv[0]), " ".join(sys.argv[1:]))
        os.system(cmd)
    
### START OF MAIN ROUTINE ###
# Decrease verbosity for REST requests
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)
requests.packages.urllib3.disable_warnings(InsecurePlatformWarning)
requests.packages.urllib3.disable_warnings(SNIMissingWarning)

# Set signal handlers on the main thread
signal.signal(signal.SIGTERM, sigHandler)
signal.signal(signal.SIGINT, sigHandler)

# Start clock
startTime = int(time.time())

# Invoke Main function
main()

