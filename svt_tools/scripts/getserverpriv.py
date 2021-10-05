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
#  shell_source('../testEnv.sh')
#  shell_source('/niagara/test/testEnv.sh')


   with open('/niagara/test/scripts/ISMsetup.json') as data_file:    
      data = json.load(data_file)


   private = data['appliances']['msserver01']['private']

   os.system("grep " + private + " /etc/hosts | cut -d' ' -f1")

if __name__ == "__main__":
   main(sys.argv[1:])
