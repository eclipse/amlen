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

#----------------------------------------------------------------------------
# 
#----------------------------------------------------------------------------


import os, sys, time, json, getopt, subprocess, requests
from subprocess import Popen, PIPE

def main(argv):
   ADDR = subprocess.Popen(["/niagara/test/scripts/getserver.py", ""], stdout=PIPE).communicate()[0].strip()
   PORT = subprocess.Popen(["/niagara/test/scripts/getserverport.py", ""], stdout=PIPE).communicate()[0].strip()
   SERVER = ADDR + ':' + PORT

   print SERVER

if __name__ == "__main__":
   main(sys.argv[1:])

