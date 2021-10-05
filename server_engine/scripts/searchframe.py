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

import gdb

##########################################################################################
# First some functions that print/return gdb expressions                                 #
##########################################################################################

#For a given structure print out a line listing the fields described by memberArray..
# Each entry in member array is a dict containing:
#     desc :- description to print before the variable
#     field :- variable+field must be valid gdb syntax to describe the variable
def printMembers(variable, memberArray):
   for memb in memberArray:
      try:
         print memb['desc'], gdb.parse_and_eval(variable+memb['field']),
      except gdb.MemoryError:
         print '***MEMERR** (on ',memb['desc'],')'
      except gdb.error:
         print '***ERR** (on ',memb['desc'],')'

   print #Add newline


def getExpression(expression):
    ans = 'unset'
    try:
       ans = gdb.parse_and_eval(expression)
    except gdb.MemoryError:
       ans = '***MEMERR**'
    except gdb.error:
       ans = '***ERR**'

    return ans

##########################################################################################
# Next, functions that can be called to dump data for a given function in the  backtrace #
##########################################################################################

def printVariable(func, variable, prefix):
   print "    ",func," (", prefix, "): ",
   print getExpression(variable)
   
   
def printClient(func, variable, prefix):
   print "    ",func," (", prefix, "): ",

   printMembers(variable, [{ 'desc': "id:",    'field': '->pClientId'}, 
                           { 'desc': "addr:",  'field': ''}])
                           
def printDestination(func, variableTuple, prefix):
   print "    ",func," (", prefix, "): ",
   print "type:", getExpression(variableTuple[0]), "id:",getExpression(variableTuple[1])

def printProducer(func, variable, prefix):
   print "    ",func," (", prefix, "): ",

   printMembers(variable, [{ 'desc': 'addr:',       'field': ''},{ 'desc': 'usecount:',     'field':'->UseCount'},
                           { 'desc': 'destname:',   'field': '->pDestination->pDestinationName'},
                           { 'desc': 'desttype:',   'field': '->pDestination->DestinationType'},
                           { 'desc': 'destroyed:',   'field': '->fIsDestroyed'} ])



def printConsumer(func, variable, prefix):
   print "    ",func," (", prefix, "): ",

   printMembers(variable, [{ 'desc': "addr:",       'field': ''},
                           { 'desc': 'destname:',   'field': '->pDestination->pDestinationName'},
                           { 'desc': 'desttype:',   'field': '->pDestination->DestinationType'},
                           { 'desc': 'wstatus',     'field': '->iemqWaiterStatus'},
                           { 'desc': 'cursorOId',   'field': '->iemqCursor->c.orderId'} ])

def printSimpleQ(func, variable, prefix):
   print "    ",func," (", prefix, "): ",
   
   printMembers(variable, [{'desc': 'name:',     'field': '->Common.QName'}, {'desc': 'addr:',     'field': ''},
                           {'desc': 'wstatus:',  'field': '->waiterStatus'}, 
                           {'desc': 'buffered:', 'field': '->bufferedMsgs'}, {'desc': 'HWM:',      'field': '->bufferedMsgsHWM'},
                           {'desc': 'enq:',      'field': '->enqueueCount'}, {'desc': 'deq:',      'field': '->dequeueCount'},
                           {'desc': 'avoid:',    'field': '->qavoidCount'},  {'desc': 'rejd:',     'field': '->rejectedMsgs'}])

def printIntermediateQ(func, variable, prefix):
   print "    ",func," (", prefix, "): ",

   printMembers(variable, [{'desc': 'name:',      'field': '->Common.QName'}, {'desc': 'addr:',     'field': ''},
                           {'desc': 'qId:',        'field': '->qId'},         {'desc': 'isDeld:',    'field': '->isDeleted'}, 
                           {'desc': 'wstatus:',  'field': '->waiterStatus'}, 
                           {'desc': 'buffered:',  'field': '->bufferedMsgs'}, {'desc': 'HWM:',      'field': '->bufferedMsgsHWM'},
                           {'desc': 'enq:',       'field': '->enqueueCount'}, {'desc': 'deq:',      'field': '->dequeueCount'},
                           {'desc': 'avoid:',    'field': '->qavoidCount'},   {'desc': 'rejd:',      'field': '->rejectedMsgs'},
                           {'desc': 'inflt-enq:', 'field': '->inflightEnqs'}, {'desc': 'inflt-deq:','field': '->inflightDeqs'},
                           {'desc': 'nextOId:',   'field': '->nextOrderId'},
                           ])

def printMultiConsumerQ(func, variable, prefix):
   print "    ",func," (", prefix, "): ",

   printMembers(variable, [{'desc': 'name:',      'field': '->Common.QName'}, {'desc': 'addr:',     'field': ''},
                           {'desc': 'qId',        'field': '->qId'},          {'desc': 'isDeld',    'field': '->isDeleted'}, 
                           {'desc': 'brwsrs',     'field': '->numBrowsingWaiters'},
                           {'desc': 'selctrs',    'field': '->numSelectingWaiters'},
                           {'desc': 'buffered:',  'field': '->bufferedMsgs'}, {'desc': 'HWM:',      'field': '->bufferedMsgsHWM'},
                           {'desc': 'enq:',       'field': '->enqueueCount'}, {'desc': 'deq:',      'field': '->dequeueCount'},
                           {'desc': 'rejd:',      'field': '->rejectedMsgs'},
                           {'desc': 'inflt-enq:', 'field': '->inflightEnqs'},{'desc': 'inflt-deq:','field': '->inflightDeqs'},
                           {'desc': 'nextOId:',   'field': '->nextOrderId'}, {'desc': 'scanOId:',  'field': '->scanOrderId'},
                           {'desc': 'getCOId:',   'field': '->getCursor->c.orderId'},
                           {'desc': 'headOId:',   'field': '->headPage->nodes[0].orderId'},
                           ])
   print "              ",
   printMembers(variable, [{'desc': 'waiterlock:','field': '->waiterListLock'}])
  

###########################################################################################
# Now configure which functions we find in the backtrace should cause which diagnostic    #
# Python functions to be called                                                           #
###########################################################################################

triggerFunctions = {'ism_engine_confirmMessageDeliveryBatch'
                                           : [{'func': printClient, 'var': '((ismEngine_Session_t)hSession)->pClient', 'prefix': 'acking client'}],
                    'ism_engine_confirmMessageDelivery'
                                           : [{'func': printClient, 'var': '((ismEngine_Session_t)hSession)->pClient', 'prefix': 'acking client'}],
                    'ism_engine_destroyConsumer'
                                           : [{'func': printClient,   'var': 'pSession->pClient', 'prefix': 'client'},
                                              {'func': printConsumer, 'var': 'pConsumer', 'prefix': 'cons'}],
                    'ism_engine_createConsumer'
                                           : [{'func': printClient,   'var': 'pSession->pClient', 'prefix': 'client'},
                                              {'func': printDestination, 'var': ('destinationType','pDestinationName'), 'prefix': 'dest'}],
                    'ism_engine_listUnreleasedDeliveryIds'
                                           : [{'func': printClient,   'var': 'pSession->pClient', 'prefix': 'client'}],
                    'ism_engine_removeUnreleasedDeliveryId'
                                           : [{'func': printClient,   'var': 'pSession->pClient', 'prefix': 'client'}],
                    'ism_engine_addUnreleasedDeliveryId'
                                          : [{'func': printClient,   'var': 'pSession->pClient', 'prefix': 'client'}],
                    'ism_engine_republishRetainedMessages'
                                          : [{'func': printClient,   'var': 'pSession->pClient', 'prefix': 'client'},
                                             {'func': printConsumer, 'var': 'pConsumer', 'prefix': 'cons'}],
                    'ism_engine_unsetRetainedMessageOnDestination'
                                          : [{'func': printClient,   'var': 'pSession->pClient', 'prefix': ''},
                                             {'func': printDestination, 'var': ('destinationType','pDestinationName'), 'prefix': 'dest'}],
                    'ism_engine_unsetRetainedMessage'
                                          : [{'func': printClient,   'var': 'pSession->pClient', 'prefix': 'client'},
                                             {'func': printProducer, 'var': 'pProducer', 'prefix': 'producer'}],
                    'ism_engine_putMessageWithDeliveryIdOnDestination'
                                : [{'func': printClient,   'var': 'pSession->pClient', 'prefix': 'client'},
                                   {'func': printDestination, 'var': ('destinationType','pDestinationName'), 'prefix': 'dest'}],
                    'ism_engine_putMessageWithDeliveryId'
                                : [{'func': printClient,   'var': 'pSession->pClient', 'prefix': 'client'},
                                   {'func': printProducer, 'var': 'pProducer', 'prefix': 'producer'}],
                    'ism_engine_putMessageOnDestination'
                               : [{'func': printClient,   'var': 'pSession->pClient', 'prefix': 'client'},
                                  {'func': printDestination, 'var': ('destinationType','pDestinationName'), 'prefix': 'dest'}],
                    'ism_engine_putMessage': [{'func': printClient, 'var': 'pSession->pClient', 'prefix': 'client'},
                                              {'func': printProducer, 'var': 'pProducer', 'prefix': 'producer'}],
                    'ism_engine_destroyProducer'
                                           : [{'func': printClient, 'var': 'pSession->pClient', 'prefix': 'client'},
                                              {'func': printProducer, 'var': 'pProducer', 'prefix': 'producer'}],
                    'ism_engine_createProducer'
                                           : [{'func': printClient, 'var': 'pSession->pClient', 'prefix': 'client'},
                                              {'func': printDestination, 'var': ('destinationType','pDestinationName'), 'prefix': 'dest'}],
                    'ism_engine_XARecover'
                                           : [{'func': printClient, 'var': '((ismEngine_Session_t)hSession)->pClient', 'prefix': 'client'}],
                    'ism_engine_rollbackTransaction'
                                           : [{'func': printClient, 'var': '((ismEngine_Session_t)hSession)->pClient', 'prefix': 'client'},
                                              {'func': printVariable, 'var': 'pTran', 'prefix': 'pTran:'}],
                    'ism_engine_commitTransaction'
                                           : [{'func': printClient, 'var': '((ismEngine_Session_t)hSession)->pClient', 'prefix': 'client'},
                                              {'func': printVariable, 'var': 'pTran', 'prefix': 'pTran:'}],
                    'ism_engine_prepareTransaction'
                                           : [{'func': printClient, 'var': '((ismEngine_Session_t)hSession)->pClient', 'prefix': 'client'},
                                              {'func': printVariable, 'var': 'pTran', 'prefix': 'pTran:'}],
                    'ism_engine_endGlobalTransaction'
                                           : [{'func': printClient, 'var': '((ismEngine_Session_t)hSession)->pClient', 'prefix': 'client'}],
                    'ism_engine_createGlobalTransaction'
                                           : [{'func': printClient, 'var': '((ismEngine_Session_t)hSession)->pClient', 'prefix': 'client'}],
                    'ism_engine_createLocalTransaction'
                                           : [{'func': printClient, 'var': '((ismEngine_Session_t)hSession)->pClient', 'prefix': 'client'}],
                    'ism_engine_resumeMessageDelivery'
                                           : [{'func': printClient,   'var': 'pSession->pClient', 'prefix': 'client'},
                                              {'func': printConsumer, 'var': 'pConsumer', 'prefix': 'cons'}],
                    'ism_engine_stopMessageDelivery'
                                           : [{'func': printClient,   'var': 'pSession->pClient', 'prefix': 'client'}],
                    'ism_engine_startMessageDelivery'
                                           : [{'func': printClient,   'var': 'pSession->pClient', 'prefix': 'client'}],
                    'ism_engine_destroySession'
                                           : [{'func': printClient,   'var': 'pClient', 'prefix': 'client'}],
                    'ism_engine_createSession'
                                           : [{'func': printClient,   'var': 'pClient', 'prefix': 'client'}],
                    'ism_engine_unsetWillMessage'
                                           : [{'func': printClient,   'var': 'pClient', 'prefix': 'client'}],
                    'ism_engine_setWillMessage'
                                           : [{'func': printClient,   'var': 'pClient', 'prefix': 'client'},
                                              {'func': printDestination, 'var': ('destinationType','pDestinationName'), 'prefix': 'dest'}],
                    'ism_engine_destroyClientState'
                                           : [{'func': printClient,   'var': 'pClient', 'prefix': 'client'}],
                    'ism_engine_createClientState'
                                           : [{'func': printVariable, 'var': 'pClientId', 'prefix': 'id:'}],
                    'iesq_checkWaiters'    : [{'func': printSimpleQ, 'var': 'q', 'prefix': 'simpq'}],
                    'iesq_putMessage'      : [{'func': printSimpleQ, 'var': 'Q', 'prefix': 'simpq'}],
                    'iesq_termWaiter'      : [{'func': printSimpleQ, 'var': 'Q', 'prefix': 'simpq'}],
                    'iesq_deleteQ'         : [{'func': printSimpleQ, 'var': 'Q', 'prefix': 'simpq'}],
                    'ieiq_checkWaiters'    : [{'func': printIntermediateQ, 'var': 'Q', 'prefix': 'interq'}],
                    'ieiq_putMessage'      : [{'func': printIntermediateQ, 'var': 'Q', 'prefix': 'interq'}],
                    'ieiq_acknowledge'     : [{'func': printIntermediateQ, 'var': 'Q', 'prefix': 'interq'}],
                    'ieiq_termWaiter'      : [{'func': printIntermediateQ, 'var': 'Q', 'prefix': 'interq'}],
                    'ieiq_deleteQ'         : [{'func': printIntermediateQ, 'var': 'Q', 'prefix': 'interq'}],
                    'iemq_checkWaiters'    : [{'func': printMultiConsumerQ, 'var': 'Q', 'prefix': 'mcq'}],
                    'iemq_putMessage'      : [{'func': printMultiConsumerQ, 'var': 'Q', 'prefix': 'mcq'}],
                    'iemq_acknowledge'     : [{'func': printMultiConsumerQ, 'var': 'Q', 'prefix': 'mcq'}],
                    'iemq_acknowledge'     : [{'func': printMultiConsumerQ, 'var': 'Q', 'prefix': 'mcq'}],
                    'iemq_termWaiter'      : [{'func': printMultiConsumerQ, 'var': 'Q', 'prefix': 'mcq'}],
                    'iemq_deleteQ'         : [{'func': printMultiConsumerQ, 'var': 'Q', 'prefix': 'mcq'}],
                    'iemq_locateAndDeliverMessageToWaiter'
                                           : [{'func': printConsumer, 'var': 'pConsumer', 'prefix': ''}],
                    };


#For a given entry in the backtrace call all the appropriate diagnostic functions
def printDiagnostics(funcName, diagnosticsFuncsList):
   for funcData in diagnosticsFuncsList:
        funcData['func'](funcName, funcData['var'], funcData['prefix'])
        

#Walk down the frame stack calling appropriate diagnostic functions as triggered
def walkFrameStack():
   frame = gdb.selected_frame()
   
   # Find the newest frame (without using gdb.newest_frame)
   if frame != None:
      while frame.is_valid() and frame.newer() != None:
         frame = frame.newer()

   # Go through each frame looking for engine functions
   while frame != None and frame.is_valid():
      if frame.function() != None:
         frame.select()

         funcName =  frame.name()
         print funcName
      
         diagnosticsFuncsList = triggerFunctions.get(funcName)

         if diagnosticsFuncsList != None :
            printDiagnostics(funcName, diagnosticsFuncsList)
         
      frame = frame.older()
   

walkFrameStack()
