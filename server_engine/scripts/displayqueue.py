#!/usr/bin/python
# Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

def displayQueue(expr,level=5,outfile=None):

   if outfile is None:
      outhandle=sys.stdout
   else:
      outhandle=open(outfile, 'w+')

   queue = gdb.parse_and_eval('(ismQHandle_t)('+str(expr)+')')
   
   if str(queue['QType']) == 'multiConsumer':
      displayMultiConsumerQ(queue, level, outhandle)
   elif str(queue['QType']) == 'simple':
      displaySimpleQ(queue, level, outhandle)
   elif str(queue['QType']) == 'intermediate':
      displayIntermediateQ(queue, level, outhandle)  
   else:
      print "Unknown queue type:",queue['QType'],"\n"
      

def isMultiConsumerNodeLocked(outhdl, queue, node):
   #first work out hash of node
   hashvalue = (((queue['qId'] << 24) ^ (node['orderId'])) % 24593)
 
   #Find The right hash chain....
   lockManager = gdb.parse_and_eval('ismEngine_serverGlobal.LockManager')
   hashchain = lockManager['pLockChains'][0][hashvalue % lockManager['LockTableSize']]

   #Walk the chain
   lockInChain = hashchain['pChainHead']
   
   while lockInChain != 0:
       if (    (lockInChain['LockName']['Common']['LockType'] == 0) 
           and (lockInChain['LockName']['Msg']['NodeId'] ==  node['orderId'])
           and (lockInChain['LockName']['Msg']['QId'] ==  queue['qId'])):
           return True
       lockInChain = lockInChain['pChainNext']
   return False   
   
def getMsgState(state):
    strstate = str(state)
    
    #None and Avail have the same numeric value...
    if strstate == 'ismMESSAGE_STATE_NONE' or   strstate == 'ismMESSAGE_STATE_AVAILABLE':
       return 'avail'
    if strstate == 'ismMESSAGE_STATE_DELIVERED':
       return 'DLVRD'
    if strstate == 'ismMESSAGE_STATE_RECEIVED':
       return 'RCVED'
    if strstate == 'ismMESSAGE_STATE_CONSUMED':
       return 'CONSD'
    if strstate == 'ismMESSAGE_STATE_RESERVED1':
       return 'RSVD1'
    return '?????'    
    
def displayMultiConsumerQueuePages(outhdl, queue, page):
    pageNum = 1
    
    while page != 0:
        print >>outhdl
        print >>outhdl, "     Page:           ", pageNum 
        print >>outhdl, "     Address:        ", page
        print >>outhdl, "     NodesInPage:    ", page['nodesInPage']
        print >>outhdl, "     Next:           ", page['next']
    
        for nodeNum in range(0, page['nodesInPage']):
            node = page['nodes'][nodeNum]
            
            print >>outhdl, "    ",
            print >>outhdl, "oId=",node['orderId'],            
            print >>outhdl, "addr=",node.address,
           
            locked = isMultiConsumerNodeLocked(outhdl, queue, node)
            if locked:
               print >>outhdl," LOCKED ",
            else:
               print >>outhdl,"!locked ",

            print >>outhdl,  getMsgState(node['msgState']),
            print >>outhdl,  "msg=",node['msg'],
            print >>outhdl,  "dlvyCnt=",node['deliveryCount'].cast(gdb.lookup_type('int')), 
            if node['inStore']:
                print >>outhdl, " STORE",
            else:
                print >>outhdl, "!store",
                
            if node['iemqCachedSLEHdr'] != 0:
                print >>outhdl,' DVRLYMEMALLOCD',
            else:
                print >>outhdl,'!dvrlymem      ',
            print >>outhdl,'   ',node['ackData'],
            print >>outhdl,'   ',node['deliveryId'],
            print >>outhdl
        page = page['next']
        pageNum += 1;

def displaySimpleQueuePages(outhdl, queue, page):
    pageNum = 1
    
    while page != 0:
        print >>outhdl
        print >>outhdl, "     Page:           ", pageNum 
        print >>outhdl, "     Address:        ", page
        print >>outhdl, "     NodesInPage:    ", page['nodesInPage']
        print >>outhdl, "     Next:           ", page['next']
    
        for nodeNum in range(0, page['nodesInPage']):
            node = page['nodes'][nodeNum]
            
            print >>outhdl, "    ",
            print >>outhdl, "addr=",node.address,           
            print >>outhdl,  "msg=",node['msg'],
            print >>outhdl,  "msgFlags=",node['msgFlags'].cast(gdb.lookup_type('int')),

            print >>outhdl
        page = page['next']
        pageNum += 1;
  
def displayIntermediateQueuePages(outhdl, queue, page):
    pageNum = 1
    
    while page != 0:
        print >>outhdl
        print >>outhdl, "     Page:           ", pageNum 
        print >>outhdl, "     Address:        ", page
        print >>outhdl, "     NodesInPage:    ", page['nodesInPage']
        print >>outhdl, "     Next:           ", page['next']
    
        for nodeNum in range(0, page['nodesInPage']):
            node = page['nodes'][nodeNum]
            
            print >>outhdl, "    ",
            print >>outhdl, "oId=",node['orderId'],            
            print >>outhdl, "deliveryId=",node['deliveryId'],            
            print >>outhdl, "addr=",node.address,
           
            print >>outhdl,  getMsgState(node['msgState']),
            print >>outhdl,  "msg=",node['msg'],
            
            if node['hasMDR']:
                print >>outhdl, " MDR",
            else:
                print >>outhdl, "!mdr",

            print >>outhdl,  "dlvyCnt=",node['deliveryCount'].cast(gdb.lookup_type('int')), 
            if node['inStore']:
                print >>outhdl, " STORE",
            else:
                print >>outhdl, "!store",
            
            print >>outhdl
        page = page['next']
        pageNum += 1;
        
def displayMultiConsumerQ(queue, level, outhdl=sys.stdout):
    mcqueue = queue.dereference().cast(gdb.lookup_type('iemqQueue_t'))
    print >>outhdl,"Type: MultiConsumer (iemqQueue_t *)",mcqueue.address 
    print >>outhdl,"qId: ",mcqueue['qId'],"QName:",valStr(mcqueue['Common']['QName']),"InternalName:",valStr(mcqueue['InternalName'])
    if level > 1:
        print >>outhdl,"QOptions:        ", mcqueue['QOptions']
        print >>outhdl,"isDeleted:       ", mcqueue['isDeleted']
        print >>outhdl,"getCursor.c:     ", mcqueue['getCursor']['c']
        print >>outhdl,"preDeleteCount:  ", mcqueue['preDeleteCount']
        print >>outhdl,"enqueueCount:    ", mcqueue['enqueueCount']
        print >>outhdl,"dequeueCount:    ", mcqueue['dequeueCount']
        print >>outhdl,"qavoidCount:     ", mcqueue['qavoidCount']
        print >>outhdl,"bufferedMsgsHWM: ", mcqueue['bufferedMsgsHWM']
        print >>outhdl,"rejectedMsgs:    ", mcqueue['rejectedMsgs']
        print >>outhdl,"bufferedMsgs:    ", mcqueue['bufferedMsgs']
        print >>outhdl,"inflightEnqs:    ", mcqueue['inflightEnqs']
        print >>outhdl,"inflightDeqs:    ", mcqueue['inflightDeqs']
        
    if level > 4:
        print >>outhdl,"ReportedQFull:   ", mcqueue['ReportedQueueFull']
        print >>outhdl,"hStoreObj:       ", mcqueue['hStoreObj']
        print >>outhdl,"hStoreProps:     ", mcqueue['hStoreProps']
        print >>outhdl,"QueueRefContext: ", mcqueue['QueueRefContext']
        print >>outhdl,"PageMap:         ", mcqueue['PageMap']
        print >>outhdl,"ackListsUpdating:", mcqueue['ackListsUpdating']          
        print >>outhdl,"deletionRemovesStoreObjects: ", mcqueue['deletionRemovesStoreObjects']
        print >>outhdl,"deletionCompleted: ", mcqueue['deletionCompleted']
        print >>outhdl,"headlock:        ", mcqueue['headlock']
        print >>outhdl,"headPage:        ", mcqueue['headPage']
        print >>outhdl,"deletedStoreRefCount:   ", mcqueue['deletedStoreRefCount']
        print >>outhdl,"putlock:         ", mcqueue['putlock']
        print >>outhdl,"tail:            ", mcqueue['tail']
        print >>outhdl,"nextOrderId:     ", mcqueue['nextOrderId']
        print >>outhdl,"waiterListLock:  ", mcqueue['waiterListLock']
        print >>outhdl,"firstWaiter:     ", mcqueue['firstWaiter']
        print >>outhdl,"numBrowsingWaiters:  ", mcqueue['numBrowsingWaiters']
        print >>outhdl,"numSelectingWaiters: ", mcqueue['numSelectingWaiters']
        print >>outhdl,"checkWaitersVal:     ", mcqueue['checkWaitersVal']
        print >>outhdl,"getlock:             ", mcqueue['getlock']
        print >>outhdl,"scanOrderId:         ", mcqueue['scanOrderId']
        
    if level >= 9:
        displayMultiConsumerQueuePages(outhdl, mcqueue, mcqueue['headPage'])

    print >>outhdl,"\n\n"

def displayIntermediateQ(queue, level, outhdl=sys.stdout):
    iqueue = queue.dereference().cast(gdb.lookup_type('ieiqQueue_t'))
    print >>outhdl,"Type: Intermediate (ieiqQueue_t *)",iqueue.address 
    print >>outhdl,"QName:",valStr(iqueue['Common']['QName']),"InternalName:",valStr(iqueue['InternalName'])
    if level > 1:
        print >>outhdl,"QOptions:        ", iqueue['QOptions']
        print >>outhdl,"isDeleted:       ", iqueue['isDeleted']
        print >>outhdl,"enqueueCount:    ", iqueue['enqueueCount']
        print >>outhdl,"dequeueCount:    ", iqueue['dequeueCount']
        print >>outhdl,"qavoidCount:     ", iqueue['qavoidCount']
        print >>outhdl,"bufferedMsgsHWM: ", iqueue['bufferedMsgsHWM']
        print >>outhdl,"rejectedMsgs:    ", iqueue['rejectedMsgs']
        print >>outhdl,"bufferedMsgs:    ", iqueue['bufferedMsgs']
        print >>outhdl,"inflightEnqs:    ", iqueue['inflightEnqs']
        print >>outhdl,"inflightDeqs:    ", iqueue['inflightDeqs']
        print >>outhdl,"maxInflightDeqs: ", iqueue['maxInflightDeqs']
        print >>outhdl,"qavoidCount:     ", iqueue['qavoidCount']
        print >>outhdl,"waiterStatus     ", iqueue['waiterStatus']
        print >>outhdl,"pConsumer:       (ismEngine_Consumer_t *)", iqueue['pConsumer']
        print >>outhdl,"nextOrderId:     ", iqueue['nextOrderId']        
        print >>outhdl,"cursor:          (ieiqQNode_t *)", iqueue['cursor']
                
    if level > 4:
        print >>outhdl,"ReportedQFull:   ", iqueue['ReportedQueueFull']
        print >>outhdl,"resetCursor  :   ", iqueue['resetCursor']
        print >>outhdl,"Redelivering  :  ", iqueue['Redelivering']
        print >>outhdl,"hStoreObj:       ", iqueue['hStoreObj']
        print >>outhdl,"hStoreProps:     ", iqueue['hStoreProps']
        print >>outhdl,"QueueRefContext: ", iqueue['QueueRefContext']
        print >>outhdl,"PageMap:         ", iqueue['PageMap']
        print >>outhdl,"deletionRemovesStoreObjects: ", iqueue['deletionRemovesStoreObjects']
        print >>outhdl,"deletionCompleted: ", iqueue['deletionCompleted']
        print >>outhdl,"headlock:        ", iqueue['headlock']
        print >>outhdl,"head:            (ieiqQNode_t *)", iqueue['head']       
        print >>outhdl,"headPage:        (ieiqQNodePage_t *)", iqueue['headPage']
        print >>outhdl,"deletedStoreRefCount:   ", iqueue['deletedStoreRefCount']
        print >>outhdl,"putlock:         ", iqueue['putlock']
        print >>outhdl,"tail:            (ieiqQNode_t *)", iqueue['tail']
        print >>outhdl,"redeliverCursorOId:  ", iqueue['redeliverCursorOrderId']
        print >>outhdl,"hMsgDelInfo:     ", iqueue['hMsgDelInfo']
        
    if level >= 9:
        displayIntermediateQueuePages(outhdl, iqueue, iqueue['headPage'])

    print >>outhdl,"\n\n"
    
def displaySimpleQ(queue, level, outhdl=sys.stdout):
    squeue = queue.dereference().cast(gdb.lookup_type('iesqQueue_t'))
    print >>outhdl,"Type: Simple (iesqQueue_t *)",squeue.address 
    print >>outhdl,"QName:",valStr(squeue['Common']['QName'])
    if level > 1:
        print >>outhdl,"QOptions:        ", squeue['QOptions']
        print >>outhdl,"enqueueCount:    ", squeue['enqueueCount']
        print >>outhdl,"dequeueCount:    ", squeue['dequeueCount']
        print >>outhdl,"qavoidCount:     ", squeue['qavoidCount']
        print >>outhdl,"bufferedMsgsHWM: ", squeue['bufferedMsgsHWM']
        print >>outhdl,"rejectedMsgs:    ", squeue['rejectedMsgs']
        print >>outhdl,"bufferedMsgs:    ", squeue['bufferedMsgs']
        print >>outhdl,"qavoidCount:     ", squeue['qavoidCount']
        print >>outhdl,"waiterStatus     ", squeue['waiterStatus']
        print >>outhdl,"pConsumer:       (ismEngine_Consumer_t *)", squeue['pConsumer']
                
    if level > 4:
        print >>outhdl,"ReportedQFull:   ", squeue['ReportedQueueFull']
        print >>outhdl,"hStoreObj:       ", squeue['hStoreObj']
        print >>outhdl,"hStoreProps:     ", squeue['hStoreProps']
        print >>outhdl,"head:            (iesqQNode_t *)", squeue['head']       
        print >>outhdl,"headPage:        (iesqQNodePage_t *)", squeue['headPage']
        print >>outhdl,"putlock:         ", squeue['putlock']
        print >>outhdl,"tail:            (iesqQNode_t *)", squeue['tail']
        
    if level >= 9:
        displaySimpleQueuePages(outhdl, squeue, squeue['headPage'])

    print >>outhdl,"\n\n"

def valStr(valueobj):
   try:
      thestring = valueobj.string("utf-8")
   except:
      thestring = str(valueobj)
      
   return thestring
