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
# This script is used by the POLICIES benchmark test to configure the 
# InternetNonSecureLargePolicies endpoint with the number of messaging policies required
# for each test.
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
OAUTH_CERT_PATH="certs/cert-oauth.pem"
OAUTH_CERT_NAME="cert-oauth.pem"
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
TOPIC_POLICY_LARGE="InternetTopicLargePoliciesPolicy"

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
MQCONN_MS2MQ_RULE="Perf-MS-to-MQ-rule"
MQCONN_MS2MQ_RULE_SRC="mqconn/mqtt2mq/perf"
MQCONN_MS2MQ_RULE_DST="IMA2MQ.QUEUE"
MQCONN_MQ2MS_RULE="Perf-MQ-to-MS-rule"
MQCONN_MQ2MS_RULE_SRC="MQ2IMA/TOPICTREE"
MQCONN_MQ2MS_RULE_DST="mqconn/mq2mqtt/perf"

NUMJMSQUEUES=100
JMSQUEUE_NAME="Queue"
TRACEMAX="400MB"

MAX_POLICIES=100

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
g_PolicyList = ""

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
    numPolicies = None
    
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

# Create the list of policies to apply to the large policy endpoint
def createMsgPolicies(msconfig):
    global g_retryCount, g_PolicyList

    writeLog(LOGL.INFO, "Creating {0} topic policies for endpoint {1}...".format(msconfig.numPolicies, ENDP_NONSECURE_INTERNET_LARGE_POLICIES), LOGP.EVENT)
    
    restRequest = RestRequest()
    restResponse = None

    for i in range(0, msconfig.numPolicies - 1):  # -1 is because the last policy (which authorizes the publish) is appended at the end
        policyName = TOPIC_POLICY_LARGE + str(i)
        
        ## Create a topic policy (which will not authorize any publishes)
        restRequest.method = RestMethod.POST
        restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
        restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
        restRequest.headers = {"Content-Type": "application/json"}
        payload = {"TopicPolicy": 
                    {policyName: 
                      {"Description": "Policy {0} of the large policy benchmark test".format(str(i)),
                       "ClientAddress": "127.0.0.1",
                       "ClientID": "*",
                       "Topic": "ThisWillAuthorizeNothing",
                       "ActionList": "Publish,Subscribe",
                       "MaxMessages": 1000,
                       "MaxMessagesBehavior": "DiscardOldMessages",
                       "MaxMessageTimeToLive": "1000000"}
                    }
                  }
        restRequest.payload = json.dumps(payload)
        
        restResponse = invokeRESTCall(restRequest, g_retryCount)
        responsePayload = json.loads(restResponse.response.text)
        
        if restResponse.success == False:
            writeLog(LOGL.ERROR, "Failed to create topic policy {0}. Exiting...".format(policyName), LOGP.EVENT)
            exit(1)
        else:
            if len(g_PolicyList) == 0:
                g_PolicyList = policyName + ","
            else:
                g_PolicyList = g_PolicyList + policyName + ","
    
    # Append the authorizing policy to the end of the policy list
    g_PolicyList = g_PolicyList + TOPIC_POLICY_INTERNET    

# Update the large policy endpoint with the requested number of messaging policies
def updateEndpoint(msconfig):
    global g_retryCount, g_PolicyList
    
    writeLog(LOGL.INFO, "Updating the {0} endpoint...".format(ENDP_NONSECURE_INTERNET_LARGE_POLICIES), LOGP.EVENT)
    
    restRequest = RestRequest()
    restResponse = None

    ## Create the server certificate profile
    restRequest.method = RestMethod.POST
    restRequest.URL = "https://" + msconfig.adminEndpoint + ":" + str(msconfig.adminPort) + "/ima/configuration"
    restRequest.auth = HTTPBasicAuth(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD)
    restRequest.headers = {"Content-Type": "application/json"}
    payload = {"Endpoint": 
                {ENDP_NONSECURE_INTERNET_LARGE_POLICIES: 
                  {"TopicPolicies": g_PolicyList}
                }
              }
    restRequest.payload = json.dumps(payload)
    
    restResponse = invokeRESTCall(restRequest, g_retryCount)
    responsePayload = json.loads(restResponse.response.text)
    
    if restResponse.success == False:
        writeLog(LOGL.ERROR, "Failed to update endpoint {0}. Exiting...".format(ENDP_NONSECURE_INTERNET_LARGE_POLICIES), LOGP.EVENT)
        exit(1)
    
# Process command line arguments
def readArgs(msconfig): 
    global g_logger, g_retryCount
    
    parser = argparse.ArgumentParser(description="Update the {0} endpoint for the POLICIES benchmark test".format(ENDP_NONSECURE_INTERNET_LARGE_POLICIES), formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-l', "--logLevel", type=str, dest='logLevel', metavar='<logLevel>', help="The case-insensitive level of logging to use (CRITICAL,ERROR,WARNING,INFO,DEBUG,NOTSET).")
    parser.add_argument("--requestRetries", type=int, dest="retryCount", metavar="<num retries>", help="specify the number of attempts for each REST API operation before giving up")
    parser.add_argument("--adminEndpoint", required=True, dest="adminEndpoint", metavar="<IPv4 address>", help="IPv4 address of the admin endpoint of the primary MessageSight server to send REST requests to")
    parser.add_argument("--adminPort", type=int, dest="adminPort", metavar="<TCP port>", help="TCP port number of the admin endpoint to send REST requests to")
    parser.add_argument("--numPolicies", type=int, required=True, dest="numPolicies", metavar="<num policies>", help="Number of failing message policies to add to the {0} endpoint".format(ENDP_NONSECURE_INTERNET_LARGE_POLICIES))
    
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
    msconfig.numPolicies = args.numPolicies
    
    if msconfig.numPolicies > MAX_POLICIES or msconfig.numPolicies < 1:
        writeLog(LOGL.ERROR, "The requested number of messaging policies {0}, must be greater than 0 and less than {1}".format(msconfig.numPolicies, MAX_POLICIES) , LOGP.EVENT)
        exit(1)

    writeLog(LOGL.DEBUG, "adminEndpoint = {0}".format(msconfig.adminEndpoint), LOGP.DATA)
    writeLog(LOGL.DEBUG, "API retry count = {0}".format(g_retryCount), LOGP.DATA)  

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
        createMsgPolicies(msconfig)
        updateEndpoint(msconfig)
        
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

