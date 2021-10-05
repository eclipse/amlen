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

# This script is for converting the destinations
# listed in test.config into xml that jboss 
# configuration uses

from __future__ import with_statement

def create_xml(n, t, v):
    s = \
'''
<admin-object class-name="%s" jndi-name="java:/%s" pool-name="%s">
    <config-property name="name">
        %s
    </config-property>
</admin-object>

'''
    if t == 'javax.jms.Topic':
        classname = 'com.ibm.ima.jms.impl.ImaTopic'
    else:
        classname = 'com.ibm.ima.jms.impl.ImaQueue'

    f = s % (classname, n, n, v)

    print f

with open('test.config') as f:
    dest_name = ''
    dest_type = ''
    dest_value = ''
    
    for line in f.readlines():
        line = line.strip()
        if line == '':
            continue
        elif line.startswith('dest.name'):
            dest_name = line.split('=')[1]
        elif line.startswith('dest.type'):
            dest_type = line.split('=')[1]
        elif line.startswith('dest.value'):
            dest_value = line.split('=')[1]
            create_xml(dest_name, dest_type, dest_value)
