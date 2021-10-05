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

from collections import namedtuple
import json;
import sys, getopt;

#
# Cmdline Arg Order: JSON String, Mib Order, Composite Key to find [-a]
# Options: -a (dealing with a Composite Key whose value is an array)
# Note: Parsing deals with only ONE layer of composite keys

#MonitorKeys = ([ 'Cluster', 'Connection', 'Endpoint', 'Memory', 'Server', 
#				 'Store', 'Subscription', 'Topic' ])


REQARGS = 3
IsArray = 0
#Store Options and Args, Check for Errors
try:
	Opts, Args = getopt.getopt(sys.argv[1:],"a")
except getopt.GetoptErr as Err:
	print str(Err)
	sys.exit(2)

# Get Options
for o, v in Opts:
	# Composite Key Return an Array?
	if o == "-a":
		IsArray = 1
# Get Args
for index in range(len(Args)):
	if index == 0:
		# Load json Object
		JsonString = Args[index]
		JsonObj = json.loads(JsonString)
	elif index == 1:
		# Load Mib Order
		JsonMibOrderString = Args[index]
		if (isinstance(JsonMibOrderString, basestring)):
			JsonMibOrderList = JsonMibOrderString.split()
		else:
			print "Second Argument NOT a String, Aborting Test"
			sys.exit(-1)
	elif index == 2:
		# Store Composite Key
		MonitorKey = Args[index]

# Check if Got all the required Args	
if (index+1) != REQARGS:
	print ("Too Few Required Arguments, Need: ", REQARGS)
	sys.exit(1)

# Get the list of keys and compare with keys in json string
if JsonObj.has_key(MonitorKey):
	JsonValue = JsonObj.get(MonitorKey)
	ReturnString = ""
	# Check if Value is an Array: Needs to be handled differently
	if (IsArray or isinstance(JsonValue,list)):
		for idx in range(len(JsonValue)):
			#ReturnString = ReturnString + ' '
			Entry = JsonValue[idx]
			mibIdx = 0
			for mib in JsonMibOrderList:
				if mibIdx !=  0:
					ReturnString = ReturnString + ','
				mibIdx += 1
				if Entry.has_key(mib):
					ReturnString = ReturnString + str(Entry.get(mib))
			ReturnString = ReturnString + " "
	else:
		# Return a string containing all the values in the mib order requested
		for mib in JsonMibOrderList:
			if JsonValue.has_key(mib):
				ReturnString = ReturnString + str(JsonValue.get(mib)) + ','
	
	print ReturnString
sys.exit(0)
