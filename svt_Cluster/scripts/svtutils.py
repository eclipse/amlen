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

def shell_source(script):
   if os.path.isfile(script):
      pipe = subprocess.Popen(". %s; env" % script, stdout=subprocess.PIPE, shell=True)
      output = pipe.communicate()[0]
      env = dict((line.split("=", 1) for line in output.splitlines()))
      os.environ.update(env)


def trace(log_file, message):
   formatting_string = '[%Y %m-%d %H:%M-%S00]'
   timestamp = time.strftime(formatting_string)
   print >>log_file, '\n%s %s' % (timestamp, message)

def post(url,payload):
   rc = 0	  

   print "curl -X POST \'" + payload + "\' " + url
   
   try:
      response = requests.post(url, data=payload)
      print response.json()
      print
      rc = response.status_code
   except requests.exceptions.RequestException as err:
      print(err)
      rc = -1

   return rc

def configha(SERVER,PORT,GROUP,A1_HA0,A1_HA1,A2_HA0,PREFERRED):
   rc = 0
   url = 'http://{0}:{1}/ima/v1/configuration'.format(SERVER,PORT)
   payload = '{"HighAvailability":{"EnableHA":true,"Group":"'+GROUP+'","LocalDiscoveryNIC": "'+A1_HA0+'","LocalReplicationNIC": "'+A1_HA1+'","RemoteDiscoveryNIC": "'+A2_HA0+'","PreferredPrimary": '+PREFERRED+', "HeartbeatTimeout": 60}}'
 
   rc = post(url, payload)
   return rc

def configcluster(SERVER,PORT,CLUSTER,L1,L2,A1):
   rc = 0
   url = 'http://{0}:{1}/ima/v1/configuration'.format(SERVER,PORT)
   payload = '{"ClusterMembership":{"EnableClusterMembership":true,"UseMulticastDiscovery":true,"ControlPort":9201,"ControlAddress":"'+A1+'","MessagingPort":9202,"ClusterName":"'+CLUSTER+'","DiscoveryPort":9106,"DiscoveryServerList":"'+L1+':9201,'+L2+':9201"}}' 

   rc = post(url, payload)
   return rc

def maintenancestop(SERVER,PORT):
   rc = 0
   url = 'http://{0}:{1}/ima/v1/service/restart'.format(SERVER,PORT)
   payload = '{"Service":"Server","Maintenance":"stop"}'

   rc = post(url, payload)
   return rc

def cleanstore(SERVER,PORT):
   rc = 0
   url = 'http://{0}:{1}/ima/v1/service/restart'.format(SERVER,PORT)
   payload = '{"Service":"Server","CleanStore":true}'

   rc = post(url, payload)
   return rc

def disableha(SERVER,PORT):
   rc = 0
   url = 'http://{0}:{1}/ima/v1/configuration'.format(SERVER,PORT)
   payload = '{"HighAvailability":{"EnableHA":false}}' 

   rc = post(url, payload)
   return rc

def enablecluster(SERVER,PORT):
   rc = 0
   url = 'http://{0}:{1}/ima/v1/configuration'.format(SERVER,PORT)
   payload = '{"ClusterMembership":{"EnableClusterMembership":true}}' 

   rc = post(url, payload)
   return rc

def disablecluster(SERVER,PORT):
   rc = 0
   url = 'http://{0}:{1}/ima/v1/configuration'.format(SERVER,PORT)
   payload = '{"ClusterMembership":{"EnableClusterMembership":false}}' 

   rc = post(url, payload)
   return rc

def restart(SERVER,PORT):
   rc = 0
   url = 'http://{0}:{1}/ima/v1/service/restart'.format(SERVER,PORT)
   payload = '{"Service":"Server"}' 

   rc = post(url, payload)
   return rc

def getstatus(SERVER,PORT):
   response = ""
   rc = 0
   maxwait=180
   wait=5
   retry="true"
   start=time.time()

   url = 'http://{0}:{1}/ima/v1/service/status'.format(SERVER,PORT)

   now = time.time()
   while ((retry == "true") and ((now-start) < int(maxwait) )):
      try:
         print "curl -X GET " + url
         response = requests.get(url)
         print response.json()
         print
#        print "response = requests.get("+url+", headers=headers)"
         retry="false"
      except requests.exceptions.Timeout:
#        print "except requests.exceptions.Timeout"
         print "time.sleep(" + wait + ")"
         time.sleep(float(wait))
         now = time.time()
      except requests.exceptions.TooManyRedirects:
#        print "except requests.exceptions.TooManyRedirects"
         sys.exit(3)
      except requests.exceptions.RequestException as err:
#        print "except requests.exceptions.RequestException as err"
         print(err)
         print "time.sleep(" + str(wait) + ")"
         time.sleep(float(wait))
         now = time.time()

   return response

def getmonitorcluster(SERVER,PORT):
   response = ""
   rc = 0
   maxwait=180
   wait=5
   retry="true"
   start=time.time()

   url = 'http://{0}:{1}/ima/v1/monitor/Cluster'.format(SERVER,PORT)

   now = time.time()
   while ((retry == "true") and ((now-start) < int(maxwait) )):
      try:
         print "curl -X GET " + url
         response = requests.get(url)
         print response.json()
         print
#        print "response = requests.get("+url+", headers=headers)"
         retry="false"
      except requests.exceptions.Timeout:
#        print "except requests.exceptions.Timeout"
         print "time.sleep(" + wait + ")"
         time.sleep(float(wait))
         now = time.time()
      except requests.exceptions.TooManyRedirects:
#        print "except requests.exceptions.TooManyRedirects"
         sys.exit(3)
      except requests.exceptions.RequestException as err:
#        print "except requests.exceptions.RequestException as err"
         print(err)
         print "time.sleep(" + str(wait) + ")"
         time.sleep(float(wait))
         now = time.time()

   return response
   
def getqueues(SERVER,PORT):
   response = ""
   rc = 0
   maxwait=180
   wait=5
   retry="true"
   start=time.time()

   url = 'http://{0}:{1}/ima/v1/configuration/Queue'.format(SERVER,PORT)

   now = time.time()
   while ((retry == "true") and ((now-start) < int(maxwait) )):
      try:
         print "curl -X GET " + url
         response = requests.get(url)
         print response.json()
         print
#        print "response = requests.get("+url+", headers=headers)"
         retry="false"
      except requests.exceptions.Timeout:
#        print "except requests.exceptions.Timeout"
         print "time.sleep(" + wait + ")"
         time.sleep(float(wait))
         now = time.time()
      except requests.exceptions.TooManyRedirects:
#        print "except requests.exceptions.TooManyRedirects"
         sys.exit(3)
      except requests.exceptions.RequestException as err:
#        print "except requests.exceptions.RequestException as err"
         print(err)
         print "time.sleep(" + str(wait) + ")"
         time.sleep(float(wait))
         now = time.time()

   data = response.json()
#  print data
   queues = data.get('Queue')

   return queues


def deletequeues(SERVER,PORT):
   queues = ""

   queues = getqueues(SERVER,PORT)
   for key,val in queues.items():
      url = 'http://{0}:{1}/ima/v1/configuration/Queue/{2}'.format(SERVER,PORT,key)
      print "curl -X DELETE " + str(url) 
      response = requests.delete(url)
      print response.json()
      print

   return 

def waitonstatus(SERVER,PORT):
   response = ""
   status = ""
   maxwait = 300
   wait = 5
   start = time.time()

   now = time.time()
   while ((status != "Running") and ((now-start) < int(maxwait) )):
      response = getstatus(SERVER,PORT)
      data = response.json()
      status = data.get('Server',[])['Status']
      print "status is '"+ status + "'"
      print
      if (status != "Running"):
#        print SERVER+"  status is "+ status+", time.sleep(" + str(wait) + ")"
         time.sleep(float(wait))
         now = time.time()
      else:
         break

def waitonhastatus(SERVER,PORT):
   response = ""
   harole = "UNSYNC"
   maxwait = 300
   wait = 5
   start = time.time()

   now = time.time()
   while (((harole == "UNSYNC") or (harole == "UNSYNC_ERROR")) and ((now-start) < int(maxwait) )):
      response = getstatus(SERVER,PORT)
      data = response.json()
      ha = data.get('HighAvailability',[])
      if 'NewRole' in ha:
         harole = ha['NewRole']
         print "harole is '"+ harole + "'"
         print
         if (harole == "UNSYNC"):
            print SERVER+"  harole is UNSYNC, time.sleep(" + str(wait) + ")"
            time.sleep(float(wait))
            now = time.time()
         else:
            break
      else:
        print "sleep(" + str(wait) + ")"
        time.sleep(float(wait))
        now = time.time()

def waitonclusterstatus(SERVER,PORT):
   response = ""
   clusterstatus = "Initializing"
   maxwait = 300
   wait = 5
   start = time.time()

   now = time.time()
   while ((clusterstatus == "Initializing") and ((now-start) < int(maxwait) )):
      response = getstatus(SERVER,PORT)
      data = response.json()
      clusterstatus = data.get('Cluster',[])['Status']
      print "clusterstatus is '" + clusterstatus + "'"
      print
      if (clusterstatus == "Initializing"):
         print SERVER+"  clusterstatus is Initializing,  time.sleep(" + str(wait) + ")"
         time.sleep(float(wait))
         now = time.time()
      else:
         break


def makestandby(SERVER,PORT):
   response = ""

   response = getstatus(SERVER,PORT)
   data = response.json()

   harole = data.get('HighAvailability',[])['NewRole']
   if (harole == "PRIMARY"):
      restart(SERVER,PORT)
   else:
      print SERVER + " role not PRIMARY.  Role is " + harole + ". Restart NOT necessary"
      print


def verifystatus(failures,SERVER,PORT,STATEDESCRIPTION,HAENABLED,HAROLE,HAGROUP,CLUSTERENABLED,CLUSTERNAME,CLUSTERSTATUS,CONNECTEDSERVERS):
   SERVERSTATUS = "Running"
   HASTATUS = "Active"
   DISCONNECTEDSERVERS = 0

   response = getstatus(SERVER,PORT)
   data = response.json()

#  print SERVER+":"+PORT

   serverstatus = data.get('Server',[])['Status']
   if (serverstatus == SERVERSTATUS):
     print "PASS: serverstatus == '" + SERVERSTATUS + "'"
   else:
     failures += 1
     print "FAIL: serverstatus is '" + serverstatus + "'"

   statedescription = data.get('Server',[])['StateDescription']
   if (statedescription == STATEDESCRIPTION):
     print "PASS: statedescription == '" + STATEDESCRIPTION + "'"
   else:
     failures += 1
     print "FAIL: statedescription is '" + statedescription + "' not '" + STATEDESCRIPTION +"'"

   haenabled = data.get('HighAvailability',[])['Enabled']
   if (haenabled == HAENABLED):
     print "PASS: haenabled == '" + str(HAENABLED) + "'"
   else:
     failures += 1
     print "FAIL: haenabled is '" + str(haenabled) + "'"

   if (haenabled == True):
     hagroup = data.get('HighAvailability',[])['Group']
     if (hagroup == HAGROUP):
       print "PASS: hagroup == '" + HAGROUP + "'"
     else:
       failures += 1
       print "FAIL: hagroup is '" + hagroup + "'"
  
     harole = data.get('HighAvailability',[])['NewRole']
     if (harole == HAROLE):
       print "PASS: harole == '" + HAROLE + "'"
     else:
       failures += 1
       print "FAIL: harole is '" + harole + "'"

     hastatus = data.get('HighAvailability',[])['Status']
     if (hastatus == HASTATUS):
       print "PASS: hastatus == '" + HASTATUS + "'"
     else:
       failures += 1
       print "FAIL: hastatus is '" + hastatus + "'"

   clusterenabled = data.get('Cluster',[])['Enabled']
   if (clusterenabled == CLUSTERENABLED):
     print "PASS: clusterenabled == " + str(CLUSTERENABLED)
   else:
     failures += 1
     print "FAIL: clusterenabled is " + str(clusterenabled)

   if (clusterenabled == True):
     clustername = data.get('Cluster',[])['Name']
     if (clustername == CLUSTERNAME):
       print "PASS: clustername == '" + CLUSTERNAME + "'"
     else:
       failures += 1
       print "FAIL: clustername is '" + clustername + "'"
  
     clusterstatus = data.get('Cluster',[])['Status']
     if (clusterstatus == CLUSTERSTATUS):
       print "PASS: clusterstatus == '" + CLUSTERSTATUS + "'"
     else:
       failures += 1
       print "FAIL: clusterstatus is '" + clusterstatus + "'"
  
     connectedservers = data.get('Cluster',[])['ConnectedServers']
     if (connectedservers == CONNECTEDSERVERS):
       print "PASS: connectedservers == " + str(CONNECTEDSERVERS)
     else:
       failures += 1
       print "FAIL: connectedservers is " + str(connectedservers)
  
     disconnectedservers = data.get('Cluster',[])['DisconnectedServers']
     if (disconnectedservers == DISCONNECTEDSERVERS):
       print "PASS: disconnectedservers == " + str(DISCONNECTEDSERVERS)
     else:
       failures += 1
       print "FAIL: disconnectedservers is " + str(disconnectedservers)

     if ((haenabled == True) and (harole == "PRIMARY")):
# curl -X GET http://10.160.190.181:5003/ima/v1/monitor/Cluster
       print
       response = getmonitorcluster(SERVER,PORT)
       data = response.json()

       cluster = data.get('Cluster',[])
       if (len(cluster) == CONNECTEDSERVERS):
         print "PASS: len(Cluster[]) == " + str(CONNECTEDSERVERS)
       else:
         failures += 1
         print "FAIL: len(Cluster[]) is " + str(len(cluster))
  
       for node in cluster:	 
         status = node.get('Status',[])
         if (status == "Active"):
           print "PASS: monitor cluster status == '" + status + "'"
         else:
           failures += 1
           print "FAIL: monitor cluster status is '" + status + "'"
  
         health = node.get('Health',[])
         if (health == "Green"):
           print "PASS: monitor cluster health == '" + health + "'"
         else:
           failures += 1
           print "FAIL: monitor cluster health is '" + health + "'"
  
         hastatus = node.get('HAStatus',[])
         if (haenabled == True):
           if (hastatus == "Pair"):
             print "PASS: monitory cluster hastatus == '" + hastatus + "'"
           else:
             failures += 1
             print "FAIL: monitor cluster hastatus is '" + hastatus + "'"
         else:
           if (hastatus == "None"):
             print "PASS: monitor cluster hastatus == '" + hastatus + "'"
           else:
             failures += 1
             print "FAIL: monitor cluster hastatus is '" + hastatus + "'"

   print
   return failures


