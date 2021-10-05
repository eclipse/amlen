#!/usr/bin/python
# Copyright (c) 2019-2021 Contributors to the Eclipse Foundation
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

def dumpClientList(outfile=None):
   if outfile is None:
      outhandle=sys.stdout
   else:
      outhandle=open(outfile, 'w+')

   clientptr = gdb.parse_and_eval('clientsListTail')
   
   while clientptr is not None and str(clientptr) != '0x0':
      print >>outhandle, clientptr
      clientptr = clientptr['prev']

def dumpPartialClientList(listelem, outfile=None):
   if outfile is None:
      outhandle=sys.stdout
   else:
      outhandle=open(outfile, 'w+')

   clientptr = gdb.parse_and_eval('(mqttProtoObj_t *)'+listelem)
   
   while clientptr is not None and str(clientptr) != '0x0':
      print >>outhandle, clientptr
      clientptr = clientptr['next']
