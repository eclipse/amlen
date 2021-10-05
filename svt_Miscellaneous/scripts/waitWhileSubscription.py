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
#   Queries stats for a subscription and waits while the stat is less than
#   specified value
#
# Usage:
#   waitWhileSubscription.py -a <address>:<port> -i <clientid> -k <field> -c <comparison> -v <value> -w <wait> -m <maxwait>'
#
# Example command:
#   component[$((++lc))]="cAppDriverWait,m1,-e|./waitWhileSubscription.py,-o|-a|${A1_IPv4_1}:${A1_PORT}|-i|u0000021|-k|PublishedMsgs|-c|lt|-v|$((${MESSAGES}/10))|-w|5|-m|60"
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
def get(endpoint, clientid, topic, start, wait, maxwait):
   data = ''
   headers = {'Accept': 'application/json'}
   url = ''

   if ( topic == '' ):
     url = 'http://{0}/ima/v1/monitor/Subscription?ClientID={1}'.format(endpoint, urllib.quote_plus(clientid))
   else:
     url = 'http://{0}/ima/v1/monitor/Subscription?TopicString={1}'.format(endpoint, urllib.quote_plus(topic))


   print url
   retry="true"

   now = time.time()
   while ((retry == "true") and ((now-start) < int(maxwait) )):
      try:
         response = requests.get(url, headers=headers)
         print "response = requests.get(url, headers=headers)"
         retry="false"
      except requests.exceptions.Timeout:
         print "except requests.exceptions.Timeout"
         print "time.sleep(" + wait + ")"
         time.sleep(float(wait))
         now = time.time()
      except requests.exceptions.TooManyRedirects:
         print "except requests.exceptions.TooManyRedirects"
         sys.exit(3)
      except requests.exceptions.RequestException as err:
         print "except requests.exceptions.RequestException as err"
         print(err)
         print "time.sleep(" + wait + ")"
         time.sleep(float(wait))
         now = time.time()

   if(now-start) >= int(maxwait):
      print "maxwait exceeded"
      sys.exit(3)

   data = response.json()
   print 'data is {0}'.format(data)

   return data





#----------------------------------------------------------------------------
#----------------------------------------------------------------------------
#----------------------------------------------------------------------------
def process(comparison, data, value, field, maxwait, wait, endpoint, clientid, start, topic):

   print data
   now = start
   if (( comparison == "-lt" ) or ( comparison == "lt" )):
      print 'wait while {0}({1})<{2}'.format(field,data['Subscription'][0][field],value)
      while (( int(data['Subscription'][0][field]) < int(value) ) and ((now-start) < int(maxwait) )):
         print time.asctime( time.localtime(time.time()) )
         print "time.sleep(" + wait + ")"
         time.sleep(float(wait))
         data = get(endpoint, clientid, topic, start, wait, maxwait)
         print 'wait while {0}({1})<{2}'.format(field,data['Subscription'][0][field],value)
         now = time.time()
   elif (( comparison == "-gt" ) or ( comparison == "gt" )):
      print 'wait while {0}({1})>{2}'.format(field,data['Subscription'][0][field],value)
      while (( int(data['Subscription'][0][field]) > int(value) ) and ((now-start) < int(maxwait) )):
         print time.asctime( time.localtime(time.time()) )
         print "time.sleep(" + wait + ")"
         time.sleep(float(wait))
         data = get(endpoint, clientid, topic, start, wait, maxwait)
         print 'wait while {0}({1})>{2}'.format(field,data['Subscription'][0][field],value)
         now = time.time()
   elif (( comparison == "-eq" ) or ( comparison == "eq" )):
      print 'wait while {0}({1})=={2}'.format(field,data['Subscription'][0][field],value)
      while (( int(data['Subscription'][0][field]) == int(value) ) and ((now-start) < int(maxwait) )):
         print time.asctime( time.localtime(time.time()) )
         print "time.sleep(" + wait + ")"
         time.sleep(float(wait))
         data = get(endpoint, clientid, topic, start, wait, maxwait)
         print 'wait while {0}({1})=={2}'.format(field,data['Subscription'][0][field],value)
         now = time.time()
   elif (( comparison == "-ne" ) or ( comparison == "ne" )):
      print 'wait while {0}({1})!={2}'.format(field,data['Subscription'][0][field],value)
      while (( int(data['Subscription'][0][field]) != int(value) ) and ((now-start) < int(maxwait) )):
         print time.asctime( time.localtime(time.time()) )
         print "time.sleep(" + wait + ")"
         time.sleep(float(wait))
         data = get(endpoint, clientid, topic, start, wait, maxwait)
         print 'wait while {0}({1})!={2}'.format(field,data['Subscription'][0][field],value)
         now = time.time()
   else:
      usage("comparison parameter not recognized.")


   print time.asctime( time.localtime(time.time()) )
   if ((now-start) >= int(maxwait)):
      print "maxwait exceeded"
   else:
      print '{0}({1}) not {2} {3}'.format(field,data['Subscription'][0][field],comparison,value)
      print 'SUCCESS'
   return





#----------------------------------------------------------------------------
#   Main routine that processes arguments,
#   consolodates results from restAPI calls.
#   Sorts and prints the results.
#----------------------------------------------------------------------------
def main(argv):
   endpointlist = ''
   endpoint = ''
   clientid = ''
   topic = ''
   data = ''
   sublist = []
   sub = ''
   retry = ''
   comparison="-lt"
   field="PublishedMsgs"
   wait=5
   maxwait=300

   try:
      options, optargs = getopt.getopt(argv,"a:i:t:c:v:w:m:k:r:h",["endpoint1=","clientid=","comparison=","value=","field=","wait=","maxwait=","retry=","topic="])
   except getopt.GetoptError as err:
      usage(err)

   for option, optarg in options:
      if option in ('-a', "--endpoint"):
         endpoint = optarg
      elif option in ('-i', "--clientid"):
         clientid = optarg
      elif option in ('-t', "--topic"):
         topic = optarg
      elif option in ('-c', "--comparison"):
         comparison = optarg
      elif option in ('-v', "--value"):
         value = optarg
      elif option in ('-k', "--field"):
         field = optarg
      elif option in ('-w', "--wait"):
         wait = optarg
      elif option in ('-m', "--maxwait"):
         maxwait = optarg
      elif option in ('-r', "--retry"):
         retry = optarg
      elif option == '-h':
         usage('')

   if endpoint == '':
      usage("Admin endpoint not specifed")

   print endpoint
   print time.asctime( time.localtime(time.time()) )
   start = time.time()
   data = get(endpoint, clientid, topic, start, wait, maxwait)
   print data
   print time.asctime( time.localtime(time.time()) )

   print 'retry is {0}'.format(retry)
   if ((retry == 'true') and (data.get('Subscription',[]) is None)):
      now = start
      print 'now: {0}, start: {1}, maxwait: {2}'.format(now, start, maxwait)
      while ((data.get('Subscription',[]) is None) and ((now-start) < int(maxwait) )):
         print time.asctime( time.localtime(time.time()) )
         print 'Subscription not in {0}'.format(data)
         time.sleep(float(wait))
#        data = get(endpoint, clientid)
         data = get(endpoint, clientid, topic, start, wait, maxwait)
         now = time.time()
         print 'now: {0}, start: {1}, maxwait: {2}'.format(now, start, maxwait)

#  if ((retry == 'true') and ((data.get('Subscription',[]) is None) or (len(data['Subscription']) == 0))):

   if 'Subscription' in data:
      if len(data['Subscription']) > 0:
         if field in data['Subscription'][0]:
            process(comparison, data, value, field, maxwait, wait, endpoint, clientid, start, topic)
         else:
            print 'field {0} not found in {1}'.format(field,data['Subscription'])
      else:
         print 'subscription does not exist: {0}'.format(data)
   else:
      print 'Subscription not in {0}'.format(data)

#----------------------------------------------------------------------------
#  Call main routine
#----------------------------------------------------------------------------
if __name__ == "__main__":
   main(sys.argv[1:])
