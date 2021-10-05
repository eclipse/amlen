#!/usr/bin/python
# -*- coding: utf-8 -*-
# Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

# This script takes in a list of connection IDs and searches a log file,
# and expecteds the total number of messages received to a given value.  It searches
# the log file for "ISMTEST1152" which will print the number of messages received on the
# connection whenever a null message is received.  The script looks for the last instance
# of this message in the log file for each connection ID
#
# When using the ReceiveMessages action, add the ActionParameter printNumberReceived = true

import sys
import argparse
import traceback
import time
import os

FILENAME = 'sumReceived.py'

#------------------------------------------------------------------------------
# Trace Functions
#------------------------------------------------------------------------------
def trace(traceLevel, message):
  if DEBUG >= traceLevel:
    formatting_string = '[%Y %m-%d %H:%M-%S00]'
    timestamp = time.strftime(formatting_string)
    print( '\n%s %s' % (timestamp, message) )

def traceEntry(method):
  trace(1, '>>> ' + FILENAME + ':' + method)

def traceExit(method):
  trace(1, '<<< ' + FILENAME + ':' + method)

#------------------------------------------------------------------------------
# Print the end result, attempt to close log file if it exists, and exit
#------------------------------------------------------------------------------
def exit(rc):
  if rc == 0:
    print( 'sumReceived.py: Test result is Success!' )
  else:
    print( 'sumReceived.py: Test result is Failure!' )
  try:
    LOG.close()
    sys.stdout = OLD_STDOUT
  except:
    pass
  sys.exit( rc )

#------------------------------------------------------------------------------
# Search log and compare
#------------------------------------------------------------------------------
def searchAndCompare(argv):
    traceEntry('searchAndCompare')
    rc = 0
    expected = argv.t
    connectionIDs = argv.c

    actual = 0

    #create a second list to store values and keep only one
    storehouse = [-1] * len(connectionIDs)
    duplicates = [0] * len(connectionIDs)

    try:
        logToRead = open(argv.l)
    except:
        trace(0, 'Failed to open log file: ' + argv.l)
        rc = 1
        return(rc)

    for line in logToRead:
        checkline = ""
        getID = ""
        #parse line into array
        checkline = line.split(" ")
        if "ISMTEST1152:" in checkline:
            #find last item in array and remove .
            getID = checkline[-1]
            getID = getID.split(".")
            getID = getID[0]
            #expected to input if equal return 5 from the end
            for x in range(len(connectionIDs)):
                if getID == connectionIDs[x]:
                    trace(0, line)
                    #store in storehouse, if duplicate change duplicate value
                    if storehouse[x] != -1:
                        duplicates[x] = -1
                        storehouse[x] = int(checkline[-5].replace(',',''))
                    else:
                        storehouse[x] = int(checkline[-5].replace(',',''))

    #sum up storehouse
    for x in range(len(storehouse)):
        if storehouse[x] != -1:
            actual = actual + int(storehouse[x])

    # handle case where stats for given connectionID not found
    for x in range(len(storehouse)):
        if storehouse[x] == -1:
            trace(0,"No output for connectionID " + connectionIDs[x] + " found. Exiting..")
            rc = 1
            return(rc)

    #if duplicates print duplcates
    for x in range(len(duplicates)):
        if duplicates[x] != 0:
            trace(1,"Duplicate lines found for connectionID " + connectionIDs[x] + ". Taking value of last one.")

    if argv.ge == 1:
        if int(actual) >= expected:
            trace(0,"Test passed, expected value: " + str(expected) + " actual value: " + str(actual))
            rc = 0
        else:
            trace(0,"Test failed, expected value: " + str(expected) + " actual value: " + str(actual))
            rc = 1
    else:
        if(int(actual) == expected):
            trace(0,"Test passed, expected value: " + str(expected) + " actual value: " + str(actual))
            rc = 0
        else:
            trace(0,"Test failed, expected value: " + str(expected) + " actual value: " + str(actual))
            rc = 1

    traceExit('searchAndCompare')
    return (rc)


#------------------------------------------------------------------------------
# Main
#------------------------------------------------------------------------------
def main(argv):
    parser = argparse.ArgumentParser(description='Search log and validate total messages received')
    parser.add_argument('-l', metavar='logToSearch', required=True, type=str, help='The name of the log file to search')
    parser.add_argument('-t', metavar='totalMessages', required=True, type=int, help='The total number of expected messages')
    parser.add_argument('-c', metavar='connectionIDs', required=True, type=str, nargs='+', help='List of connectionIDs to count messages on')
    parser.add_argument('-v', metavar='verbose', default=0, required=False, type=int, help='Set the verbosity of the tracing output' )
    parser.add_argument('-f', metavar='log', required=False, type=str, help='The name of the log file')
    parser.add_argument('-ge', metavar='greaterThan', default=0, required=False, type=int, help='Set to 1 to allow >= expected value')
    parser.add_argument('args', help='Remaining arguments', nargs=argparse.REMAINDER)
    args=parser.parse_args()
    print (args)

    global LOG, OLD_STDOUT, DEBUG

    DEBUG = args.v

    if args.f:
        try:
            OLD_STDOUT = sys.stdout
            LOG = open(args.f, "w", 1)
            sys.stdout = LOG
        except:
            trace(0, 'Failed to open log file: ' + args.f)
            exit(1)

    # Sleep a few seconds to give run-scenarios time to find the PID
    time.sleep(3)
    # Run the function
    rc = searchAndCompare(args)

    return(rc)


#------------------------------------------------------------------------------
# Check and invoke main
#------------------------------------------------------------------------------
rc = 1
if __name__ == "__main__":

  rc = main( sys.argv[1:] )

exit(rc)
