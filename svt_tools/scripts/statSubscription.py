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
#  Constructs and executes the restAPI for statSubscription
#  Formats output similar to original appliance imaserver stat Subscription command
#  
#  reorder entries in keylist below to reorder output
# 
# Usage:
#    statSubscription.py -a <AdminEndpoint(s)> -s <SubscriptionName> -i <ClientID> -t <StatType> -r <ResultCount>
#
#    -a <AdminEndpoint(s)>  Comma separated list of Admin Endpoints in format of Server:Port,Server2:port
#    -s <SubscriptionName>  SubscriptionName for restAPI (optional)
#    -i <ClientID>          ClientID for restAPI (optional)
#    -t <StatType>          Stat Type (optional), such as:
#                               
#                             PublishedMsgsHighest
#                             PublishedMsgsLowest
#                             BufferedMsgsHighest
#                             BufferedMsgsLowest
#                             BufferedPercentHighest
#                             BufferedPercentLowest
#                             BufferedHWMPercentHighest
#                             BufferedHWMPercentLowest
#                             RejectedMsgsHighest
#                             RejectedMsgsLowest
#                             DiscardedMsgsHighest
#                             DiscardedMsgsLowest
#                             ExpiredMsgsHighest
#                             ExpiredMsgsLowest
#
#    -r <ResultCount>       options are 10, 25, 50, 100 (optional)
#
# Basic example command:
#
#    ./statSubscription.py -a 10.120.16.98:9089 
#
#     Will construct the following restAPI:
#        http://10.120.16.98:9089/ima/v1/monitor/Subscription
#
#     And print the following sample output
#        "SubName","TopicString","ClientID","IsDurable","BufferedMsgs","BufferedMsgsHWM","BufferedPercent","MaxMessages","PublishedMsgs","RejectedMsgs","BufferedHWMPercent","IsShared","Consumers","DiscardedMsgs","ExpiredMsgs","MessagingPolicy"
#        "/APP/1/#","/APP/1/#","backend","True",0,0,0.0,5000,0,0,0.0,"False",1,0,0,"DemoMessagingPolicy"
# 
#
# Example command:
#
#    ./statSubscription.py -a 10.120.16.98:9089 -t BufferedMsgsHighest -r 10
#
#     Will construct the following restAPI:
#        http://10.120.16.98:9089/ima/v1/monitor/Subscription?StatType=BufferedMsgsHighest&ResultCount=10
#
#     And print the following sample output
#        "SubName","TopicString","ClientID","IsDurable","BufferedMsgs","BufferedMsgsHWM","BufferedPercent","MaxMessages","PublishedMsgs","RejectedMsgs","BufferedHWMPercent","IsShared","Consumers","DiscardedMsgs","ExpiredMsgs","MessagingPolicy"
#        "/APP/1/#","/APP/1/#","backend","True",0,0,0.0,5000,0,0,0.0,"False",1,0,0,"DemoMessagingPolicy"
# 
#
#----------------------------------------------------------------------------

import sys, json, getopt, requests

keylist = [ 
            "AdminEndpoint",
            "SubName", 
            "TopicString",
            "ClientID",
            "IsDurable",
            "BufferedMsgs",
            "BufferedMsgsHWM",
            "BufferedPercent",
            "MaxMessages",
            "PublishedMsgs",
            "RejectedMsgs",
            "BufferedHWMPercent",
            "IsShared",
            "Consumers",
            "DiscardedMsgs",
            "ExpiredMsgs",
            "MessagingPolicy"
          ];

#----------------------------------------------------------------------------
#  Print usage information                                                   
#----------------------------------------------------------------------------
def usage(err):
   print(err)
   print __file__ + ' -a <AdminEndpoint(s)> -s <SubscriptionName> -i <ClientID> -t <StatType> -r <ResultCount>'
   print ''
   print '   AdminEndpoint(s) can be a comma separated list with no spaces.'
   print ''
   print '   Example StatTypes:'
   print '            PublishedMsgsHighest'
   print '            PublishedMsgsLowest'
   print '            BufferedMsgsHighest'
   print '            BufferedMsgsLowest'
   print '            BufferedPercentHighest'
   print '            BufferedPercentLowest'
   print '            BufferedHWMPercentHighest'
   print '            BufferedHWMPercentLowest'
   print '            RejectedMsgsHighest'
   print '            RejectedMsgsLowest'
   print '            DiscardedMsgsHighest'
   print '            DiscardedMsgsLowest'
   print '            ExpiredMsgsHighest'
   print '            ExpiredMsgsLowest'
   print ''
   print '  ResultCount options are 10, 25, 50, 100'
   sys.exit(2)


#----------------------------------------------------------------------------
#   Construct the restAPI URL, execute and return json result
#----------------------------------------------------------------------------
def get(endpoint, subscription, clientid, stattype, resultcount):
   data = ''
   headers = {'Accept': 'application/json'}
   url = 'http://{0}/ima/v1/monitor/Subscription'.format(endpoint)

   if subscription != "":
      url = url + "/" + subscription

   if clientid != "":
      url = url + "/" + clientid

   if ((stattype != "") or (resultcount != "")):
      url = url + "?"

   if stattype != "":
      url = url + "StatType=" + stattype

   if ((stattype != "") and (resultcount != "")):
      url = url + "&"

   if resultcount != "":
      url = url + "ResultCount=" + resultcount

#  print url

   try: 
      response = requests.get(url, headers=headers)
   except requests.exceptions.RequestException as err:
      print(err)
      sys.exit(3)

   data = response.json()
   return data


#----------------------------------------------------------------------------
#   Print keylist headers
#----------------------------------------------------------------------------
def printheaders():
   out = ''
   for key in keylist:
      if out != '':
         out = out + ","
      out = out + "\"" + key + "\""
   print out


#----------------------------------------------------------------------------
#   Print values for a subscriptions in same order as keylist headers
#----------------------------------------------------------------------------
def printit(sub):
   out = ''
   for key in keylist:
      if out != '':
         out = out + ","

      value = sub[key]

      if type(value) is int:
         out = out + '{0}'.format(value)
      elif type(value) is float:
         out = out + '{0}'.format(value)
      elif type(value) is bool:
         out = out + '{0}'.format(value)
      else:
         out = out + "\"" + value + "\""
   print out


#----------------------------------------------------------------------------
#   Method used by sort() to return PublishedMsgs
#----------------------------------------------------------------------------
def PublishedMsgs(sub):
    try:
        return int(sub['PublishedMsgs'])
    except KeyError:
        return 0

#----------------------------------------------------------------------------
#   Method used by sort() to return BufferedMsgs
#----------------------------------------------------------------------------
def BufferedMsgs(sub):
    try:
        return int(sub['BufferedMsgs'])
    except KeyError:
        return 0

#----------------------------------------------------------------------------
#   Method used by sort() to return BufferedPercent
#----------------------------------------------------------------------------
def BufferedPercent(sub):
    try:
        return int(sub['BufferedPercent'])
    except KeyError:
        return 0

#----------------------------------------------------------------------------
#   Method used by sort() to return BufferedHWMPercent
#----------------------------------------------------------------------------
def BufferedHWMPercent(sub):
    try:
        return int(sub['BufferedHWMPercent'])
    except KeyError:
        return 0


#----------------------------------------------------------------------------
#   Sort subscription list by stattype
#----------------------------------------------------------------------------
def sort(sublist,stattype):
   if stattype == "PublishedMsgsHighest":
      sublist.sort(key=PublishedMsgs, reverse=True)

   elif stattype == "PublishedMsgsLowest":
      sublist.sort(key=PublishedMsgs, reverse=False)

   elif stattype == "BufferedMsgsHighest":
      sublist.sort(key=BufferedMsgs, reverse=True)

   elif stattype == "BufferedMsgsLowest":
      sublist.sort(key=BufferedMsgs, reverse=False)

   elif stattype == "BufferedPercentHighest":
      sublist.sort(key=BufferedPercent, reverse=True)

   elif stattype == "BufferedPercentLowest":
      sublist.sort(key=BufferedPercent, reverse=False)

   elif stattype == "BufferedHWMPercentHighest":
      sublist.sort(key=BufferedHWMPercent, reverse=True)

   elif stattype == "BufferedHWMPercentLowest":
      sublist.sort(key=BufferedHWMPercent, reverse=False)

   elif stattype == "RejectedMsgsHighest":
      sublist.sort(key=RejectedMsgs, reverse=True)

   elif stattype == "RejectedMsgsLowest":
      sublist.sort(key=RejectedMsgs, reverse=False)

   elif stattype == "DiscardedMsgsHighest":
      sublist.sort(key=DiscardedMsgs, reverse=True)

   elif stattype == "DiscardedMsgsLowest":
      sublist.sort(key=DiscardedMsgs, reverse=False)

   elif stattype == "ExpiredMsgsHighest":
      sublist.sort(key=ExpiredMsgs, reverse=True)

   elif stattype == "ExpiredMsgsLowest":
      sublist.sort(key=ExpiredMsgs, reverse=False)

   return sublist



#----------------------------------------------------------------------------
#   Main routine that processes arguments, 
#   consolodates results from restAPI calls.  
#   Sorts and prints the results.
#----------------------------------------------------------------------------
def main(argv):
   endpointlist = ''
   endpoint = '' 
   subscription = ''
   clientid = ''
   stattype = ''
   resultcount = ''
   data = ''
   sublist = []
   sub = ''

   try:
      options, optargs = getopt.getopt(argv,"a:s:i:t:r:h",["endpoint=","subscription=","clientid=","stattype=","resultcount="])
   except getopt.GetoptError as err:
      usage(err)

   for option, optarg in options:
      if option in ('-a', "--serverlist"):
         endpointlist = optarg.split(',')
      elif option in ('-s', "--subscription"):
         subscription = optarg
      elif option in ('-i', "--clientid"):
         clientid = optarg
      elif option in ('-t', "--stattype"):
         stattype = optarg
      elif option in ('-r', "--resultcount"):
         resultcount = optarg
      elif option == '-h':
         usage('')

   if endpointlist == '':
      usage("Admin endpoint not specifed")

   for endpoint in endpointlist:
      data = get(endpoint, subscription, clientid, stattype, resultcount)

      if 'Subscription' in data:
         for sub in data['Subscription']:
            sub['AdminEndpoint'] = endpoint
            sublist.append(sub)
      elif 'Code' in data:
         print endpoint + '  ' + data['Code'] + ': ' + data['Message']

   if len(sublist) > 0:
      if (stattype != ''):
         sublist = sort(sublist,stattype)

      printheaders()
      for sub in sublist:
         printit(sub)


#----------------------------------------------------------------------------
#  Call main routine 
#----------------------------------------------------------------------------
if __name__ == "__main__":
   main(sys.argv[1:])


