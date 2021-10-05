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

#----------------------------------------------------------------------------
#
#   Queries stats for a Queue and waits while the stat is less than
#   specified value
#
# Usage:
#   waitWhileQueue.py -a <address>:<port> -i <name> -f <field> -c <comparison> -v <value> -w <wait> -m <maxwait>'
#
# Example command:
#   component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileQueue.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-i|u0000021|-f|PublishedMsgs|-c|lt|-v|$((${MESSAGES}/10))|-w|5|-m|60"
#
#----------------------------------------------------------------------------

import sys, json, getopt, requests, urllib, time
from urllib import urlencode

#----------------------------------------------------------------------------
#  Print usage information
#----------------------------------------------------------------------------
def usage(err):
   print(err)
   print __file__ + ' -a <address>:<port> -i <name> -f <field> -c <comparison> -v <value> -w <wait> -m <maxwait>'
   print ''
   sys.exit(2)


#----------------------------------------------------------------------------
#   Construct the restAPI URL, execute and return json result
#----------------------------------------------------------------------------
def get(endpoint, name):
   data = ''
   headers = {'Accept': 'application/json'}
   url = 'http://{0}/ima/v1/monitor/Queue?name={1}'.format(endpoint, urllib.quote_plus(name))

#  print url

   try:
      response = requests.get(url, headers=headers)
   except requests.exceptions.RequestException as err:
      print(err)
      sys.exit(3)

   data = response.json()

   return data


#----------------------------------------------------------------------------
#   Main routine that processes arguments,
#   consolodates results from restAPI calls.
#   Sorts and prints the results.
#----------------------------------------------------------------------------
def main(argv):
   endpointlist = ''
   endpoint = ''
   name = ''
   data = ''
   sublist = []
   sub = ''
   comparison="-lt"
   field="PublishedMsgs"
   wait=5
   maxwait=300

   try:
      options, optargs = getopt.getopt(argv,"a:i:c:v:w:m:f:h",["endpoint1=","name=","comparison=","value=","field=","wait=","maxwait="])
   except getopt.GetoptError as err:
      usage(err)

   for option, optarg in options:
      if option in ('-a', "--endpoint"):
         endpoint = optarg
      elif option in ('-i', "--name"):
         name = optarg
      elif option in ('-c', "--comparison"):
         comparison = optarg
      elif option in ('-v', "--value"):
         value = optarg
      elif option in ('-f', "--field"):
         field = optarg
      elif option in ('-w', "--wait"):
         wait = optarg
      elif option in ('-m', "--maxwait"):
         maxwait = optarg
      elif option == '-h':
         usage('')

   if endpoint == '':
      usage("Admin endpoint not specifed")

   print endpoint
   data = get(endpoint, name)
   if 'Queue' in data:
      start = time.time()
      now = start
      if (( comparison == "-lt" ) or ( comparison == "lt" )):
         print 'wait while {0}({1})<{2}'.format(field,data['Queue'][0][field],value)
         while (( int(data['Queue'][0][field]) < int(value) ) and ((now-start) < int(maxwait) )):
            print "time.sleep(" + wait + ")"
            time.sleep(float(wait))
            data = get(endpoint, name)
            print 'wait while {0}({1})<{2}'.format(field,data['Queue'][0][field],value)
            now = time.time()
      elif (( comparison == "-gt" ) or ( comparison == "gt" )):
         print 'wait while {0}({1})>{2}'.format(field,data['Queue'][0][field],value)
         while (( int(data['Queue'][0][field]) > int(value) ) and ((now-start) < int(maxwait) )):
            print "time.sleep(" + wait + ")"
            time.sleep(float(wait))
            data = get(endpoint, name)
            print 'wait while {0}({1})>{2}'.format(field,data['Queue'][0][field],value)
            now = time.time()
      elif (( comparison == "-eq" ) or ( comparison == "eq" )):
         print 'wait while {0}({1})=={2}'.format(field,data['Queue'][0][field],value)
         while (( int(data['Queue'][0][field]) == int(value) ) and ((now-start) < int(maxwait) )):
            print "time.sleep(" + wait + ")"
            time.sleep(float(wait))
            data = get(endpoint, name)
            print 'wait while {0}({1})=={2}'.format(field,data['Queue'][0][field],value)
            now = time.time()
      elif (( comparison == "-ne" ) or ( comparison == "ne" )):
         print 'wait while {0}({1})!={2}'.format(field,data['Queue'][0][field],value)
         while (( int(data['Queue'][0][field]) != int(value) ) and ((now-start) < int(maxwait) )):
            print "time.sleep(" + wait + ")"
            time.sleep(float(wait))
            data = get(endpoint, name)
            print 'wait while {0}({1})!={2}'.format(field,data['Queue'][0][field],value)
            now = time.time()
      else:
         usage("comparison parameter not recognized.")


      if ((now-start) >= int(maxwait)):
         print "maxwait exceeded"
      else:
         print '{0}({1}) not {2} {3}'.format(field,data['Queue'][0][field],comparison,value)
         print 'SUCCESS'


#----------------------------------------------------------------------------
#  Call main routine
#----------------------------------------------------------------------------
if __name__ == "__main__":
   main(sys.argv[1:])
