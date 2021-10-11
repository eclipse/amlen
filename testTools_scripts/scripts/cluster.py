#!/usr/bin/python
# -*- coding: utf-8 -*-
# Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

# MessageSight script for performing cluster related actions
# Expectations: If creating a cluster or adding a cluster member,
#               the control and messaging address are expected to
#               have already been set for the servers being added.
# 
# Actions for a single server:
#  addClusterMember, removeClusterMember, killClusterMember, stopClusterMember,
#  startClusterMember, verifyStatus, restartClusterMember, cleanStore
# 
# Actions for a group of servers:
#  createCluster, tearDown, stopCluster, startCluster
# 
# Cluster Config Properties:
#  EnableClusterMembership      True | False, default False
#  ClusterName                  max length 256 character string
#  UseMulticastDiscovery        True | False, default True
#  MulticastDiscoveryTTL        1 - 256, default 1
#  DiscoveryServerList          max length 65535 character string. format: serveruid@serverip:port
#  ControlAddress               IP Address
#  ControlPort                  1 - 65535, default 9104
#  MessagingAddress             IP Address
#  MessagingPort                1 - 65535, default 9105
#  MessagingUseTLS              True | False, default False
#  DiscoveryPort                1 - 65535, default 9106
#  DiscoveryTime                1 - 2147483647, default 10
#  ControlExternalAddress       IP Address
#  ControlExternalPort          1 - 65535
#  MessagingExternalAddress     IP Address
#  MessagingExternalPort        1 - 65535
#  
# Example payload:
#  {"ClusterMembership":{"EnableClusterMembership":true,"ControlAddress":"10.10.2.15","ClusterName":"Cluster1"}}
#
# Notes on configuring HA in a cluster:
#  Configure EnableHA, LocalDiscoveryNIC, LocalReplicationNIC, RemoteDiscoveryNIC, Group using
#  curl or run-cli.sh or some other method on the 2 servers that will form an HA pair.
#
#  *** DO NOT restart the server where you have configured HA, yet. ***
#
#  Run cluster.py createCluster specifying each server in the -l argument (including standby).
#  This will set matching cluster configuration on both servers that will form the HA pair.
#  The servers will then be restarted and the intended standby will come up in standby
#  while the primary joins the cluster.
#

import sys
import os
import time
import subprocess
import argparse
import traceback
import collections
import requests
import json

FILENAME='cluster.py'

# Set imaserver status constants
STATUS = {'STATUS_RUNNING': 'Running (production)',
    'STATUS_STANDBY': 'Standby',
    'STATUS_MAINTENANCE': 'Running (maintenance)',
    'STATUS_STOPPED': 'The server is stopped.',
    'STATUS_STARTING': 'server is starting',
    'STATUS_ALREADY_RUNNING': 'already running'}

# Set cluster status contants
CLUSTER_STATUS = {'ACTIVE': 'Active',
    'INITIALIZING': 'Initializing',
    'STANDBY': 'Standby',
    'REMOVED': 'Removed',
    'DISABLED': 'Inactive'}

# Special error message for removing last server from cluster -- not an error.
LAST_MEMBER = 'Cluster remove local server timed out without acknowledgement from remote servers'

#------------------------------------------------------------------------------
# Trace Functions
#------------------------------------------------------------------------------
def trace(traceLevel, message):
  if DEBUG >= traceLevel:
    formatting_string = '[%Y %m-%d %H:%M-%S00]'
    timestamp = time.strftime(formatting_string)
    print( '\n%s %s' % (timestamp, message) )

def traceEntry(method):
  trace(1, '>>> ' + FILENAME + ':' + method)

def traceExit(method):
  trace(1, '<<< ' + FILENAME + ':' + method)
  
def traceRestCall(action, url, payload):
  if action == 'POST':
    trace(0, '\n\t\t'+action+' PAYLOAD='+str(payload)+'\n\t\tURL: '+url)
  elif action == 'GET':
    trace(0, '\n\t\t'+action+' URL: '+url)

def traceRestFail(action, description, response):
  trace(0, '\n\t\tFAILED: '+action+' '+description+'\n\t\tRESPONSE.STATUS_CODE: '+str(response.status_code)+'\n\t\tRESPONSE.CONTENT: '+response.content)
  
#------------------------------------------------------------------------------
# Print the end result, attempt to close log file if it exists, and exit
#------------------------------------------------------------------------------
def exit(rc):
  if rc == 0:
    print( 'Cluster.py: Test result is Success!' )
  else:
    print( 'Cluster.py: Test result is Failure!' )
  try:
    LOG.close()
    sys.stdout = OLD_STDOUT
  except:
    pass
  sys.exit( rc )

#------------------------------------------------------------------------------
# Parse properties from ISMsetup or testEnv input file
# Expected format:
#  'export PROPERTY_NAME=PROPERTY_VALUE'
#------------------------------------------------------------------------------
def parseConfig(filename):
  COMMENT_CHAR = '#'
  OPTION_CHAR =  '='
  EXPORT_CHAR = ' '
  OPTIONS = {}
  f = open(filename,'r')
  data = f.read()
  f.close()
  data = data.split("\n")
  for line in data:
    if COMMENT_CHAR in line:
      line, comment = line.split(COMMENT_CHAR, 1)
    if OPTION_CHAR in line:
      if line[0] != 'e':
        continue
      export, newline = line.split(EXPORT_CHAR, 1)
      option, value = newline.split(OPTION_CHAR, 1)
      option = option.strip()
      value = value.strip()
      OPTIONS[option] = value
  return OPTIONS

#------------------------------------------------------------------------------
# Run the command specified by args
# Check that the output of the command matches expected
#------------------------------------------------------------------------------
def runCommand(args, expected):
  traceEntry('runCommand')
  trace(0, args )

  rc = 1
  output = ''
  try:
    res = subprocess.Popen(args, stderr=subprocess.PIPE, stdout=subprocess.PIPE).communicate()
    if res[0] == '':
      output = res[1]
    else:
      output = res[0]
  except:
    pass
  trace(0, output )

  if expected != '':
    if expected in output:
      rc = 0
    else:
      trace(0, 'runCommand rc=1 -- Output: '+output+'\nExpected: '+expected )
  else:
    rc = 0

  resultTuple = collections.namedtuple('Result', ['rc', 'message'])
  result = resultTuple(rc, output)

  traceExit('runCommand')
  return ( result )

#------------------------------------------------------------------------------
# Call status imaserver in a loop
# parameters:
#   status - The desired status response
#   timeout - The amount of time to wait for the desired status to be reached
#   serverID - The A_COUNT index for the server to check
#   inverse - boolean. If true, pass when the status response DOES NOT contain
#       the desired status
#------------------------------------------------------------------------------
def statusLoop(status, timeout, serverID, inverse):
  traceEntry('statusLoop')
  rc=1

  url = OPTIONS['A'+serverID+'_REST_ARGS']+'/ima/v1/service/status' 
  elapsed=0
  output = ''
  while elapsed < timeout:
    elapsed+=1
    time.sleep(1)
    try:
      trace(1, 'GET ' + url)
      response = requests.get(url=url)

      trace(2, 'response.status_code: ' + str(response.status_code))
      jsonPayload = json.loads(response.content)
      output = jsonPayload['Server']['StateDescription']
      trace(2, 'response.content: ' + output)

      if inverse == False:
        if status in output:
          rc = 0
          break
        if status == STATUS['STATUS_RUNNING'] and STATUS['STATUS_STANDBY'] in output:
          # Allow for Status = Standby when we are looking for Status = Running if this is an HA server
          rc = 0
          break
      else:
        if status not in output:
          rc = 0
          break
    except:
      trace(2, 'statusLoop: got exception')
      if status == STATUS['STATUS_STOPPED']:
        output = 'The server is stopped.'
        rc = 0
        break
      #traceback.print_tb(sys.exc_info()[2])
      pass

  if rc == 1:
    trace(0, 'Status did not match!\nresponse.content: '+output )
  else:
    trace(0, 'Status matched! ' + output)

  resultTuple = collections.namedtuple('Result', ['rc', 'message'])
  result = resultTuple(rc, output)

  traceExit('statusLoop')
  return ( result )

#------------------------------------------------------------------------------
# Verify that the server status matches the desired status
# parameters:
#   status - The desired status response
#   timeout - The amount of time to wait for the desired status to be reached
#   serverID - The A_COUNT index for the server to check
#   inverse - boolean. If true, pass when the status response DOES NOT contain
#       the desired status
#   ignoreFail - Do not return a failure return code if we don't reach the
#        desired status in the allotted time.
#------------------------------------------------------------------------------
def verifyStatus(status, timeout, serverID, inverse, ignoreFail):
  traceEntry('verifyStatus')

  result = statusLoop(status, timeout, serverID, inverse)
  rc = result.rc
  if rc == 1 and ignoreFail == True:
    rc = 0

  traceExit('verifyStatus')
  return ( rc )

#------------------------------------------------------------------------------
# status, timeout, id, inverse, ignore
#------------------------------------------------------------------------------
def verifyStatusWrapper(args):
  traceEntry('verifyStatusWrapper')

  rc = verifyStatus(STATUS[args.s], args.t, args.m, False, False)

  traceExit('verifyStatusWrapper')
  return ( rc )

#------------------------------------------------------------------------------
# Check the Cluster status of server status API
# parameters:
#   serverID, status
#------------------------------------------------------------------------------
def checkClusterStatus(serverID, status):
  traceEntry('checkClusterStatus')

  rc = 0
  url = OPTIONS['A'+str(serverID)+'_REST_ARGS'] + '/ima/v1/service/status'
  traceRestCall('GET', url, None)
  response = requests.get(url=url)

  if response.status_code != 200:
    traceRestFail('GET', 'status for server A'+str(serverID), response)
    rc = 1
    return ( rc )

  # TODO: When defect fix goes in, just have the check for ['Cluster']['Status']
  jsonPayload = json.loads(response.content)
  if jsonPayload['Cluster'].get('Status'):
    if jsonPayload['Cluster']['Status'] == status:
      trace(0, 'response.status_code: ' + str(response.status_code))
      trace(0, 'response.content: ' + status)
    else:
      if STATUS['STATUS_STANDBY'] in jsonPayload['Server']['StateDescription'] and CLUSTER_STATUS['STANDBY'] == jsonPayload['Cluster']['Status']:
        trace(0, 'response.status_code: ' + str(response.status_code))
        trace(0, 'response.content: ' + status)
      else:
        trace(0, 'Cluster status did not match!\n\t' + jsonPayload['Cluster']['Status'])
        rc = 1

  else:
    trace(0, 'Cluster status did not match!\n\t' + json.dumps(jsonPayload['Cluster']))
    rc = 1

  traceExit('checkClusterStatus')
  return ( rc )


#------------------------------------------------------------------------------
# serverID, status(ACTIVE, INITIALIZING, STANDBY, REMOVED, DISABLED)
#------------------------------------------------------------------------------
def checkClusterStatusWrapper(argv):
  traceEntry('checkClusterStatusWrapper')

  rc = checkClusterStatus(argv.m, CLUSTER_STATUS[argv.cs])

  traceExit('checkClusterStatusWrapper')
  return ( rc )

#------------------------------------------------------------------------------
# Call the Admin Remove Inactive Member REST API
# does a GET on monitor/Cluster, finds the first "Inactive" member and calls the Admin Remove Inactive Member API
# parameters:
#   argv.m - the machine to call the API from, argv.m2r - the machine number of member to remove from cluster
#------------------------------------------------------------------------------

def removeInactiveMember(argv):

  traceEntry('removeInactiveMember')

  if argv.m == '' or argv.m == None:
    trace(0, 'removeInactiveMember requires -m argument' )
    sys.exit( 1 )

  rc = 0
  
  url = OPTIONS['A'+argv.m+'_REST_ARGS'] + '/ima/v1/monitor/Cluster'
  traceRestCall('GET', url, None)
  response = requests.get(url=url)
  trace(0, json.dumps(response.content))

  if response.status_code != 200:
    traceRestFail('GET', 'monitor/Cluster for server A'+argv.m, response)
    rc = 1
    return ( rc )

  jsonPayload = json.loads(response.content)
  serverList = jsonPayload['Cluster']

  #if member to remove is not specified then find the first inactive member
  if argv.m2r == '' or argv.m2r == None:
    for server in range(len(serverList)):
      if serverList[server]['Status'] == "Inactive":
        serverName = serverList[server]['ServerName']
        serverUID = serverList[server]['ServerUID']
        trace(0,'Found inactive server: ServerName: '+serverName + ', ServerUID: ' + serverUID)
        break
    else:
      #if no inactive members
      trace(0, 'No inactive members found.\n\t' + json.dumps(jsonPayload['Cluster']))
      rc = 1
      return (rc)
  else:
    #if m2r was specified then get its ServerName and serverUID
    serverName = OPTIONS['A'+str(argv.m2r)+'_IPv4_HOSTNAME_1']
    for server in range(len(serverList)):
      if serverList[server]['ServerName'] == serverName:
        serverUID = serverList[server]['ServerUID']
        trace(0,'Got serverUID for ServerName: '+serverName + ', ServerUID: ' + serverUID)
        break
    else:
      trace(0, 'Server m2r was not found.\n\t' + json.dumps(jsonPayload['Cluster']))
      rc = 1
      return (rc)

  #rest-admin remove
  url = OPTIONS['A'+argv.m+'_REST_ARGS']+'/ima/v1/service/remove/inactiveClusterMember'
  payload = '{"ServerName":"'+serverName + '",' + '"ServerUID":"' + serverUID + '"}'
  traceRestCall('POST', url, payload)
  response = requests.post(url=url, data=payload)

  if response.status_code != 200:
    traceRestFail('POST', 'Remove Inactive Cluster Member for server A'+argv.m, response)
    rc = 1
    return ( rc )  


  traceExit('removeInactiveMember')
  return ( rc )



#------------------------------------------------------------------------------
# Clean the store of a server
# parameters:
#   serverID - The A_COUNT index for the server to clean
#------------------------------------------------------------------------------
def cleanStore(argv):
  traceEntry('cleanStore')

  rc = 0
  try:
    url = OPTIONS['A'+argv.m+'_REST_ARGS']+'/ima/v1/service/restart'
    payload = '{"Service":"Server","CleanStore":true}'
    traceRestCall('POST', url, payload)
    response = requests.post(url=url, data=payload)

    if response.status_code != 200:
      traceRestFail('POST', 'CleanStore on server A'+argv.m, response)
      rc = 1
      return ( rc )

    time.sleep(5)
    rc = verifyStatus(STATUS['STATUS_RUNNING'], 30, argv.m, False, False)
    trace(1, 'status check: ' + str(rc) )
  except:
    raise

  traceExit('cleanStore')
  return ( rc )

#------------------------------------------------------------------------------
# Compare two Server UID's
#------------------------------------------------------------------------------
def compareServerUID(a,b,shouldMatch):
  if shouldMatch == True:
    return ( a == b )
  else:
    return ( a != b )

#------------------------------------------------------------------------------
# Get the ServerUID of a server
#------------------------------------------------------------------------------
def getServerUID(serverID):
  traceEntry('getServerUID')

  serverUID = ''
  url = OPTIONS['A'+serverID+'_REST_ARGS']+'/ima/configuration/ServerUID'
  traceRestCall('GET', url, None)
  response = requests.get(url=url)
  if response.status_code != 200:
    traceRestFail('GET', 'Get ServerUID on server A'+serverID, response)
    rc = 1
    return ( rc )

  traceExit('getServerUID')
  return ( serverUID )

#------------------------------------------------------------------------------
# Get and verify the ClusterMembership configuration of a server
#------------------------------------------------------------------------------
def getClusterMembership(serverID, expected):
  traceEntry('getClusterMembership')

  rc = 0
  try:
    url = OPTIONS['A'+serverID+'_REST_ARGS']+'/ima/configuration/ClusterMembership'
    traceRestCall('GET', url, None)
    response = requests.get(url=url)
    if response.status_code != 200:
      traceRestFail('GET', 'Get ClusterMembership on server A'+serverID, response)
      rc = 1
      return ( rc )
  except:
    raise

  # verify cluster membership properties
  #for prop in expected:
  #  jsonpropvalue = get prop from json response
  #  if jsonpropvalue != propvalue
  #    fail

  traceExit('getClusterMembership')
  return ( rc )

#------------------------------------------------------------------------------
# Update the ClusterMembership configuration of a server
#------------------------------------------------------------------------------
def updateClusterMembership(serverID, payload):
  traceEntry('updateClusterMembership')

  rc = 0
  try:
    url = OPTIONS['A'+serverID+'_REST_ARGS']+'/ima/configuration'
    traceRestCall('POST', url, payload)
    response = requests.post(url=url, data=payload)
    if response.status_code != 200:
      if LAST_MEMBER in response.content:
        trace(1, 'last member to leave cluster, not an error' )
      else:
        traceRestFail('POST', 'POST ClusterMembership on server A'+serverID, response)
        rc = 1
        return ( rc )
  except:
    raise

  traceExit('updateClusterMembership')
  return ( rc )

#------------------------------------------------------------------------------
# Kill Cluster Member
# Kills a specific server in a cluster
#------------------------------------------------------------------------------
def killClusterMember(argv):
  traceEntry('killClusterMember')

  if argv.m == '' or argv.m == None:
    trace(0, 'stopClusterMember requires -m argument' )
    sys.exit( 1 )

  rc = 0
  try:
    #if OPTIONS['A'+argv.m+'_TYPE'] == 'DOCKER':
    # TODO: IF DOCKER ELSE IF RPM
    #  dockerArgs = ['docker', 'stop', OPTIONS['A'+argv.m+'_SRVCONTID']]
    #  result = runCommand(OPTIONS['A'+argv.m+'_SSH_ARGS'] + dockerArgs, '')
    #  if result.rc != 0:
    #    trace(0, 'FAILED Docker stop A'+argv.m)
    #    rc = 1
    #    return ( rc )

    #  rc = verifyStatus(STATUS['STATUS_STOPPED'], 15, argv.m, False, False)
    #  trace(1, 'status check: ' + str(rc) )
    #elif OPTIONS['A'+argv.m+'_TYPE'] == 'RPM':
    serverControlArgs = ['../scripts/serverControl.sh', '-a', 'killServer', '-i', argv.m]
    result = runCommand(serverControlArgs, '')
    if result.rc != 0:
      trace(0, 'FAILED serverControl.sh killServer A'+argv.m)
      rc = 1
      return ( rc )
  
    rc = verifyStatus(STATUS['STATUS_STOPPED'], 15, argv.m, False, False)
    trace(1, 'status check: ' + str(rc) )
  except:
    raise

  traceExit('killClusterMember')
  return ( rc )

#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
def restartClusterMember(argv):
  traceEntry('restartClusterMember')

  if argv.m == '' or argv.m == None:
    trace(0, 'restartClusterMember requires -m argument')
    sys.exit( 1 )

  rc = 0
  
  url = OPTIONS['A'+argv.m+'_REST_ARGS']+'/ima/v1/service/restart'
  payload = '{"Service":"Server"}'
  traceRestCall('POST', url, payload)
  response = requests.post(url=url, data=payload)

  if response.status_code != 200:
    traceRestFail('POST', 'RestartClusterMember for server A'+argv.m, response)
    rc = 1

  traceExit('restartClusterMember')
  return( rc )
  
#------------------------------------------------------------------------------
# Start Cluster Member
# Starts a specific server in a cluster
#------------------------------------------------------------------------------
def startClusterMember(argv):
  traceEntry('startClusterMember')

  if argv.m == '' or argv.m == None:
    trace(0, 'stopClusterMember requires -m argument' )
    sys.exit( 1 )

  rc = 0
  try:
    serverControlArgs = ['../scripts/serverControl.sh', '-a', 'startServer', '-i', argv.m]
    result = runCommand(serverControlArgs, '')
    if result.rc != 0:
      trace(0, 'FAILED serverControl.sh startServer A'+argv.m)
      rc = result.rc
  
    # TODO: IF DOCKER ELSE IF RPM
    #dockerArgs = ['docker', 'start', OPTIONS['A'+str(argv.m)+'_SRVCONTID']]
    #result = runCommand(OPTIONS['A'+argv.m+'_SSH_ARGS'] + dockerArgs, '')
    #rc = result.rc
  except:
    raise

  traceExit('startClusterMember')
  return ( rc )

#------------------------------------------------------------------------------
# Stop Cluster Member
# Stops a specific server in a cluster
#------------------------------------------------------------------------------
def stopClusterMember(argv):
  traceEntry('stopClusterMember')

  if argv.m == '' or argv.m == None:
    trace(0, 'stopClusterMember requires -m argument' )
    sys.exit( 1 )
  
  rc = 0
  url = OPTIONS['A'+argv.m+'_REST_ARGS']+'/ima/v1/service/stop'
  payload = '{"Service":"Server"}'
  traceRestCall('POST', url, payload)
  response = requests.post(url=url, data=payload)

  if response.status_code != 200:
    traceRestFail('POST', 'StopClusterMember for server A'+argv.m, response)
    rc = 1
    return ( rc )


  rc = verifyStatus(STATUS['STATUS_STOPPED'], 15, argv.m, False, False)
  
  traceExit('stopClusterMember')
  return ( rc )

#------------------------------------------------------------------------------
# Construct DiscoveryServerList argument
#------------------------------------------------------------------------------
def buildBootstrap(argv):
  traceEntry('buildBootstrap')
  
  bootstrapList = ',"DiscoveryServerList":"'
  rc = 0
  for server in argv.b:
    url = OPTIONS['A'+str(server)+'_REST_ARGS']+'/ima/v1/configuration/ServerUID'
    traceRestCall('GET', url, None)
    response = requests.get(url=url)
    if response.status_code != 200:
      traceRestFail('GET', 'Server UID for server A'+str(server), response)
      rc = 1
      return ( rc )

    jsonPayload = json.loads(response.content)
    serverUID = jsonPayload['ServerUID']

    if argv.i == '6':
      bootstrapEntry = '['+OPTIONS['A'+str(server)+'_IPv6_INTERNAL_1']+']:9104'
    else:
      bootstrapEntry = OPTIONS['A'+str(server)+'_IPv4_INTERNAL_1']+':9104'
    bootstrapList = bootstrapList + bootstrapEntry
    if argv.b[len(argv.b)-1] != server:
      bootstrapList = bootstrapList + ','

  bootstrapList = bootstrapList + '"'
  trace(0, bootstrapList)
  traceExit('buildBootstrap')
  return( bootstrapList )

#------------------------------------------------------------------------------
# 
#------------------------------------------------------------------------------
def resetClusterDefaults(argv, serverID, skipExtraProps):
  traceEntry('resetClusterDefaults')
  
  enablecluster = '"EnableClusterMembership":false'
  name = ',"ClusterName":"'+argv.n+'"' if argv.n != None else ',"ClusterName":""'
  
  if skipExtraProps == False:
    controladdress = ',"ControlAddress":"'+argv.ca+'"' if argv.ca != None else ',"ControlAddress":""'
    controlport = ',"ControlPort":'+argv.cp+'' if argv.cp != None else ',"ControlPort":9104'
    messagingaddress = ',"MessagingAddress":"'+argv.ma+'"' if argv.ma != None else ',"MessagingAddress":""'
    messagingport = ',"MessagingPort":'+argv.mp+'' if argv.mp != None else ',"MessagingPort":9105'
    discoveryport = ',"DiscoveryPort":'+argv.dp+'' if argv.dp != None else ',"DiscoveryPort":9106'
    ttl = ',"MulticastDiscoveryTTL":'+argv.ttl+'' if argv.ttl != None else ',"MulticastDiscoveryTTL":1'
    usemc = ',"UseMulticastDiscovery":'+argv.u+'' if argv.u != None else ',"UseMulticastDiscovery":true'
    usetls = ',"MessagingUseTLS":'+argv.tls+'' if argv.tls != None else ',"MessagingUseTLS":false'
    bootstrap = buildBootstrap(argv) if argv.b != None else ',"DiscoveryServerList":""'
    
    payload = '{"ClusterMembership":{'+enablecluster+name+controladdress+controlport+messagingaddress+messagingport+discoveryport+bootstrap+ttl+usemc+usetls+'}}'
  else:
    payload = '{"ClusterMembership":{'+enablecluster+name+'}}'
    
  traceExit('resetClusterDefaults')
  return ( payload )
  
#------------------------------------------------------------------------------
# 
#------------------------------------------------------------------------------
def buildClusterMembershipPayload(argv, serverID, enable, skipExtraProps):
  traceEntry('buildClusterMembershipPayload')

  enablecluster = '"EnableClusterMembership":true' if enable else '"EnableClusterMembership":false'
  name = ',"ClusterName":"'+argv.n+'"' if argv.n != None else ''

  if argv.gvt == 1:
    name=',"ClusterName":"𠀀𠀁𠀂𠀃𠀄𪛔𪛕𪛖𠀋𪆐𪚲ÇàâｱｲｳДфэअइउC̀̂รकÅéﾊハ_'+argv.n+'_Mए￦鷗㐀葛渚噓か啊€㐁ᠠꀀༀU䨭𪘀抎駡郂"' if argv.n != None else ''

  if skipExtraProps == False:
    controladdress = ',"ControlAddress":"'+argv.ca+'"' if argv.ca != None else ''
    controlport = ',"ControlPort":'+argv.cp+'' if argv.cp != None else ''
    messagingaddress = ',"MessagingAddress":"'+argv.ma+'"' if argv.ma != None else ''
    messagingport = ',"MessagingPort":'+argv.mp+'' if argv.mp != None else ''
    discoveryport = ',"DiscoveryPort":'+argv.dp+'' if argv.dp != None else ''
    ttl = ',"MulticastDiscoveryTTL":'+argv.ttl+'' if argv.ttl != None else ''
    usemc = ',"UseMulticastDiscovery":'+argv.u+'' if argv.u != None else ''
    usetls = ',"MessagingUseTLS":'+argv.tls+'' if argv.tls != None else ''
    bootstrap = buildBootstrap(argv) if argv.b != None else ''
    if bootstrap == 1:
      return ( bootstrap )

    payload = '{"ClusterMembership":{'+enablecluster+name+controladdress+controlport+messagingaddress+messagingport+discoveryport+bootstrap+ttl+usemc+usetls+'}}'
  else:
    payload = '{"ClusterMembership":{'+enablecluster+name+'}}'
    payload = payload.encode('utf-8')

  traceExit('buildClusterMembershipPayload')
  return ( payload )

#------------------------------------------------------------------------------
# Remove Cluster Member
# Removes a server from a cluster
#------------------------------------------------------------------------------
def removeClusterMember(argv):
  traceEntry('removeClusterMember')

  if argv.m == '' or argv.m == None:
    trace(0, 'removeClusterMember requires -m argument' )
    sys.exit( 1 )

  rc = 0
  try:
    #updateArgs = buildClusterMembershipPayload(argv=argv, serverID=argv.m, enable=False, skipExtraProps=False)
    updateArgs = resetClusterDefaults(argv=argv, serverID=argv.m, skipExtraProps=False)
    rc = updateClusterMembership(argv.m, updateArgs)
    if rc == 1:
      return ( rc )

    rc = checkClusterStatus(argv.m, CLUSTER_STATUS['REMOVED'])
    if rc != 0:
      # Allow teardown to work on a server that wasn't clustered to begin with
      rc = checkClusterStatus(server, CLUSTER_STATUS['DISABLED'])
      if rc != 0:
        return ( rc )

    #rc = restartClusterMember(argv)
    #if rc != 0:
    #  trace(0, 'FAILED POST RestartClusterMember for server A'+argv.m)
    #  return ( rc )
    if argv.dkil == 1:
      serverControlArgs = ['../scripts/serverControl.sh', '-a', 'killServer', '-i', argv.m]
      trace(0, 'Killing server A' + argv.m + ' during remove process')
      result = runCommand(serverControlArgs, '')
      if result.rc != 0:
        trace(0, 'FAILED serverControl.sh killServer A'+argv.m)
        rc = 1
        return ( rc )
      return (rc)

    time.sleep(10)
    
    rc = verifyStatus(STATUS['STATUS_RUNNING'], 45, argv.m, False, False)
    trace(1, 'status check: ' + str(rc) )
    if rc != 0:
      return ( rc )

    rc = checkClusterStatus(argv.m, CLUSTER_STATUS['DISABLED'])
  except:
    raise

  traceExit('removeClusterMember')
  return ( rc )

#------------------------------------------------------------------------------
# Add Cluster Member
# Configures a server to be a cluster member
#------------------------------------------------------------------------------
def addClusterMember(argv):
  traceEntry('addClusterMember')

  if argv.n == '' or argv.n == None or argv.m == '' or argv.m == None:
    trace(0, 'addClusterMember requires -n and -m arguments' )
    sys.exit( 1 )

  rc = 0
  try:
    #serverUIDBefore = getServerUID(argv.m)
    updateArgs = buildClusterMembershipPayload(argv=argv, serverID=argv.m, enable=True, skipExtraProps=False)
    rc = updateClusterMembership(argv.m, updateArgs)
    if rc == 1:
      return ( rc )

    rc = restartClusterMember(argv)
    if rc != 0:
      trace(0, 'FAILED: POST RestartClusterMember for server A'+argv.m)
      return ( rc )
    time.sleep(10)
    
    rc = verifyStatus(STATUS['STATUS_RUNNING'], 45, argv.m, False, False)
    trace(1, 'status check: ' + str(rc) )
    if rc != 0:
      return ( rc )

    rc = checkClusterStatus(argv.m, CLUSTER_STATUS['ACTIVE'])
    #serverUIDAfter = getServerUID(argv.m)
    #if compareServerUID(serverUIDBefore, serverUIDAfter, False) == False:
    #  rc = 1
  except:
    raise

  traceExit('addClusterMember')
  return ( rc )

#------------------------------------------------------------------------------
# Teardown Cluster
# Teardown a cluster and clean each servers store
#------------------------------------------------------------------------------
def tearDown(argv):
  traceEntry('tearDown')

  rc = 0
  for server in argv.l:
    try:
      updateArgs = resetClusterDefaults(argv=argv, serverID=str(server), skipExtraProps=True)
      #updateArgs = buildClusterMembershipPayload(argv=argv, serverID=str(server), enable=False, skipExtraProps=True)
      rc = updateClusterMembership(str(server), updateArgs)
      if rc == 1:
        break
     
      if argv.skv == 1:
        trace(0, "Skipping verify status..")
        continue

      rc = checkClusterStatus(server, CLUSTER_STATUS['REMOVED'])
      if rc != 0:
        # Allow teardown to work on a server that wasn't clustered to begin with
        rc = checkClusterStatus(server, CLUSTER_STATUS['DISABLED'])
        if rc != 0:
          return ( rc )

      #restartArgs = argparse.Namespace(m=str(server), parent=None)
      #rc = restartClusterMember(restartArgs)
      #if rc != 0:
      #  trace(0, 'FAILED: POST RestartClusterMember for server A'+str(server))
      #  break
      time.sleep(10)

      rc = verifyStatus(STATUS['STATUS_RUNNING'], 45, str(server), False, False)
      trace(1, 'status check: ' + str(rc) )
      if rc != 0:
        return ( rc )

      rc = checkClusterStatus(server, CLUSTER_STATUS['DISABLED'])

    except:
      raise

  traceExit('tearDown')
  return ( rc )

#------------------------------------------------------------------------------
# Teardown Clustered HA pair
# Teardown a clustered HA pair
#------------------------------------------------------------------------------
def tearDownHA(argv):
  traceEntry('tearDownHA')

  if len(argv.l) != 2 or argv.l == None:
    trace(0, 'tearDownHA requires the machine #s of the HA pair to be specified' )
    sys.exit( 1 )

  rc = 0

  #determine primary
  num_primary = 1
  num_standby = 2
  isM1Running = 1
  isM2Running = 1
  try:
    responseA = requests.get(url=OPTIONS['A'+str(argv.l[0])+'_REST_ARGS']+'/ima/v1/service/status')
  except:
    trace(0, 'A'+str(argv.l[0])+' is not running')
    isM1Running = 0

  try:
    responseB = requests.get(url=OPTIONS['A'+str(argv.l[1])+'_REST_ARGS']+'/ima/v1/service/status')
  except:
    trace(0, 'A'+str(argv.l[1])+' is not running')
    isM2Running = 0

  if isM1Running == 0 and isM2Running == 0:
    num_primary = argv.l[0]
    num_standby = argv.l[1]
  elif isM1Running == 0:
    num_primary = argv.l[1]
    num_standby = argv.l[0]
  elif isM2Running == 0:
    num_primary = argv.l[0]
    num_standby = argv.l[1]

  trace(0, responseA.content)
  trace(0, responseB.content)

  jsonPayloadA = json.loads(responseA.content)
  aStatus = jsonPayloadA['Server']['StateDescription']
  jsonPayloadB = json.loads(responseB.content)
  bStatus = jsonPayloadB['Server']['StateDescription']

  if STATUS['STATUS_RUNNING'] in aStatus:
    num_primary = argv.l[0]
    num_standby = argv.l[1]
  elif STATUS['STATUS_RUNNING'] in bStatus:
    num_primary = argv.l[1]
    num_standby = argv.l[0]
  else:
    num_primary = argv.l[0]
    num_standby = argv.l[1]


  #teardown cluster on primary
  try:
    updateArgs = resetClusterDefaults(argv=argv, serverID=str(num_primary), skipExtraProps=True)
    #updateArgs = buildClusterMembershipPayload(argv=argv, serverID=str(server), enable=False, skipExtraProps=True)
    rc = updateClusterMembership(str(num_primary), updateArgs)
    if rc == 1:
      return ( rc )
   
    rc = checkClusterStatus(num_primary, CLUSTER_STATUS['REMOVED'])
    if rc != 0:
      # Allow teardown to work on a server that wasn't clustered to begin with
      rc = checkClusterStatus(num_primary, CLUSTER_STATUS['DISABLED'])
      if rc != 0:
        return ( rc )

    #restartArgs = argparse.Namespace(m=str(server), parent=None)
    #rc = restartClusterMember(restartArgs)
    #if rc != 0:
    #  trace(0, 'FAILED: POST RestartClusterMember for server A'+str(server))
    #  break
    time.sleep(10)

    rc = verifyStatus(STATUS['STATUS_RUNNING'], 45, str(num_primary), False, False)
    trace(1, 'status check: ' + str(rc) )
    if rc != 0:
      return ( rc )

    rc = checkClusterStatus(num_primary, CLUSTER_STATUS['DISABLED'])

  except:
    raise

  traceExit('tearDownHA')
  return ( rc )

#------------------------------------------------------------------------------
# Create Cluster
# Creates a cluster using the machines listed in the -l argument
#------------------------------------------------------------------------------
def createCluster(argv):
  traceEntry('createCluster')

  if argv.n == '' or argv.n == None or argv.l == None:
    trace(0, 'createCluster action requires -n and -l arguments' )
    sys.exit( 1 )

  rc = 0
  for server in argv.l:
    try:
      #serverUIDBefore = getServerUID(str(server))
      updateArgs = buildClusterMembershipPayload(argv=argv, serverID=str(server), enable=True, skipExtraProps=True)
      rc = updateClusterMembership(str(server), updateArgs)
      if rc == 1:
        break

      restartArgs = argparse.Namespace(m=str(server), parent=None)
      rc = restartClusterMember(restartArgs)
      if rc != 0:
        trace(0, 'FAILED: POST RestartClusterMember for server A'+str(server))
        break
      time.sleep(10)
      
      if argv.skv == 1:
        trace(0,'skipping verify status')
        continue

      rc = verifyStatus(STATUS['STATUS_RUNNING'], 45, str(server), False, False)
      trace(1, 'status check: ' + str(rc) )
      if rc != 0:
        return ( rc )

      rc = checkClusterStatus(server, CLUSTER_STATUS['ACTIVE'])
      #serverUIDAfter = getServerUID(str(server))
      #if compareServerUID(serverUIDBefore, serverUIDAfter, False) == False:
      #  rc = 1
    except:
      traceback.print_tb(sys.exc_info()[2])
      raise

  traceExit('createCluster')
  return ( rc )

#------------------------------------------------------------------------------
# Main Function for cluster tool
#------------------------------------------------------------------------------
def main(argv):
  parser = argparse.ArgumentParser(description='Perform cluster related actions')
  parser.add_argument('-a', metavar='action', required=True, type=str, help='The action to perform, ex: addClusterMember', choices=['createCluster','addClusterMember','removeClusterMember','tearDown','tearDownHA','stopCluster','startCluster','killClusterMember','stopClusterMember','startClusterMember','verifyStatus','restartClusterMember','cleanStore','removeInactiveMember','checkClusterStatus'])
  parser.add_argument('-n', metavar='name', required=False, type=str, help='The name of the cluster')
  parser.add_argument('-m', metavar='machine', required=False, type=str, help='The machine number')
  parser.add_argument('-f', metavar='log', required=False, type=str, help='The name of the log file')
  parser.add_argument('-s', metavar='status', required=False, type=str, help='The status variable for verifyStatus action. For verifyStatus action only.', choices=STATUS)
  parser.add_argument('-cs', metavar='clusterstatus', required=False, type=str, help='The status variable for checkClusterStatus action. For checkClusterStatus action only.', choices=CLUSTER_STATUS)
  parser.add_argument('-t', metavar='timeout', required=False, type=int, help='The timeout for the verifyStatusAction. For verifyStatus action only.')
  parser.add_argument('-v', metavar='verbose', default=0, required=False, type=int, help='Set the verbosity of the tracing output' )
  parser.add_argument('-l', metavar='machinelist', required=False, type=int, nargs='+', help='The list of machines. For create and teardown cluster actions only.')
  parser.add_argument('-b', metavar='bootstraplist', required=False, type=int, nargs='+', help='The Discovery Server List. For add and remove member actions only')
  parser.add_argument('-ca', metavar='controladdress', required=False, type=str, help='The Cluster Control Address. For add and remove member actions only.')
  parser.add_argument('-cp', metavar='controlport', required=False, type=str, help='The Cluster Control Port. For add and remove member actions only.')
  parser.add_argument('-ma', metavar='messagingaddress', required=False, type=str, help='The Cluster Messaging Address. For add and remove member actions only.')
  parser.add_argument('-mp', metavar='messagingport', required=False, type=str, help='The Cluster Messaging Port. For add and remove member actions only.')
  parser.add_argument('-dp', metavar='discoveryport', required=False, type=str, help='The Cluster Discovery Port. For add and remove member actions only.')
  parser.add_argument('-ttl', metavar='multicastttl', required=False, type=str, help='The Cluster Multicast Discovery TTL. For add and remove member actions only.')
  parser.add_argument('-u', metavar='usemulticast', required=False, type=str, help='The Cluster Use Multicast Discovery Flag. For add and remove member actions only.')
  parser.add_argument('-i', metavar='ip', required=False, type=str, help='Use IPv4 or IPv6 for DiscoveryServerList.', choices=['4','6'])
  parser.add_argument('-tls', metavar='usetls', required=False, type=str, help='The Cluster Use TLS Flag. For add and remove member actions only.')
  parser.add_argument('-gvt', metavar='gvt', default=0, required=False, type=int, help='1 for non-english characters')
  parser.add_argument('-m2r', metavar='machineToRemove', required=False, type=str, help='The machine number of the inactive member to be removed. For removeInactiveMember action only.')
  parser.add_argument('-skv', metavar='skipVerify', default=0, required=False, type=int, help='Skip verify status, for setting up HA cluster - 3-2-16')
  parser.add_argument('-dkil', metavar='dockerkill', default=0, required=False, type=int, help='Set to 1 to kill server during removeClusterMember - for store verify test')
  parser.add_argument('args', help='Remaining arguments', nargs=argparse.REMAINDER)
  args = parser.parse_args()
  print( args )

  global LOG, OLD_STDOUT, DEBUG, OPTIONS

  DEBUG = args.v

  if args.f:
    try:
      OLD_STDOUT = sys.stdout
      LOG = open(args.f, "w", 1)
      sys.stdout = LOG
    except:
      trace(0, 'Failed to open log file: ' + args.f )
      exit(1)

  # Parse ISMsetup.sh / testEnv.sh
  if os.path.isfile(os.path.dirname(os.path.realpath(__file__))+'/../testEnv.sh'):
    OPTIONS = parseConfig(os.path.dirname(os.path.realpath(__file__))+'/../testEnv.sh')
  elif os.path.isfile(os.path.dirname(os.path.realpath(__file__))+'/ISMsetup.sh'):
    OPTIONS = parseConfig(os.path.dirname(os.path.realpath(__file__))+'/ISMsetup.sh')
  else:
    trace(0, 'Could not load testEnv.sh or ISMsetup.sh' )
    exit(1)

  # Set some properties based on environment
  for serverCount in range(1, int(OPTIONS['A_COUNT'])+1):
    hostip = OPTIONS['A'+str(serverCount)+'_HOST']
    OPTIONS['A'+str(serverCount)+'_SSH_ARGS'] = ['ssh', OPTIONS['A'+str(serverCount)+'_USER']+'@'+hostip]
    OPTIONS['A'+str(serverCount)+'_REST_ARGS'] = 'http://'+OPTIONS['A'+str(serverCount)+'_IPv4_1']+':'+OPTIONS['A'+str(serverCount)+'_PORT']
    trace(2, 'A'+str(serverCount)+'_SSH_ARGS: '+OPTIONS['A'+str(serverCount)+'_SSH_ARGS'][0]+' '+OPTIONS['A'+str(serverCount)+'_SSH_ARGS'][1])
    trace(2, 'A'+str(serverCount)+'_REST_ARGS: '+OPTIONS['A'+str(serverCount)+'_REST_ARGS'] )

  if args.ma != None and args.ma == '0':
    args.ma = '';

  # Run the command specified by -a input flag
  actions = {'createCluster': createCluster,
         'tearDown': tearDown,
         'tearDownHA': tearDownHA,
         'addClusterMember': addClusterMember,
         'removeClusterMember': removeClusterMember,
         'stopClusterMember': stopClusterMember,
         'startClusterMember': startClusterMember,
         'restartClusterMember': restartClusterMember,
         'killClusterMember': killClusterMember,
         'verifyStatus': verifyStatusWrapper,
         'cleanStore': cleanStore,
         'removeInactiveMember': removeInactiveMember,
         'checkClusterStatus': checkClusterStatusWrapper}

  rc = 1
  # args.a should always be a valid key since ArgumentParser validates it
  if args.a in actions.keys():
    try:
      trace(0, 'Running action '+args.a )
      rc = actions[args.a](args)
    except:
      trace(0, 'Action '+args.a+' failed!' )
      traceback.print_tb(sys.exc_info()[2])
      trace(0, sys.exc_info() )
      pass
  else:
    trace(0, "Invalid action specified: " + args.a )

  return ( rc )

#------------------------------------------------------------------------------
# Check and invoke main
#------------------------------------------------------------------------------
rc = 1
if __name__ == "__main__":

  rc = main( sys.argv[1:] )

exit(rc)
