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
#   Queries stats for a Subscription and waits while the stat is less than
#   specified value
#
# Usage:
#   verifySubscription.py -a <address>:<port> -i <clientid> -k <field> -c <comparison> -v <value>'
#
# Example command:
#   component[$((++lc))]="cAppDriverWait,m1,-e|./verifySubscriptionStat.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-i|u0000021|-k|PublishedMsgs|-c|lt|-v|$((${MESSAGES}/10))"
#
#----------------------------------------------------------------------------

import sys, json, getopt, requests, urllib, time
from urllib import urlencode

#----------------------------------------------------------------------------
#  Print usage information
#----------------------------------------------------------------------------
def usage(err):
   print(err)
   print __file__ + ' -a <address>:<port> -i <clientid> -k <field> -c <comparison> -v <value> -w <wait> -m <maxwait>'
   print ''
   sys.exit(2)


#----------------------------------------------------------------------------
#   Construct the restAPI URL, execute and return json result
#----------------------------------------------------------------------------
def get(endpoint, clientid):
   data = ''
   headers = {'Accept': 'application/json'}
   url = 'http://{0}/ima/v1/monitor/Subscription?ClientID={1}'.format(endpoint, urllib.quote_plus(clientid))

   print url

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
   clientid = ''
   data = ''
   sublist = []
   sub = ''
   comparison="-lt"
   field="PublishedMsgs"

   try:
      options, optargs = getopt.getopt(argv,"a:i:c:v:k:f:h",["endpoint1=","clientid=","comparison=","value=","field="])
   except getopt.GetoptError as err:
      usage(err)

   for option, optarg in options:
      if option in ('-a', "--endpoint"):
         endpoint = optarg
      elif option in ('-i', "--clientid"):
         clientid = optarg
      elif option in ('-c', "--comparison"):
         comparison = optarg
      elif option in ('-v', "--value"):
         value = optarg
      elif option in ('-k', "--field"):
         field = optarg
      elif option in ('-f' ):
         logfile = optarg
      elif option == '-h':
         usage('')

   if endpoint == '':
      usage("Admin endpoint not specifed")

   print time.asctime( time.localtime(time.time()) )
   print endpoint
   data = get(endpoint, clientid)

   if data.get('Subscription',[]) is None:
      print time.asctime( time.localtime(time.time()) )
      print 'Subscription not in {0}'.format(data)
   elif len(data['Subscription']) == 0:
      print time.asctime( time.localtime(time.time()) )
      print 'subscription for clientid {0} does not exist: {1}'.format(clientid,data)
   else:
      print data['Subscription'][0]
      print field
      print value
      if 'Subscription' in data:
         if (( comparison == "-lt" ) or ( comparison == "lt" )):
            print '{0}({1})<{2}'.format(field,data['Subscription'][0][field],value)
            if ( int(data['Subscription'][0][field]) < int(value) ):
              print "Test result is Success!"
            else:
              print "verified failed"
         elif (( comparison == "-gt" ) or ( comparison == "gt" )):
            print '{0}({1})>{2}'.format(field,data['Subscription'][0][field],value)
            if ( int(data['Subscription'][0][field]) > int(value) ):
              print "Test result is Success!"
            else:
              print "verified failed"
         elif (( comparison == "-le" ) or ( comparison == "le" )):
            print '{0}({1})<={2}'.format(field,data['Subscription'][0][field],value)
            if ( int(data['Subscription'][0][field]) <= int(value) ):
              print "Test result is Success!"
            else:
              print "verified failed"
         elif (( comparison == "-ge" ) or ( comparison == "ge" )):
            print '{0}({1})>={2}'.format(field,data['Subscription'][0][field],value)
            if ( int(data['Subscription'][0][field]) >= int(value) ):
              print "Test result is Success!"
            else:
              print "verified failed"
         elif (( comparison == "-eq" ) or ( comparison == "eq" )):
            print '{0}({1})=={2}'.format(field,data['Subscription'][0][field],value)
            if ( int(data['Subscription'][0][field]) == int(value) ):
              print "Test result is Success!"
            else:
              print "verified failed"
         elif (( comparison == "-ne" ) or ( comparison == "ne" )):
            print '{0}({1})!={2}'.format(field,data['Subscription'][0][field],value)
            if ( int(data['Subscription'][0][field]) != int(value) ):
              print "Test result is Success!"
            else:
              print "verified failed"
         elif (( comparison == "-==" ) or ( comparison == "==" )):
            print '{0}({1})=={2}'.format(field,data['Subscription'][0][field],value)
            if ( data['Subscription'][0][field] == value ):
              print "Test result is Success!"
            else:
              print "verified failed"
         elif (( comparison == "-!=" ) or ( comparison == "!=" )):
            print '{0}({1})=={2}'.format(field,data['Subscription'][0][field],value)
            if ( data['Subscription'][0][field] == value ):
              print "Test result is Success!"
            else:
              print "verified failed"
         else:
            usage("comparison parameter not recognized.")
   
   

#----------------------------------------------------------------------------
#  Call main routine
#----------------------------------------------------------------------------
if __name__ == "__main__":
   main(sys.argv[1:])

