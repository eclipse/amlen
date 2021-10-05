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

   wait = 10
   maxwait = 180
   start = time.time()

   if (os.path.isfile("../testEnv.sh")):
      shell_source('../testEnv.sh')
   elif (os.path.isfile("../scripts/ISMsetup.sh")):
      shell_source('../scripts/ISMsetup.sh')

   try:
      OPTIONS, OPTARGS = getopt.getopt(argv,"f:",[])
   except getopt.GetoptError as err:
      print(err)
      print __file__ + ' -f <logfile>'
      sys.exit(2)

   for OPTION, OPTARG in OPTIONS:
      if OPTION == '-f':
         LOG_FILE=OPTARG
         log_file=open(LOG_FILE, 'w+')
         SCREENOUT_FILE=OPTARG + '.screenout.log'
         screenout_file=open(SCREENOUT_FILE, 'w+')


   A_COUNT = os.getenv('A_COUNT')

   MQKEY = os.getenv('MQKEY')
   L1 = os.getenv('A1_IPv4_1')
   L2 = os.getenv('A2_IPv4_1')


   print
   print "Restart even machines to ensure they are standby"
   print

   INDEX = 1
   while (INDEX < int(A_COUNT)):
      HOST2 = os.getenv('A{0}_HOST'.format(INDEX + 1))
      PORT2 = os.getenv('A{0}_PORT'.format(INDEX + 1))
      makestandby(HOST2,PORT2)
      INDEX += 2

   time.sleep(10)

   INDEX = 1
   while (INDEX <= int(A_COUNT)):
      HOST1 = os.getenv('A{0}_HOST'.format(INDEX))
      PORT1 = os.getenv('A{0}_PORT'.format(INDEX))
      waitonstatus(HOST1,PORT1)
      waitonhastatus(HOST1,PORT1)
      INDEX += 1

   print
   print "Disable cluster on primary machines"
   print

   INDEX = 1
   while (INDEX <= int(A_COUNT)):
     HOST1 = os.getenv('A{0}_HOST'.format(INDEX))
     PORT1 = os.getenv('A{0}_PORT'.format(INDEX))
     disablecluster(HOST1,PORT1)
     INDEX += 2

   time.sleep(10)

   INDEX = 1
   while (INDEX <= int(A_COUNT)):
      HOST1 = os.getenv('A{0}_HOST'.format(INDEX))
      PORT1 = os.getenv('A{0}_PORT'.format(INDEX))
      waitonstatus(HOST1,PORT1)
      waitonhastatus(HOST1,PORT1)
      INDEX += 1

   print
   print "Cleanstore on all machines"
   print

   INDEX = 1
   while (INDEX <= int(A_COUNT)):
      HOST1 = os.getenv('A{0}_HOST'.format(INDEX))
      PORT1 = os.getenv('A{0}_PORT'.format(INDEX))
      cleanstore(HOST1,PORT1)
      INDEX += 1

   time.sleep(10)

   INDEX = 1
   while (INDEX <= int(A_COUNT)):
      HOST1 = os.getenv('A{0}_HOST'.format(INDEX))
      PORT1 = os.getenv('A{0}_PORT'.format(INDEX))
      waitonstatus(HOST1,PORT1)
      INDEX += 1

   print
   print "Configure cluster on machines"
   print

   INDEX = 1
   while (INDEX < int(A_COUNT)):
      HOST1 = os.getenv('A{0}_HOST'.format(INDEX))
      PORT1 = os.getenv('A{0}_PORT'.format(INDEX))
      A1 = os.getenv('A{0}_IPv4_1'.format(INDEX))

      HOST2 = os.getenv('A{0}_HOST'.format(INDEX + 1))
      PORT2 = os.getenv('A{0}_PORT'.format(INDEX + 1))
      A2 = os.getenv('A{0}_IPv4_1'.format(INDEX + 1))

      CLUSTERNAME = '{0}_CLUSTER'.format(MQKEY)
      configcluster(HOST1,PORT1,CLUSTERNAME,L1,L2,A1)
      configcluster(HOST2,PORT2,CLUSTERNAME,L1,L2,A2)
      INDEX += 2

   INDEX = 1
   while (INDEX <= int(A_COUNT)):
      HOST1 = os.getenv('A{0}_HOST'.format(INDEX))
      PORT1 = os.getenv('A{0}_PORT'.format(INDEX))
      restart(HOST1,PORT1)
      INDEX += 1

   time.sleep(10)

   INDEX = 1
   while (INDEX <= int(A_COUNT)):
      HOST1 = os.getenv('A{0}_HOST'.format(INDEX))
      PORT1 = os.getenv('A{0}_PORT'.format(INDEX))
      waitonstatus(HOST1,PORT1)
      waitonhastatus(HOST1,PORT1)
      waitonclusterstatus(HOST1,PORT1)
      INDEX += 1

   INDEX = 1
   while (INDEX < int(A_COUNT)):
      HOST1 = os.getenv('A{0}_HOST'.format(INDEX))
      PORT1 = os.getenv('A{0}_PORT'.format(INDEX))
      HOST2 = os.getenv('A{0}_HOST'.format(INDEX + 1))
      PORT2 = os.getenv('A{0}_PORT'.format(INDEX + 1))
      HAGROUP = '{0}_HA{1}{2}'.format(MQKEY,INDEX,INDEX + 1)
      CLUSTERNAME = '{0}_CLUSTER'.format(MQKEY)
      failures = verifystatus(failures,HOST1,PORT1,"Running (production)",True,"PRIMARY",HAGROUP,True,CLUSTERNAME,"Active",int(A_COUNT)/2-1)
      failures = verifystatus(failures,HOST2,PORT2,"Standby",True,"STANDBY",HAGROUP,True,CLUSTERNAME,"Standby",0)
      INDEX += 2

   if (failures == 0): 
      trace(log_file, "Test result is Success!")
   else:
      trace(log_file, "TEST FAILED:  " + str(failures) + " failures occured")



if __name__ == "__main__":
   main(sys.argv[1:])
