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
#------------------------------------------------------------------------------
# verifyConnectedServers.py
# 
# Usage: ./verifyConnectedServers.py -s <ip:port> -c <#connected> -d <#disconnected>
# 
# Write a nice long script to basically do:
#
#   output=`curl -X GET -sS ${A1_HOST}/ima/service/status | \
#           python -c "import json,sys;obj=json.load(sys.stdin); \
#                      print obj[\"Cluster\"][\"ConnectedServers\"]"`
#
#   if [ ${output} -ne 1 ] ; then
#     echo "failure: wrong number of connected servers"
#   fi 
#
# Use -t to wait t seconds until number of servers matches expected, otherwise 
# just do a yes/no check
#------------------------------------------------------------------------------
import json
import sys
import argparse
import requests
import time


def statusLoop(connected, disconnected, timeout, serverURI):
  print ("Entering statusLoop..")
  print ("Waiting for status to match..")
  rc=1

  url = 'http://'+serverURI+'/ima/service/status' 
  elapsed=0
  output = ''
  while elapsed < timeout:
    conmatch=1
    disconmatch=1

    elapsed+=1
    time.sleep(1)
    print ("Elapsed: " + str(elapsed))
    try:
      
      print ( 'GET ' + url )
      response = requests.get(url=url)
      obj = json.loads(response.content)
      
      print ( obj["Cluster"] )

      if obj["Cluster"]["Status"] == "Inactive":
        print ( 'Cluster is inactive..' )
        continue
        
      if connected != None:
        conmatch=0
        print ( 'Checking that ConnectedServers == ' + str(connected) )
        print ( 'ConnectedServers == ' + str(obj["Cluster"]["ConnectedServers"]) )
        if obj["Cluster"]["ConnectedServers"] == connected:
          print ( 'Match: ConnectedServers == ' + str(obj["Cluster"]["ConnectedServers"]) + ', Expected ' + str(connected) )
          conmatch=1
        
      if disconnected != None:
        disconmatch=0
        print ( 'Checking that DisconnectedServers == ' + str(disconnected) )
        print ( 'DisconnectedServers == ' + str(obj["Cluster"]["DisconnectedServers"]) )
        if obj["Cluster"]["DisconnectedServers"] == disconnected:
          print ( 'Match: DisconnectedServers == ' + str(obj["Cluster"]["DisconnectedServers"]) + ', Expected ' + str(disconnected) )
          disconmatch=1
      if conmatch==disconmatch==1:
        rc=0
        break
    except:
      print ( 'received exception' )
      

  #------
  if rc == 1:
    print ( 'Failure: ConnectedServers == ' + str(obj["Cluster"]["ConnectedServers"]) + ', Expected ' + str(connected) )
    print ( 'Failure: DisconnectedServers == ' + str(obj["Cluster"]["DisconnectedServers"]) + ', Expected ' + str(disconnected) )
  else:
    print ( 'Success: ConnectedServers == ' + str(obj["Cluster"]["ConnectedServers"]) + ', Expected ' + str(connected) )
    print ( 'Success: DisconnectedServers == ' + str(obj["Cluster"]["DisconnectedServers"]) + ', Expected ' + str(disconnected) )


  print("Exiting statusLoop..")
  return(rc)



def main(argv):
  rc = 0
  parser = argparse.ArgumentParser(description='Perform HA related actions')

  parser.add_argument('-c', metavar='connected', required=False, type=int, help='The expected value for ConnectedServers')
  parser.add_argument('-d', metavar='disconnected', required=False, type=int, help='The expected value for DisconnectedServers')
  parser.add_argument('-s', metavar='serverURI', required=True, type=str, help='The URI of the server to check')
  parser.add_argument('-f', metavar='log', required=False, type=str, help='The log file')
  parser.add_argument('-t', metavar='timeout', required=False, type=int,help='Time to wait for expected values')
  parser.add_argument('args', help='Remaining arguments', nargs=argparse.REMAINDER)
  args = parser.parse_args()

  global LOG, OLD_STDOUT
  if args.f != None:
    try:
      OLD_STDOUT = sys.stdout
      LOG = open(args.f, "w", 1)
      sys.stdout = LOG
    except:
      print ( 'Failed to open log file: ' + args.f )
      exit ( 1 )

  if args.t == None:
    try:
      url = 'http://'+args.s+'/ima/service/status'
      print ( 'GET ' + url )
      response = requests.get(url=url)
      obj = json.loads(response.content)
      
      print ( obj["Cluster"] )

      if obj["Cluster"]["Status"] == "Inactive":
        print ( 'Cluster is inactive. Exiting' )
        return ( 0 )
        
      if args.c != None:
        print ( 'Checking that ConnectedServers == ' + str(args.c) )
        if obj["Cluster"]["ConnectedServers"] != args.c:
          print ( 'FAILURE: ConnectedServers == ' + str(obj["Cluster"]["ConnectedServers"]) + ', Expected ' + str(args.c) )
          rc += 1
        
      if args.d != None:
        print ( 'Checking that DisconnectedServers == ' + str(args.d) )
        if obj["Cluster"]["DisconnectedServers"] != args.d:
          print ( 'FAILURE: DisconnectedServers == ' + str(obj["Cluster"]["DisconnectedServers"]) + ', Expected ' + str(args.d) )
          rc += 1
        
    except:
      print ( 'received exception' )
      rc += 1
  else:
    rc = statusLoop(args.c, args.d, args.t, args.s)

  if rc == 0:
    print ( 'Test result was Success!' )
  else:
    print ( 'Test result was Failure!' )
  return ( rc )

global rc
if __name__ == "__main__":

  rc = main( sys.argv[1:] )
  print ( 'Exiting with RC='+str(rc))

  time.sleep(3)

exit ( rc )
