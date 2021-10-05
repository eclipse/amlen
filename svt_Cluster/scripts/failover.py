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

from svtutils import *

def main(argv):
   LOG_FILE = ''
   log_file = sys.stdout
   failures = 0
   SERVER = ''
   rc = 200
   A=0

   wait = 10
   maxwait = 180
   start = time.time()

   if (os.path.isfile("../testEnv.sh")):
      shell_source('../testEnv.sh')
   elif (os.path.isfile("../scripts/ISMsetup.sh")):
      shell_source('../scripts/ISMsetup.sh')

   try:
      OPTIONS, OPTARGS = getopt.getopt(argv,"f:a:",[])
   except getopt.GetoptError as err:
      print(err)
      print __file__ + ' -f <logfile> -a <appliance>'
      sys.exit(2)

   for OPTION, OPTARG in OPTIONS:
      if OPTION == '-f':
         LOG_FILE=OPTARG
         log_file=open(LOG_FILE, 'w+')
         SCREENOUT_FILE=OPTARG + '.screenout.log'
         screenout_file=open(SCREENOUT_FILE, 'w+')
      if OPTION == '-a':
         A=OPTARG

   if (A == 0):
      print __file__ + ' -f <logfile> -a <appliance>'
      sys.exit(2)

   A_COUNT = os.getenv('A_COUNT')

   MQKEY = os.getenv('MQKEY')
   L1 = os.getenv('A1_IPv4_1')
   L2 = os.getenv('A2_IPv4_1')

#failover all primary machines
   INDEX = int(A)
   if (INDEX <= int(A_COUNT)):
      HOST1 = os.getenv('A{0}_HOST'.format(INDEX))
      PORT1 = os.getenv('A{0}_PORT'.format(INDEX))
      restart(HOST1,PORT1)

      time.sleep(10)

      waitonstatus(HOST1,PORT1)
      waitonhastatus(HOST1,PORT1)
      waitonclusterstatus(HOST1,PORT1)

      CLUSTERNAME = '{0}_CLUSTER'.format(MQKEY)
      print "CLUSTERNAME is " + CLUSTERNAME

      if (0 == (INDEX%2)):
        HAGROUP = '{0}_HA{1}{2}'.format(MQKEY, INDEX-1, INDEX)
      else:
        HAGROUP = '{0}_HA{1}{2}'.format(MQKEY, INDEX, INDEX+1)

      print "HAGROUP is " + HAGROUP
      failures = verifystatus(failures,HOST1,PORT1,"Standby",True,"STANDBY",HAGROUP,True,CLUSTERNAME,"Standby",0)

   else:
      failures = 1
      print INDEX + " not less than " + A_COUNT

   if (failures == 0): 
      print >>log_file, "Test result is Success!"
   else:
      print >>log_file, "TEST FAILED:  " + str(failures) + " failures occured"



if __name__ == "__main__":
   main(sys.argv[1:])
