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

#------------------------------------------------------------
# Show different aspects of a clientState.
#------------------------------------------------------------
def showClientState(show=None, state=None, address=None):
    if state == None:
        if address == None:
            print('ERROR: Specify the address of a client state to display')
            return
        else:
            state = gdb.parse_and_eval('*(ismEngine_ClientState_t *)'+str(address));

    if show == 'struct' or show == 'all' or show == None:
        print(state)
        if show != 'all':
            return

    matchCount = 0
    sessionAddress = state['pSessionHead']
    while sessionAddress != 0:
        sessionString = '(*(ismEngine_Session_t *)'+str(sessionAddress)+')'
        session = gdb.parse_and_eval(sessionString)
        if show == 'sessions' or show == 'all':
            print('\nSession found at '+sessionString)
            print(session)

        if show == 'consumers' or show == 'all':
            consumerAddress = session['pConsumerHead']
            while consumerAddress != 0:
                consumerString = '(*(ismEngine_Consumer_t *)'+str(consumerAddress)+')'
                consumer = gdb.parse_and_eval(consumerString)
                print('\nConsumer found at '+consumerString)
                print(consumer)
                consumerAddress = consumer['pNext']

        if show == 'producers' or show == 'all':
            producerAddress = session['pProducerHead']
            while producerAddress != 0:
                producerString = '(*(ismEngine_Producer_t *)'+str(producerAddress)+')'
                producer = gdb.parse_and_eval(producerString)
                print('\nProducer found at '+producerString)
                print(producer)
                producerAddress = producer['pNext']

        sessionAddress = session['pNext']

#------------------------------------------------------------
# Find a client state for the specified client id, or which is
# using s specified storeHandle for it's CSR or CPR.
#
# If the optional show option is specified the found client
# state structure is shown. The value can be:
#
#  struct     - print the client state structure
#  sessions   - print the sessions of the client state
#  consumers  - print the consumers for each session
#  producers  - print the producers for each session
#  all        - print all of the above
#
# examples:
#   Find a single client state for client id "ExampleClientId"
#      py findClientState(id="ExampleClientId")
#   Find a single client state which is using storeHandle 0x12345678
#      py findClientState(storeHandle=0x12345678)
#   Find all clients and display their entire structures
#      py findClientState(show="all")
#   Find the top of a client state steal chain
#      py findClientState(id="ChainedClient", thief="NULL")
#   Find the client state whose thief is a particular client state (ie. follow the chain)
#      py findClientState(id="ChainedClient", thief="0x0deadbeef0")
#------------------------------------------------------------
def findClientState(id=None, storeHandle=None, show=None, zombie=None, thief=None, startChain=0, endChain=-1):
    if show == '?' or show == 'help':
        print('findClientState(id=None, storeHandle=None, show=None, zombie=None, thief=None)')
        return

    chainCount = gdb.parse_and_eval('ismEngine_serverGlobal.ClientTable.ChainCount')
    
    if startChain < 0 or startChain > chainCount:
        print('startChain ['+str(startChain)+'] outside ClientTable chain range (0-'+str(chainCount)+')')
        return
        
    if endChain < 0:
        endChain = chainCount
    elif endChain > chainCount:
        print('endChain ['+str(endChain)+'] outside ClientTable chain range (0-'+str(chainCount)+')')
        return

    allThieves = False
    if thief == 'ALL':
        allThieves = True
        thief = 'NULL'
        
    matchCount = 1    
    while matchCount > 0:        
        matchCount = 0
        seenCount = 0
        for chainIndex in range(startChain, endChain):
            chain = gdb.parse_and_eval('ismEngine_serverGlobal.ClientTable.pChains['+str(chainIndex)+']')
            for entryIndex in range(0, chain['Limit']):
                match = True
                entry = gdb.parse_and_eval('ismEngine_serverGlobal.ClientTable.pChains['+str(chainIndex)+'].pEntries['+str(entryIndex)+']')
            
                if entry['pValue'] != 0:
                    seenCount += 1
                    clientState = gdb.parse_and_eval('*(ismEngine_ClientState_t *)'+str(entry['pValue']))

                    # See if it is a client state we are interested in
                    if match == True and zombie != None:
                        if str(clientState['fIsZombie']) != zombie:
                            match=False

                    if match == True and id != None:
                        if str(clientState['pClientId'].string()) != id:
                            match = False

                    if match == True and storeHandle != None:
                        if clientState['hStoreCSR'] != storeHandle and clientState['hStoreCPR'] != storeHandle:
                            match = False

                    if match == True and thief != None:
                        if thief == 'NULL':
                            if clientState['pThief'] != 0:
                                match = False
                        else:
                            if str(clientState['pThief']) != thief:
                                match = False
                        
                    if match == True:
                        matchCount += 1
                        print('ClientState ('+str(chainIndex)+','+str(entryIndex)+') "'+str(clientState['pClientId'].string())+'" found at (*(ismEngine_ClientState_t *)'+str(clientState.address)+') '+str(clientState['OpState']))

                        if show !=  None:
                            showClientState(show=show, state=clientState)
                        if allThieves == True:
                            thief = str(entry['pValue'])
                            entryIndex = chain['Limit']

        if matchCount == 0:
            print('No matching ClientStates found, '+str(seenCount)+' seen.')
        elif allThieves == False:
            print(str(matchCount)+' matching ClientStates found, '+str(seenCount)+' seen.')
            matchCount = 0            

#------------------------------------------------------------
# Calculate the hash value for a specified queue name
# (see ieqn_generateQueueNameHash)
#------------------------------------------------------------
def queueNameHash(name):
    nameHash = 5381
    for char in name:    
        nameHash = (nameHash * 33)%pow(2,32)
        nameHash = (nameHash ^ ord(char))%pow(2,32)
    return nameHash

#------------------------------------------------------------
# Find a queue with the specified name or which is using a
# specified storeHandle for it's QDR or QPR.
#
# If the optional show option specifies "y" the found queue and
# queuehandle structures are shown. The value can be:
#
#   struct   - print the namespace queue and underlying queue
#   all      - print all of the above
#   dumpscreen - dump queue (including queue pages etc.) to the screen
#   dumpfile   - dump queue (including queue pages etc.) to a file /tmp/qDump.<qName>
#
# examples:
#   Find a the queue named "ExampleQueue"
#      py findQueue(name="ExampleQueue")
#   Find a the queue which is using storeHandle 0x12345678
#      py findQueue(storeHandle=0x12345678)
#   Find all queues and display their structures (and queueHandles)
#      py findQueue(show="all")
#   Dump the contents of a queue to a file in /tmp:
#      py findQueue(name="ExampleQueue",show="dumpfile")
#------------------------------------------------------------
def findQueue(name=None, storeHandle=None, show=None):
    if show == '?' or show == 'help':
        print('findQueue(name=None, storeHandle=None, show=None)')
        return

    matchCount = 0
    seenCount = 0
    capacity = int(gdb.parse_and_eval('ismEngine_serverGlobal.queues.names.capacity'))
    startIndex = 0
    endIndex = capacity

    # If we have been given a name, we can hash straight to the right chain
    if name != None:
        startIndex = queueNameHash(name) % capacity
        endIndex = startIndex+1

    for chainIndex in range(startIndex, endIndex):
        chain = gdb.parse_and_eval('ismEngine_serverGlobal.queues.names.chains['+str(chainIndex)+']')
        for entryIndex in range(0, int(chain['count'])):
            match = True
            entry = gdb.parse_and_eval('((ieutHashEntry_t *)'+str(chain['entries'])+')['+str(entryIndex)+']')
            if entry['value'] != 0:
                seenCount += 1
                namedQueue = gdb.parse_and_eval('*(ieqnQueue_t *)'+str(entry['value']))
                queueCommon = namedQueue['queueHandle']
                queueType = str(queueCommon['QType'])

                # The different queue types have different structures	    
                if queueType == 'multiConsumer':
                    queueDataType = 'iemqQueue_t'
                elif queueType == 'intermediate':
                    queueDataType = 'ieiqQueue_t'
                else:
                    queueDataType = 'iesqQueue_t'

                queueString = '(*('+queueDataType+' *)'+str(namedQueue['queueHandle'])+')'	    
                queue = gdb.parse_and_eval(queueString)

                # See if it is a queue we are interested in
                if match == True and name != None:
                    if entry['key'].string() != name:
                        match = False

                if match == True and storeHandle != None:
                    if queue['hStoreObj'] != storeHandle and queue['hStoreProps'] != storeHandle:
                        match = False

                if match == True:
                    matchCount += 1
                    print('Queue "'+entry['key'].string()+'" found at (*(ieqnQueue_t *)'+str(namedQueue.address)+\
                          '), queueHandle is '+queueString)
                    if show == 'all' or show == 'struct':
                        print(namedQueue)
                        print(queue)
                    elif show == 'dumpscreen':
                        dumpQueueIfCodeLoaded(queue.address)
                    elif show == 'dumpfile':
                        fileName = '/tmp/queueDump.'+entry['key'].string()
                        dumpQueueIfCodeLoaded(queue.address, fileName)

    if matchCount == 0:
        print('No matching Named Queues found, '+str(seenCount)+' seen.')

#--------------------------------------------------------------------------------------
# Find a subscription(s) with the specified name, on a specific
# topic string, from a specified client or which is using a
# specified storeHandle for it's SDR or SPR.
#
# If the show option is specified, the found subscription and
# queuehandle structures are shown. The value can be:
#
#   struct   - print the subscription and underlying queue
#   all      - print all of the above
#   dumpscreen - dump all backing queues (including queue pages etc.) to the screen
#   dumpfile   - dump all backing queues (including queue pages etc.) to a file /tmp/subDump.XXX
#-------------------------------------------------------------------------------------
def findSubscription(name=None, topic=None, id=None, storeHandle=None, show=None, activeConsumer=None):
    if show == '?' or show == 'help':
        print('findSubscription(name=None, topic=None, id=None, storeHandle=None, show=None, activeConsumer=None)')
        return

    matchCount = 0
    seenCount = 0
    subscriptionAddress = gdb.parse_and_eval('ismEngine_serverGlobal.maintree.subscriptionHead')

    while subscriptionAddress != 0:
        seenCount += 1
        subscriptionString = '(*(ismEngine_Subscription_t *)'+str(subscriptionAddress)+')'
        subscription = gdb.parse_and_eval(subscriptionString)
        queueCommon = subscription['queueHandle']
        queueType = str(queueCommon['QType'])

        # The different queue types have different structures	    
        if queueType == 'multiConsumer':
            queueDataType = 'iemqQueue_t'
        elif queueType == 'intermediate':
            queueDataType = 'ieiqQueue_t'
        else:
            queueDataType = 'iesqQueue_t'

        queueString = '(*('+queueDataType+' *)'+str(subscription['queueHandle'])+')'
        queue = gdb.parse_and_eval(queueString)

        matchedObject='SUBSCRIPTION'
         
        match = True
        if match == True and name != None:
            if subscription['subName'].string() != name:
                match = False

        if match == True and id != None:
            if subscription['clientId'].string() != id:
                match = False

        if match == True and topic != None:
            node = gdb.parse_and_eval('*(iettSubsNode_t *)'+str(subscription['node']))
            if node['topicString'].string() != topic:
                match = False

        if match == True and storeHandle != None:
            if queue['hStoreObj'] != storeHandle and queue['hStoreProps'] != storeHandle:
                match = False

        if match == True and activeConsumer != None:
            if queue['pConsumer'] != 0:
                matchedObject = 'QUEUE'
            elif subscription['consumerCount'] != 0:
                matchedObject = 'SUBSCRIPTION'
            else:
                match = False
                
        if match == True:
           matchCount += 1

           print('Subscription "'+subscription['subName'].string()+'" found at '+subscriptionString+\
                 ', queueHandle is '+queueString+' matchedObject is '+matchedObject)
           if show == 'all' or show == 'struct':
               print(subscription)
               print(queue)
           elif show == 'dumpscreen':
               dumpQueueIfCodeLoaded(queue.address)
           elif show == 'dumpfile':
               pathName = '/tmp/'
               fileName = 'subDump.'+subscription['subName'].string()+'.'+str(subscription['queueHandle'])
               fileName = fileName.replace('/','_')
               dumpQueueIfCodeLoaded(queue.address, pathName+'/'+fileName)

        subscriptionAddress = subscription['next']

    if matchCount == 0:
        print('No matching Subscriptions found, '+str(seenCount)+' seen.')

#--------------------------------------------------------------------------------------
# Find a global transaction(s)
#
# Transactions in a specific state can be found by specifying the state option, this
# can be one of 'Inflight', 'Prepared', 'CommitOnly' or 'RollbackOnly' - with this not
# specified transactions in all states are shown.
#
# By default this shows the address of the found transactions and the XID string. In 
# addition, the show option can be used to specify more detail be printed:
#
# 'struct'            - Show the transaction structure
# 'SLEs'              - Show the SLEs (header and detail)
# 'all'               - All of the above
#-------------------------------------------------------------------------------------
def findGlobalTransaction(show=None, state=None):
    if show == '?' or show == 'help':
        print('findGlobalTransaction(show=None, state=None)')
        return

    matchCount = 0
    seenCount = 0
    transactionTable = gdb.parse_and_eval('ismEngine_serverGlobal.TranControl.GlobalTranTable')
    chainIndex = 0
    
    while chainIndex < transactionTable['capacity']:
        chainString = 'ismEngine_serverGlobal.TranControl.GlobalTranTable.chains['+str(chainIndex)+']'
        transactionChain = gdb.parse_and_eval(chainString)
        
        entryIndex = 0
        
        while entryIndex < transactionChain['count']:
            seenCount += 1
            match = True
            transactionEntry = chainString+'.entries['+str(entryIndex)+']'
            transactionAddress = gdb.parse_and_eval(transactionEntry+'.value')
            transaction = gdb.parse_and_eval('*(ismEngine_Transaction_t *)'+str(transactionAddress))
        
            # Have we been asked for transactions in a specific state
            if match == True and state != None:
                if state == 'Inflight' and transaction['TranState'] != 1:
                    match = False
                if state == 'Prepared' and transaction['TranState'] != 2:
                    match = False
                elif state == 'CommitOnly' and transaction['TranState'] != 3:
                    match = False
                elif state == 'RollbackOnly' and transaction['TranState'] != 4:
                    match = False
                    
            if match == True:
                matchCount += 1
                
                print('Transaction *(ismEngine_Transaction_t *)'+str(transactionAddress)+' XID: '+gdb.parse_and_eval(chainString+'.entries['+str(entryIndex)+'].key').string("utf-8"))
                
                if show == 'all' or show == 'struct':
                    print(transaction)
                    
                if show == 'all' or show == 'SLEs':
                    sleAddress = transaction['pSoftLogHead']
                    sleCount = 0
                    while sleAddress != 0:
                        sleHeaderString = '*(ietrSLE_Header_t *)'+str(sleAddress)
                        sleHeader = gdb.parse_and_eval(sleHeaderString)

                        # Interesting (specific) part of an SLE is after the header                        
                        sleDetailString = None
                        sleDetail = None
                        
                        if sleHeader['Type'] == 1:    # ietrSLE_IQ_PUT
                            sleDetailString = '*(ieiqSLEPut_t *)'+str(sleAddress+1)                        
                        elif sleHeader['Type'] == 2:  # ietrSLE_TT_STORE_SUBSC_DEFN
                            sleDetailString = '*(iettSLEStoreSubscDefn_t *)'+str(sleAddress+1)                            
                        elif sleHeader['Type'] == 3:  # ietrSLE_TT_STORE_SUBSC_PROPS
                            sleDetailString = '*(iettSLEStoreSubscProps_t *)'+str(sleAddress+1)
                        elif sleHeader['Type'] == 4:  # ietrSLE_TT_ADDSUBSCRIPTION
                            sleDetailString = '*(iettSLEAddSubscription_t *)'+str(sleAddress+1)
                        elif sleHeader['Type'] == 5:  # ietrSLE_TT_UPDATERETAINED
                            sleDetailString = '*(iettSLEUpdateRetained_t *)'+str(sleAddress+1)
                        elif sleHeader['Type'] == 6:  # ietrSLE_CS_ADDUNRELMSG
                            sleDetailString = '*(iestSLEAddUMS_t *)'+str(sleAddress+1)
                        elif sleHeader['Type'] == 7:  # ietrSLE_CS_RMVUNRELMSG
                            sleDetailString = '*(iestSLERemoveUMS_t *)'+str(sleAddress+1)
                        elif sleHeader['Type'] == 8:  # ietrSLE_SQ_PUT
                            sleDetailString = '*(iesqSLEPut_t *)'+str(sleAddress+1)
                        elif sleHeader['Type'] == 9:  # ietrSLE_TT_FREESUBLIST
                            sleDetailString = '*(iettSLEFreeSubList_t *)'+str(sleAddress+1)
                        elif sleHeader['Type'] == 10: # ietrSLE_SM_ADDQMGRXID
                            sleDetailString = '*(iesmSLEAddQMgrXID_t *)'+str(sleAddress+1)
                        elif sleHeader['Type'] == 11: # ietrSLE_SM_RMVQMGRXID
                            sleDetailString = '*(iesmSLERemoveQMgrXID_t *)'+str(sleAddress+1)
                        elif sleHeader['Type'] == 12: # ietrSLE_MQ_PUT
                            sleDetailString = '*(iemqSLEPut_t *)'+str(sleAddress+1)
                        elif sleHeader['Type'] == 13: # ietrSLE_MQ_CONSUME_MSG
                            sleDetailString = '*(iemqSLEConsume_t *)'+str(sleAddress+1)
                            
                        print('SLE Header '+sleHeaderString)
                        print(sleHeader)
                        
                        if sleDetailString != None:
                            print('SLE '+sleDetailString)
                            print(gdb.parse_and_eval(sleDetailString))
                            
                        sleAddress = sleHeader['pNext']
                        sleCount += 1
                    
                    if sleCount > 0:
                        print('')
            
            entryIndex += 1
            
        chainIndex += 1

    if matchCount == 0:
        print('No matching Transactions found, '+str(seenCount)+' seen.')

#------------------------------------------------------------
# Find a connections in the connection monitor table
#------------------------------------------------------------
def findConnections():
    totalEntries = gdb.parse_and_eval('monitor_used')

    for monitorIndex in range(0, totalEntries):
        transportAddr = gdb.parse_and_eval('monitorlist['+str(monitorIndex)+']')
        if transportAddr.address != None:
            transportObj = gdb.parse_and_eval('*(ism_transport_t *)'+str(transportAddr))
            if transportObj['monitor_id'] == monitorIndex:
                print('Address '+str(monitorIndex)+' == '+str(transportAddr.address))
                print(transportObj)

#------------------------------------------------------------
# Find entries in the specified recovery table
#------------------------------------------------------------
def findRecoveryTableEntries(table=None, ignoreNumEntries=False):
    if table == '?' or table == 'help' or table == None:
        gdb.write('findRecoveryTableEntries(table=expr,ignoreNumEntries=False)\n')
        return

    tableAddr = gdb.parse_and_eval(table)
    if tableAddr != 0:
        tableStr = '((iertTable_t *)'+str(tableAddr)+')'
        tableObj = gdb.parse_and_eval('*'+tableStr)
        gdb.write(str(tableObj)+'\n')
        entriesFound = 0
        for arrayIndex in range(0, tableObj['numChains']):
            if ignoreNumEntries == True or entriesFound != tableObj['numEntries']:
                chainEntry = gdb.parse_and_eval(tableStr+'->chainArray['+str(arrayIndex)+']')
                if chainEntry != 0 and chainEntry != None:
                    gdb.write('Index '+str(arrayIndex)+'\n')
                    while chainEntry != 0:
                        entriesFound += 1
                        if tableObj['valueIsEntry'] == False:
                            chainEntryObj = gdb.parse_and_eval('*(iertEntry_t *)'+str(chainEntry))
                            gdb.write(str(chainEntry)+': '+str(chainEntryObj)+'\n')
                        else:
                            gdb.write(str(chainEntry)+'\n')
                        chainEntry = gdb.parse_and_eval('*(void **)(((uint8_t *)'+str(chainEntry)+'+'+str(tableObj['nextOffset'])+'))')
        gdb.write('Found '+str(entriesFound)+' entries, expected '+str(tableObj['numEntries'])+' entries.\n')
    else:
        gdb.write('Table is NULL\n')

#------------------------------------------------------------
# Dump a queue if displayqueue code is loaded
#------------------------------------------------------------
def dumpQueueIfCodeLoaded(queueExpression, outputFile=None):
    try:
        displayQueue(queueExpression, 9, outputFile)
    except:
        import traceback, sys
        traceback.print_exc(file=sys.stdout)
        print('Unable to dump queue...has source  displayqueue.py been done? Trying to autoload it...')
        try:
           import os
           import sys
           #Add the directory this file is in to the import path...
           sys.path.append(os.path.dirname(__file__))
           import displayqueue

           displayqueue.displayQueue(queueExpression, 9, outputFile)
        except:
           import traceback, sys
           traceback.print_exc(file=sys.stdout)
           print('Unable to dump queue...could displayqueue.py be found?')
