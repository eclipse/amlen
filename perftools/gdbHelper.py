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

import gdb

def showConnection(con=None):
    if con == None:
        return
    transport = con['transport']
    print 'Connection: id=' + str(con['id']) + ' tid=' + str(transport['tid']) + ' protocol=' + str(transport['protocol'])+\
       ' clientID=' + str((transport['clientID'])) + ' transport=' + str(transport)


def findConnectionByTID(id=-1):
    connection = gdb.parse_and_eval('activeConnections')
    while connection != 0:
        transport = connection['transport']
        tid = transport['tid']
        if(id == -1 or id == tid):
            showConnection(con=connection)
        connection =  connection['conListNext']

def findConnectionByProtocol(prot=None, state=-1):
    counter = 0
    connection = gdb.parse_and_eval('activeConnections')
    while connection != 0:
        transport = connection['transport']
        protocol = transport['protocol']
        p = str(protocol.string())
        st = transport['state']
        if((prot == None  or prot == p) and (state == -1 or state == st)):
            showConnection(con=connection)
            counter = counter + 1
        connection =  connection['conListNext']
    print str(counter) + ' connections were found'
