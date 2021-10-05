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
   print __file__ + ' -c <ClusterName> -e <Endpoint> -a <appliance>'
   sys.exit(2)


def main(argv):
   CLUSTER = ''
   ENDPOINT = ''
   APPLIANCE = ''

   try:
      OPTIONS, OPTARGS = getopt.getopt(argv,"c:e:a:",["cluster=","endpoint=","appliance="])
   except getopt.GetoptError as err:
      usage(err)

   for OPTION, OPTARG in OPTIONS:
      if OPTION in ('-c', "--cluster"):
         CLUSTER = OPTARG
      if OPTION in ('-e', "--endpoint"):
         ENDPOINT = OPTARG
      if OPTION in ('-a', "--appliance"):
         APPLIANCE = OPTARG

   if CLUSTER == '':
      usage("ClusterName not specifed")

   if ENDPOINT == '':
      usage("Endpoint not specifed")

   if APPLIANCE == '':
      usage("Appliance not specifed")

   path = os.path.dirname(os.path.realpath(__file__))
   test = re.sub(r"test/.*","test",path)

   cluster_file = '{0}/clusters/{1}/cluster.json'.format(test, CLUSTER)
#  print cluster_file

   with open(cluster_file, 'r') as data_file:
      data = json.load(data_file)

   if data['ima-server']['appliances'][APPLIANCE]['Endpoint'][ENDPOINT]['Interface'] == 'All':
      data['ima-server']['appliances'][APPLIANCE]['Endpoint'][ENDPOINT]['Interface'] == data['ima-server']['appliances'][APPLIANCE]['host']

   ADDR = subprocess.Popen(["grep " + data['ima-server']['appliances'][APPLIANCE]['Endpoint'][ENDPOINT]['Interface'] + " /etc/hosts | cut -d' ' -f1", ""], stdout=PIPE, shell=True).communicate()[0].strip()
   print '{0}:{1}'.format(ADDR,data['ima-server']['appliances'][APPLIANCE]['Endpoint'][ENDPOINT]['Port'])



if __name__ == "__main__":
   main(sys.argv[1:])
