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

def main(argv):
   LOG_FILE = ''
   SCREENOUT_FILE = ''
   SERVER = ''

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
         LOG_FILE = OPTARG
         SCREENOUT_FILE = OPTARG + '.screenout.log'
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
   url = 'http://{0}/ima/v1/configuration/Endpoint'.format(SERVER)
   params = {'ResultCount':100}

   try: 
      response = requests.get(url, headers=headers, params=params)
   except requests.exceptions.RequestException as err:
      print(err)
      sys.exit(3)

   if response.status_code != 200:
      print response.text
      sys.exit(4)

   data = response.json()
   for item in data['Endpoint']:
      url2 = 'http://{0}/ima/v1/configuration/Endpoint/{1}'.format(SERVER, item)
      try: 
         response = requests.delete(url2, headers=headers)
      except requests.exceptions.RequestException as err:
         print err 
         sys.exit(4)

      print(response.text)
      rc = response.status_code
      if rc != 200:
         status = 'MQTTClient delete failed with status_code = {0}'.format(rc)
         break

if __name__ == "__main__":
   main(sys.argv[1:])

