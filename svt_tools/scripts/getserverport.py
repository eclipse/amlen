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


import os, sys, time, json, getopt, subprocess, requests
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
   if (os.path.isfile("/niagara/test/testEnv.sh") or os.path.isfile("../testEnv.sh")):
      shell_source('../testEnv.sh')
      shell_source('/niagara/test/testEnv.sh')
      PORT = os.getenv('A1_PORT')

   elif (os.path.isfile("/niagara/test/scripts/ISMsetup.sh") or os.path.isfile("../ISMsetup.sh")):
      shell_source('../ISMSetup.sh')
      shell_source('/niagara/test/scripts/ISMsetup.sh')
      PORT = os.getenv('A1_PORT')

   else:
      with open('/niagara/test/scripts/cluster.json') as data_file:    
         data = json.load(data_file)

      PORT = data['appliances']['msserver01A']['endpoints']['admin']['port']

   print PORT

if __name__ == "__main__":
   main(sys.argv[1:])

