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
import inspect
from optparse import OptionParser
import re

# -------------------------------------------------------------------
# Accept the license
# -------------------------------------------------------------------
def acceptLicense( serverType, serverName, adminAddress=None, adminPort=None, licensedUsage=None ):
    if serverType != "MS" and serverType != "IOTMS":
        print("No implementation of %s for serverType %s" % (inspect.currentframe().f_code.co_name, serverType))
        return
    
    print("Accepting license usage %s on server %s" % (licensedUsage, serverName), file=stdoutFileHandle)

    HEADERS_data = {'Content-type': 'application/json', 'Accept': 'text/plain'}
    OBJECT_data = {'Accept':True, 'LicensedUsage':licensedUsage}

    r = requests.post("http://%s:%d/ima/v1/configuration" % (adminAddress, adminPort),
                      data=json.dumps(OBJECT_data), headers=HEADERS_data)

    RESPONSE_data = json.loads(r.text)

    if r.status_code != 200:
        print("%s (%s)" % (RESPONSE_data['Message'], RESPONSE_data['Code']), file=stderrFileHandle)
        sys.exit(20)
    else:
        print("%s (%s)" % (RESPONSE_data['Message'], RESPONSE_data['Code']), file=stdoutFileHandle)

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
# Wait for the named server on specified admin port to be Running
# -------------------------------------------------------------------
def waitForRunningState( serverType, serverName, adminAddress=None, adminPort=None ):
    if serverType != "MS" and serverType != "IOTMS":
        print("No implementation of %s for serverType %s" % (inspect.currentframe().f_code.co_name, serverType))
        return
    
    print("Waiting for Running status for %s" % (serverName), file=stdoutFileHandle)
    serverStatusURL = "http://%s:%d/ima/v1/service/status" % (adminAddress, adminPort)
    retryCount = 0
    licenseAccepted = False
    while True:
        try:
            r = requests.get(serverStatusURL, timeout=5)
            if r.status_code == 200:
                RESPONSE_data = json.loads(r.text)
                state = RESPONSE_data["Server"]["State"] 
                if state == 1:
                    break;
                elif licenseAccepted == False and RESPONSE_data["Server"]["ErrorCode"] == 387:
                    acceptLicense( serverType, serverName, adminAddress, adminPort, "Production" )
                    licenseAccepted = True
            print("Waiting for server %s to get to Running state [statusCode:%d]" % (serverName, r.status_code))
        except Exception as e:
            print("Waiting for server %s to get to Running state [exception:%s]" % (serverName, str(e)))

        # We've given it 60 seconds...
        if retryCount == 60:
            sys.exit(23)
        else:
            retryCount += 1
            time.sleep(1)

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
        print("Failed to create %s for docker container %s [%s]" % (directory, serverName, str(e)), file=stderrFileHandle)
        sys.exit(24)

# ================================================================== #
# PROGRAM BEGINS                                                     #
# ================================================================== #
parser = OptionParser()
parser.add_option("-i", "--internal-volumes", action="store_false", dest="exposeVolumes", default=True, help="keep messagesight volumes internal to the container")
parser.add_option("-l", "--log-file", dest="logFile", help="write report to FILE", metavar="FILE")
parser.add_option("-c", "--cluster-name", dest="clusterName", default=None, help="Specify a cluster name to include this server in")
parser.add_option("-m", "--memory-limit", dest="serverMemGb", type=int, default=0, help="Specify the amount of memory in gigabytes to limit this server to")
parser.add_option("-t", "--server-type", dest="serverType", type=str, default="MS", help="Specify what type of server you are adding (MS, IOTMS or MQ)")
parser.add_option("-b", "--bridge-network", dest="bridge", default=None, help="Specify the bridge network name to use")
(options, args) = parser.parse_args()

if len(args) != 1:
    parser.error("Expected one argument, the server specification")

if options.logFile != None:
    if options.logFile == "/dev/null":
        stdoutFileHandle = open(os.devnull, 'w')
        stderrFileHandle = open(os.devnull, 'w')
    else:
        stdoutFileHandle = open(options.logFile, 'w')
        stderrFileHandle = open(options.logFile, 'w')
else:
    stdoutFileHandle = sys.stdout
    stderrFileHandle = sys.stderr

serverSpec = args[0]

(serverCharacter,qualifiedLevel) = serverSpec.split(':')

if len(serverCharacter) != 1 or ord(serverCharacter) < ord('A') or ord(serverCharacter) > ord('Z'):
    print("Server are expected to be a single character between A and Z", file=stderrFileHandle)
    sys.exit(1)
else:
    serverNumber = ord(serverCharacter[0])-ord('A')

if qualifiedLevel == None:
    qualifiedLevel = "V.next.Beta"

userName = os.environ.get('USER')
if userName == None:
    userName = 'default'

dockerCmd = os.environ.get('DOCKER_CMD')
if dockerCmd == None:
    dockerCmd = 'docker'

serverType = options.serverType
serverName = userName+"_server_"+serverType+serverCharacter

if os.environ.get('IMA_SRVCFG_ROOT') == None:
    cfgDir = os.environ['HOME']+'/serverCfg/'+serverName
else:
    cfgDir = os.environ['IMA_SRVCFG_ROOT']+'/'+serverName

volumes = ''

if options.serverMemGb == 0:
    memOption = ''
else:
    memOption = "-m %dg" % (options.serverMemGb)

# Find out if a server of that name already exists
print("Checking for existence of %s" % (serverName), file=stdoutFileHandle)
rc = subprocess.call("%s ps -a | grep %s" % (dockerCmd, serverName), shell=True, stdout=stdoutFileHandle, stderr=stderrFileHandle)

if serverType == "MS" or serverType == "IOTMS":
    if rc == 0:
        print("A server named %s already exists skipping creation" % (serverName), file=stdoutFileHandle)
    else:
        if serverType == "MS":
            repoName = "imaserver"
        else:
            repoName = "wiotp-docker-local.artifactory.swg-devops.com/wiotp-connect/messagesight"
        serverAdminAddress = "127.0.0.1"
        serverAdminPort = 9089
        externalServerAdminPort = serverAdminPort + serverNumber
        serverDemoMqttEndpointPort = 1883
        externalServerDemoMqttEndpointPort = serverDemoMqttEndpointPort + serverNumber
        serverDemoEndpointPort = 16102
        externalServerDemoEndpointPort = serverDemoEndpointPort + serverNumber
    
        options.clusterExternalAddress = "127.0.0.1" # Should be an external?
        options.clusterControlPort = 9200 + serverNumber
        options.clusterControlAddress = "127.0.0.1"
        options.clusterControlExternalAddress = options.clusterExternalAddress
        options.clusterMessagingPort = 9300 + serverNumber
        options.clusterMessagingAddress = "127.0.0.1"
        options.clusterMessagingExternalAddress = options.clusterExternalAddress
    
        # Create the directories for this container
        createDirectory(cfgDir, serverName)
    
        customDir = cfgDir+'/custom'
        createDirectory(customDir, serverName)
        volumes += "-v %s:/custom " % (customDir)
    
        # Expose messagesight files outside the container
        if options.exposeVolumes:
            volumeDir = cfgDir+'/var/messagesight'
            createDirectory(volumeDir, serverName)
            volumes += "-v %s:/var/messagesight " % (volumeDir)
    
            logDir = volumeDir+'/diag/logs'
        else:
            logDir = None
    
        if os.environ.get('SROOT') != None:
            srootDir = os.environ['SROOT']
            volumes += "-v %s:%s " % (srootDir, srootDir)
    
            # Create a custom entrypoint
            customEntrypoint = "%s/customEntrypoint.sh" % (customDir)
            shutil.copyfile("%s/server_engine/scripts/customEntrypoint.sh" % (srootDir), customEntrypoint)
            try:
                os.chmod(customEntrypoint, stat.S_IRWXU|stat.S_IRWXG|stat.S_IRWXO)
            except OSError as e:
                print("Failed to create modify custom entrypoint %s [%s]" % (customEntrypoint, str(e)), file=stderrFileHandle)
                sys.exit(10)
            customEntrypoint = "/custom/customEntrypoint.sh"
        else:
            customEntrypoint = None
    
        if os.environ.get('BROOT') != None:
            volumes += "-v %s:%s " % (os.environ['BROOT'], os.environ['BROOT'])
    
        if os.environ.get('IMA_ISM_ROOT') != None:
            volumeDir = os.environ['IMA_ISM_ROOT']+'/'+qualifiedLevel+'/source'
        volumes += "-v %s:/home/itbld/jazz-build-engines/jazza06/jazza06.ism.ism-ilnx-1/build " % (volumeDir)
        volumes += "-v %s:/home/itbld/jazz-build-engines/jazza06/jazza06.ism.ism-ilnx-r6/build " % (volumeDir)
        volumes += "-v %s:/home/itbld/jazz-build-engines/jazza06/jazza06.ism.ism-ilnx-c7/build " % (volumeDir)
        volumes += "-v %s:/home/ismbld/IMA15a/workspace " % (volumeDir)
        volumes += "-v %s:/home/ismbld/DefaultPersonalBuild/workspace" % (volumeDir)
    
        overrides = '--security-opt seccomp:unconfined '
        # overrides += '--ulimit core=-1' (cap-add SYS_PTRACE etc?)
    
        # docker network create --driver=bridge --subnet=192.168.10.0/16 ha0
        if options.bridge != None:
            ipBase = serverNumber+1
            networks = "--network %s --ip 192.168.10.%d " % (options.bridge, ipBase)
        else:
            networks = ""
        networks +=  "--publish=%d:%d " % (externalServerAdminPort, serverAdminPort)
        networks += "--publish=%d:%d " % (externalServerDemoMqttEndpointPort, serverDemoMqttEndpointPort)
        networks += "--publish=%d:%d " % (externalServerDemoEndpointPort, serverDemoEndpointPort)
    
        if options.clusterName != None:
            networks += "--publish=%d:%d " % (options.clusterControlPort, options.clusterControlPort)
            networks += "--publish=%d:%d " % (options.clusterMessagingPort, options.clusterMessagingPort)
    
        # Write an environment file for this server
        envFilename = cfgDir+"/"+serverName+".env"
        print("Writing new environment file %s" % (envFilename), file=stdoutFileHandle)
        envFileHandle = open(envFilename, 'w')
    
        print("LICENSE=accept", file=envFileHandle)
        print("MESSAGESIGHT_MEMORY_SIZE=%d" % (options.serverMemGb), file=envFileHandle)
        print("MESSAGESIGHT_CONTAINER_NAME=imaserver", file=envFileHandle)
        print("MESSAGESIGHT_ADMIN_HOST=All", file=envFileHandle)
        print("MESSAGESIGHT_ADMIN_PORT=%d" % (serverAdminPort), file=envFileHandle)
        print("MESSAGESIGHT_SERVICE_NAME=imaserver", file=envFileHandle)
        print("MESSAGESIGHT_SERVICE_VERSION=5.0", file=envFileHandle)
    
        envFileHandle.close()
    
        # Create the docker container for the server
        print("Creating %s docker container %s" % (serverType, serverName), file=stdoutFileHandle)
    
        if customEntrypoint == None:
            dockerCommand = "%s run --name %s --env-file=%s %s %s %s %s %s:%s" % (dockerCmd, serverName, envFilename, networks, volumes, overrides, memOption, repoName, qualifiedLevel)
        else:
            dockerCommand = "%s run --name %s --env-file=%s %s %s %s --entrypoint=%s %s %s:%s" % (dockerCmd, serverName, envFilename, networks, volumes, overrides, customEntrypoint, memOption, repoName, qualifiedLevel)
        print(dockerCommand, file=stdoutFileHandle)
        p = subprocess.Popen(dockerCommand, shell=True, stdout=stdoutFileHandle, stderr=stderrFileHandle)
    
    # Wait for the server to exist
    print("Waiting for container %s" % (serverName), file=stdoutFileHandle)
    while subprocess.call("%s ps | grep %s" % (dockerCmd, serverName), shell=True, stdout=stdoutFileHandle, stderr=stderrFileHandle) != 0:
        print("Waiting for container %s to appear" % (serverName), file=stdoutFileHandle)
        time.sleep(1)

    # Copy the host system's libicu files into the container so we can easily update the binaries
    icuFileRegEx = re.compile('libicu.*\.so\..*')
    icuFiles = [f for f in os.listdir('/lib64') if icuFileRegEx.match(f) and os.path.isfile(os.path.join('/lib64',f))]
    for icuFile in icuFiles:
        dockerCommand = "%s cp %s %s:/lib64" % (dockerCmd, os.path.join('/lib64', icuFile), serverName)
        p = subprocess.Popen(dockerCommand, shell=True, stdout=stdoutFileHandle, stderr=stderrFileHandle)

    # Wait for server to get to Running state
    waitForRunningState(serverType, serverName, serverAdminAddress, externalServerAdminPort)

    # Grab the serverUID for information if it's available
    if logDir != None:
        (otherStuff, serverUID) = subprocess.check_output('grep "new ServerUID" %s/imatrace.log' % (logDir), shell=True).decode("utf-8").rstrip().split('=')
    else:
        serverUID = None
    
    infoFilename = cfgDir+'/'+serverName+'.info'
    
    print("Writing new information file %s" % (infoFilename), file=stdoutFileHandle)
    
    infoFileHandle = open(infoFilename, 'w')
    
    print("# ======================================================= GENERAL", file=infoFileHandle)
    if serverUID != None:
        print("# serverUID=%s" % (serverUID), file=infoFileHandle)
    print("# serverAdminAddress=%s" % (serverAdminAddress), file=infoFileHandle)
    print("# externalServerAdminPort=%d" % (externalServerAdminPort), file=infoFileHandle)
    print("# ===================================================== ENDPOINTS", file=infoFileHandle)
    print("# externalServerDemoMqttEndpointPort=%d" % (externalServerDemoMqttEndpointPort), file=infoFileHandle)
    print("# externalServerDemoEndpointPort=%d" % (externalServerDemoEndpointPort), file=infoFileHandle)
    
    # Update our docker container name to include the serverUID
    # if serverUID != None:
    #     subprocess.call("%s rename %s %s-%s" % (dockerCmd, serverName, serverName, serverUID), shell=True,
    #                     stdout=stdoutFileHandle, stderr=stderrFileHandle)
    
    # Update ServerName
    updateObject(serverName, serverAdminAddress, externalServerAdminPort, None, None,
                 {'ServerName':serverName})
    
    # Update DemoMqttEndpoint
    updateObject(serverName, serverAdminAddress, externalServerAdminPort, 'Endpoint', 'DemoMqttEndpoint',
                 {'Port':serverDemoMqttEndpointPort,
                  'Enabled':True})
    
    # Update DemoEndpoint
    updateObject(serverName, serverAdminAddress, externalServerAdminPort, 'Endpoint', 'DemoEndpoint',
                 {'Port':serverDemoEndpointPort,
                  'Enabled':True})
    
    # TODO SET A SERVERNAME
    
    # Set up cluster membership
    if options.clusterName != None:
        print("# ======================================================= CLUSTER", file=infoFileHandle)
        print("# clusterName=%s" % (options.clusterName), file=infoFileHandle)
        print("# clusterExternalAddress=%s" % (options.clusterExternalAddress), file=infoFileHandle)
        print("# clusterControlPort=%s" % (options.clusterControlPort), file=infoFileHandle)
        print("# clusterControlAddress=%s" % (options.clusterControlAddress), file=infoFileHandle)
        print("# clusterControlExternalAddress=%s" % (options.clusterControlExternalAddress), file=infoFileHandle)
        print("# clusterMessagingPort=%s" % (options.clusterMessagingPort), file=infoFileHandle)
        print("# clusterMessagingAddress=%s" % (options.clusterMessagingAddress), file=infoFileHandle)
        print("# clusterMessagingExternalAddress=%s" % (options.clusterMessagingExternalAddress), file=infoFileHandle)
    
        # Update the ClusterMembership attributes
        updateObject(serverName, serverAdminAddress, externalServerAdminPort, 'ClusterMembership', None,
                     {'ClusterName':options.clusterName,
                      'UseMulticastDiscovery':True,
                      'ControlAddress':options.clusterControlAddress,
                      'ControlPort':options.clusterControlPort,
                      'ControlExternalAddress':options.clusterControlExternalAddress,
                      'MessagingAddress':options.clusterMessagingAddress,
                      'MessagingPort':options.clusterMessagingPort,
                      'MessagingExternalAddress':options.clusterMessagingExternalAddress,
                      'EnableClusterMembership':True})
    
        # Request a restart to complete cluster membership
        requestRestart(serverName, serverAdminAddress, externalServerAdminPort)
    
        # Wait for server to be running again
        waitForRunningState(serverType, serverName, serverAdminAddress, externalServerAdminPort)

        infoFileHandle.close()
elif serverType == "MQ":
    if rc == 0:
        print("A server named %s already exists skipping creation" % (serverName), file=stdoutFileHandle)
    else:
        listenerPort = 1414 + serverNumber
        externalListenerPort = listenerPort
        qmName = "QM"+serverCharacter
        
        # Create the directories for this container
        createDirectory(cfgDir, serverName)
    
        # Expose MQ files outside the container
        if options.exposeVolumes:
            volumeDir = cfgDir+'/var/mqm'
            createDirectory(volumeDir, serverName)
            volumes += "-v %s:/var/mqm " % (volumeDir)

        customDir = cfgDir+'/custom'
        createDirectory(customDir, serverName)
        volumes += "-v %s:/custom " % (customDir)
    
        # Create a custom entrypoint
        customEntrypoint = "%s/customEntrypoint.sh" % (customDir)
        customEntrypointFileHandle = open(customEntrypoint, 'w')

        msGroup    = "msgroup"
        msUser     = "msuser"
        msPassword = "mspassword"

        print("#!/bin/bash", file=customEntrypointFileHandle)
        print("if [ ! -f /custom/initialSetupFinished.txt ]", file=customEntrypointFileHandle)
        print("then", file=customEntrypointFileHandle)
        print("    groupadd %s" % (msGroup), file=customEntrypointFileHandle)
        print("    useradd -m %s -g %s" % (msUser, msGroup), file=customEntrypointFileHandle)
        print("    echo %s:%s | chpasswd" % (msUser, msPassword), file=customEntrypointFileHandle)
        print("    touch /custom/initalSetupFinished.txt", file=customEntrypointFileHandle)
        print("fi", file=customEntrypointFileHandle)
        print("/usr/local/bin/mq.sh", file=customEntrypointFileHandle)
        print("#sleep 1000000", file=customEntrypointFileHandle)
        
        customEntrypointFileHandle.close()
        
        try:
            os.chmod(customEntrypoint, stat.S_IRWXU|stat.S_IRWXG|stat.S_IRWXO)
        except OSError as e:
            print("Failed to create modify custom entrypoint %s [%s]" % (customEntrypoint, str(e)), file=stderrFileHandle)
            sys.exit(10)

        customEntrypoint = "/custom/customEntrypoint.sh"

        # Add custom configuration items
        volumeDir = cfgDir+'/etc/mqm'
        createDirectory(volumeDir, serverName)
        volumes += "-v %s:/etc/mqm " % (volumeDir)
    
        configFilename = volumeDir+'/config.mqsc'
        configFileHandle = open(configFilename, 'w')

        print("SET AUTHREC OBJTYPE(QMGR) GROUP('%s') AUTHADD(CONNECT, INQ, DSP)" % (msGroup), file=configFileHandle)
        print("SET AUTHREC PROFILE('SYSTEM.DEFAULT.MODEL.QUEUE') OBJTYPE(QUEUE) GROUP('%s') AUTHADD(DSP, GET)" % (msGroup), file=configFileHandle)
        print("SET AUTHREC PROFILE('SYSTEM.ADMIN.COMMAND.QUEUE') OBJTYPE(QUEUE) GROUP('%s') AUTHADD(DSP, PUT)" % (msGroup), file=configFileHandle)
        print("SET AUTHREC PROFILE('SYSTEM.IMA.*') OBJTYPE(QUEUE) GROUP('%s') AUTHADD(CRT, PUT, GET, BROWSE)" % (msGroup), file=configFileHandle)
        print("SET AUTHREC PROFILE('SYSTEM.DEFAULT.LOCAL.QUEUE') OBJTYPE(QUEUE) GROUP('%s') AUTHADD(DSP)" % (msGroup), file=configFileHandle)
        print("DEFINE LISTENER('LISTEN_ON_%s') TRPTYPE(TCP) CONTROL(QMGR) PORT(%d)" % (qmName, listenerPort), file=configFileHandle)
        print("START LISTENER('LISTEN_ON_%s')" % (qmName), file=configFileHandle)

        print("ALTER QMGR CHLAUTH(ENABLED)", file=configFileHandle)
        print("DEFINE CHANNEL('MS_TO_%s') CHLTYPE(SVRCONN) TRPTYPE(TCP)" % (qmName), file=configFileHandle)
        print("SET CHLAUTH('MS_TO_%s') TYPE(ADDRESSMAP) ADDRESS('*') USERSRC(MAP) MCAUSER('%s') CHCKCLNT(REQUIRED)" % (qmName, msUser), file=configFileHandle)

        print("DEFINE QLOCAL('TEST1')", file=configFileHandle)
        print("SET AUTHREC PROFILE('TEST1') OBJTYPE(QUEUE) GROUP('%s') AUTHADD(PUT)" % (msGroup), file=configFileHandle)
        print("DEFINE TOPIC('TEST2') TOPICSTR('/TEST2')", file=configFileHandle)
        print("SET AUTHREC PROFILE('TEST2') OBJTYPE(TOPIC) GROUP('%s') AUTHADD(PUB, DSP)" % (msGroup), file=configFileHandle)
        print("DEFINE QLOCAL('TEST3')", file=configFileHandle)
        print("SET AUTHREC PROFILE('TEST3') OBJTYPE(QUEUE) GROUP('%s') AUTHADD(GET)" % (msGroup), file=configFileHandle)
        print("DEFINE TOPIC('TEST4') TOPICSTR('/TEST4')", file=configFileHandle)
        print("SET AUTHREC PROFILE('TEST4') OBJTYPE(TOPIC) GROUP('%s') AUTHADD(SUB, CTRL, DSP)" % (msGroup), file=configFileHandle)
        print("DEFINE QLOCAL('TEST5')", file=configFileHandle)
        print("SET AUTHREC PROFILE('TEST5') OBJTYPE(QUEUE) GROUP('%s') AUTHADD(PUT)" % (msGroup), file=configFileHandle)
        print("DEFINE TOPIC('TEST6') TOPICSTR('/TEST6')", file=configFileHandle)
        print("SET AUTHREC PROFILE('TEST6') OBJTYPE(TOPIC) GROUP('%s') AUTHADD(PUB, DSP)" % (msGroup), file=configFileHandle)
        print("DEFINE TOPIC('TEST7') TOPICSTR('/TEST7')", file=configFileHandle)
        print("SET AUTHREC PROFILE('TEST7') OBJTYPE(TOPIC) GROUP('%s') AUTHADD(PUB, DSP)" % (msGroup), file=configFileHandle)
        print("DEFINE TOPIC('TEST8') TOPICSTR('/TEST8')", file=configFileHandle)
        print("SET AUTHREC PROFILE('TEST8') OBJTYPE(TOPIC) GROUP('%s') AUTHADD(SUB, CTRL, DSP)" % (msGroup), file=configFileHandle)
        print("DEFINE TOPIC('TEST9') TOPICSTR('/TEST9')", file=configFileHandle)
        print("SET AUTHREC PROFILE('TEST9') OBJTYPE(TOPIC) GROUP('%s') AUTHADD(SUB, CTRL, DSP)" % (msGroup), file=configFileHandle)
        print("DEFINE QLOCAL('TEST10')", file=configFileHandle)
        print("SET AUTHREC PROFILE('TEST10') OBJTYPE(QUEUE) GROUP('%s') AUTHADD(PUT)" % (msGroup), file=configFileHandle)
        print("DEFINE TOPIC('TEST11') TOPICSTR('/TEST11')", file=configFileHandle)
        print("SET AUTHREC PROFILE('TEST11') OBJTYPE(TOPIC) GROUP('%s') AUTHADD(PUB, DSP)" % (msGroup), file=configFileHandle)
        print("DEFINE QLOCAL('TEST12')", file=configFileHandle)
        print("SET AUTHREC PROFILE('TEST12') OBJTYPE(QUEUE) GROUP('%s') AUTHADD(GET)" % (msGroup), file=configFileHandle)
        print("DEFINE TOPIC('TEST13') TOPICSTR('/TEST13')", file=configFileHandle)
        print("SET AUTHREC PROFILE('TEST13') OBJTYPE(TOPIC) GROUP('%s') AUTHADD(SUB, CTRL, DSP)" % (msGroup), file=configFileHandle)
        print("DEFINE TOPIC('TEST14') TOPICSTR('/TEST14')", file=configFileHandle)
        print("SET AUTHREC PROFILE('TEST14') OBJTYPE(TOPIC) GROUP('%s') AUTHADD(SUB, CTRL, DSP)" % (msGroup), file=configFileHandle)

        # overrides = '--ulimit core=-1'
        overrides = ''
    
        networks = "--publish=%d:%d" % (externalListenerPort, listenerPort)
    
        # Write an environment file for this server
        envFilename = cfgDir+"/"+serverName+".env"
        print("Writing new environment file %s" % (envFilename), file=stdoutFileHandle)
        envFileHandle = open(envFilename, 'w')
    
        print("LICENSE=accept", file=envFileHandle)
        print("MQ_QMGR_NAME=%s" % (qmName), file=envFileHandle)
    
        envFileHandle.close()
    
        # Create the docker container for the server
        print("Creating %s docker container %s" % (serverType, serverName), file=stdoutFileHandle)
    
        dockerCommand = "%s run --name %s --entrypoint %s --env-file=%s %s %s %s %s ibmcom/mq:%s" % (dockerCmd, serverName, customEntrypoint, envFilename, networks, volumes, overrides, memOption, qualifiedLevel)

        print(dockerCommand, file=stdoutFileHandle)
        p = subprocess.Popen(dockerCommand, shell=True, stdout=stdoutFileHandle, stderr=stderrFileHandle)
    
    # Wait for the server to exist
    print("Waiting for container %s" % (serverName), file=stdoutFileHandle)
    while subprocess.call("%s ps | grep %s" % (dockerCmd, serverName), shell=True, stdout=stdoutFileHandle, stderr=stderrFileHandle) != 0:
        print("Waiting for container %s to appear" % (serverName), file=stdoutFileHandle)
        time.sleep(1)
    
    waitForRunningState(serverType, serverName)
    
    infoFilename = cfgDir+'/'+serverName+'.info'
    
    print("Writing new information file %s" % (infoFilename), file=stdoutFileHandle)
    
    infoFileHandle = open(infoFilename, 'w')
    
    print("# ======================================================= GENERAL", file=infoFileHandle)
    print("# serverExternalPort=%d" % (externalListenerPort), file=infoFileHandle)
    print("# ======================================================= USERGRP", file=infoFileHandle)
    print("# msGroup=%s" % (msGroup), file=infoFileHandle)
    print("# msUser=%s" % (msUser), file=infoFileHandle)
    
    infoFileHandle.close()
