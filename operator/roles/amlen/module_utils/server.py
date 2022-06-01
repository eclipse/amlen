#!/usr/bin/env python3
# Copyright (c) 2022 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0

import socket
import subprocess
import time
import sys
import os
import json
import requests
import logging
import traceback
import re
import base64
import yaml
import argparse
import shutil
from subprocess import Popen, PIPE, check_output, TimeoutExpired
from requests.auth import HTTPBasicAuth

from requests.packages.urllib3.exceptions import InsecureRequestWarning
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)

# Files with username/password
PASSWORD_FILE="/secrets/adminpassword/password"

# Certificates
CA_PUBLIC_CERT = "/secrets/internal_certs/ca.crt"
MSSERVER_CERT  = "/secrets/internal_certs/tls.crt"
MSSERVER_KEY   = "/secrets/internal_certs/tls.key"

def getUsername():
    return "sysadmin"

def getPassword():
    with open(PASSWORD_FILE) as f:
        return f.readline().rstrip()

class Server:
    'Server instance'

    def __init__(self, server, logger):
        self.serverName = server

        self.logger = logger

        self.logger.info("Created Amlen object with name %s" % (self.serverName))

        self.groupName = "amlen-k8s"
        self.files=[]
        self.config = {
                "MessageHub": {
                        "IoTMessageHub": {
                                "Description": "Amlen Message Hub"
                        }
                },
                "ConnectionPolicy": {
                        "IoTConnectionPolicy": {
                                "Protocol": "MQTT",
                                "AllowDurable": True,
                                "AllowPersistentMessages": True,
                                "Description": "Amlen Connection Policy",
                                "MaxSessionExpiryInterval": 3888000
                        }
                },
                "TopicPolicy": {
                        "IoTMessagingPolicy": {
                                "Topic": "*",
                                "ActionList": "Publish,Subscribe",
                                "Protocol": "MQTT",
                                "MaxMessages": 5000,
                                "MaxMessagesBehavior": "DiscardOldMessages",
                                "MaxMessageTimeToLive": "unlimited"
                        }
                },
                "SubscriptionPolicy": {
                        "IoTSubscriptionPolicy": {
                                "Subscription": "*",
                                "ActionList": "Receive,Control",
                                "Protocol": "MQTT",
                                "MaxMessages": 100000,
                                "MaxMessagesBehavior": "DiscardOldMessages"
                        }
                },
                "TrustedCertificate": [
                        {
                                "TrustedCertificate": "ca.crt",
                                "SecurityProfileName": "IoTSecurityProfile",
                                "Overwrite": True
                        }
                ],
                "CertificateProfile": {
                        "IoTCertificate": {
                                "Certificate": "tls.crt",
                                "Key": "tls.key",
                                "Overwrite": True
                        }
                },
                "SecurityProfile": {
                        "IoTSecurityProfile": {
                                "MinimumProtocolMethod": "TLSv1.2",
                                "MinimumProtocolMethod": "TLSv1.2",
                                "UseClientCertificate": False,
                                "Ciphers": "Fast",
                                "CertificateProfile": "IoTCertificate",
                                "UseClientCipher": False,
                                "UsePasswordAuthentication": False,
                                "TLSEnabled": True
                        }
                },
                "Endpoint": {
                        "IoTSecureEndpoint0": {
                                "Enabled": True,
                                "Port": 8883,
                                "Protocol": "MQTT",
                                "SecurityProfile": "IoTSecurityProfile",
                                "MessageHub": "IoTMessageHub",
                                "Interface": "all",
                                "MaxMessageSize": "1024KB",
                                "ConnectionPolicies": "IoTConnectionPolicy",
                                "TopicPolicies": "IoTMessagingPolicy",
                                "SubscriptionPolicies": "IoTSubscriptionPolicy"
                        }
                },
                "TraceBackupCount": 100
        }

    def setConfig(self,config):
        self.config = config

    def setCredentials(self,password):
        self.username = getUsername()
        self.password = password

    def readCredentials(self):
        self.username = getUsername()
        self.password = getPassword()

    def loadFile(self,name,contents):
        self.files.append({'file': (name,contents)})

    def readFiles(self):
        for filename in [CA_PUBLIC_CERT, MSSERVER_CERT, MSSERVER_KEY]:
            name = os.path.basename(filename)
            self.files.append ({'file': (name, open(filename, 'rb')) })

    def restart(self, cleanStore=False, maintenance=None):
        DELAY = 5
        
        serverName = self.serverName
        
        payload = None
        if cleanStore:
            payload = json.dumps({"Service":"Server", "CleanStore":cleanStore})
            msConfigURL = "https://%s:9089/ima/v1/service/restart" % (serverName)
            self.logger.info("* Restarting %s (cleanstore = %s)" % (serverName, cleanStore))
        elif maintenance != None:
            payload = json.dumps({"Service":"Server", "Maintenance" : maintenance})
            msConfigURL = "https://%s:9089/ima/v1/service/restart" % (serverName)
            self.logger.info("* Restarting %s (maintenance = %s)" % (serverName, maintenance))        
        else:
            payload = json.dumps({"Service":"Server"})
            msConfigURL = "https://%s:9089/ima/v1/service/stop" % (serverName)
            self.logger.info("* Restarting %s gracefully" % (serverName))
        
        oldStatus = self.getStatus()
        
        # Try to restart 10 times, with 5-second delay between attempts
        restartInitiated = False
        for _ in range(0, 10):
            try:
                response = requests.post(url=msConfigURL, data=payload, auth=HTTPBasicAuth(self.username, self.password), verify=False)
                if response.status_code == 200:
                    restartInitiated = True
                    break
                else:
                    self.logger.info("* Restart attempt failed with HTTP code %s, retrying in %s seconds" % (response.status_code, DELAY))
            except Exception as ex:
                self.logger.info("* Restart attempt failed, retrying in %s seconds (%s)" % (DELAY, ex))
            
            time.sleep(DELAY)
                
        if not restartInitiated:
            error_text = "Server %s could not be restarted" % (serverName)
            self.logger.critical(error_text)
            raise Exception(error_text)
    
        self.logger.info("* Restart requested for server %s" % (serverName))
    
        self.waitForRestart(initialUptime=oldStatus["Server"]["UpTimeSeconds"], delay=DELAY)
        
    def switchToMaintMode(self):
        status = self.getStatus()
        if status == None:
            error_text = "Server %s is down" % (self.serverName)
            self.logger.info(error_text)
            raise Exception(error_text)
        
        maintMode = (status["Server"]["State"] == 9)
        
        if not maintMode:
            self.restart(maintenance="start")
        
    def switchToProductionMode(self):
        status = self.getStatus()
        if status == None:
            error_text = 'Server %s is down. Check for "Unable to bind socket" message in the server trace file.' % (self.serverName)
            self.logger.info(error_text)
            raise Exception(error_text)
                
        maintMode = (status["Server"]["State"] == 9)
        
        if maintMode:
            self.logger.info("Moving server %s from maintenance to production mode" % (self.serverName))
            self.restart(maintenance="stop")
        else:
            self.logger.info("Server %s is not in maintenance mode (%s - %s)" % (self.serverName, status["Server"]["State"], status["Server"]["Status"]))
        
    def cleanStore(self):
        self.restart(cleanStore=True)

    def configureStandAloneHA(self, enableHA=False):
        restart = False
        
        inputData = {
            "HighAvailability": {
                "Group": self.groupName,
                "RemoteDiscoveryNIC": "None",
                "LocalDiscoveryNIC": self.serverName,
                "LocalReplicationNIC": self.serverName,
                "StartupMode": "StandAlone"
            }
        }

        # If the current state doesn't match desired state, update it 
        isHA, state = self.getHAState(wait=True)        
        if isHA != enableHA:
            inputData['HighAvailability']['EnableHA'] = enableHA
            restart = True
            self.logger.info("Changing high availability status to %s, will restart the server %s" % (enableHA, self.serverName))
        elif state == "UNSYNC" or state == "UNSYNC_ERROR" or state == "HADISABLED":
            restart = True
            self.logger.info("Force restart for %s to correct HA state %s" % (self.serverName, state))
        
        status_code, response = self.postConfigurationRequest(inputData)
        if status_code != 200:
            error_text = "Unexpected response code switching server %s to StartupMode=StandAlone: %s" % (self.serverName, status_code)
            self.logger.error(error_text) 
            self.logger.error(response)
            raise Exception(error_text)

        if restart:
            self.restart()
            _, state = self.getHAState(wait=True)
            if state == "UNSYNC" or state == "UNSYNC_ERROR":
                error_text = "Could not configure HA on server %s: %s" % (self.serverName, state)
                self.logger.critical(error_text) 
                raise Exception(error_text)           

    def getHASyncNodes(self):
        jstatus = self.getStatus(False)
        if 'HighAvailability' not in jstatus:
            return 0
        if 'Enabled' not in jstatus['HighAvailability']:
            return 0
        if jstatus["HighAvailability"]["Enabled"] != True:
            return 0
        return jstatus["HighAvailability"]["SyncNodes"]


    def getHAState(self, wait=False):
        isHA = False
    
        state = "None"
        # Wait for up to 5 minutes (15 iterations, 20 seconds each)
        for _ in range(0, 15):
            jstatus = self.getStatus(wait)
            if jstatus != None:
                if 'HighAvailability' not in jstatus:
                    isHA = False
                    break
                if 'Enabled' not in jstatus['HighAvailability']:
                    isHA = False
                    break
                if jstatus["HighAvailability"]["Enabled"] == True:
                    isHA = True
                    state = jstatus["HighAvailability"]["NewRole"]
                    if state == "UNKNOWN" or state == "UNSYNC":
                        server_errorcode = jstatus["Server"]["ErrorCode"]
                        if server_errorcode == 0:
                            self.logger.info("Server %s is in transition state %s" % (self.serverName, state))
                        else:
                            self.logger.info("Server %s is in error state %s (%s)" % (self.serverName, state, server_errorcode))
                            break
                    else:
                        break
                else:
                    isHA = False
                    break                
                
            if not wait:
                break
    
            time.sleep(20);
    
        self.logger.info("HA enabled for %s: %s - %s" % (self.serverName, str(isHA), state))
        
        return isHA, state
    
    def getStatus(self, wait=True):
        status = None
        retries = 10
        for _ in range(0, retries):
            statusURL = "https://%s:9089/ima/v1/service/status" % (self.serverName)
            try:
                response = requests.get(url=statusURL, auth=HTTPBasicAuth(self.username, self.password), timeout=10, verify=False)
                if response.status_code == 200:
                    status_text = response.content
                    self.logger.debug(response.content)
                    status = json.loads(status_text)
                    break
                elif response.status_code == 400 or response.status_code == 401:
                    print ("Accessing %s with %s:%s failed" % (statusURL, self.username, self.password))
                    raise Exception("Error accessing %s - %s" % (self.serverName, response.status_code))
            except requests.exceptions.RequestException:
                self.logger.info("Failed to connect to %s, retrying" % (statusURL))
                pass
            except requests.exceptions.Timeout:
                self.logger.info("Timed out waiting to connect to %s, retrying" % (statusURL))
                pass
    
            if not wait:
                break
    
            time.sleep(5)
                
        return status
    
    def checkHAStatus(self):
        ok = False
        state = "None"
        for _ in range(0, 600, 10):
            jstatus = self.getStatus()
            if jstatus != None:
                if 'NewRole' in jstatus["HighAvailability"]:
                    state = jstatus["HighAvailability"]["NewRole"]
                    if jstatus["HighAvailability"]["NewRole"] == "UNSYNC_ERROR":
                        ok = False
                        break
                    elif jstatus["HighAvailability"]["NewRole"] != "UNSYNC":
                        ok = True
                        break
                else:
                    ok = False
                    break
            time.sleep(10)
            
        self.logger.info("HA state for %s: %s" % (self.serverName, state))
            
        return ok
    
    def isStandbyNode(self, wait=True):
        state = self.getHAState(wait)[1]
        if state == "STANDBY":
            return True
        else:
            return False

    def configureUsernamePassword(self):
        self.logger.info("Configuring admin endpoint")
        
        self.logger.info("First wait for admin endpoint to come up")
        status = self.getStatus(wait=True)
        
        adminSecProfileInput = {
            "AdminUserID": self.username,
            "AdminUserPassword": self.password
        }
        
        status_code, response_text = self.postConfigurationRequest(adminSecProfileInput, hide=True)
        if status_code != 200:
            error_text = "Unexpected response code when configuring admin credentials for Amlen server %s: %s" % (self.serverName, status_code)
            self.logger.error(error_text) 
            self.logger.error(response_text)
            raise Exception(error_text)

        self.logger.info("Username and password have been set")

    def configureIoTFPolicy(self):
        imaserverName = self.serverName

        self.logger.info("Setting up default configuration for %s " % (imaserverName))

        if self.isStandbyNode(wait=False):
            self.logger.info("* Skipping standby node %s" % (imaserverName))
            return



        success = False

        self.logger.info("FILES %d"%(len(self.files)))
        try:
            # Upload certificates
            uploadFailed = False
            for file in self.files:
                status_code, response = self.uploadFile(file)
                if status_code in [200, 201, 204]:
                   self.logger.info("* Successfully uploaded %s" % (file))
                else:
                   self.logger.info("* Failed to upload %s: %s (%s)" % (file, status_code, response))
                   uploadFailed = True
                   break

            if not uploadFailed:
                self.logger.info("Applying config %s" % (self.config))
                status_code, response = self.postConfigurationRequest(self.config)
                if status_code == 200:
                    self.logger.info("* Configuration for %s updated successfully" % (imaserverName))
                    success = True
                else:
                    error_text = "* Configuration for %s was not updated: %s (%s)" % (imaserverName, status_code, response)
            else:
                error_text = "* Could not upload all necessary certificates"
        except Exception as ex:
            error_text = "Error (%s): %s" % (sys.exc_info()[0], ex)

        if not success:
            self.logger.error(error_text)
            raise Exception(error_text)

    def configureAdminEndpoint(self):
        self.configureUsernamePassword()

        if self.isStandbyNode(wait=False):
            self.logger.info("Don't configure admin endpoint on standby node")
            return
        
        adminSecProfileInput = {
            "SecurityProfile":  {
                "AdminDefaultSecProfile": {
                    "CertificateProfile" : "IoTCertificate",
                    "TLSEnabled" : True,
                    "UsePasswordAuthentication" : True
                }
            },
            "AdminEndpoint":  { "SecurityProfile" : "AdminDefaultSecProfile" } 
        }
        
        status_code, response_text = self.postConfigurationRequest(adminSecProfileInput)
        if status_code != 200:
            error_text = "Unexpected response code when configuring Admin Endpoint for Amlen server %s: %s" % (self.serverName, status_code)
            self.logger.error(error_text) 
            self.logger.error(response_text)
            raise Exception(error_text)
        else:
            self.logger.info("Admin endpoint has been configured")

    def uploadFile(self, f):
        
        auth = HTTPBasicAuth(self.username, self.password)

        url = "https://%s:9089/ima/v1/file/%s" % (self.serverName, f['file'][0])

        response = requests.put(url=url, files=f, auth=auth, verify=False)
        return response.status_code, response.content

    def postConfigurationRequest(self, request, hide=False):
        auth = HTTPBasicAuth(self.username, self.password)
        headers={'content-type': 'application/json'}

        url = "https://%s:9089/ima/v1/configuration/" % (self.serverName)

        data = request if not hide else "hidden payload" 
        self.logger.info("Attempting to connect to %s to post %s" % (url, data))
        response = requests.post(url=url, data=json.dumps(request), headers=headers, auth=auth, verify=False)
        return response.status_code, response.content
    
    def cleanupDemoEntries(self):
        auth = HTTPBasicAuth(self.username, self.password)
        isStandby = self.isStandbyNode(wait=False)
        if isStandby == True:
            return
    
        self.logger.info("Remove demo entries for Amlen server: %s" % self.serverName)
    
        try:
            url = "https://%s:9089/ima/v1/configuration/Endpoint/DemoEndpoint" % (self.serverName)
            requests.delete(url, auth=auth, verify=False)
            
            url = "https://%s:9089/ima/v1/configuration/Endpoint/DemoMqttEndpoint" % (self.serverName)
            requests.delete(url, auth=auth, verify=False)
            
            url = "https://%s:9089/ima/v1/configuration/MessageHub/DemoHub" % (self.serverName)
            requests.delete(url, auth=auth, verify=False)
        
            url = "https://%s:9089/ima/v1/configuration/ConnectionPolicy/DemoConnectionPolicy" % (self.serverName)
            requests.delete(url, auth=auth, verify=False)
            
            url = "https://%s:9089/ima/v1/configuration/TopicPolicy/DemoTopicPolicy" % (self.serverName)
            requests.delete(url, auth=auth, verify=False)
            
            url = "https://%s:9089/ima/v1/configuration/SubscriptionPolicy/DemoSubscriptionPolicy" % (self.serverName)
            requests.delete(url, auth=auth, verify=False)
            
            url = "https://%s:9089/ima/v1/configuration/Endpoint/IoTEndpoint0" % (self.serverName)
            requests.delete(url, auth=auth, verify=False)

        except:
            pass

    def configureFileLogging(self):
        commonFileName = os.getenv("LOG_FILE", "/var/lib/amlen-server/diag/logs/imaserver.log")
        
        syslogInput = {"Syslog":  {}}
        syslogInput["Syslog"]={
            "Host"    : "127.0.0.1",
            "Port"    : 514,
            "Protocol": "tcp",
            "Enabled" : False           
        }
        fileLocationInput = {"LogLocation":  {}}
        fileLocationInput["LogLocation"]["DefaultLog"]={
            "LocationType" : "file",
            "Destination" : commonFileName 
        }
        fileLocationInput["LogLocation"]["ConnectionLog"]={
            "LocationType" : "file",
            "Destination" : commonFileName
        } 
        fileLocationInput["LogLocation"]["SecurityLog"]={
            "LocationType" : "file",
            "Destination" : commonFileName
        }  
        fileLocationInput["LogLocation"]["AdminLog"]={
            "LocationType" : "file",
            "Destination" : commonFileName
        }  
        fileLocationInput["LogLocation"]["MQConnectivityLog"]={
            "LocationType" : "file",
            "Destination" : commonFileName
        }

        fileEnabled = False

        try:
            rc, content = self.postConfigurationRequest(fileLocationInput)
            if rc != 200:
                self.logger.warn("Unexpected response code when configuring file locations for Amlen server %s: %s (%s)" % (self.serverName, rc, content)) 
            else:
                rc, content = self.postConfigurationRequest(syslogInput)
                if rc != 200:
                    self.logger.warn("Unexpected response code when configuring file for Amlen server %s: %s (%s)" % (self.serverName, rc, content)) 
                else:
                    fileEnabled = True
        except:
            error_text = "Unexpected exception when configuring file for Amlen server %s: %s" % (self.serverName, sys.exc_info()[0])
            self.logger.warn(error_text)

        if fileEnabled:
            self.logger.info("File-based logging configured successfully for Amlen server %s" % (self.serverName))
        else:
            error_text = "File-based logging could not be configured for Amlen server %s" % (self.serverName)
            self.logger.error(error_text)
            raise Exception(error_text)

        self.cleanupDemoEntries()
        self.configureIoTFPolicy()
        self.configureAdminEndpoint()
        
        return True
    
    def waitForRestart(self, initialUptime=0, delay=5, targetStatus=None):
        initialServerStartTime = 0
    
        if initialUptime > 0:
            initialServerStartTime = int(time.time()) - initialUptime
            self.logger.info("* Last server startup time was %s" % initialServerStartTime)

        time.sleep(delay)

        serverName = self.serverName

        RESTART_ITERATIONS = 60
        WAIT_FOR_MODE_ITERATIONS = 60
        
        restarted = False

        # Wait for up to 5 minutes (60 iterations, 5 seconds each)
        for i in range(1, RESTART_ITERATIONS+1):
            try:
                currentStatus = self.getStatus(wait=False)
                if currentStatus and currentStatus["Server"]["Status"] != "Stopping":
                    self.logger.info("Current status: %s" % json.dumps(currentStatus["Server"]))
                    newServerStartTime = int(time.time()) - currentStatus["Server"]["UpTimeSeconds"]
                    # Compare times allowing for rounding
                    if newServerStartTime > (initialServerStartTime + 1):
                        self.logger.info("* Server %s is back up (start time: %s)" % (serverName, newServerStartTime))
                        restarted = True
                        break
            except:
                pass
    
            self.logger.info("* Still waiting for server %s to come up (%s/%s)" % (serverName, i, RESTART_ITERATIONS))
            time.sleep(delay)
            
        if restarted and targetStatus and currentStatus:
            # Wait for up to 5 minutes (60 iterations, 5 seconds each)
            for i in range(1, WAIT_FOR_MODE_ITERATIONS+1):
                try:
                    currentStatus = self.getStatus(wait=False)
                    if currentStatus:
                        if targetStatus == currentStatus["Server"]["Status"]:
                            self.logger.info("* Server %s has reached status %s" % (serverName, targetStatus))
                            break
                    else:
                        self.logger.info("* Current status not available (likely just rebooted)")
                        time.sleep(delay)
                        continue
                except:
                    pass
    
                self.logger.info("* Still waiting for server %s to reach target status - %s/%s (%s/%s)" % 
                            (serverName, targetStatus, currentStatus["Server"]["Status"], i, WAIT_FOR_MODE_ITERATIONS))
                time.sleep(delay)
                
        if not restarted:
            error_text = "Server %s failed to restart in time" % self.serverName
            self.logger.error(error_text)
            raise Exception(error_text)

