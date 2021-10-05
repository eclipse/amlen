#!/usr/bin/python
# -*- coding: utf-8 -*-
# Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

# MessageSight script for performing HA related actions
# Expectations: Actions that act on a single server will
#               specify that server with the -m argument
#               
#               Actions that act on the primary server will
#               specify the primary with the -p argument
#               
#               Actions that act on the standby server will
#               specify the standby with the -s argument
#
#               Actions that act on both the primary and
#               standby will specify them with the -p and
#               -s arguments
#
# Due to clustering, it will not always be just 'server 1'
# and 'server 2'. An HA pair may be 'server 3' and 'server 5'
#
# Actions for a single server:
#  crashServer, stopServer, startServer, cleanStore, verifyStatus
#
# Actions for primary server:
#  setupPrimary, stopPrimary, startPrimary, crashPrimary
#
# Actions for standby server:
#  setupStandby, stopStandby, startStandby, crashStandbt
#
# Actions for primary and standby:
#  setupHA, autoSetup, disableHA, crashBoth, swapPrimary,
#  stopBoth, startBoth
#
# HA Config Properties:
#  EnableHA               True | False, default False
#  StartupMode            AutoDetect | StandAlone, default AutoDetect
#  RemoteDiscoveryNIC     IP Address
#  LocalDiscoveryNIC      IP Address
#  LocalReplicationNIC    IP Address
#  DiscoveryTimeout       10 - 2147483647, default 600
#  HeartbeatTimeout       1 - 2147483647, default 10
#  PreferredPrimary       True | False, default True
#  Group                  max length 128 character string
#
# Example payload:
#  {"HighAvailability":{"EnableHA":true,"Group":"HAGroup1"}}
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

FILENAME = 'haFunctions.py'

# Set imaserver status constants
STATUS = {'STATUS_RUNNING': 'Running (production)',
          'STATUS_MAINTENANCE': 'Running (maintenance)',
          'STATUS_STANDBY': 'Standby',
          'STATUS_STOPPED': 'The server is stopped',
          'STATUS_STARTING': 'server is starting',
          'STATUS_ALREADY_RUNNING': 'already running'
          }

# Set start time for determining timeout
start = time.time()

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
    trace(0, '\n\t\t'+action+' PAYLOAD='+payload+'\n\t\tURL: '+url)
  elif action == 'GET':
    trace(0, '\n\t\t'+action+' URL: '+url)

def traceRestFail(action, description, response):
  trace(0, '\n\t\tFAILED: '+action+' '+description+'\n\t\tRESPONSE.STATUS_CODE: '+str(response.status_code)+'\n\t\tRESPONSE.CONTENT: '+response.content)

#------------------------------------------------------------------------------
# Print the end result, attempt to close log file if it exists, and exit
#------------------------------------------------------------------------------
def exit(rc):
  if rc == 0:
    print( 'haFunctions.py: Test result is Success!' )
  else:
    print( 'haFunctions.py: Test result is Failure!' )
  try:
    LOG.close()
    sys.stdout = OLD_STDOUT
  except:
    pass
  sys.exit( rc )

#------------------------------------------------------------------------------
# Parse properties from ISMsetup or testEnv input file
# Expected format:
#    'export PROPERTY_NAME=PROPERTY_VALUE'
# Skip if line starts with '#'
# or line does not start with 'e'
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
  trace(1, args )

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
# statusLoop
#------------------------------------------------------------------------------
def statusLoop(status, timeout, serverID):
  traceEntry('statusLoop')
  rc = 1

  elapsed = time.time() - start
  output = ''
  trace(1, 'timeout is {0}, {1} seconds remain'.format(timeout, int(timeout - (time.time() - start))))
  while elapsed < timeout:
    time.sleep(1)
    elapsed = time.time() - start
    trace(1, 'timeout is {0}, {1} seconds remain'.format(timeout, int(timeout - (time.time() - start))))
    
    # Make status request
    try:
      response = requests.get(url=OPTIONS['A'+serverID+'_REST_ARGS']+"/ima/v1/service/status")

      # Print status response
      trace(2, response.status_code)
      jsonPayload = json.loads(response.content)
      trace(2, jsonPayload['Server']['StateDescription'])

      # Verify status 
      if status in jsonPayload['Server']['StateDescription']:
        output = status
        rc = 0
        break
    except:
      if status == STATUS['STATUS_STOPPED']:
        output = 'Server is not running'
        rc = 0
        break
      else:
        trace(2, 'statusLoop: got exception')
      #traceback.print_tb(sys.exc_info()[2])
      pass

  resultTuple = collections.namedtuple('Result', ['rc', 'message'])
  result = resultTuple(rc, output)

  trace(1, 'timeout is {0}, {1} seconds remain'.format(timeout, int(timeout - (time.time() - start))))
  traceExit('statusLoop')
  return ( result )

#------------------------------------------------------------------------------
# verifyStatus
#------------------------------------------------------------------------------
def verifyStatus(status, timeout, serverID):
  traceEntry('verifyStatus')

  result = statusLoop(status, timeout, serverID)
  rc = result.rc
  if rc != 0:
    trace(0, 'verifyStatus failed')
  
  traceExit('verifyStatus')
  return ( rc )
  
#------------------------------------------------------------------------------
# status, timeout, id, inverse, ignore
#------------------------------------------------------------------------------
def verifyStatusWrapper(args):
  traceEntry('verifyStatusWrapper')

  rc = verifyStatus(STATUS[args.status], args.t, args.m1)
  if rc != 0:
    trace(0, 'verifyStatusWrapper failed')

  traceExit('verifyStatusWrapper')
  return( rc )

#------------------------------------------------------------------------------
# Clean the store of a server
# parameters:
#   serverID - The A_COUNT index for the server to clean
#------------------------------------------------------------------------------
def cleanStore(argv):
  traceEntry('cleanStore')

  rc = 0
  try:
    payload = '{"Service":"Server","CleanStore":true}' 
    url = OPTIONS['A'+argv.m1+'_REST_ARGS']+'/ima/v1/service/restart'
    traceRestCall('POST', url, payload)
    response = requests.post(url=url, data=payload)
    if response.status_code != 200:
      traceRestFail('POST', 'cleanStore failed on restart server', response)
      rc = 1
      return ( rc )

    rc = verifyStatus(STATUS['STATUS_RUNNING'], argv.t, argv.m1)
    trace(1, 'status check: ' + str(rc) )
    if rc != 0:
      trace(0, 'cleanStore failed on verify status running')
  except:
    raise

  traceExit('cleanStore')
  return ( rc )

#------------------------------------------------------------------------------
# restart the server specified by argv.p
#------------------------------------------------------------------------------
def restartPrimary(argv):
  traceEntry('restartPrimary')

  rc = 0
  try:
    payload = '{"Service":"Server","CleanStore":false}' 
    url = OPTIONS['A'+argv.p+'_REST_ARGS']+'/ima/v1/service/restart'
    traceRestCall('POST', url, payload)
    response = requests.post(url=url, data=payload)
    if response.status_code != 200:
      traceRestFail('POST', 'restart primary server failed', response)
      rc = 1
      return ( rc )

    rc = verifyStatus(STATUS['STATUS_STANDBY'], argv.t, argv.p)
    trace(1, 'status check: ' + str(rc) )
    if rc != 0:
      trace(0, 'retart primary server failed on verify status standby')
  except:
    raise

  traceExit('restartPrimary')
  return ( rc )


#------------------------------------------------------------------------------
# Kill the server specified by argv.m1 using docker stop command
#------------------------------------------------------------------------------
def crashServer(argv):
  traceEntry('killServer')

  if argv.m1 == '' or argv.m1 == None:
    trace(0, 'killServer requires -m argument' )
    sys.exit( 1 )

  rc = 0
  try:
    serverControlArgs = ['../scripts/serverControl.sh', '-a', 'killServer', '-i', argv.m1]
    result = runCommand(serverControlArgs, '')
    rc = result.rc
    if rc != 0:
      trace(0, 'FAILED serverControl.sh crashServer A'+argv.m1)
      return ( rc )

    rc = verifyStatus(STATUS['STATUS_STOPPED'], argv.t, argv.m1)
    trace(1, 'status check: ' + str(rc) )
    if rc != 0:
      trace(0, 'crashServer failed on verify status stopped')
  except:
    raise

  traceExit('killServer')
  return ( rc )

#------------------------------------------------------------------------------
# crash primary server argv.p
#------------------------------------------------------------------------------
def crashPrimary(argv):
  traceEntry('crashPrimary')

  argv.m1 = argv.p
  rc = crashServer(argv)
  if rc != 0:
    trace(0, 'crashPrimary failed')

  traceExit('crashPrimary')
  return ( rc )
  
#------------------------------------------------------------------------------
# crash standby server argv.s
#------------------------------------------------------------------------------
def crashStandby(argv):
  traceEntry('crashStandby')

  argv.m1 = argv.s
  rc = crashServer(argv)
  if rc != 0:
    trace(0, 'crashStandby failed')

  traceExit('crashStandby')
  return ( rc )

#------------------------------------------------------------------------------
# crash both primary and standby
#------------------------------------------------------------------------------
def crashBoth(argv):
  traceEntry('crashBoth')
  rc = 0

  argv.m1 = argv.p
  rc = crashServer(argv)
  if rc != 0:
    trace(0, 'crashBoth failed on crashServer primary')
    return ( rc )

  argv.m1 = argv.s
  rc = crashServer(argv)
  if rc != 0:
    trace(0, 'crashBoth failed on crashServer standby')
    return ( rc )

  traceExit('crashBoth')
  return ( rc )
  
#------------------------------------------------------------------------------
# setupHA: set argv.p as primary and argv.s as standby
#------------------------------------------------------------------------------
def setupHA(argv):
  traceEntry('setupHA')
  rc = 0

  # Standby HA configuration payload
  localDiscoveryNICStandby = ',"LocalDiscoveryNIC":"'+OPTIONS['A'+argv.s+'_IPv4_INTERNAL_1']+'"'
  localReplicationNICStandby = ',"LocalReplicationNIC":"'+OPTIONS['A'+argv.s+'_IPv4_INTERNAL_1']+'"'
  remoteDiscoveryNICStandby = ',"RemoteDiscoveryNIC":"'+OPTIONS['A'+argv.p+'_IPv4_INTERNAL_1']+'"'
  payload_str = '{"HighAvailability":{"EnableHA":true,"Group":"'+argv.g+'"'+localDiscoveryNICStandby+localReplicationNICStandby+remoteDiscoveryNICStandby+', "PreferredPrimary": false}}';

  if argv.gvt == 1:
    payload_str = '{"HighAvailability":{"EnableHA":true,"Group":"'+'𠀀𠀁𠀂𠀃𠀄𪛔𪛕𪛖𠀋𪆐𪚲ÇàâｱｲｳДфэअइउC̀̂รकÅéﾊハMए￦鷗㐀葛渚噓か啊€㐁ᠠꀀༀU䨭𪘀抎駡郂_'+argv.g+'_葛渚噓'+'"'+localDiscoveryNICStandby+localReplicationNICStandby+remoteDiscoveryNICStandby+', "PreferredPrimary": false}}';
 
  payload = json.loads(payload_str)

  # Send request to standby
  urlStandby = OPTIONS['A'+argv.s+'_REST_ARGS']+"/ima/v1/configuration"
  traceRestCall('POST', urlStandby, payload_str)
  response = requests.post(url=urlStandby, data=json.dumps(payload))

  # Standby response
  trace(0, response.status_code)
  if response.status_code != 200:
    rc = 1
    traceRestFail('POST', 'setupHA failed post HA update to standby', response)
    return ( rc )
  else:
    jsonPayload = json.loads(response.content)
    trace(0, json.dumps(jsonPayload, sort_keys=True, indent=4, separators=(',', ':')))

  # Primary HA configuration payload
  localDiscoveryNICPrimary = ',"LocalDiscoveryNIC":"'+OPTIONS['A'+argv.p+'_IPv4_INTERNAL_1']+'"'
  localReplicationNICPrimary = ',"LocalReplicationNIC":"'+OPTIONS['A'+argv.p+'_IPv4_INTERNAL_1']+'"'
  remoteDiscoveryNICPrimary = ',"RemoteDiscoveryNIC":"'+OPTIONS['A'+argv.s+'_IPv4_INTERNAL_1']+'"'
  payload_str = '{"HighAvailability":{"EnableHA":true,"Group":"'+argv.g+'"'+localDiscoveryNICPrimary+localReplicationNICPrimary+remoteDiscoveryNICPrimary+',"PreferredPrimary": true}}';
  
  if argv.gvt == 1:
    payload_str = '{"HighAvailability":{"EnableHA":true,"Group":"'+'𠀀𠀁𠀂𠀃𠀄𪛔𪛕𪛖𠀋𪆐𪚲ÇàâｱｲｳДфэअइउC̀̂รकÅéﾊハMए￦鷗㐀葛渚噓か啊€㐁ᠠꀀༀU䨭𪘀抎駡郂_'+argv.g+'_葛渚噓'+'"'+localDiscoveryNICPrimary+localReplicationNICPrimary+remoteDiscoveryNICPrimary+', "PreferredPrimary": true}}';
 
  payload = json.loads(payload_str)

  # Send request to primary
  urlPrimary = OPTIONS['A'+argv.p+'_REST_ARGS']+"/ima/v1/configuration"
  traceRestCall('POST', urlPrimary, payload_str)
  response = requests.post(url=urlPrimary, data=json.dumps(payload))

  # Primary response
  trace(0, response.status_code)
  if response.status_code != 200:
    rc = 1
    traceRestFail('POST', 'setupHA failed on post HA update to primary', response)
    return ( rc )
  else:
    jsonPayload = json.loads(response.content)
    trace(0, json.dumps(jsonPayload, sort_keys=True, indent=4, separators=(',', ':')))

  # When adding a standby to a server already in a cluster, skip the restarts for now
  # Will modify this later -Denny 3-1-16
  if argv.skrs == 1:
    trace(0, 'skipping restart of servers..')
    return (rc)

  # Primary restart
  payload_str = '{"Service":"Server"}'
  payload = json.loads(payload_str)
  urlPrimary = OPTIONS['A'+argv.p+'_REST_ARGS']+"/ima/v1/service/restart"
  traceRestCall('POST', urlPrimary, payload_str)
  response = requests.post(url=urlPrimary, data=json.dumps(payload))
  if response.status_code != 200:
    rc = 1
    traceRestFail('POST', 'setupHA failed restart of primary', response)
    return ( rc )

  # Standby restart and clean store
  payload_str = '{"Service":"Server","CleanStore":true}'
  payload = json.loads(payload_str)
  urlStandby = OPTIONS['A'+argv.s+'_REST_ARGS']+"/ima/v1/service/restart"
  traceRestCall('POST', urlStandby, payload_str)
  response = requests.post(url=urlStandby, data=json.dumps(payload))
  if response.status_code != 200:
    rc = 1
    traceRestFail('POST', 'setupHA failed restart of standby', response)
    return ( rc )

  time.sleep(5)
  
  # Verify status primary
  rc = verifyStatus(STATUS['STATUS_RUNNING'], argv.t, argv.p)
  if rc != 0:
    trace(0, 'setupHA failed on verify status of primary')
    return ( rc )

  # Verify status standby
  rc = verifyStatus(STATUS['STATUS_STANDBY'], argv.t, argv.s)
  if rc != 0:
    trace(0, 'setupHA failed on verify status of standby')
    return ( rc )

  traceExit('setupHA')
  return ( rc )

#------------------------------------------------------------------------------
# set argv.p as primary
#------------------------------------------------------------------------------
def setupPrimary(argv):
  traceEntry('setupPrimary')
  rc = 0

  # Primary HA configuration payload
  localDiscoveryNICPrimary = ',"LocalDiscoveryNIC":"'+OPTIONS['A'+argv.p+'_IPv4_INTERNAL_1']+'"'
  localReplicationNICPrimary = ',"LocalReplicationNIC":"'+OPTIONS['A'+argv.p+'_IPv4_INTERNAL_1']+'"'
  remoteDiscoveryNICPrimary = ',"RemoteDiscoveryNIC":"'+OPTIONS['A'+argv.s+'_IPv4_INTERNAL_1']+'"'
  payload_str = '{"HighAvailability":{"EnableHA":true,"Group":"'+argv.g+'"'+localDiscoveryNICPrimary+localReplicationNICPrimary+remoteDiscoveryNICPrimary+'}}';
  payload = json.loads(payload_str)

  # Send request to primary
  url = OPTIONS['A'+argv.p+'_REST_ARGS']+"/ima/v1/configuration"
  traceRestCall('POST', url, payload_str)
  response = requests.post(url=url, data=json.dumps(payload))

  # Primary response
  trace(0, response.status_code)
  if response.status_code != 200:
    rc = 1
    traceRestFail('POST', 'setupPrimary failed on post HA update to primary', response)
    return ( rc )
  else:
    jsonPayload = json.loads(response.content)
    trace(0, json.dumps(jsonPayload, sort_keys=True, indent=4, separators=(',', ':')))

  # Primary restart
  payload_str = '{"Service":"Server"}'
  payload = json.loads(payload_str)
  url = OPTIONS['A'+argv.p+'_REST_ARGS']+"/ima/v1/service/restart"
  traceRestCall('POST', url, payload_str)
  response = requests.post(url=url, data=json.dumps(payload))
  if response.status_code != 200:
    rc = 1
    traceRestFail('POST', 'setupPrimary failed restart of primary', response)
    return ( rc )

  # Verify primary status
  rc = verifyStatus(STATUS['STATUS_RUNNING'], argv.t, argv.p)
  if rc != 0:
    trace(0, 'setupPrimary failed verify status of primary')
    return ( rc )

  traceExit('setupPrimary')
  return ( rc )

#------------------------------------------------------------------------------
# set argv.s as standby
#------------------------------------------------------------------------------
def setupStandby(argv):
  traceEntry('setupStandby')
  rc = 0

  # Standby HA configuration payload
  localDiscoveryNICStandby = ',"LocalDiscoveryNIC":"'+OPTIONS['A'+argv.s+'_IPv4_INTERNAL_1']+'"'
  localReplicationNICStandby = ',"LocalReplicationNIC":"'+OPTIONS['A'+argv.s+'_IPv4_INTERNAL_1']+'"'
  remoteDiscoveryNICStandby = ',"RemoteDiscoveryNIC":"'+OPTIONS['A'+argv.p+'_IPv4_INTERNAL_1']+'"'
  payload_str = '{"HighAvailability":{"EnableHA":true,"Group":"'+argv.g+'"'+localDiscoveryNICStandby+localReplicationNICStandby+remoteDiscoveryNICStandby+'}}';
  payload = json.loads(payload_str)

  # Send request to standby
  url = OPTIONS['A'+argv.s+'_REST_ARGS']+"/ima/v1/configuration"
  traceRestCall('POST', url, payload_str)
  response = requests.post(url=url, data=json.dumps(payload))

  # Standby response
  trace(0, response.status_code)
  if response.status_code != 200:
    rc = 1
    traceRestFail('POST', 'setupStandby failed restart of standby', response)
    return ( rc )
  else:
    jsonPayload = json.loads(response.content)
    trace(0, json.dumps(jsonPayload, sort_keys=True, indent=4, separators=(',', ':')))

  # Standby restart
  payload_str = '{"Service":"Server"}'
  payload = json.loads(payload_str)

  url = OPTIONS['A'+argv.s+'_REST_ARGS']+"/ima/v1/service/restart"
  traceRestCall('POST', url, payload_str)
  response = requests.post(url=url, data=json.dumps(payload))
  if response.status_code != 200:
    rc = 1
    traceRestFail('POST', 'setupStandby failed restart of standby', response)
    return ( rc )

  # Verify standby status
  rc = verifyStatus(STATUS['STATUS_STANDBY'], argv.t, argv.s)
  if rc != 0:
    trace(0, 'setupStandby failed verify status of standby')
    return ( rc )

  traceExit('setupStandby')
  return ( rc )

#------------------------------------------------------------------------------
# disable HA
#------------------------------------------------------------------------------
def disableHA(argv):
  traceEntry('disableHA')
  rc = 0

  # no-op if already running. just make sure servers are up so we can disable HA.
  startBoth(argv)
  time.sleep(10)
  
  # Determine if HA is currently disabled.. 
  rc = verifyStatus(STATUS['STATUS_STANDBY'], 60, argv.s)
  if rc != 0:
    statusafterdisable = STATUS['STATUS_RUNNING']
  else:
    statusafterdisable = STATUS['STATUS_MAINTENANCE']
  
  # Payload for disable HA update
  payload_str = '{"HighAvailability":{"EnableHA":false}}';
  payload = json.loads(payload_str)

  # Send request to standby
  url = OPTIONS['A'+argv.s+'_REST_ARGS']+"/ima/v1/configuration"
  traceRestCall('POST', url, payload_str)
  response = requests.post(url=url, data=json.dumps(payload))

  # View standby response
  trace(0, response.status_code)
  if response.status_code != 200:
    rc = 1
    traceRestFail('POST', 'disableHA failed on post HA update to standby', response)
    return ( rc )
  else:
    jsonPayload = json.loads(response.content)
    trace(0, json.dumps(jsonPayload, sort_keys=True, indent=4, separators=(',', ':')))

  # Send request to primary
  url = OPTIONS['A'+argv.p+'_REST_ARGS']+"/ima/v1/configuration"
  traceRestCall('POST', url, payload_str)
  response = requests.post(url=url, data=json.dumps(payload))

  # View primary response
  trace(0, response.status_code)
  if response.status_code != 200:
    rc = 1
    traceRestFail('POST', 'disableHA failed on post HA update to primary', response)
    return ( rc )
  else:
    jsonPayload = json.loads(response.content)
    trace(0, json.dumps(jsonPayload, sort_keys=True, indent=4, separators=(',', ':')))

  #Restart standby leaving store intact (into maintenance mode)
  if argv.rs == 1:
    payload_str = '{"Service":"Server"}'
    payload = json.loads(payload_str)

    url = OPTIONS['A'+argv.s+'_REST_ARGS']+"/ima/v1/service/restart"
    traceRestCall('POST', url, payload_str)
    response = requests.post(url=url, data=json.dumps(payload))
    if response.status_code != 200:
      rc = 1
      traceRestFail('POST', 'disableHA failed on restart of standby', response)
      return ( rc )
  else:
    # To bring to production mode, restart standby (cleanstore true) then restart standby(maintenance stop)
    # Standby restart
    payload_str = '{"Service":"Server","CleanStore":true}'
    payload = json.loads(payload_str)

    url = OPTIONS['A'+argv.s+'_REST_ARGS']+"/ima/v1/service/restart"
    traceRestCall('POST', url, payload_str)
    response = requests.post(url=url, data=json.dumps(payload))
    if response.status_code != 200:
      rc = 1
      traceRestFail('POST', 'disableHA failed on restart of standby', response)
      return ( rc )

    time.sleep(5)  
    # Verify status standby in maintenance mode
    rc = verifyStatus(statusafterdisable, argv.t, argv.s)
    if rc != 0:
      trace(0, 'disableHA failed verify status of standby (expected STATUS_MAINTENANCE)')
      return ( rc )

    if statusafterdisable == STATUS['STATUS_MAINTENANCE']:
      payload_str = '{"Service":"Server","Maintenance":"stop"}'
      payload = json.loads(payload_str)

      url = OPTIONS['A'+argv.s+'_REST_ARGS']+"/ima/v1/service/restart"
      traceRestCall('POST', url, payload_str)
      response = requests.post(url=url, data=json.dumps(payload))
      if response.status_code != 200:
        rc = 1
        traceRestFail('POST', 'disableHA failed on restart of standby', response)
        return ( rc )
    
  # Primary restart
  payload_str = '{"Service":"Server"}'
  payload = json.loads(payload_str)
  
  url = OPTIONS['A'+argv.p+'_REST_ARGS']+"/ima/v1/service/restart"
  traceRestCall('POST', url, payload_str)
  response = requests.post(url=url, data=json.dumps(payload))
  if response.status_code != 200:
    rc = 1
    traceRestFail('POST', 'disableHA failed on restart of primary', response)
    return ( rc )
    
  time.sleep(5)

  # Verify status primary
  rc = verifyStatus(STATUS['STATUS_RUNNING'], argv.t, argv.p)
  if rc != 0:
    trace(0, 'disableHA failed verify status of primary')
    return ( rc )

  # Verify status standby
  
  if argv.rs == 1:
    rc = verifyStatus(STATUS['STATUS_MAINTENANCE'], argv.t, argv.s)
    if rc != 0:
      trace(0, 'disableHA failed verify status of standby (expected STATUS_MAINTENANCE)')
      return ( rc )
  else:
    rc = verifyStatus(STATUS['STATUS_RUNNING'], argv.t, argv.s)
    if rc != 0:
      trace(0, 'disableHA failed verify status of standby')
      return ( rc )

  traceExit('disableHA')
  return ( rc )

#------------------------------------------------------------------------------
# run HA auto setup (specify no IP address configuration)
#------------------------------------------------------------------------------
def autoSetup(argv):
  traceEntry('autoSetup')
  rc = 0

  # Primary update HighAvailability payload
  localDiscoveryNICPrimary = ',"LocalDiscoveryNIC":"'+OPTIONS['A'+argv.p+'_IPv4_INTERNAL_1']+'"'
  localReplicationNICPrimary = ',"LocalReplicationNIC":"'+OPTIONS['A'+argv.p+'_IPv4_INTERNAL_1']+'"'
  remoteDiscoveryNICPrimary = ',"RemoteDiscoveryNIC":"'+OPTIONS['A'+argv.s+'_IPv4_INTERNAL_1']+'"'
  payload_str = '{"HighAvailability":{"EnableHA":true,"Group":"'+argv.g+'"'+localDiscoveryNICPrimary+localReplicationNICPrimary+remoteDiscoveryNICPrimary+'}}';
  payload = json.loads(payload_str)

  # Send request to primary
  url = OPTIONS['A'+argv.p+'_REST_ARGS']+"/ima/v1/configuration"
  traceRestCall('POST', url, payload_str)
  response = requests.post(url=url, data=json.dumps(payload))

  # View response from primary
  if response.status_code != 200:
    rc = 1
    traceRestFail('POST', 'autoSetup failed on post HA update to primary', response)
    return ( rc )
  else:
    jsonPayload = json.loads(response.content)
    trace(0, json.dumps(jsonPayload, sort_keys=True, indent=4, separators=(',', ':')))

  # Standby update HighAvailability payload
  localDiscoveryNICStandby = ',"LocalDiscoveryNIC":"'+OPTIONS['A'+argv.s+'_IPv4_INTERNAL_1']+'"'
  localReplicationNICStandby = ',"LocalReplicationNIC":"'+OPTIONS['A'+argv.s+'_IPv4_INTERNAL_1']+'"'
  remoteDiscoveryNICStandby = ',"RemoteDiscoveryNIC":"'+OPTIONS['A'+argv.p+'_IPv4_INTERNAL_1']+'"'
  payload_str = '{"HighAvailability":{"EnableHA":true,"Group":"'+argv.g+'"'+localDiscoveryNICStandby+localReplicationNICStandby+remoteDiscoveryNICStandby+'}}';
  payload = json.loads(payload_str)

  # Send request to standby
  url = OPTIONS['A'+argv.s+'_REST_ARGS']+"/ima/v1/configuration"
  traceRestCall('POST', url, payload_str)
  response = requests.post(url=url, data=json.dumps(payload))

  # View response from standby
  if response.status_code != 200:
    rc = 1
    traceRestFail('POST', 'autoSetup failed on post HA update to standby', response)
    return ( rc )
  else:
    jsonPayload = json.loads(response.content)
    trace(0, json.dumps(jsonPayload, sort_keys=True, indent=4, separators=(',', ':')))

  # Primary restart
  payload_str = '{"Service":"Server"}'
  payload = json.loads(payload_str)

  url = OPTIONS['A'+argv.p+'_REST_ARGS']+"/ima/v1/service/restart"
  traceRestCall('POST', url, payload_str)
  response = requests.post(url=url, data=json.dumps(payload))
  if response.status_code != 200:
    rc = 1
    traceRestFail('POST', 'autoSetup failed on restart of primary', response)
    return ( rc )
  
  # Standby restart and clean store
  payload_str = '{"Service":"Server","CleanStore":true}'
  payload = json.loads(payload_str)

  url = OPTIONS['A'+argv.s+'_REST_ARGS']+"/ima/v1/service/restart"
  traceRestCall('POST', url, payload_str)
  response = requests.post(url=url, data=json.dumps(payload))
  if response.status_code != 200:
    rc = 1
    traceRestFail('POST', 'autoSetup failed on restart and cleanstore of standby', response)
    return ( rc )
  
  # verify status of both
  rc = verifyStatus(STATUS['STATUS_RUNNING'], argv.t, argv.p)
  if rc != 0:
    trace(0, 'autoSetup failed on verify status of primary')
    return ( rc )

  rc = verifyStatus(STATUS['STATUS_STANDBY'], argv.t, argv.s)
  if rc != 0:
    trace(0, 'autoSetup failed on verify status of standby')
    return ( rc )

  traceExit('autoSetup')
  return ( rc )

#------------------------------------------------------------------------------
# Swap the primary and standby servers
#------------------------------------------------------------------------------
def swapPrimary(argv):
  traceEntry('swapPrimary')
  rc = 0


  rc = stopPrimary(argv)
  if rc != 0:
    trace(0, 'swapPrimary failed on stop primary')
    return ( rc )

  rc = verifyStatus(STATUS['STATUS_STOPPED'], argv.t, argv.p)
  if rc != 0:
    trace(0, 'swapPrimary failed on verify primary stopped')
    return ( rc )

  rc = startPrimary(argv)
  if rc != 0:
    trace(0, 'swapPrimary failed on start primary into standby')
    return ( rc )

  rc = verifyStatus(STATUS['STATUS_STANDBY'], argv.t, argv.p)
  if rc != 0:
    trace(0, 'swapPrimary failed on verify primary new standby status')
    return ( rc )

  rc = verifyStatus(STATUS['STATUS_RUNNING'], argv.t, argv.s)
  if rc != 0:
    trace(0, 'swapPrimary failed on verify standby new primary status')

  traceExit('swapPrimary')
  return ( rc )

#------------------------------------------------------------------------------
# stop the server specified by argv.m1 using the rest api
#------------------------------------------------------------------------------
def stopServer(argv):
  traceEntry('stopServer')
  rc = 0

  # Primary stop payload
  payload_str = '{"Service":"Server"}';
  payload = json.loads(payload_str)

  # Primary request
  try:
    url = OPTIONS['A'+argv.m1+'_REST_ARGS']+"/ima/v1/service/stop"
    traceRestCall('POST', url, payload_str)
    response = requests.post(url=url, data=json.dumps(payload))

    # Primary response
    if response.status_code != 200:
      rc = 1
      traceRestFail('POST', 'stopServer failed on request', response)
      return ( rc )
  except:
    trace(0, 'Exception when stopping server')
    traceback.print_tb(sys.exc_info()[2])
    pass

  rc = verifyStatus(STATUS['STATUS_STOPPED'], argv.t, argv.m1)
  if rc != 0:
    trace(0, 'stopServer failed on verify status')

  time.sleep(5)
  traceExit('stopServer')
  return ( rc )

#------------------------------------------------------------------------------
# stop the server specified by argv.p
#------------------------------------------------------------------------------
def stopPrimary(argv):
  traceEntry('stopPrimary')
  rc = 0

  argv.m1 = argv.p
  rc = stopServer(argv)
  if rc != 0:
    trace(0, 'stopPrimary failed')

  traceExit('stopPrimary')
  return ( rc )

#------------------------------------------------------------------------------
# stop the server specified by argv.s
#------------------------------------------------------------------------------
def stopStandby(argv):
  traceEntry('stopStandby')
  rc = 0

  argv.m1 = argv.s
  rc = stopServer(argv)
  if rc != 0:
    trace(0, 'stopStandby failed')

  traceExit('stopStandby')
  return ( rc )

#------------------------------------------------------------------------------
# stop the standby then the primary
#------------------------------------------------------------------------------
def stopBoth(argv):
  traceEntry('stopBoth')
  rc = 0

  rc = stopStandby(argv)
  if rc != 0:
    trace(0, 'stopStandby failed on stopStandby')
    return ( rc )

  rc = stopPrimary(argv)
  if rc != 0:
    trace(0, 'stopPrimary failed on stopPrimary')

  traceExit('stopBoth')
  return ( rc )

#------------------------------------------------------------------------------
# start the server specified by argv.m1
#------------------------------------------------------------------------------
def startServer(argv):
  traceEntry('startServer')
  rc = 0

  serverControlArgs = ['../scripts/serverControl.sh', '-a', 'startServer', '-i', argv.m1]
  result = runCommand(serverControlArgs, '')
  rc = result.rc
  if rc != 0:
    trace(0, 'FAILED serverControl.sh startServer A'+argv.m1)

  time.sleep(5)
  # TODO: verify server is running status.. or maintenance?
  #rc = verifyStatus(STATUS['STATUS_RUNNING'], argv.t, argv.m1)
  #if rc != 0:
  #  trace(0, 'startServer failed on verify status')

  traceExit('startServer')
  return ( rc )

#------------------------------------------------------------------------------
# start the primary server argv.p
#------------------------------------------------------------------------------
def startPrimary(argv):
  traceEntry('startPrimary')
  rc = 0

  argv.m1 = argv.p
  rc = startServer(argv)
  if rc != 0:
    trace(0, 'startPrimary failed')

  traceExit('startPrimary')
  return ( rc )

#------------------------------------------------------------------------------
# start the standby server argv.s
#------------------------------------------------------------------------------
def startStandby(argv):
  traceEntry('startStandby')
  rc = 0

  argv.m1 = argv.s
  rc = startServer(argv)
  if rc != 0:
    trace(0, 'startStandby failed')

  traceExit('startStandby')
  return ( rc )

#------------------------------------------------------------------------------
# start the primary then the standby server
#------------------------------------------------------------------------------
def startBoth(argv):
  traceEntry('startBoth')
  rc = 0

  rc = startPrimary(argv)
  if rc != 0:
    trace(0, 'startPrimary failed')
    return ( rc )

  rc = startStandby(argv)
  if rc != 0:
    trace(0, 'startStandby failed')

  #rc = verifyStatus(STATUS['STATUS_RUNNING'], argv.t, argv.p)
  #if rc != 0:
  #  trace(0, 'startServer failed on verify status')

  #rc = verifyStatus(STATUS['STATUS_STANDBY'], argv.t, argv.s)
  #if rc != 0:
  #  trace(0, 'startServer failed on verify status')

  traceExit('startBoth')
  return ( rc )

#------------------------------------------------------------------------------
# start both servers and choose 1 to be primary
#------------------------------------------------------------------------------
def startAndPickPrimary(argv):
  traceEntry('startAndPickPrimary')
  rc = 0
  
  startBoth(argv)
  
  rcPrunning = verifyStatus(STATUS['STATUS_RUNNING'], argv.t, argv.p)
  rcPstandby = verifyStatus(STATUS['STATUS_STANDBY'], argv.t, argv.p)
  if rcPrunning == 0 or rcPstandby == 0:
    trace(0, 'Primary server came up running in production mode on its own')
  else:
    trace(0, 'Primary server did not come up in production mode on its own')
    rc = 1 
  
  traceExit('startAndPickPrimary')
  return ( rc )

#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
def determinePrimary(argv):
  traceEntry('determinePrimary')

  if argv.p != None:
    # argv.p already specified primary server
    if argv.s != None:
      # argv.s already specified standby server
      return ( argv )
    else:
      # argv.p set but not argv.s
      if argv.m1 != argv.p:
        argv.s = argv.m1
      elif argv.m2 != argv.p:
        argv.s = argv.m2
      return ( argv )
  if argv.s != None:
    if argv.p != None:
      return ( argv )
    else:
      if argv.m1 != argv.s:
        argv.p = argv.m1
      elif argv.m2 != argv.s:
        argv.p = argv.m2
      return ( argv )

  isM1Running = 1
  isM2Running = 1
  try:
    responseA = requests.get(url=OPTIONS['A'+argv.m1+'_REST_ARGS']+'/ima/v1/service/status')
  except:
    trace(0, 'A'+argv.m1+' is not running')
    isM1Running = 0

  try:
    responseB = requests.get(url=OPTIONS['A'+argv.m2+'_REST_ARGS']+'/ima/v1/service/status')
  except:
    trace(0, 'A'+argv.m2+' is not running')
    isM2Running = 0

  if isM1Running == 0 and isM2Running == 0:
    argv.p = argv.m1
    argv.s = argv.m2
    return ( argv )
  elif isM1Running == 0:
    argv.p = argv.m2
    argv.s = argv.m1
    return ( argv )
  elif isM2Running == 0:
    argv.p = argv.m1
    argv.s = argv.m2
    return ( argv )

  trace(0, responseA.content)
  trace(0, responseB.content)

  jsonPayloadA = json.loads(responseA.content)
  aStatus = jsonPayloadA['Server']['StateDescription']
  jsonPayloadB = json.loads(responseB.content)
  bStatus = jsonPayloadB['Server']['StateDescription']

  if STATUS['STATUS_RUNNING'] in aStatus:
    argv.p = argv.m1
    argv.s = argv.m2
  elif STATUS['STATUS_RUNNING'] in bStatus:
    argv.p = argv.m2
    argv.s = argv.m1
  else:
    argv.p = argv.m1
    argv.s = argv.m2

  traceExit('determinePrimary')
  return ( argv )

#------------------------------------------------------------------------------
# Main Function for HA tool
#------------------------------------------------------------------------------
def main(argv):
  parser = argparse.ArgumentParser(description='Perform HA related actions')
  parser.add_argument('-a', metavar='action', required=True, type=str, help='The action to perform, ex: setupHA', choices=['setupHA','setupPrimary','setupStandby','autoSetup','disableHA','stopServer','stopPrimary','stopStandby','stopBoth','startServer','startPrimary','startStandby','startBoth','startAndPickPrimary','crashServer','crashPrimary','crashStandby','crashBoth','swapPrimary','verifyStatus', 'cleanStore', 'restartServer', 'restartPrimary'])
  parser.add_argument('-f', metavar='log', required=False, type=str, help='The name of the log file')
  parser.add_argument('-g', metavar='group', required=False, type=str, help='The group name to use for the HA pair')
  parser.add_argument('-i', metavar='ipversion', required=False, type=str, help='The IP version to use', choices=['4','6'])
  parser.add_argument('-m1', metavar='machineA', required=False, default='1', type=str, help='The A#_COUNT index of a server')
  parser.add_argument('-m2', metavar='machineB', required=False, default='2', type=str, help='The A#_COUNT index of a server')
  parser.add_argument('-p', metavar='primary', required=False, type=str, help='The A#_COUNT index of the primary server')
  parser.add_argument('-s', metavar='standby', required=False, type=str, help='The A#_COUNT index of the standby server')
  parser.add_argument('-t', metavar='timeout', default=120, required=False, type=int, help='The timeout for verifying server status' )
  parser.add_argument('-v', metavar='verbose', default=2, required=False, type=int, help='Set the verbosity of the tracing output' )
  parser.add_argument('-status', metavar='status', required=False, type=str, help='The expected status for verifying server status' )
  parser.add_argument('-gvt', metavar='gvt', default=0, required=False, type=int, help='1 for use nonenglish group name')
  parser.add_argument('-rs', metavar='retainStore', default=0, required=False, type=int, help='1: skip second restart in disableHA and leave in maintenance mode with store intact')
  parser.add_argument('-skrs', metavar='skipRestart', default=0, required=False, type=int, help='1: skip restarts in setupHA (to configure cluster)')
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
    trace(2, 'A'+str(serverCount)+'_REST_ARGS: '+OPTIONS['A'+str(serverCount)+'_REST_ARGS'])

  if args.g == None or args.g == '':
    args.g = OPTIONS['MQKEY']

  # Run the command specified by -a input flag
  actions = {
    'setupHA': setupHA,
    'setupPrimary': setupPrimary,
    'setupStandby': setupStandby,
    'autoSetup': autoSetup,
    'disableHA': disableHA,
    'stopServer': stopServer,
    'stopPrimary': stopPrimary,
    'stopStandby': stopStandby,
    'stopBoth': stopBoth,
    'startServer': startServer,
    'startPrimary': startPrimary,
    'startStandby': startStandby,
    'startBoth': startBoth,
    'startAndPickPrimary': startAndPickPrimary,
    'crashServer': crashServer,
    'crashPrimary': crashPrimary,
    'crashStandby': crashStandby,
    'crashBoth': crashBoth,
    'swapPrimary': swapPrimary,
    'verifyStatus': verifyStatusWrapper,
    'cleanStore': cleanStore,
    'restartPrimary': restartPrimary,
    }

  args = determinePrimary(args)
  trace(0, args)

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
