#!/usr/bin/python
# Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

def displayThreadStats():
    droppedMsgCount              = 0
    expiredMsgCount              = 0
    bufferedMsgCount             = 0
    bufferedExpiryMsgCount       = 0
    internalRetainedMsgCount     = 0
    externalRetainedMsgCount     = 0
    retainedExpiryMsgCount       = 0
    remoteServerBufferedMsgBytes = 0

    threadData = gdb.parse_and_eval('ismEngine_serverGlobal->threadDataHead')

    while threadData != 0:
        stats = threadData['stats']

        droppedMsgCount              = droppedMsgCount              + stats['droppedMsgCount']
        expiredMsgCount              = expiredMsgCount              + stats['expiredMsgCount']
        bufferedMsgCount             = bufferedMsgCount             + stats['bufferedMsgCount']
        bufferedExpiryMsgCount       = bufferedExpiryMsgCount       + stats['bufferedExpiryMsgCount']
        internalRetainedMsgCount     = internalRetainedMsgCount     + stats['internalRetainedMsgCount']
        externalRetainedMsgCount     = externalRetainedMsgCount     + stats['externalRetainedMsgCount']
        retainedExpiryMsgCount       = retainedExpiryMsgCount       + stats['retainedExpiryMsgCount']
        remoteServerBufferedMsgBytes = remoteServerBufferedMsgBytes + stats['remoteServerBufferedMsgBytes']

        threadData = threadData['next']

    print "droppedMsgCount",              droppedMsgCount
    print "expiredMsgCount",              expiredMsgCount
    print "bufferedMsgCount",             bufferedMsgCount
    print "bufferedExpiryMsgCount",       bufferedExpiryMsgCount
    print "internalRetainedMsgCount",     internalRetainedMsgCount
    print "externalRetainedMsgCount",     externalRetainedMsgCount
    print "retainedExpiryMsgCount",       retainedExpiryMsgCount
    print "remoteServerBufferedMsgBytes", remoteServerBufferedMsgBytes


