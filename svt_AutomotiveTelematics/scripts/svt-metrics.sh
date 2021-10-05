#!/bin/bash
#
# Description:
# This script pulls the lines from ${SVT_FILE} that match the string ${SVT_TEXT} and redirect into $SVT_FILE.csv.log .
#
# Environment Variables REQUIRED
#   ${SVT_TEXT} - the text string to 'grep' for
#   ${SVT_FILE} - the file being searched for occurrances of the text string
#
# Input Parameter:
#   none 
#
#
# Criteria for passing the test:
# There exist at least one occurances of ${SVT_TEXT}
#
# Return Code:
#  0 - Pass.
#  1 - Fail.

if [[ ${SVT_TEXT} == "" || ${SVT_FILE} == "" ]];  then

   clear
   echo "Usage:  " ${0} " Requires two environment variables set:"
   echo "  SVT_TEXT      - The text to be 'grepped' for."
   echo "  SVT_FILE      - The file to be 'grepped'."
   echo
   exit 1
fi
set -x

csvlogfile=${SVT_FILE}.csv.log
typeset -i rc

c=$(grep ${SVT_TEXT} ${SVT_FILE} > ${csvlogfile} )       # Almost works, but can't handle spaces
#c=$(grep /'${SVT_TEXT}/' ${SVT_FILE} > ${csvlogfile} )   # What I want, but doesn't work
#$(grep 'finished__' ${SVT_FILE} > ${csvlogfile} )
rc=$?
lines=`wc -l <${csvlogfile}`
echo "Lines Found=" $lines >> ${csvlogfile}


# If the overall test result is successful, the test has succeeded
if [[ ${rc} -ge 1 ]] ; then
   exit 1
else
   exit 0
fi
