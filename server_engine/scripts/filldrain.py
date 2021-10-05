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
import os
import resource
import subprocess
import shlex
import time
import sys
import select
import csv

server_address  = '9.20.87.51'
server_mqttport = '16102'
ld_library_path = '/home/changeme/mqttbench/output/server_ship/lib'
mqttbench_command = "/home/changeme/perftools/mqttbench"

drainargs = " -d 0 -rx 1:0:t:400:2500:1:0"
fillargs = " -d 0 -tx 1:0:t:1000000:5:2:0 -rrs -s 4096-4096 -r 100000"

def setup():
   os.environ['IMAClient']='MQTT'
   os.environ['PipeCommands'] = '0'
   os.environ['BatchingDelay'] = '1'
   os.environ['LargeConn'] = '1'
   os.environ['DelayTime'] = '1'
   os.environ['DelayCount'] = '1'
   os.environ['UseSecureConn'] = '0'
   os.environ['CERTSIZE'] = '2048'

   os.environ['IMAServer'] = server_address
   os.environ['IMAPort'] = server_mqttport
   os.environ['LD_LIBRARY_PATH'] = ld_library_path

   resource.setrlimit(resource.RLIMIT_NOFILE, [4096, 4096])
   
def makesubs():
    print "filldrain.py:  Starting to make subs at ", time.asctime( time.localtime(time.time()) )
    cmdline = mqttbench_command + drainargs
    args = shlex.split(cmdline)
    p = subprocess.Popen(args,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    
    makeSubsFinished = False
    while not makeSubsFinished:
       reads = [p.stdout.fileno(), p.stderr.fileno()]
       ret = select.select(reads, [], [])
 
       readlines = []

       for fd in ret[0]:
           if fd == p.stdout.fileno():
              inline = p.stdout.readline()
              sys.stdout.write('makesubs stdout: ' + inline)
              readlines.append(inline)
           if fd == p.stderr.fileno():
              inline = p.stderr.readline()
              sys.stderr.write('makesubs stderr: ' + inline)    
              readlines.append(inline)

       
       for line in readlines: 
          if "Successfully have created all subscriptions" in line:
              makeSubsFinished = True
              break
              
       if p.poll() != None:
          makeSubsFinished = True
          break

          
    print "filldrain.py:  Ending make subs"       
    p.communicate("q\n")
    
    print 'filldrain.py:  make subs completed with returncode ',str(p.returncode), ' at ',time.asctime( time.localtime(time.time()) )
    return  p.returncode
    
def areThereConnections():
    cmdline = 'sshpass -p admin ssh admin@'+server_address+' imaserver stat Connection'
    args = shlex.split(cmdline)
    
    ConnectionsFound = True
    while ConnectionsFound:
        print 'filldrain.py: Checking for connections at ',time.asctime( time.localtime(time.time()) )
        p = subprocess.Popen(args,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    
        commandFinished = False
        
        while not commandFinished:
            reads = [p.stdout.fileno(), p.stderr.fileno()]
            ret = select.select(reads, [], [])
 
            readlines = []

            for fd in ret[0]:
                if fd == p.stdout.fileno():
                   inline = p.stdout.readline()
                   sys.stdout.write('connectioncheck stdout: ' + inline)
                   readlines.append(inline)
                if fd == p.stderr.fileno():
                   inline = p.stderr.readline()
                   sys.stderr.write('connectioncheck stderr: ' + inline)    
                   readlines.append(inline)

            if p.poll() != None:
                commandFinished = True
       
            for line in readlines: 
                if "No connection data is found" in line:
                    ConnectionsFound = False
                    
        if p.returncode > 1:
            #It currently seems to fail with rc=1 but still work :-S
            print 'Exiting as sshpass command to check for connections, failed with rc ',p.returncode
            print 'NB: The host ',server_address,' must be in known hosts for this to work'
            sys.exit(p.returncode)
    
        if ConnectionsFound:
            #We're going to loop... wait a while
            print "Connections Present... waiting for a bit to see if they remain.."
            time.sleep(15)
     
    print
    print 'filldrain.py: ConnectionsFound: ', ConnectionsFound,' at ', time.asctime( time.localtime(time.time()) )
    return ConnectionsFound
    
def fillappliance(fillnumber):
    strfillnum = str(fillnumber)
    print 'filldrain.py:  Starting fill ',strfillnum, ' at ', time.asctime( time.localtime(time.time()) )

    cmdline = mqttbench_command + fillargs    
    args = shlex.split(cmdline)
    p = subprocess.Popen(args,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    
    commandFinished = False
    
    while not commandFinished:
        reads = [p.stdout.fileno(), p.stderr.fileno()]
        ret = select.select(reads, [], [])
 
        readlines = []

        for fd in ret[0]:
            if fd == p.stdout.fileno():
               inline = p.stdout.readline()
               sys.stdout.write('fillappliance '+strfillnum+ ' stdout: ' + inline)
               readlines.append(inline)
            if fd == p.stderr.fileno():
               inline = p.stderr.readline()
               sys.stderr.write('fillappliance '+strfillnum+ ' stderr: ' + inline)    
               readlines.append(inline)

        if p.poll() != None:
            commandFinished = True
    
        for line in readlines: 
            if (("Producer didn't disconnect cleanly" in line) or 
                ("Shutting down connection" in line)):
                p.kill()
                commandFinished = True
    
    print 'filldrain.py: Finishing fill ',strfillnum, ' at ', time.asctime( time.localtime(time.time()) )
    return
    
def isDrained():
    cmdline = 'sshpass -p admin ssh admin@'+server_address+' imaserver stat Subscription ResultCount=10 StatType=BufferedMsgsHighest'
    args = shlex.split(cmdline)
    
    print 'filldrain.py: Checking whether drained at ',time.asctime( time.localtime(time.time()) )
    p = subprocess.Popen(args,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    
    AllDrained = True
    
    statsCSV=[]
    
    commandFinished = False
    
    while not commandFinished:
        reads = [p.stdout.fileno(), p.stderr.fileno()]
        ret = select.select(reads, [], [])
 
        readlines = []

        for fd in ret[0]:
            if fd == p.stdout.fileno():
               inline = p.stdout.readline()
               sys.stdout.write('isDrained stdout: ' + inline)
               readlines.append(inline)
               statsCSV.append(inline)
            if fd == p.stderr.fileno():
               inline = p.stderr.readline()
               sys.stderr.write('isDrained stderr: ' + inline)    
               readlines.append(inline)

        if p.poll() != None:
            commandFinished = True
                    
    if p.returncode > 1:
        #It currently seems to fail with rc=1 but still work :-S
        print 'Exiting as sshpass command in isDrained, failed with rc ',p.returncode
        print 'NB: The host ',server_address,' must be in known hosts for this to work'
        sys.exit(p.returncode)

    print
    
    doneHeader=False
    statsreader = csv.reader(statsCSV, delimiter=',', quotechar='"')
    
    for row in statsreader:
       if not doneHeader:
          if len(row) >= 5:
              if row[4] and row[4] != "BufferedMsgs":
                 print "Failed to parse substats CSV"
                 sys.exit(20)
              else:
                 doneHeader=True
          else:
               print "Non-CSVy row: ",row
       else:
          if len(row) >= 5:
             if row[4] and int(row[4]) > 0:
                 AllDrained = False
             #else:
             #    print 'Found ',str(row[4]),' buffered messages on a sub'
          else:
               print "Non-CSVy row: ",row
    
    if not doneHeader:
        print "Failed o parse CSV....never found header row"
        sys.exit(21)
        
    print 'filldrain.py: AllDrained: ', AllDrained,' at ', time.asctime( time.localtime(time.time()) )
    return AllDrained
    
def drainappliance(drainnumber):
    strdrainnum = str(drainnumber)
    print 'filldrain.py:  Starting drain ',strdrainnum, ' at ', time.asctime( time.localtime(time.time()) )

    cmdline = mqttbench_command + drainargs    
    args = shlex.split(cmdline)
    drainproc = subprocess.Popen(args,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    
    commandFinished = False
    lastdrainchecktime = 0
    
    while not commandFinished:
        reads = [drainproc.stdout.fileno(), drainproc.stderr.fileno()]
        ret = select.select(reads, [], [], 10) #timeout after 10seconds
 
        readlines = []

        for fd in ret[0]:
            if fd == drainproc.stdout.fileno():
               inline = drainproc.stdout.readline()
               sys.stdout.write('drainappliance '+strdrainnum+ ' stdout: ' + inline)
               readlines.append(inline)
            if fd == drainproc.stderr.fileno():
               inline = drainproc.stderr.readline()
               sys.stderr.write('drainappliance '+strdrainnum+ ' stderr: ' + inline)    
               readlines.append(inline)

        if drainproc.poll() != None:
            commandFinished = True
            
        if (time.time() - lastdrainchecktime) >= 10:
            lastdrainchecktime = time.time()
            doneDrain = isDrained()
            
            if doneDrain:
               print 'Stopping drain....'
               drainproc.communicate("q\n")
               commandFinished = True
              
    print 'filldrain.py: Finishing drain ',strdrainnum, ' at ', time.asctime( time.localtime(time.time()) )
    return
    
def filldrainloop():    
    setup()

    rc = makesubs()
    if rc != 0:
       return rc
    
    connectionsFound = areThereConnections()
    
    if connectionsFound:
       print "Connections still found after making subs... giving up"
       return 10 

    for fillnum in range(1,11):
       fillappliance(fillnum)
       drainappliance(fillnum)
       
       connectionsFound = areThereConnections()
    
       if connectionsFound:
           print 'Connections still found after drain',fillnum,'... giving up'
           return 10 
    
    print "Done 10 fill/drain cycles... seems ok"
      
rc = filldrainloop()
sys.exit(rc)
