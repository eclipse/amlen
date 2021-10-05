#!/usr/bin/python
# Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

define nonzeroArrList(arr, len):
   parsedArr=gdb.parse_and_eval(arr)

    for entry in range(0, len):
      if parsedArr[entry] != 0:
         print 'Nonzero',Str(entry),' = ',parsedArr[entry]


