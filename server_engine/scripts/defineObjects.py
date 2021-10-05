#!/usr/bin/python3
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
import os
import sys
import subprocess
import time
import requests
import json
import shutil
import stat

# -------------------------------------------------------------------
# Wait for the named server on specified admin port to be Running
# -------------------------------------------------------------------
def waitForRunningState( serverName, adminAddress, adminPort ):
    print("Waiting for Running status for %s" % (serverName), file=stdoutFileHandle)
    serverStatusURL = "http://%s:%d/ima/v1/service/Status" % (adminAddress, adminPort)
    print("URL: %s" % (serverStatusURL))
    retryCount=0
    while True:
        try:
            response = requests.get(serverStatusURL, timeout=5)
            if response.status_code == 200:
                j = json.loads(response.text)
                if j["Server"]["State"] == 1:
                    break;
            print("Waiting for server %s to get to Running state" % (serverName, response.status_code))
        except:
            print("Waiting for server %s to get to Running state" % (serverName))

        # We've given it 60 seconds...
        if retryCount == 60:
            sys.exit(20)
        else:
            retryCount += 1
            time.sleep(1)

# -------------------------------------------------------------------
# Request a restart for the named server on specified admin port
# -------------------------------------------------------------------
def requestRestart( serverName, adminAddress, adminPort ):
    print("Requesting restart of %s" % (serverName), file=stdoutFileHandle)
    r = requests.post("http://%s:%d/ima/v1/service/restart" % (adminAddress, adminPort), data="{\"Service\":\"Server\"}")
    if r.status_code != 200:
       print("Failed to issue restart status_code:%d reason:%s" % (r.status_code, r.reason), file=stderrFileHandle)
       sys.exit(21)

# -------------------------------------------------------------------
# Request an update of an Endpoint
# -------------------------------------------------------------------
def updateObject( serverName, adminAddress, adminPort, objectType, objectName, objectDefn ):
    print("Requesting update of %s %s on server %s" % (objectType, objectName, serverName), file=stdoutFileHandle)

    HEADERS_data = {'Content-type': 'application/json', 'Accept': 'text/plain'}

    OBJECT_data = {}
    if objectName == None:
        OBJECT_data = objectDefn
    else:
        OBJECT_data["%s" % objectName] = objectDefn

    if objectType == None:
        REST_data = OBJECT_data
    else:
        REST_data = {}
        REST_data[objectType] = OBJECT_data

    r = requests.post("http://%s:%d/ima/v1/configuration" % (adminAddress, adminPort),
                      data=json.dumps(REST_data), headers=HEADERS_data)

    RESPONSE_data = json.loads(r.text)

    if r.status_code != 200:
        print("%s (%s)" % (RESPONSE_data['Message'], RESPONSE_data['Code']), file=stderrFileHandle)
        sys.exit(22)
    else:
        print("%s (%s)" % (RESPONSE_data['Message'], RESPONSE_data['Code']), file=stdoutFileHandle)

# -------------------------------------------------------------------
# Create a directory, removing the directory first if it already exists
# -------------------------------------------------------------------
def createDirectory( directory, serverName ):
    print("Creating directory %s for docker container %s" % (directory, serverName), file=stdoutFileHandle)
    if os.path.exists(directory):
        shutil.rmtree(directory)

    try:
        os.makedirs(directory, mode=0o777, exist_ok=True)
    except OSError as e:
        print("Failed to create %s for docker contained %s [%s]" % (directory, serverName, str(e)), file=stderrFileHandle)
        sys.exit(23)

# ================================================================== #
# PROGRAM BEGINS                                                     #
# ================================================================== #
serverName = sys.argv[1]

if len(sys.argv) > 2:
    logFile = sys.argv[2]
    if logFile == "/dev/null":
        stdoutFileHandle = open(os.devnull, 'w')
        stderrFileHandle = open(os.devnull, 'w')
    else:
        stdoutFileHandle = open(logFile, 'w')
        stderrFileHandle = open(logFile, 'w')
else:
    stdoutFileHandle = sys.stdout
    stderrFileHandle = sys.stderr

if os.environ.get('IMA_SRVCFG_ROOT') == None:
    cfgDir = os.environ['HOME']+'/serverCfg/'+serverName
else:
    cfgDir = os.environ['IMA_SRVCFG_ROOT']+'/'+serverName

infoFilename = cfgDir+'/'+serverName+'.info'

if os.path.isfile(infoFilename):
    (ignored, serverAdminAddress) = subprocess.check_output("grep serverAdminAddress %s" % (infoFilename),
                                                            shell=True).decode("utf-8").rstrip().split('=')
    (ignored, serverAdminPortS) = subprocess.check_output("grep serverAdminPort %s" % (infoFilename),
                                                            shell=True).decode("utf-8").rstrip().split('=')
else:
    (serverAdminAddress, serverAdminPortS) = serverName.split(':')

serverAdminPort = int(serverAdminPortS)

# Wait for server to get to Running state
waitForRunningState(serverName, serverAdminAddress, serverAdminPort)

# Update / Create TEST10 & TEST11
updateObject(serverName, serverAdminAddress, serverAdminPort, 'Queue', 'TEST10', {})
updateObject(serverName, serverAdminAddress, serverAdminPort, 'Queue', 'TEST11', {})
updateObject(serverName, serverAdminAddress, serverAdminPort, 'Queue', 'TEST12', {})
updateObject(serverName, serverAdminAddress, serverAdminPort, 'Queue', 'TEST13', {})
updateObject(serverName, serverAdminAddress, serverAdminPort, 'Queue', 'TEST14', {})
