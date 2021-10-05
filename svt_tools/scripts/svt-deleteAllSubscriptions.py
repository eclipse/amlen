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
#----------------------------------------------------------------------------


import os, sys, time, json, getopt, subprocess, requests, urllib
from subprocess import Popen, PIPE
from urllib import urlencode

# path = os.path.dirname(os.path.realpath(__file__))
# execfile(path + '/restapiPythonLib.py')

def shell_source(script):
   if os.path.isfile(script):
      pipe = subprocess.Popen(". %s; env" % script, stdout=subprocess.PIPE, shell=True)
      output = pipe.communicate()[0]
      env = dict((line.split("=", 1) for line in output.splitlines()))
      os.environ.update(env)




#----------------------------------------------------------------------------
#   Construct the restAPI URL, execute and return json result
#----------------------------------------------------------------------------
def get(url, start, wait, maxwait):
   data = ''
   headers = {'Accept': 'application/json'}

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



def main(argv):
   LOG_FILE = ''
   SCREENOUT_FILE = ''
   SERVER = ''
   rc = 200
   status = 'Test result is Success!'

   wait = 10
   maxwait = 180
   start = time.time()

#  shell_source('../scripts/ISMsetup.sh')
#  shell_source('/niagara/test/scripts/ISMsetup.sh')
   shell_source('../testEnv.sh')
   shell_source('/niagara/test/testEnv.sh')

   try:
      OPTIONS, OPTARGS = getopt.getopt(argv,"f:a:",[])
   except getopt.GetoptError as err:
      print(err)
      print __file__ + ' -a <server> -f <logfile>'
      sys.exit(2)

   for OPTION, OPTARG in OPTIONS:
      if OPTION == '-f':
         LOG_FILE=OPTARG
         log_file=open(LOG_FILE, 'w+')
         SCREENOUT_FILE=OPTARG + '.screenout.log'
         screenout_file=open(SCREENOUT_FILE, 'w+')
      elif OPTION == '-a':
         SERVER = OPTARG

   if SERVER == '':
      if (os.path.isfile("/niagara/test/testEnv.sh") or (os.path.isfile("../testEnv.sh"))):
         SERVER = os.getenv('A1_HOST') + ':' + os.getenv('A1_PORT')
      else:
         SERVER = subprocess.Popen(["/niagara/test/scripts/getserverandport.py", ""], stdout=PIPE).communicate()[0].strip()

   print 'LOG_FILE = ', LOG_FILE
   print 'SCREENOUT_FILE = ', SCREENOUT_FILE
   print 'SERVER = ', SERVER

   headers = {'Accept': 'application/json'}
   url1 = 'http://{0}/ima/v1/monitor/Subscription'.format(SERVER)

#  try:
#     response = requests.get(url1, headers=headers)
#  except requests.exceptions.RequestException as err:
#     print(err)
#     sys.exit(3)
#  data = response.json()

   data = get(url1, start, wait, maxwait)

   print rc
   print data

   while(data.get('Subscription') and (rc == 200)):
      for item in data['Subscription']:
         url2 = 'http://{0}/ima/v1/service/Subscription/{1}/{2}'.format(SERVER, item['ClientID'], urllib.quote_plus(item['SubName']))
         print '{0}'.format(url2)
         try:
            response = requests.delete(url2, headers=headers)
         except requests.exceptions.RequestException as err:
            print(err)
            sys.exit(3)
         print(response.text)
         rc = response.status_code
         if rc != 200:
            status = 'Subscription delete failed with rc = {0}'.format(rc)
            break;

      previous = data
#     try:
#        response = requests.get(url1, headers=headers)
#     except requests.exceptions.RequestException as err:
#        print(err)
#        sys.exit(3)
#     data = response.json()

      data = get(url1, start, wait, maxwait)

      if sorted(data.items()) == sorted(previous.items()):
         break

   if LOG_FILE != '':
      print >>log_file, status
      print >>log_file, "Test result is Success!"
   else:
      print status


if __name__ == "__main__":
   main(sys.argv[1:])
