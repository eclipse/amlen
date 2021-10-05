#!/usr/bin/python
# Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

import sys
import gdb

def dumpTraceHistory(outfile,rawdump=False):
   
   if not rawdump:
       outhandle=open(outfile, 'w+')

   threadData = gdb.parse_and_eval('ismEngine_serverGlobal.threadDataHead')
   
   while threadData is not None and str(threadData) != '0x0':
      if rawdump:
          rawDumpThreadTraceData(outfile, threadData, "active")
      else:
          dumpThreadTraceData(outhandle, threadData, "active")
      threadData = threadData['next']

   #If we have the number of stored threads in a var use it, otherwise work it out
   numStored = 0
   try:
      numStored = int(gdb.parse_and_eval('g_ieutTRACEHISTORY_SAVEDTHREADS'))
   except:
      numStored = int(gdb.parse_and_eval('sizeof(ismEngine_serverGlobal.storedThreadData)/sizeof(ismEngine_StoredThreadData_t)'))
   
   for entry in range(0, numStored):
      threadExpr = '&ismEngine_serverGlobal.storedThreadData['+str(entry)+']'
      storedThread = gdb.parse_and_eval(threadExpr)
      
      if rawdump:
          rawDumpThreadTraceData(outfile, storedThread, "saved")
      else:
          dumpThreadTraceData(outhandle, storedThread, "saved")

def dumpThreadTraceData(outhdl, threadData, prefix):
    print("Parsing new thread")
    try:
      print("NEWTHREAD-"+prefix+": ",threadData, file=outhdl)

      startingTracePoint = threadData['traceHistoryBufPos']
      currTracePoint = startingTracePoint
      keepGoing = True
      threadHistoryBuffSize = gdb.parse_and_eval('g_ieutTRACEHISTORY_BUFFERSIZE')
      while keepGoing:
         if threadData['traceHistoryIdent'][currTracePoint] != 0:
             print('TP[{0},{1}]: {2} {3}'.format(threadData, currTracePoint,
                                                       threadData['traceHistoryIdent'][currTracePoint],
                                                       threadData['traceHistoryValue'][currTracePoint]), file=outhdl)

         currTracePoint = (currTracePoint +1) % threadHistoryBuffSize

         if currTracePoint == startingTracePoint:
            keepGoing = False
    except Exception as err:
      print("Exception processing threadData for threadData at: ", threadData)
      print(str(err))
    
    print("ENDTHREAD: ", threadData, file=outhdl)
    print("\n\n", file=outhdl)

def rawDumpThreadTraceData(outfile, threadData, prefix):
    print("RawDumping new thread")
    
    #Converting the threadData to a string can result in strings like:
    #0x7ffb0bb2f1b0 <ismEngine_serverGlobal+16777840>
    #So we split out first word to get just raw, absolute address
    threadDataAddr = str(threadData).split()[0]
    #print("Abs addr: ", threadDataAddr)
    
    try:
      startingTracePoint = threadData['traceHistoryBufPos']
      threadHistoryBuffSize = gdb.parse_and_eval('g_ieutTRACEHISTORY_BUFFERSIZE')
      
      filename = (outfile+"thread_"+threadDataAddr+"_"+prefix+"_"+
                    str(startingTracePoint)+"_"+str(threadHistoryBuffSize)+
                    ".rawconcattrc")

      identStartAddr = str(threadData['traceHistoryIdent'][0].address).split()[0]
      identEndAddr   = str(threadData['traceHistoryIdent'][threadHistoryBuffSize].address).split()[0]
          
      gdbcmd = "dump binary memory "+filename+" "+identStartAddr+" "+identEndAddr
      #print("First dump ", gdbcmd)
      gdb.execute(gdbcmd)
    
      valueStartAddr = str(threadData['traceHistoryValue'][0].address).split()[0]
      valueEndAddr   = str(threadData['traceHistoryValue'][threadHistoryBuffSize].address).split()[0]
      
      gdbcmd = "append binary memory "+filename+" "+valueStartAddr+" "+valueEndAddr
      #print(gdbcmd)
      gdb.execute(gdbcmd)
    except Exception as err:
      print("Exception rawdump processing threadData for threadData at: ", threadData)
      print(str(err))


def valStr(valueobj):
   try:
      thestring = valueobj.string("utf-8")
   except:
      thestring = str(valueobj)
      
   return thestring
