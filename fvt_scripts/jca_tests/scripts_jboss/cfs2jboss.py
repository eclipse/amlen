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

# This script is for converting the ConnectionFactory objects
# defined in tests.config into XML that JBOSS configuration uses.
# >>>             IMPORTANT                       <<<
# >>> JBOSS does NOT support "default" properties <<<
# >>> You must set every property explicitly!     <<<

from __future__ import with_statement


template = \
    '''
<connection-definition class-name="com.ibm.ima.ra.outbound.ImaManagedConnectionFactory" jndi-name="java:/%s" pool-name="%s_pool">
  <config-property name="port">
    %s
  </config-property>
  <config-property name="transactionSupportLevel">
    %s
  </config-property>
  <config-property name="convertMessageType">
    %s
  </config-property>
  <config-property name="protocol">
    %s
  </config-property>
  <config-property name="securityConfiguration">
    %s
  </config-property>
  <config-property name="traceLevel">
    %s
  </config-property>
  <config-property name="server">
    %s
  </config-property>
  <config-property name="user">
    %s
  </config-property>
  <config-property name="clientId">
    %s
  </config-property>
  <config-property name="securitySocketFactory">
    %s
  </config-property>
</connection-definition>
'''

class CF:
    def __init__(self, num):
        self.num = num
        self.name = 'DEFAULT_NAME'
        self.tipe = 'DEFAULT_TYPE'
        self.port = 'DEFAULT_PORT'
        self.clientid = ''
        self.secure = 'DEFAULT_SECURE'
        self.securitysocketfactory = ''
        self.securityconfiguration = ''
        self.user = ''
        self.password = ''
        self.transactionsupportlevel = 'XATransaction'
        self.maximumConnections = 'DEFAULT_MAXIMUMCONNECTIONS'
        self.convertmessagetype = 'auto'
        self.protocol = 'tcp'
        self.tracelevel = '-1'
        self.server = 'IMA_IP'

    def set(self, key, value):
        #print 'Setting %s => %s' % (key, value)
        if 'name' in key:
            self.name = value
        elif 'type' in key:
            self.tipe = value
        elif 'port' in key:
            self.port = value
        elif 'secure' in key and 'true' in value:
            self.protocol = 'tcps'
        elif 'secure' in key and 'false' in value:
            self.protocol = 'tcp'
        elif 'securitySocketFactory' in key:
            self.securitysocketfactory = value
        elif 'securityConfiguration' in key:
            self.securityconfiguration = value
        elif 'user' in key:
            self.user = value
        elif 'password' in key:
            self.password = value
        elif 'clientid' in key:
            self.clientid = value
        elif 'tranLevel' in key:
            self.transactionsupportlevel = value
        elif 'maximumConnections' in key:
            self.maximumconnections = value
        else:
            raise Exception('Tried to set unknown key type: %s' % key)

    def __str__(self):
        return template % (self.name,
                           self.name,
                           self.port,
                           self.transactionsupportlevel,
                           self.convertmessagetype,
                           self.protocol,
                           self.securityconfiguration,
                           self.tracelevel,
                           self.server,
                           self.user,
                           self.clientid,
                           self.securitysocketfactory)

if __name__ == '__main__':
    with open('test.config') as f:
        cf = None
        for line in f.readlines():
            if line.startswith('cf.') and not line.startswith('cf.count'):
                s = line.split('=')
                k, v = s[0].strip(), s[1].strip()
                cfnum = int(k.split('.')[2])
                if cf == None:
                    cf = CF(cfnum)
                elif cf.num < cfnum:
                    #print '\n\n\n>>> %d' % cf.num
                    print str(cf)
                    cf = CF(cfnum)
                cf.set(k, v)
        # last one
        #print '\n\n\n>>> %d' % cf.num
        print str(cf)


                
