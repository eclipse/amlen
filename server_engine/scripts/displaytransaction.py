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

#--------------------------------------------------------------------------------------
# Display a transaction
#-------------------------------------------------------------------------------------
def displayTransaction(address=None, outfile=None):
    
    if outfile is None:
       outhandle=sys.stdout
    else:
       outhandle=open(outfile, 'w+')
       
    txnString = '(*(ismEngine_Transaction_t *)'+str(address)+')'
    transaction = gdb.parse_and_eval(txnString)

    if transaction['StrucId'].string() != "ETRN":
        print ''+str(address)+' does not appear to be a valid Transaction ('+transaction['StrucId'].string()+')'
    else:
        print >>outhandle, 'Transaction: '+txnString
        print >>outhandle, transaction

        sleCount = 0
        sleAddress = transaction['pSoftLogHead']
        
        while sleAddress != 0:
            sleString = '(*(struct ietrSLE_Header_t *)'+str(sleAddress)+')'
            sle = gdb.parse_and_eval(sleString)
            
            print >>outhandle, 'SLE '+str(sleCount)+' '+sleString
            print >>outhandle, sle
            
            sleType = str(sle['Type'])
            sleDetailAddress = gdb.parse_and_eval(str(sleAddress)+'+ sizeof(ietrSLE_Header_t)')
            
            # Additional detail for some SLEs - this is just the set I need at the moment            
            if sleType == 'ietrSLE_TT_FREESUBLIST':
                    sleDetailString = '(*(iettSLEFreeSubList_t *)'+str(sleDetailAddress)+')'
                    sleDetail = gdb.parse_and_eval(sleDetailString)
                    
                    print >>outhandle, sleType+' '+sleDetailString
                    print >>outhandle, sleDetail
            elif sleType == 'ietrSLE_TT_UPDATERETAINED':
                    sleDetailString = '(*(iettSLEUpdateRetained_t *)'+str(sleDetailAddress)+')'
                    sleDetail = gdb.parse_and_eval(sleDetailString)
                    
                    print >>outhandle, sleType+' '+sleDetailString
                    print >>outhandle, sleDetail
            elif sleType == 'ietrSLE_IQ_PUT':
                    sleDetailString = '(*(ieiqSLEPut_t *)'+str(sleDetailAddress)+')'
                    sleDetail = gdb.parse_and_eval(sleDetailString)
                    
                    print >>outhandle, sleType+' '+sleDetailString
                    print >>outhandle, sleDetail
            
            sleCount += 1
            sleAddress = sle['pNext']

    if outfile is not None:
        outhandle.close()
