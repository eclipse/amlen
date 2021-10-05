#!/bin/bash
#
# Last Updated:  08/19/08
#
# Description:
# This script is to verify whether the test scenario executed by the transmitter/receiver are passed.
# Test scenraio is constructed by several RMM API calls and is assigned to a tag name, such as rmdt
# for transmitter API calls and rmdr for receiver API calls. At the end of the test execution, a 
# "completed successfully" message associated to the tag name will be logged in the log file. A 
# successful run message will be looked like this 
#   For Java LLM Test driver - "Action CompositeAction:transmitter.rmdt completed successfully"
#   For C LLM Test driver - "Action CompositeAction:rmdt completed successfully"
#
# This script will look for a "completed successfully" message for the given tag name. 
#
#
# Input Parameter:
#   $1  -  required: logfile to be verified. 
#   $2  -  required: tag name of the RMM instance.
#
#
# Criteria for passing the test:
# There is a "completed successfully" message logged for the given tag name in the given log file.
#
# Return Code:
#  0 - Pass.
#  1 - Fail.

logfile=$1
type=$2

if [[ ($# -ne 2) ]] ; then
   clear
   echo "Usage:  " $0 " logfile tag"
   echo "  logfile      - either the transmitter or the receiver log file"
   echo "  tag          - tag name associated to <transmitter> or <receiver> in the testcase xml file"
   echo
   exit 1
fi

typeset -i c

# 2/6/14 - need to account for type being a substring of another composite action name
c=$(grep "Action CompositeAction:${type} completed successfully" ${logfile} | wc -l)

if [[ ${c} -eq 1 ]] ; then
   exit 0
else
   exit 1
fi
