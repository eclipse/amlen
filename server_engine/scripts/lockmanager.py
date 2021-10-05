#!/usr/bin/python
# Copyright (c) 2018-2021 Contributors to the Eclipse Foundation
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

def lockChainsInfo():
   lockManager = gdb.parse_and_eval('ismEngine_serverGlobal.LockManager')
   hashchainNum = 0
   
   longestChain = 0
   shortestChain = 4000000000
   numlocks = 0
   
   for hashchainNum in range(0, lockManager['LockTableSize']):
      hashChain = lockManager['pLockChains'][0][hashchainNum]
      numlocksInChain = 0
      #Walk the chain
      lockInChain = hashChain['pChainHead']

      while lockInChain != 0:
         numlocksInChain += 1
         lockInChain = lockInChain['pChainNext']
      
      if hashchainNum%512 == 0:
         print "Chain "+str(hashchainNum)+": "+str(numlocksInChain)+" locks"
      
      numlocks += numlocksInChain
      if numlocksInChain < shortestChain:
         shortestChain = numlocksInChain
      if numlocksInChain > longestChain:
         longestChain = numlocksInChain
         
    
   print "Total Chains:   "+str(lockManager['LockTableSize'])
   print "Total Locks:    "+str(numlocks)
   print "Shortest Chain: "+str(shortestChain) + " locks long"
   print "Longest Chain:  "+str(longestChain)+" locks long"
      
   
