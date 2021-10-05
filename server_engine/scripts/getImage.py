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
#
# This script retrieves the docker images & source for a particular build level
# specified as <buildType><buildStyle>:<buildLabel> where:
#
# - buildType can be d/D for development, p/P for production and x/X for personal.
# - buildStyle can be i/I for image download (default for development & production),
#      b/B for local build (default for personal), r/R for redhat:latest based local build,
#      c/C for a custom local build and  m/M for an MQConnectivity local build (using the
#      MQConnectivity docker file).
#
# In addition to downloading the images, the images are loaded and tagged with
# the build level in the local docker repository.
#
# The companion addServer.py can be used to create a container of these loaded
# images which have various volumes mounted, including the source for debugging.
#
# A number of variables must be set before this function can be used
# Sample text for inclusion in a .bashrc file:
#
# export IMA_ISM_ROOT=/ism
# export IMA_STREAM=STREAM
# export RTC_URI=https://changeme.jazzserver.example.com:9443/ccm
# export RTC_CMDLINE_HOME=/home/jon/bin/RTCcmdline/jazz
# export RTC_USER=changeme@company.com
# export RTC_WORKSPACE_PREFIX="ChangeMe_Workspace"
#
# Note: RTC_USER is in fact expected to be your general intranet userId.
#
# e.g.
#    getImage.py d:20150617-0400

import os
import sys
import subprocess
import shutil

GETIMAGES = True
LOADIMAGES = True
GETSOURCE = True

supportMQTTV5 = True

# -------------------------------------------------------------------
# Create a directory, removing the directory first if it already exists
# -------------------------------------------------------------------
def createDirectory( directory ):
    try:
        os.makedirs(directory, mode=0o777, exist_ok=True)
    except OSError as e:
        print("Failed to create %s [%s]" % (directory, str.format(e.args)), file=stderrFileHandle)
        sys.exit(23)

# ================================================================== #
# PROGRAM BEGINS                                                     #
# ================================================================== #
level = sys.argv[1]

if level == None:
    print("There must be a level argument in the form d:LEVEL or p:LEVEL.")
    sys.exit(1)

(buildType, level) = level.split(':')

buildStream = None
buildServerDir = None
centosVariant = False

if buildType.startswith('c') or buildType.startswith('C'):
    buildType = buildType[1:]
    centosVariant = True

if buildType.startswith('d') or buildType.startswith('D'):
    buildStyle = buildType[1:]
    if centosVariant:
        buildType = 'centos_development'
    else:
        buildType = 'development'
    if buildStyle == '':
        buildStyle = 'i'
elif buildType.startswith('p') or buildType.startswith('P'):
    buildStyle = buildType[1:]
    if centosVariant:
        buildType = 'centos_production'
    else:
        buildType = 'production'
    if buildStyle == '':
        buildStyle = 'i'
elif buildType.startswith('x') or buildType.startswith('X'):
    buildStyle = buildType[1:]
    buildType = 'personal'
    if buildStyle == '':
        buildStyle = 'b'
elif buildType.startswith('i') or buildType.startswith('I'):
    buildStyle = buildType[1:]
    buildType = 'iotf'
    buildServerDir = 'dependencies/messagesight'
    buildStream = 'iotf'
    if buildStyle == '':
        buildStyle = 'b'
else:
    print("Invalid build type '%s' specified, must 'p', 'd', 'i' or 'x'" % (buildType[1:1]))
    exit(2)

if buildStyle.startswith('i') or buildStyle.startswith('I'):
    if buildType == 'personal':
        print("No images produced for build type %s, switching to buildType 'b'")
        buildStyle = 'build'
    else:
        buildStyle = 'image'
else:
    if buildStyle.startswith('b') or buildStyle.startswith('B'):
        newBuildStyle = 'build'
    elif buildStyle.startswith('m') or buildStyle.startswith('M'):
        newBuildStyle = 'mqconnectivity'
    elif buildStyle.startswith('r') or buildStyle.startswith('R'):
        newBuildStyle = 'rhel7'
    elif buildStyle.startswith('d') or buildStyle.startswith('D'):
        newBuildStyle = 'msDevel'
    elif buildStyle.startswith('f') or buildStyle.startswith('F'):
        newBuildStyle = 'fedora'
    elif buildStyle.startswith('c') or buildStyle.startswith('C'):
        newBuildStyle = 'custom'
    elif buildStyle.startswith('w') or buildStyle.startswith('W'):
        newBuildStyle = 'WIoTP'
    else:
        print("Invalid build style '%s' specified, must be 'i' or 'b'." % (buildStyle))
        exit(3)

    if buildStyle.endswith('l') or buildStyle.endswith('L'):
        broot = os.environ.get('BROOT')
        if broot == None:
            print("Must set BROOT to location of build output")
            exit(4)
        sroot = os.environ.get('SROOT')
        if sroot == None:
            print("Must set SROOT to location of source")
            exit(5)
        newBuildStyle += 'Local'

    buildStyle = newBuildStyle

sourceRoot = os.environ.get('IMA_ISM_ROOT')
if sourceRoot == None:
    print('Must set IMA_ISM_ROOT to location for source')
    sys.exit(6)

rtcUserId = os.environ.get('RTC_USER')
if rtcUserId == None:
    print("Must set RTC_USER as the intranet userId to use")
    sys.exit(7)

# Assume the RTC and intranet user Ids are the same
intranetUserId = rtcUserId

if buildStream == None:
    buildStream = os.environ.get('IMA_STREAM')

if buildStream == None:
    if buildType != 'personal':
        print("Must set \$IMA_STREAM to be e.g. IMA100, IMA15a")
        sys.exit(8)

# The stream name used in snapshots is sometimes different to the buildStream
if buildStream == 'iotf' or buildStream == 'IMA15a-IoT-IFIX':
    snapBuildStream = 'IMA15a IoT IFIX'
elif buildStream == 'IMA15a' and (buildType == 'production' or buildType == 'centos_production'):
    snapBuildStream = 'IMA15a Integration'
elif buildStream == 'IMA15a-IFIX' and (buildType == 'production' or buildType == 'centos_production'):
    snapBuildStream = 'IMA15a IFIX'
else:
    snapBuildStream = buildStream

if buildServerDir == None:
    buildServerDir = buildStream

# At the moment, the builds for IMA100 are actually written to ISM13a
if buildServerDir == 'IMA100':
    buildServerDir = 'ISM13a'

rtcURI = os.environ.get('RTC_URI')
if rtcURI == None:
    if buildType != 'personal':
        print("Must set RTC_URI for the RTC server to connect to")
        sys.exit(9)

rtcCmdLineHome = os.environ.get('RTC_CMDLINE_HOME')
if rtcCmdLineHome == None:
    rtcCmdLineHome = os.environ.get('RTC_HOME')
if rtcCmdLineHome == None:
    if buildType != 'personal':
        print('Must set RTC_HOME to the location of the RTC client')
        sys.exit(10)

rtcWorkspacePrefix = os.environ.get('RTC_WORKSPACE_PREFIX')
if rtcWorkspacePrefix == None:
    if buildType != 'personal':
        print("Must set RTC_WORKSPACE_PREFIX")
        sys.exit(11)

dockerCmd = os.environ.get('DOCKER_CMD')
if dockerCmd == None:
    dockerCmd = 'docker'

rc = 0

qualifiedLevel = "%s-%s-%s" % (buildType, buildStyle, level)
levelDir = "%s/%s" % (sourceRoot, qualifiedLevel)
createDirectory(levelDir)

# Whenever a level changes the version number, we need to add something to the head
# of this if statement
if level >= '20181008-0042':
    imageVersion = '5.0.0.0'
    imageQualifier = imageVersion
elif level >= '20170829-0042':
    imageVersion = '2.0.0.2'
    imageQualifier = imageVersion
elif level >= "20160914-1004":
    imageVersion = '2.0.0.1'
    imageQualifier = imageVersion
elif level >= '20160823-2009':
    imageVersion = '2.0.0'
    imageQualifier = imageVersion
elif level >= '20160811-0400':
    imageVersion = '2.0'
    imageQualifier = imageVersion
elif level >= '20151231-1500':
    imageVersion = '2.0'
    imageQualifier = imageVersion+'.Beta'

# GET IMAGES
if GETIMAGES == True:
    if buildType == 'personal':
        imageServer = 'mar028.test.example.com'
        imageDirectory = "/home/%s/personalBuildPublish/bedrock/%s.%s/bedrock-sdk" % (intranetUserId, imageQualifier, level)
        imageProtocol = 'scp'
    elif buildType == 'iotf':
        imageServer = 'iot-test-01.hursley.ibm.com'
        imageDirectory = "%s/%s" % (buildServerDir, level)
        imageProtocol = 'https'
    else:
        imageServer = 'mar145.test.example.com'
        imageDirectory = "/gsacache/release/%s/%s/%s/appliance" % (buildServerDir, buildType, level)
        imageProtocol = 'scp'

    # Get the RPMs ready to build our own image
    if not buildStyle == 'image':
        if not os.path.isfile("%s/imaserver.rpm" % (levelDir)):
            print("Copying rpms from %s" % (imageServer))
            if imageProtocol == 'scp':
                rc = subprocess.call("scp -r %s@%s:%s/IBMWIoTPMessageGateway*%s*.tz %s" %
                                     (intranetUserId, imageServer, imageDirectory, imageQualifier, levelDir),
                                     shell=True)
            else:
                rc = subprocess.call("wget --user %s --ask-password --no-check-certificate -P %s %s://%s/%s/IBMWIoTPMessageGatewayServer-%s.%s.tz" %
                                     (intranetUserId, levelDir, imageProtocol, imageServer, imageDirectory, imageQualifier, level),
                                     shell=True)
                rc = subprocess.call("wget --user %s --ask-password --no-check-certificate -P %s %s://%s/%s/IBMWIoTPMessageGatewayWebUI-%s.%s.tz" %
                                     (intranetUserId, levelDir, imageProtocol, imageServer, imageDirectory, imageQualifier, level),
                                     shell=True)

            if rc != 0:
                print("Copying rpms returned %d" % (rc))
                sys.exit(21)

            print("Extracting IBMWIoTPMessageGatewayServer files")
            rc = subprocess.call("tar -C %s -xzf %s/IBMWIoTPMessageGatewayServer-%s.%s.tz" % (levelDir, levelDir, imageQualifier, level), shell=True)
            if rc != 0:
                print("Extracting IBMWIoTPMessageGatewayServer image returned %d" % (rc))
                sys.exit(22)

            try:
                print("%s/IBMWIoTPMessageGatewayServer-%s.%s.tz" %(levelDir, imageQualifier, level))
                os.remove("%s/IBMWIoTPMessageGatewayServer-%s.%s.tz" % (levelDir, imageQualifier, level))
            except Exception as e:
                print("%s exception removing zip file" % (type(e).__name__))
                print(e.args)
                sys.exit(23)

            try:
                os.rename("%s/IBMWIoTPMessageGatewayServer-%s-%s.x86_64.rpm" % (levelDir, imageVersion, level.replace("-", ".")), "%s/imaserver.rpm" % (levelDir))
                os.rename("%s/Dockerfile" % (levelDir), "%s/IBMWIoTPMessageGatewayServer-Dockerfile" % (levelDir))
            except Exception as e:
                print("%s exception renaming IBMWIoTPMessageGatewayServer rpm/Dockerfile" % (type(e).__name__))
                print(e.args)
                sys.exit(24)

        if not os.path.isfile("%s/imawebui.rpm" % (levelDir)):
            print("Extracting IBMWIoTPMessageGatewayWebUI files")
            rc = subprocess.call("tar -C %s -xzf %s/IBMWIoTPMessageGatewayWebUI-%s.%s.tz" % (levelDir, levelDir, imageQualifier, level), shell=True)
            if rc != 0:
                print("Extracting IBMWIoTPMessageGatewayWebUI image returned %d" % (rc))
                sys.exit(25)

            try:
                os.remove("%s/IBMWIoTPMessageGatewayWebUI-%s.%s.tz" % (levelDir, imageQualifier, level))
            except Exception as e:
                print("%s exception removing zip file" % (type(e).__name__))
                print(e.args)
                sys.exit(26)

            try:
                os.rename("%s/IBMWIoTPMessageGatewayWebUI-%s-%s.x86_64.rpm" % (levelDir, imageVersion, level.replace("-",".")), "%s/imawebui.rpm" % (levelDir))
                os.rename("%s/Dockerfile" % (levelDir), "%s/IBMWIoTPMessageGatewayWebUI-Dockerfile" % (levelDir))
            except Exception as e:
                print("%s exception renaming IBMWIoTPMessageGatewayWebUI rpm/Dockerfile" % (type(e).__name__))
                print(e.args)
                sys.exit(27)
    # Get the prebuilt images
    else:
        print("Copying docker images from %s" % (imageServer))
        rc = subprocess.call("scp -r %s@%s:%s/IBMWIoTPMessageGateway*%s*.zip %s" %
                             (intranetUserId, imageServer, imageDirectory, imageQualifier, levelDir),
                             shell=True)
        if rc != 0:
            print("Copying docker images returned %d" % (rc))
            sys.exit(41)

        print("Extracting IBMWIoTPMessageGatewayServer image")
        imageArchive = "%s/IBMWIoTPMessageGatewayServer-%s-docker.zip" % (levelDir, imageQualifier)
        rc = subprocess.call("unzip -n %s -d %s" % (imageArchive, levelDir), shell=True)
        if rc != 0:
            print("Extracting IBMWIoTPMessageGatewayServer image returned %d" % (rc))
            sys.exit(42)

        try:
            os.remove(imageArchive)
        except Exception as e:
            print("%s exception removing archive file %s" % (type(e).__name__, imageArchive))
            print(e.args)
            sys.exit(43)

        print("Extracting IBMWIoTPMessageGatewayWebUI image")
        imageArchive = "%s/IBMWIoTPMessageGatewayWebUI-%s-docker.zip" % (levelDir, imageQualifier)
        rc = subprocess.call("unzip -n %s -d %s" % (imageArchive, levelDir), shell=True)
        if rc != 0:
            print("Extracting IBMWIoTPMessageGatewayWebUI image returned %d" % (rc))
            sys.exit(44)

        try:
            os.remove(imageArchive)
        except Exception as e:
            print("%s exception removing archive file %s" % (type(e).__name__, imageArchive))
            print(e.args)
            sys.exit(45)

# LOAD IMAGES
if LOADIMAGES == True:
    # Any of the options that mean we're going to build an image...
    if not buildStyle == 'image':
        dockerFileName = "%s/IBMWIoTPMessageGatewayServer-Dockerfile" % (levelDir)
        modifiedDockerFileName = "%s/IBMWIoTPMessageGatewayServer-Modified-Dockerfile" % (levelDir)
        outputFile = open(modifiedDockerFileName, "w")
        with open ("%s" % (dockerFileName), "r") as inputFile:
            if buildStyle.startswith('mqconnectivity'):
                print("Altering Dockerfile to be based on mqclient:8.0.0.3")
                for inputLine in inputFile:
                    if inputLine.startswith('FROM '):
                        outputFile.write('FROM mqclient:8.0.0.3\n')
                    else:
                        outputFile.write(inputLine)
                        if inputLine.startswith('RUN yum -y install /tmp/imaserver.rpm'):
                            outputFile.write('RUN sed -i "/AllowMQTTv5/d" /opt/ibm/imaserver/config/server.cfg\n')
                            if supportMQTTV5 == True:
                                outputFile.write('RUN echo "AllowMQTTv5 = 1" >> /opt/ibm/imaserver/config/server.cfg\n')
            elif buildStyle.startswith('rhel7'):
                print("Altering Dockerfile to be based on rhel7:latest")
                for inputLine in inputFile:
                    if inputLine.startswith('FROM '):
                        outputFile.write('FROM rhel7:latest\n')
                    else:
                        outputFile.write(inputLine)
                        if inputLine.startswith('RUN yum -y install /tmp/imaserver.rpm'):
                            outputFile.write('RUN sed -i "/AllowMQTTv5/d" /opt/ibm/imaserver/config/server.cfg\n')
                            if supportMQTTV5 == True:
                                outputFile.write('RUN echo "AllowMQTTv5 = 1" >> /opt/ibm/imaserver/config/server.cfg\n')
            elif buildStyle.startswith('fedora'):
                print("Altering Dockerfile to be based on fedora:latest")
                for inputLine in inputFile:
                    if inputLine.startswith('FROM '):
                        outputFile.write('FROM fedora:latest\n')
                    elif inputLine.startswith('RUN yum -y upgrade'):
                        outputFile.write('RUN dnf -y upgrade\n')
                        outputFile.write('RUN dnf -y install bc gdb net-tools openssh-clients openssl tar perl procps-ng dos2unix jansson net-snmp net-snmp-devel logrotate zip bzip2 unzip libicu\n')
                        outputFile.write('RUN ln -s /lib64/libnetsnmp.so /lib64/libnetsnmp.so.31\n')
                        outputFile.write('RUN ln -s /lib64/libnetsnmpagent.so /lib64/libnetsnmpagent.so.31\n')
                        outputFile.write('RUN ln -s /lib64/libnetsnmphelpers.so /lib64/libnetsnmphelpers.so.31\n')
                    elif inputLine.startswith('RUN yum -y install /tmp/imaserver.rpm'):
                        outputFile.write('RUN rpm -ivh --nodeps /tmp/imaserver.rpm\n')
                        outputFile.write('RUN sed -i "/AllowMQTTv5/d" /opt/ibm/imaserver/config/server.cfg\n')
                        if supportMQTTV5 == True:
                            outputFile.write('RUN echo "AllowMQTTv5 = 1" >> /opt/ibm/imaserver/config/server.cfg\n')
                    else:
                        outputFile.write(inputLine)
            elif buildStyle.startswith('msDevel'):
                print("Altering Dockerfile to be based on centos:msDevel")
                for inputLine in inputFile:
                    if inputLine.startswith('FROM '):
                        outputFile.write('FROM centos:msDevel\n')
                    else:
                        outputFile.write(inputLine)
                        if inputLine.startswith('RUN yum -y install /tmp/imaserver.rpm'):
                            outputFile.write('RUN sed -i "/AllowMQTTv5/d" /opt/ibm/imaserver/config/server.cfg\n')
                            if supportMQTTV5 == True:
                                outputFile.write('RUN echo "AllowMQTTv5 = 1" >> /opt/ibm/imaserver/config/server.cfg\n')
            elif buildStyle.startswith('WIoTP'):
                print("Altering Dockerfile to look like IoT container")
                for inputLine in inputFile:
                    outputFile.write(inputLine)
                    if inputLine.startswith('RUN yum -y install /tmp/imaserver.rpm'):
                        # Additional lines taken from the Dockerfile in messagesight-base
                        outputFile.write('\n# dos2unix\n')
                        outputFile.write('RUN sed -i \'s/\\r//\' /opt/ibm/imaserver/config/server.cfg\n')
                        outputFile.write('\n# Add newline, if missing\n')
                        outputFile.write('RUN sed -i -e \'$a\\\' /opt/ibm/imaserver/config/server.cfg\n')
                        outputFile.write('\n# Set correct static parameters for IoTF\n')
                        outputFile.write('RUN sed -i "/Throttle.Enabled/d" /opt/ibm/imaserver/config/server.cfg\n')
                        outputFile.write('RUN sed -i "/HA.AllowSingleNIC/d" /opt/ibm/imaserver/config/server.cfg\n')
                        outputFile.write('RUN sed -i "/Protocol.AllowMqttProxyProtocol/d" /opt/ibm/imaserver/config/server.cfg\n')
                        outputFile.write('RUN sed -i "/HA.SplitBrainPolicy/d" /opt/ibm/imaserver/config/server.cfg\n')
                        outputFile.write('RUN sed -i "/MqttProxyMonitoring/d" /opt/ibm/imaserver/config/server.cfg\n')
                        outputFile.write('RUN echo "Throttle.Enabled = false" >> /opt/ibm/imaserver/config/server.cfg\n')
                        outputFile.write('RUN echo "HA.AllowSingleNIC = 1" >> /opt/ibm/imaserver/config/server.cfg\n')
                        outputFile.write('RUN echo "Protocol.AllowMqttProxyProtocol = 2" >> /opt/ibm/imaserver/config/server.cfg\n')
                        outputFile.write('RUN echo "HA.SplitBrainPolicy = 1" >> /opt/ibm/imaserver/config/server.cfg\n')
                        outputFile.write('RUN echo "MqttProxyMonitoring.0 = iot-2/+/type/#,c:*,0,0,*,2,45" >> /opt/ibm/imaserver/config/server.cfg\n')
                        outputFile.write('RUN echo "MqttProxyMonitoring.1 = iot-2/+/app/#,*,1,45" >> /opt/ibm/imaserver/config/server.cfg\n')
                        outputFile.write('RUN sed -i "/AllowMQTTv5/d" /opt/ibm/imaserver/config/server.cfg\n')
                        if supportMQTTV5 == True:
                            outputFile.write('RUN echo "AllowMQTTv5 = 1" >> /opt/ibm/imaserver/config/server.cfg\n')
            else:
                print("Ensuring Dockerfile is based on centos:latest")
                for inputLine in inputFile:
                    if inputLine.startswith('FROM '):
                        outputFile.write('FROM centos:latest\n')
                    else:
                        outputFile.write(inputLine)
                        if inputLine.startswith('RUN yum -y install /tmp/imaserver.rpm'):
                            outputFile.write('RUN sed -i "/AllowMQTTv5/d" /opt/ibm/imaserver/config/server.cfg\n')
                            if supportMQTTV5 == True:
                                outputFile.write('RUN echo "AllowMQTTv5 = 1" >> /opt/ibm/imaserver/config/server.cfg\n')

        outputFile.close()

        # add in the various packages required to build inside the container
        # epel-release openssl-devel openldap-devel CUnit-devel pam-devel curl-devel gcc make vim-common gcc-c++ net-snmp-devel
        # subprocess.call("sed -i \"s/\\(RUN yum -y upgrade.*\\)/\\1\\nRUN yum -y install epel-release\\nRUN yum -y install openssl-devel openldap-devel CUnit-devel pam-devel curl-devel gcc make vim-common gcc-c++ net-snmp-devel/g\" %s/IBMWIoTPMessageGatewayServer-Dockerfile" % (levelDir), shell=True)

        if buildStyle.startswith('custom'):
            print("Dropping into a shell for customization of %s" % (levelDir))
            subprocess.call("/bin/bash")

        if buildStyle.endswith('Local'):
            shutil.copytree("%s/server_ship" % (broot), "%s/server_ship" % (levelDir))
            subprocess.call("echo ADD server_ship/bin /opt/ibm/imaserver/bin >> %s" % (modifiedDockerFileName), shell=True)
            subprocess.call("echo ADD server_ship/debug/bin /opt/ibm/imaserver/debug/bin >> %s" % (modifiedDockerFileName), shell=True)
            subprocess.call("echo ADD server_ship/lib /opt/ibm/imaserver/lib64 >> %s" % (modifiedDockerFileName), shell=True)
            subprocess.call("echo ADD server_ship/debug/lib /opt/ibm/imaserver/debug/lib64 >> %s" % (modifiedDockerFileName), shell=True)

        print("Building docker images from %s" % (levelDir))
        rc = subprocess.call("%s build -f %s -t imaserver:%s %s" % (dockerCmd, modifiedDockerFileName, qualifiedLevel, levelDir), shell=True)
        if rc != 0:
            print("Building IBMWIoTPMessageGatewayServer image returned %d" % (rc))
            sys.exit(61)

        #try:
        #    os.remove("%s/imaserver.rpm" % (levelDir))
        #    os.remove("%s/imaserver-Dockerfile" % (levelDir))
        #    os.remove("%s/imaserver-DockerfileIncludingMQConnectivity" % (levelDir))
        #except Exception as e:
        #    print("%s exception removing imaserver rpm/Dockerfile" % (type(e).__name__))
        #    print(e.args)
        #    sys.exit(62)

        rc = subprocess.call("%s build -f %s/IBMWIoTPMessageGatewayWebUI-Dockerfile -t imawebui:%s %s" % (dockerCmd, levelDir, qualifiedLevel, levelDir), shell=True)
        if rc != 0:
            print("Building IBMWIoTPMessageGatewayWebUI image returned %d" % (rc))
            sys.exit(63)

        #try:
        #    os.remove("%s/imawebui.rpm" % (levelDir))
        #    os.remove("%s/imawebui-Dockerfile" % (levelDir))
        #except Exception as e:
        #    print("%s exception removing imawebui rpm/Dockerfile" % (type(e).__name__))
        #    print(e.args)
        #    sys.exit(64)
    # Load the prebuilt images
    else:
        print("Loading docker images from %s" % (levelDir))
        rc = subprocess.call("%s load -i %s/IBMWIoTPMessageGatewayServer-%s-dockerImage.tar" % (dockerCmd, levelDir, imageQualifier), shell=True)
        if rc != 0:
            print("Loading IBMWIoTPMessageGatewayServer image returned %d" % (rc))
            sys.exit(81)

        rc = subprocess.call("%s tag IBMWIoTPMessageGatewayserver:%s imaserver:%s" % (dockerCmd, imageQualifier, qualifiedLevel), shell=True)
        if rc != 0:
            print("Tagging IBMWIoTPMessageGatewayserver image with tag %s returned %d" % (qualifiedLevel, rc))
            sys.exit(82)

        rc = subprocess.call("%s rmi IBMWIoTPMessageGatewayserver:%s" % (dockerCmd, imageQualifier), shell=True)
        if rc != 0:
            print("Removing IBMWIoTPMessageGatewayserver image with tag %s returned %d" % (imageQualifier, rc))
            sys.exit(83)

        try:
            os.remove("%s/IBMWIoTPMessageGatewayServer-%s-dockerImage.tar" % (levelDir, imageQualifier))
        except Exception as e:
            print("%s exception removing IBMWIoTPMessageGatewayServer tar file" % (type(e).__name__))
            print(e.args)
            sys.exit(84)

        rc = subprocess.call("%s load -i %s/IBMWIoTPMessageGatewayWebUI-%s-dockerImage.tar" % (dockerCmd, levelDir, imageQualifier), shell=True)
        if rc != 0:
            print("Loading IBMWIoTPMessageGatewayWebUI image returned %d" % (rc))
            sys.exit(85)

        rc = subprocess.call("%s tag IBMWIoTPMessageGatewaywebui:%s imawebui:%s" % (dockerCmd, imageQualifier, qualifiedLevel), shell=True)
        if rc != 0:
            print("Tagging IBMWIoTPMessageGatewaywebui image with tag %s returned %d" % (qualifiedLevel, rc))
            sys.exit(86)

        rc = subprocess.call("%s rmi IBMWIoTPMessageGatewaywebui:%s" % (dockerCmd, imageQualifier), shell=True)
        if rc != 0:
            print("Removing IBMWIoTPMessageGatewaywebui image with tag %s returned %d" % (imageQualifier, rc))
            sys.exit(87)

        try:
            os.remove("%s/IBMWIoTPMessageGatewayWebUI-%s-dockerImage.tar" % (levelDir, imageQualifier))
        except Exception as e:
            print("%s exception removing IBMWIoTPMessageGatewayWebUI tar file" % (type(e).__name__))
            print(e.args)
            sys.exit(88)

# GET SOURCE
if GETSOURCE == True:
    print("Creating source directories under %s" % (levelDir))
    sourceDir = "%s/source" % (levelDir)

    if buildType == 'personal':
        sourceServer = 'mar028.test.example.com'
        remoteSourceDirectory = "/home/%s/personalBuildPublish/workspace" % (intranetUserId)

        if not os.path.isdir(sourceDir):
            createDirectory(sourceDir)
            print("Copying source from %s" % (sourceServer))
            rc = subprocess.call("rsync -tr --include='*/' --include='*.c' --include='*.h' --include='*.cpp' --exclude='*' --prune-empty-dirs %s@%s:%s/* %s" %
                                 (intranetUserId, sourceServer, remoteSourceDirectory, sourceDir), shell=True)
            if rc != 0:
                print("Copying source returned %d" % (rc))
                sys.exit(101)

            #print("*TEMPORARILY* Copying ICU libraries")
            #icuDir = os.environ.get('IMA_ICU_HOME')
            #if icuDir == None:
            #    icuDir = '/usr/lib64'
            #else:
            #    icuDir += '/lib'

            #createDirectory(sourceDir+"/server_ship/lib")

            #rc = subprocess.call("cp %s/*icu*.so* %s/server_ship/lib" % (icuDir, sourceDir), shell=True)
            #if rc != 0:
            #    print("Copying ICU libraries returned %d" %(rc))
            #    sys.exit(102)
    else:
        if not os.path.isdir(sourceDir):
            createDirectory(sourceDir)
            if snapBuildStream == 'IMA15aX':
                snapshotNameA = "%s - CentOS Experimental Build_%s" % (snapBuildStream, level)
                snapshotNameB = "%s - Austin Experimental Build_%s" % (snapBuildStream, level)
            elif buildType == 'development' or buildType == 'centos_development':
                snapshotNameA = "%s - CentOS Development Build_%s" % (snapBuildStream, level)
                snapshotNameB = "%s - Austin Development Build_%s" % (snapBuildStream, level)
            else:
                snapshotNameA = "%s - CentOS Production Build_%s" % (snapBuildStream, level)
                snapshotNameB = "%s - Hursley Production Build_%s" % (snapBuildStream, level)

            scmTool = "%s/scmtools/eclipse/lscm" % (rtcCmdLineHome)
            workspaceName = "%s_%s_%s" % (rtcWorkspacePrefix, buildType, level)

            error = 0

            print("Logging into RTC server %s" % (rtcURI))
            rc = subprocess.call("%s login -n rtc_server -r %s -u %s -c" % (scmTool, rtcURI, rtcUserId),
                                 shell=True)
            if rc != 0:
                print("Logging into RTC server returned %d" % (rc))
                error = 103
            else:
                print("Creating workspace %s" % (workspaceName))
                rc = subprocess.call("%s create workspace \"%s\" -r rtc_server --snapshot \"%s\"" % (scmTool, workspaceName, snapshotNameA),
                                 shell=True)
                if rc != 0:
                    print("Creating workspace based on snapshot '%s' returned %d" % (snapshotNameA, rc))
                    if snapshotNameB == None:
                        error = 104
                    else:
                        rc = subprocess.call("%s create workspace \"%s\" -r rtc_server --snapshot \"%s\"" % (scmTool, workspaceName, snapshotNameB),
                                             shell=True)
                        if rc != 0:
                            print("Creating workspace based on snapshot '%s' returned %d" % (snapshotNameB, rc))
                            error = 104

                if error == 0:
                    print("Loading workspace %s" % (workspaceName))
                    rc = subprocess.call("%s load \"%s\" -r rtc_server --force -d %s" % (scmTool, workspaceName, sourceDir),
                                     shell=True)
                    if rc != 0:
                        print("Loading workspace returned %d" % (rc))
                        error = 106

                    print("Deleting workspace %s" % (workspaceName))
                    rc = subprocess.call("%s workspace delete -r rtc_server \"%s\"" % (scmTool, workspaceName),
                                         shell=True)
                    if rc != 0:
                        print("Deleting workspace returned %d" % (rc))
                        error = 107

                print("Logging out of RTC server")
                rc = subprocess.call("%s logout -r rtc_server" % (scmTool), shell=True)
                if rc != 0:
                    print("Logging out of RTC returned %d" % (rc))
                    error = 107
            
            if error != 0:
                sys.exit(error)
