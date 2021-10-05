#!/bin/bash
# Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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
WorkDir=~/lion/

if [ $# -ne 0 ]
then
  WorkDir=$1
fi

pathname=""
theyear=$(date +%Y)

errfile="copy.err"

################################################################################
# Function for copyright check
# For a new segment, make sure only the current year is listed, and not
# multiple years, as in  "2004, 2005".
# For a segment that is not new, warn if there is not 2 years in the
# copyright statement. Might be valid though, if segment was new earlier in
# same year.
################################################################################
function copyright_check {

   case $pathname in
      */copycheck.sh ) ;;
      */server_build/docs/* ) ;;
      */README ) ;;
      */objectMoficationTest.txt ) ;;
      */testPolicy ) ;;
      */testMessagingPolicy ) ;;
      */bin/* ) ;;
      */lib/* )  ;;
      */lib_s/* )  ;;
      *.gz ) ;;
      */readme.txt ) ;;
      */.settings/* ) ;;
      *.class ) ;;
      */.* ) ;;
      */lafiles/* ) ;;
      *.png ) ;;
      *.gif ) ;;
      *.jar ) ;;
      */manufacturingISO/* ) ;;
      */dojobuild/* ) ;;
      */confidence_maps/* ) ;;
      *.jks ) ;;
      */README ) ;;
      *.zip ) ;;
      *.json ) ;;
      */Licenses/* ) ;;
      *.crt ) ;;
      *.pem ) ;;
      *.p12 ) ;;
      *.key ) ;;
      *.jpg ) ;;
      *.ico ) ;;
      *.MF ) ;;
      */IMA_Splash_Screen_Template.csv ) ;;
      */WebContent/*/templates/* ) ;;
      * ) if [[ $(grep -c 5725-F96 $pathname) -ge 1 ]]
           then
                  echo "Processing $pathname"
                  if [[ $(grep -ic "Copyright IBM Corp.*$theyear" $pathname ) -eq 0 ]]
                     then
                       echo " "
                       echo "File $pathname"
                       echo "     ERROR: Incorrect copyright statement in $pathname. "
                       echo "            The current year must be present."
                       RC=4
                  fi
                  if [[ $(grep -i "Copyright IBM Corp" $pathname | grep -vi pragma | grep -vi println | grep -vic setc ) -ge 2 ]]
                     then
                       echo " "
                       echo "File $pathname"
                       echo "     ERROR: Multiple IBM copyright statements are in $pathname."
                       echo "            There should only be one IBM copyright statement."
                       echo "            Contact Build team for assistance."
                       RC=4
                  fi

                  notnew=1
                  if [[ $notnew == 1 ]] &&
                     [[ ($(grep -ic "Copyright IBM Corp.*," $pathname ) -eq 0) ]]
                     then
                       echo " "
                       echo "File $pathname"
                       echo "     WARNING: Copyright statement contains only one year. If the first    "
                       echo "              release of this segment was in the current year, no change is needed. "
                       echo "              But if the first release of this segment was in a prior year,"
                       echo "              then prior and current year must be listed, separated by a comma."
                       echo "              View the previous version of the segment to determine the year of  "
                       echo "              first release.  DO NOT GUESS!!!!  "
                       echo "              Format example: Copyright IBM Corp. year1, $theyear "
                       echo " "
                       RC=4
                  fi

           else
             echo " "
             echo "File $pathname"
             echo "     WARNING: MessageSight-specific copyright information was not found "
             echo "              in $pathname."
             echo "              Please verify that it is valid for MessageSight copyright info "
             echo "              to be absent from $pathname."
             RC=4
          fi >> $errfile
      ;;
    esac
return
}

rm -rf "$errfile" 

for x in `find $WorkDir -type f`
do
   pathname=$x
   copyright_check
done
