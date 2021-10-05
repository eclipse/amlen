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


import os, sys, time, subprocess, json, getopt, requests
from subprocess import Popen, PIPE

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
      OPTIONS, OPTARGS = getopt.getopt(argv,"f:a:m:",[])
   except getopt.GetoptError as err:
      print(err)
      print __file__ + ' -a <server> -m <maxwait> -f <logfile>'
      sys.exit(2)

   for OPTION, OPTARG in OPTIONS:
      if OPTION == '-f':
         LOG_FILE=OPTARG
         log_file=open(LOG_FILE, 'w+')
         SCREENOUT_FILE=OPTARG + '.screenout.log'
         screenout_file=open(SCREENOUT_FILE, 'w+')
      elif OPTION == '-a':
         SERVER = OPTARG
      elif OPTION == '-m':
         maxwait = OPTARG

   if SERVER == '':
      if (os.path.isfile("/niagara/test/testEnv.sh") or (os.path.isfile("../testEnv.sh"))):
         SERVER = os.getenv('A1_HOST') + ':' + os.getenv('A1_PORT')
      else:
         SERVER = subprocess.Popen(["/niagara/test/scripts/getserverandport.py", ""], stdout=PIPE).communicate()[0].strip()

   if LOG_FILE != '':
      print >>log_file, 'LOG_FILE = ', LOG_FILE
      print >>log_file, 'SCREENOUT_FILE = ', SCREENOUT_FILE
      print >>log_file, 'SERVER = ', SERVER
   else:
      print 'LOG_FILE = ', LOG_FILE
      print 'SCREENOUT_FILE = ', SCREENOUT_FILE
      print 'SERVER = ', SERVER


   headers = {'Accept': 'application/json'}
   url1 = 'http://{0}/ima/v1/monitor/MQTTClient'.format(SERVER)
   params = {'ResultCount':25}

   if SCREENOUT_FILE != '':
      print >>screenout_file, url1

#  try:
#     response = requests.get(url1, headers=headers, params=params)
#  except requests.exceptions.RequestException as err:
#     if SCREENOUT_FILE != '':
#        print >>screenout_file, err
#     else:
#        print err
#     sys.exit(3)
#
#   data = response.json()
   data = get(url1, start, wait, maxwait)

   print rc
   print data


   while(data.get('MQTTClient') and (rc == 200)):
      for item in data['MQTTClient']:
         url2 = 'http://{0}/ima/v1/service/MQTTClient/{1}'.format(SERVER, item['ClientID'])
         print 'url = {0}'.format(url2)
         try:
            response = requests.delete(url2, headers=headers)
         except requests.exceptions.RequestException as err:
            if SCREENOUT_FILE != '':
               print >>screenout_file, err
            else:
               print err
            sys.exit(4)

         print(response.text)
         rc = response.status_code
         if rc != 200:
            status = 'MQTTClient delete failed with status_code = {0}'.format(rc)
            break

      previous = data
#     try:
#        response = requests.get(url1, headers=headers, params=params)
#     except requests.exceptions.RequestException as err:
#        if SCREENOUT_FILE != '':
#           print >>screenout_file, err
#        else:
#           print err
#        sys.exit(5)
#
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
