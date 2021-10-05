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

TRACE_BUFFER_LINES = 90000
TRACE_BUFFER_LINE_LENGTH = 120 

def dumptrace(outfile=None):

   if outfile is None:
      outhandle=sys.stdout
   else:
      outhandle=open(outfile, 'w+')

   startline   = gdb.parse_and_eval('nextTraceLine')
   TraceBuffer = gdb.parse_and_eval('TraceBuffer')
   
   traceline = startline
   
   while True:
      
      if traceline == TRACE_BUFFER_LINES:
          traceline = 0;
          
      #Print the line up to the first null... 
      try:   
         print >>outhandle, TraceBuffer[traceline].string("utf-8"),
      except:
         print >>outhandle, "***NOT UTF-8..raw buffer: ", str(TraceBuffer[traceline]),
      
      traceline +=1 
      
      if traceline == startline :
          break
   
   if outfile is not None:
       outhandle.close()

