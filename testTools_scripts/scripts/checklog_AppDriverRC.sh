#!/bin/bash +x
#
# Description:
# This script is to verify whether the test scenario executed by (C/Java)AppDriverRC/echoRC passed.
#
# This script queries the last line of the screeout logfile for RC= 
#   Which is set by echoRC.sh at the conclusion of the script.
#
#
# Input Parameter:
#   $1  -  required: logfile to be verified. 
#
#
# Criteria for passing the test:
# The occurrance of RC=# as the last line of the screenout logfile
# Return Code:
#  0 - Pass.
#  NOT ZERO - Fail.

logfile=$1

if [[ ($# -ne 1) ]] ; then
   clear
   echo "Usage:  " $0 " logfile"
   echo "  logfile      - The screenout log file used by echoRC.sh"
   echo
   exit 1
fi

typeset -i c

# TRICKY:  c=RC=0 is what results, it is interpreted as c is = to RC is = to 0, therefore c is = 0
# set -x
c=$(tail -1 ${logfile} )
# set +x


# If the overall test result is successful(0), then test has succeeded, otherwise map fail to 1
if [[ ${c} == "0" ]] ; then
   exit 0
else
   exit 1
fi
