#!/bin/bash
#
# Last Updated:  08/16/11
#
# Description:
# This script is to verify whether the test scenario executed by the transmitter/receiver are passed.
#
# This script verifies that no negative conditions occured during the test, looking for
# a /Test.*Failure!/ regular expression match.
#
# This script is suitable for tests that do not specify a single transmitter/receiver/CompositeAction to run,
# but instead run all defined actions in the XML (e.g. JMS testing).
#
#
# Input Parameter:
#   $1  -  required: logfile to be verified. 
#
#
# Criteria for passing the test:
# There exist no occurances of a /Test.*Failure!/ regex match
#
# Return Code:
#  0 - Pass.
#  1 - Fail.

logfile=$1

if [[ ($# -ne 1) ]] ; then
   clear
   echo "Usage:  " $0 " logfile"
   echo "  logfile      - The log file"
   echo
   exit 1
fi

typeset -i c

c=$(grep 'Test.*Success!' ${logfile} | wc -l)

# If the overall test result is successful, the test has succeeded
if [[ ${c} -ge 1 ]] ; then
   exit 0
else
   exit 1
fi
