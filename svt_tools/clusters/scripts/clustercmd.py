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
#  Runs a command on each of the nodes in cluster.json
#----------------------------------------------------------------------------


import os, sys, json, getopt, re, subprocess
from subprocess import Popen, PIPE


#----------------------------------------------------------------------------
#  Pass one of the variable names below as the -o parameter
#----------------------------------------------------------------------------

status='curl -X GET  http://{ADMINHOST}:{ADMINPORT}/ima/v1/service/status'
ClusterMembership='curl -X GET http://{ADMINHOST}:{ADMINPORT}/ima/v1/configuration/ClusterMembership'
LDAP='curl -X GET  http://{ADMINHOST}:{ADMINPORT}/ima/v1/configuration/LDAP'
EnableCluster='curl  -X POST -d \'{{\"ClusterMembership\":{{\"EnableClusterMembership\":true }}}}\' http://{ADMINHOST}:{ADMINPORT}/ima/v1/configuration'
DisableCluster='curl  -X POST -d \'{{\"ClusterMembership\":{{\"EnableClusterMembership\":false }}}}\' http://{ADMINHOST}:{ADMINPORT}/ima/v1/configuration'
restart='curl -X POST -d \'{{\"Service\":\"Server\",\"CleanStore\":false}}\' http://{ADMINHOST}:{ADMINPORT}/ima/v1/service/restart'
CleanStore='curl -X POST -d \'{{\"Service\":\"Server\",\"CleanStore\":true}}\' http://{ADMINHOST}:{ADMINPORT}/ima/v1/service/restart'
dockerstatus='ssh {HOST} docker ps'
dockerstop='ssh {HOST} docker stop {CONTAINERID}'
dockerstart='ssh {HOST} docker start {CONTAINERID}'
dockerrestart='ssh {HOST} docker restart {CONTAINERID}'
dockerlscoresdir='ssh {HOST} docker exec {CONTAINERID} ls /var/messagesight/diag/cores'


def usage(err):
   print(err)
   print ''
   print __file__ + ' -c <ClusterName> -o <Option>'
   print 'where <Option> is:'
   print '  status                GET service/status on each node'
   print '  ClusterMembership     GET configuration/ClusterMembership on each node'
   print '  LDAP                  GET configuration/LDAP on each node'
   print '  EnableCluster         EnableClusterMembership:true on each node'
   print '  DisableCluster        EnableClusterMembership:false on each node'
   print '  restart               service/restart restAPI on each node'
   print '  CleanStore            service/restart CleanStore:true on each node'
   print '  dockerstatus          ssh docker ps on each node'
   print '  dockerstop            ssh docker stop on each node'
   print '  dockerstart           ssh docker start on each node'
   print '  dockerrestart         ssh docker restart on each node'
   print '  dockerlscoresdir      ssh docker exec ls /var/messagesight/diag/cores on each node'
   sys.exit(2)


def main(argv):
   CLUSTER = ''

   try:
      OPTIONS, OPTARGS = getopt.getopt(argv,"c:o:",["cluster=","option="])
   except getopt.GetoptError as err:
      usage(err)

   for OPTION, OPTARG in OPTIONS:
      if OPTION in ('-c', "--cluster"):
         CLUSTER = OPTARG
      if OPTION in ('-o', "--option"):
         COMMAND = OPTARG

   if CLUSTER == '':
      usage("ClusterName not specifed")

   if COMMAND == '':
      usage("Option not specifed")

#  print CMD

   path = os.path.dirname(os.path.realpath(__file__))
   test = re.sub(r"test/.*","test",path)

   cluster_file = '{0}/clusters/{1}/cluster.json'.format(test, CLUSTER)
#  print cluster_file

   if os.path.isfile(cluster_file) == False:
      usage("File not found:  "+cluster_file)

   with open(cluster_file, 'r') as data_file:
      data = json.load(data_file)

   for client, values in data['appliances'].iteritems():
      if 'containerid' in values:

         CONTAINERID = values['containerid']
         HOSTA = values['host']
         HOST = subprocess.Popen([test+"/scripts/resolveaddr.py", "-a", HOSTA], stdout=PIPE).communicate()[0].strip()
         ADMINPORT = values['Endpoint']['admin']['Port']
         ADMINHOSTA = values['Endpoint']['admin']['Interface']
         ADMINHOST = subprocess.Popen([test+"/scripts/resolveaddr.py", "-a", ADMINHOSTA], stdout=PIPE).communicate()[0].strip()
#        print eval(COMMAND)
         CMD=eval(COMMAND).format(ADMINHOST=ADMINHOST,ADMINPORT=ADMINPORT,HOST=HOST,CONTAINERID=CONTAINERID)
         print CMD
         os.system(CMD)
         print ''

if __name__ == "__main__":
   main(sys.argv[1:])
