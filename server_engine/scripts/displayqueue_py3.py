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
      print("Unknown queue type:",queue['QType'],"\n")
      

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
        print(file=outhdl)
        print("     Page:           ", pageNum, file=outhdl) 
        print("     Address:        ", page, file=outhdl)
        print("     NodesInPage:    ", page['nodesInPage'], file=outhdl)
        print("     Next:           ", page['next'], file=outhdl)
    
        for nodeNum in range(0, int(page['nodesInPage'])):
            node = page['nodes'][nodeNum]
            
            print("    ", end=' ', file=outhdl)
            print("oId=",node['orderId'], end=' ', file=outhdl)            
            print("addr=",node.address, end=' ', file=outhdl)
           
            locked = isMultiConsumerNodeLocked(outhdl, queue, node)
            if locked:
               print(" LOCKED ", end=' ', file=outhdl)
            else:
               print("!locked ", end=' ', file=outhdl)

            print(getMsgState(node['msgState']), end=' ', file=outhdl)
            print("msg=",node['msg'], end=' ', file=outhdl)
            print("dlvyCnt=",node['deliveryCount'].cast(gdb.lookup_type('int')), end=' ', file=outhdl) 
            if node['inStore']:
                print(" STORE", end=' ', file=outhdl)
            else:
                print("!store", end=' ', file=outhdl)
                
            if node['iemqCachedSLEHdr'] != 0:
                print(' DVRLYMEMALLOCD', end=' ', file=outhdl)
            else:
                print('!dvrlymem      ', end=' ', file=outhdl)
            print('   ',node['ackData'], end=' ', file=outhdl)
            print('   ',node['deliveryId'], end=' ', file=outhdl)
            print(file=outhdl)
        page = page['next']
        pageNum += 1;

def displaySimpleQueuePages(outhdl, queue, page):
    pageNum = 1
    
    while page != 0:
        print(file=outhdl)
        print("     Page:           ", pageNum, file=outhdl) 
        print("     Address:        ", page, file=outhdl)
        print("     NodesInPage:    ", page['nodesInPage'], file=outhdl)
        print("     Next:           ", page['next'], file=outhdl)
    
        for nodeNum in range(0, page['nodesInPage']):
            node = page['nodes'][nodeNum]
            
            print("    ", end=' ', file=outhdl)
            print("addr=",node.address, end=' ', file=outhdl)           
            print("msg=",node['msg'], end=' ', file=outhdl)
            print("msgFlags=",node['msgFlags'].cast(gdb.lookup_type('int')), end=' ', file=outhdl)

            print(file=outhdl)
        page = page['next']
        pageNum += 1;
  
def displayIntermediateQueuePages(outhdl, queue, page):
    pageNum = 1
    
    while page != 0:
        print(file=outhdl)
        print("     Page:           ", pageNum, file=outhdl) 
        print("     Address:        ", page, file=outhdl)
        print("     NodesInPage:    ", page['nodesInPage'], file=outhdl)
        print("     Next:           ", page['next'], file=outhdl)
    
        for nodeNum in range(0, int(page['nodesInPage'])):
            node = page['nodes'][nodeNum]
            
            print("    ", end=' ', file=outhdl)
            print("oId=",node['orderId'], end=' ', file=outhdl)            
            print("deliveryId=",node['deliveryId'], end=' ', file=outhdl)            
            print("addr=",node.address, end=' ', file=outhdl)
           
            print(getMsgState(node['msgState']), end=' ', file=outhdl)
            print("msg=",node['msg'], end=' ', file=outhdl)
            
            if node['hasMDR']:
                print(" MDR", end=' ', file=outhdl)
            else:
                print("!mdr", end=' ', file=outhdl)

            print("dlvyCnt=",node['deliveryCount'].cast(gdb.lookup_type('int')), end=' ', file=outhdl) 
            if node['inStore']:
                print(" STORE", end=' ', file=outhdl)
            else:
                print("!store", end=' ', file=outhdl)
            
            print(file=outhdl)
        page = page['next']
        pageNum += 1;
        
def displayMultiConsumerQ(queue, level, outhdl=sys.stdout):
    mcqueue = queue.dereference().cast(gdb.lookup_type('iemqQueue_t'))
    print("Type: MultiConsumer (iemqQueue_t *)",mcqueue.address, file=outhdl) 
    print("qId: ",mcqueue['qId'],"QName:",valStr(mcqueue['Common']['QName']),"InternalName:",valStr(mcqueue['InternalName']), file=outhdl)
    if level > 1:
        print("QOptions:        ", mcqueue['QOptions'], file=outhdl)
        print("isDeleted:       ", mcqueue['isDeleted'], file=outhdl)
        print("getCursor.c:     ", mcqueue['getCursor']['c'], file=outhdl)
        print("preDeleteCount:  ", mcqueue['preDeleteCount'], file=outhdl)
        print("enqueueCount:    ", mcqueue['enqueueCount'], file=outhdl)
        print("dequeueCount:    ", mcqueue['dequeueCount'], file=outhdl)
        print("qavoidCount:     ", mcqueue['qavoidCount'], file=outhdl)
        print("bufferedMsgsHWM: ", mcqueue['bufferedMsgsHWM'], file=outhdl)
        print("rejectedMsgs:    ", mcqueue['rejectedMsgs'], file=outhdl)
        print("bufferedMsgs:    ", mcqueue['bufferedMsgs'], file=outhdl)
        print("inflightEnqs:    ", mcqueue['inflightEnqs'], file=outhdl)
        print("inflightDeqs:    ", mcqueue['inflightDeqs'], file=outhdl)
        
    if level > 4:
        print("ReportedQFull:   ", mcqueue['ReportedQueueFull'], file=outhdl)
        print("hStoreObj:       ", mcqueue['hStoreObj'], file=outhdl)
        print("hStoreProps:     ", mcqueue['hStoreProps'], file=outhdl)
        print("QueueRefContext: ", mcqueue['QueueRefContext'], file=outhdl)
        print("PageMap:         ", mcqueue['PageMap'], file=outhdl)
        print("ackListsUpdating:", mcqueue['ackListsUpdating'], file=outhdl)          
        print("deletionRemovesStoreObjects: ", mcqueue['deletionRemovesStoreObjects'], file=outhdl)
        print("deletionCompleted: ", mcqueue['deletionCompleted'], file=outhdl)
        print("headlock:        ", mcqueue['headlock'], file=outhdl)
        print("headPage:        ", mcqueue['headPage'], file=outhdl)
        print("deletedStoreRefCount:   ", mcqueue['deletedStoreRefCount'], file=outhdl)
        print("putlock:         ", mcqueue['putlock'], file=outhdl)
        print("tail:            ", mcqueue['tail'], file=outhdl)
        print("nextOrderId:     ", mcqueue['nextOrderId'], file=outhdl)
        print("waiterListLock:  ", mcqueue['waiterListLock'], file=outhdl)
        print("firstWaiter:     ", mcqueue['firstWaiter'], file=outhdl)
        print("numBrowsingWaiters:  ", mcqueue['numBrowsingWaiters'], file=outhdl)
        print("numSelectingWaiters: ", mcqueue['numSelectingWaiters'], file=outhdl)
        print("checkWaitersVal:     ", mcqueue['checkWaitersVal'], file=outhdl)
        print("getlock:             ", mcqueue['getlock'], file=outhdl)
        print("scanOrderId:         ", mcqueue['scanOrderId'], file=outhdl)
        
    if level >= 9:
        displayMultiConsumerQueuePages(outhdl, mcqueue, mcqueue['headPage'])

    print("\n\n", file=outhdl)

def displayIntermediateQ(queue, level, outhdl=sys.stdout):
    iqueue = queue.dereference().cast(gdb.lookup_type('ieiqQueue_t'))
    print("Type: Intermediate (ieiqQueue_t *)",iqueue.address, file=outhdl) 
    print("QName:",valStr(iqueue['Common']['QName']),"InternalName:",valStr(iqueue['InternalName']), file=outhdl)
    if level > 1:
        print("QOptions:        ", iqueue['QOptions'], file=outhdl)
        print("isDeleted:       ", iqueue['isDeleted'], file=outhdl)
        print("enqueueCount:    ", iqueue['enqueueCount'], file=outhdl)
        print("dequeueCount:    ", iqueue['dequeueCount'], file=outhdl)
        print("qavoidCount:     ", iqueue['qavoidCount'], file=outhdl)
        print("bufferedMsgsHWM: ", iqueue['bufferedMsgsHWM'], file=outhdl)
        print("rejectedMsgs:    ", iqueue['rejectedMsgs'], file=outhdl)
        print("bufferedMsgs:    ", iqueue['bufferedMsgs'], file=outhdl)
        print("inflightEnqs:    ", iqueue['inflightEnqs'], file=outhdl)
        print("inflightDeqs:    ", iqueue['inflightDeqs'], file=outhdl)
        print("maxInflightDeqs: ", iqueue['maxInflightDeqs'], file=outhdl)
        print("qavoidCount:     ", iqueue['qavoidCount'], file=outhdl)
        print("waiterStatus     ", iqueue['waiterStatus'], file=outhdl)
        print("pConsumer:       (ismEngine_Consumer_t *)", iqueue['pConsumer'], file=outhdl)
        print("nextOrderId:     ", iqueue['nextOrderId'], file=outhdl)        
        print("cursor:          (ieiqQNode_t *)", iqueue['cursor'], file=outhdl)
                
    if level > 4:
        print("ReportedQFull:   ", iqueue['ReportedQueueFull'], file=outhdl)
        print("resetCursor  :   ", iqueue['resetCursor'], file=outhdl)
        print("Redelivering  :  ", iqueue['Redelivering'], file=outhdl)
        print("hStoreObj:       ", iqueue['hStoreObj'], file=outhdl)
        print("hStoreProps:     ", iqueue['hStoreProps'], file=outhdl)
        print("QueueRefContext: ", iqueue['QueueRefContext'], file=outhdl)
        print("PageMap:         ", iqueue['PageMap'], file=outhdl)
        print("deletionRemovesStoreObjects: ", iqueue['deletionRemovesStoreObjects'], file=outhdl)
        print("deletionCompleted: ", iqueue['deletionCompleted'], file=outhdl)
        print("headlock:        ", iqueue['headlock'], file=outhdl)
        print("head:            (ieiqQNode_t *)", iqueue['head'], file=outhdl)       
        print("headPage:        (ieiqQNodePage_t *)", iqueue['headPage'], file=outhdl)
        print("deletedStoreRefCount:   ", iqueue['deletedStoreRefCount'], file=outhdl)
        print("putlock:         ", iqueue['putlock'], file=outhdl)
        print("tail:            (ieiqQNode_t *)", iqueue['tail'], file=outhdl)
        print("redeliverCursorOId:  ", iqueue['redeliverCursorOrderId'], file=outhdl)
        print("hMsgDelInfo:     ", iqueue['hMsgDelInfo'], file=outhdl)
        
    if level >= 9:
        displayIntermediateQueuePages(outhdl, iqueue, iqueue['headPage'])

    print("\n\n", file=outhdl)
    
def displaySimpleQ(queue, level, outhdl=sys.stdout):
    squeue = queue.dereference().cast(gdb.lookup_type('iesqQueue_t'))
    print("Type: Simple (iesqQueue_t *)",squeue.address, file=outhdl) 
    print("QName:",valStr(squeue['Common']['QName']), file=outhdl)
    if level > 1:
        print("QOptions:        ", squeue['QOptions'], file=outhdl)
        print("enqueueCount:    ", squeue['enqueueCount'], file=outhdl)
        print("dequeueCount:    ", squeue['dequeueCount'], file=outhdl)
        print("qavoidCount:     ", squeue['qavoidCount'], file=outhdl)
        print("bufferedMsgsHWM: ", squeue['bufferedMsgsHWM'], file=outhdl)
        print("rejectedMsgs:    ", squeue['rejectedMsgs'], file=outhdl)
        print("bufferedMsgs:    ", squeue['bufferedMsgs'], file=outhdl)
        print("qavoidCount:     ", squeue['qavoidCount'], file=outhdl)
        print("waiterStatus     ", squeue['waiterStatus'], file=outhdl)
        print("pConsumer:       (ismEngine_Consumer_t *)", squeue['pConsumer'], file=outhdl)
                
    if level > 4:
        print("ReportedQFull:   ", squeue['ReportedQueueFull'], file=outhdl)
        print("hStoreObj:       ", squeue['hStoreObj'], file=outhdl)
        print("hStoreProps:     ", squeue['hStoreProps'], file=outhdl)
        print("head:            (iesqQNode_t *)", squeue['head'], file=outhdl)       
        print("headPage:        (iesqQNodePage_t *)", squeue['headPage'], file=outhdl)
        print("putlock:         ", squeue['putlock'], file=outhdl)
        print("tail:            (iesqQNode_t *)", squeue['tail'], file=outhdl)
        
    if level >= 9:
        displaySimpleQueuePages(outhdl, squeue, squeue['headPage'])

    print("\n\n", file=outhdl)

def valStr(valueobj):
   try:
      thestring = valueobj.string("utf-8")
   except:
      thestring = str(valueobj)
      
   return thestring
