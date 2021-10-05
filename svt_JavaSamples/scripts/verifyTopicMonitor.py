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
#   Queries stats for a TopicMonitor and verifies the value
#
# Usage:
#   verifyTopicMonitor.py -a <address>:<port> -t <topic> -k <field> -c <comparison> -v <value>'
#
# Example command:
#   component[$((++lc))]="cAppDriverWait,m1,-e|./verifyTopicMonitor.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-k|PublishedMsgs|-v|11000"
#
#----------------------------------------------------------------------------

import sys, json, getopt, requests, urllib, time
from urllib import urlencode

#----------------------------------------------------------------------------
#  Print usage information
#----------------------------------------------------------------------------
def usage(err):
   print(err)
   print __file__ + ' -a <address>:<port> -i <clientid> -k <field> -c <comparison> -v <value>'
   print ''
   sys.exit(2)


#----------------------------------------------------------------------------
#   Construct the restAPI URL, execute and return json result
#----------------------------------------------------------------------------
def get(endpoint, topic):
   data = ''
   headers = {'Accept': 'application/json'}
   url = 'http://{0}/ima/v1/monitor/Topic?TopicString={1}'.format(endpoint, urllib.quote_plus(topic))

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
   topic = "#"
   data = ''
   sublist = []
   sub = ''
   comparison="-eq"
   field="PublishedMsgs"
   value = 0
   log_file = sys.stdout

   try:
      options, optargs = getopt.getopt(argv,"a:t:c:v:k:f:h",["endpoint1=","topic=","comparison=","value=","field="])
   except getopt.GetoptError as err:
      usage(err)

   for option, optarg in options:
      if option in ('-a', "--endpoint"):
         endpoint = optarg
      elif option in ('-t', "--topic"):
         topic = optarg
      elif option in ('-c', "--comparison"):
         comparison = optarg
      elif option in ('-v', "--value"):
         value = int(optarg)
      elif option in ('-k', "--field"):
         field = optarg
      elif option in ('-f' ):
         LOG_FILE=optarg
         log_file=open(LOG_FILE, 'w+')
      elif option == '-h':
         usage('')

   if endpoint == '':
      usage("Admin endpoint not specifed")

   print time.asctime( time.localtime(time.time()) )
   print endpoint
   data = get(endpoint, topic)

   if data.get('Topic',[]) is None:
      print time.asctime( time.localtime(time.time()) )
      print 'Topic not in {0}'.format(data)
   elif len(data['Topic']) == 0:
      print time.asctime( time.localtime(time.time()) )
      print 'Topic for TopicString {0} does not exist: {1}'.format(topic,data)
   else:
      print "returned values: " + str(data['Topic'][0])
      print "verifying: " + field +" "+ comparison +" "+ str(value)
      if 'Topic' in data:
         if (( comparison == "-lt" ) or ( comparison == "lt" )):
            print '{0}({1})<{2}'.format(field,data['Topic'][0][field],value)
            if ( int(data['Topic'][0][field]) < int(value) ):
              print >> log_file, "Test result is Success!"
            else:
              print >> log_file, "verified failed"
         elif (( comparison == "-gt" ) or ( comparison == "gt" )):
            print '{0}({1})>{2}'.format(field,data['Topic'][0][field],value)
            if ( int(data['Topic'][0][field]) > int(value) ):
              print >> log_file, "Test result is Success!"
            else:
              print >> log_file, "verified failed"
         elif (( comparison == "-le" ) or ( comparison == "le" )):
            print '{0}({1})<={2}'.format(field,data['Topic'][0][field],value)
            if ( int(data['Topic'][0][field]) <= int(value) ):
              print >> log_file, "Test result is Success!"
            else:
              print >> log_file, "verified failed"
         elif (( comparison == "-ge" ) or ( comparison == "ge" )):
            print '{0}({1})>={2}'.format(field,data['Topic'][0][field],value)
            if ( int(data['Topic'][0][field]) >= int(value) ):
              print >> log_file, "Test result is Success!"
            else:
              print >> log_file, "verified failed"
         elif (( comparison == "-eq" ) or ( comparison == "eq" )):
            print '{0}({1})=={2}'.format(field,data['Topic'][0][field],value)
            if ( int(data['Topic'][0][field]) == int(value) ):
              print >> log_file, "Test result is Success!"
            else:
              print >> log_file, "verified failed"
         elif (( comparison == "-ne" ) or ( comparison == "ne" )):
            print '{0}({1})!={2}'.format(field,data['Topic'][0][field],value)
            if ( int(data['Topic'][0][field]) != int(value) ):
              print >> log_file, "Test result is Success!"
            else:
              print >> log_file, "verified failed"
         elif (( comparison == "-==" ) or ( comparison == "==" )):
            print '{0}({1})=={2}'.format(field,data['Topic'][0][field],value)
            if ( data['Topic'][0][field] == value ):
              print >> log_file, "Test result is Success!"
            else:
              print >> log_file, "verified failed"
         elif (( comparison == "-!=" ) or ( comparison == "!=" )):
            print '{0}({1})=={2}'.format(field,data['Topic'][0][field],value)
            if ( data['Topic'][0][field] == value ):
              print >> log_file, "Test result is Success!"
            else:
              print >> log_file, "verified failed"
         else:
            usage("comparison parameter not recognized.")
   
   

#----------------------------------------------------------------------------
#  Call main routine
#----------------------------------------------------------------------------
if __name__ == "__main__":
   main(sys.argv[1:])

