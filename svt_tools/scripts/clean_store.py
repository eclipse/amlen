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
# clean_store.py
#
# Description:
#   This script follows the documented procedure for cleaning the store on
#   the MessageSight appliance.
#
# Usage:
#   ./clean_store.py
#
#----------------------------------------------------------------------------

import os, sys, time, subprocess, json, getopt, requests
from subprocess import Popen, PIPE

def shell_source(script):
   if os.path.isfile(script):
      pipe = subprocess.Popen(". %s; env" % script, stdout=subprocess.PIPE, shell=True)
      output = pipe.communicate()[0]
      env = dict((line.split("=", 1) for line in output.splitlines()))
      os.environ.update(env)

def cleanstore(SERVER):
   print 'cleanstore', SERVER
#  curl -X GET -f -s http://10.120.16.98:9089/ima/v1/service/status/Server
   url = 'http://{0}/ima/v1/service/restart'.format(SERVER)
   payload = json.JSONEncoder().encode({"Service": "imaserver", "CleanStore": True})

   try: 
      response = requests.post(url, data=payload)
   except requests.exceptions.RequestException as err:
      print(err)

   rc = response.status_code
   return rc


def status(SERVER):
   print 'status', SERVER

#  curl -X GET -f -s http://10.120.16.98:9089/ima/v1/service/status/Server
   url = 'http://{0}/ima/v1/service/status/Server'.format(SERVER)

   rc = 0
   count = 0
   max = 50
   sleep = 2

   while((rc != 200) and (count < max)):
      repeat = True
      while(repeat and (count < max)):
         count += 1
         try: 
            response = requests.get(url)
            repeat = False
         except requests.exceptions.RequestException as err:
            print(err)
            time.sleep(sleep)

      rc = response.status_code

      if rc != 200:
         time.sleep(sleep)
   
   data = response.json()
   state = data['Server']['State']

   return state



def main(argv):
   LOG_FILE = ''
   SCREENOUT_FILE = ''
   SERVER = ''
   sleep = 2

#  shell_source('../scripts/ISMsetup.sh')
#  shell_source('/niagara/test/scripts/ISMsetup.sh')
#  shell_source('../testEnv.sh')
#  shell_source('/niagara/test/testEnv.sh')

   try:
      OPTIONS, OPTARGS = getopt.getopt(argv,"f:a:",[])
   except getopt.GetoptError as err:
      print(err)
      print __file__ + ' -a <server> -f <logfile>'
      sys.exit(2)
   
   for OPTION, OPTARG in OPTIONS:
      if OPTION == '-f':
         LOG_FILE = OPTARG
         SCREENOUT_FILE = OPTARG + '.screenout.log'
      elif OPTION == '-a':
         SERVER = OPTARG
   
   if SERVER == '':
      ADDR = subprocess.Popen(["/niagara/test/scripts/getserver.py", ""], stdout=PIPE).communicate()[0].strip()
      PORT = subprocess.Popen(["/niagara/test/scripts/getserverport.py", ""], stdout=PIPE).communicate()[0].strip()
      SERVER = '{0}:{1}'.format(ADDR, PORT)
#     SERVER = os.getenv('A1_HOST') + ':' + os.getenv('A1_PORT') 

   rc = cleanstore(SERVER)

   if rc != 200:
      print rc
      sys.exit(4)

   time.sleep(sleep)

   state = 0
   count = 0
   max = 10

   while((state != 1) and (count < max)):
     count += 1
     state = status(SERVER)
     if state != 1:
       time.sleep(sleep)


if __name__ == "__main__":
   main(sys.argv[1:])

