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


import os, sys, json, getopt, re, subprocess
from subprocess import Popen, PIPE


def usage(err):
   print(err)
   print __file__ + ' -c <ClusterName> -e <Endpoint>'
   sys.exit(2)


def main(argv):
   CLUSTER = ''
   ENDPOINT = ''

   try:
      OPTIONS, OPTARGS = getopt.getopt(argv,"c:e:",["cluster=","endpoint="])
   except getopt.GetoptError as err:
      usage(err)

   for OPTION, OPTARG in OPTIONS:
      if OPTION in ('-c', "--cluster"):
         CLUSTER = OPTARG
      if OPTION in ('-e', "--endpoint"):
         ENDPOINT = OPTARG

   if CLUSTER == '':
      usage("ClusterName not specifed")

   if ENDPOINT == '':
      usage("Endpoint not specifed")

   path = os.path.dirname(os.path.realpath(__file__))
   test = re.sub(r"test/.*","test",path)

   cluster_file = '{0}/clusters/{1}/cluster.json'.format(test, CLUSTER)
#  print cluster_file

   with open(cluster_file, 'r') as data_file:
      data = json.load(data_file)

   for client, values in data['ima-server']['appliances'].iteritems():
      if type(values) is dict:
         if values['host'] != '':
#           print values['host']
            ADDR = subprocess.Popen(["grep " + values['host'] + " /etc/hosts | cut -d' ' -f1", ""], stdout=PIPE, shell=True).communicate()[0].strip()
            print '{0}:{1}'.format(ADDR,values['Endpoint'][ENDPOINT]['Port'])



if __name__ == "__main__":
   main(sys.argv[1:])
