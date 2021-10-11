#!/bin/sh
#
# Author: Frank Colca
#
# Description:
# This script is used to request a resource from the STAF resource pool.
# 
#
#----------------------------------------------------------------------------------
#set -x

source /staf/STAFEnv.sh STAF
if [ $# -ne 2 ] ; then
  clear
  echo ""
  echo ""
  echo "This script is used to request a resource from the STAF resource pool."
  echo ""
  echo ""
  echo "Usage:"
  echo ""
  echo "  $0 <resource> <timeout>"
  echo ""
  echo "    resource = name of the resource you want to request (ie. mar111, was1, etc)"
  echo "    timeout  = time, in seconds, to wait."
  echo ""
  echo ""
  echo "Return Codes:"
  echo ""
  echo "    0        = request successful"
  echo "    non-zero = request failed"
  echo ""
  echo ""
  echo "Valid resources are:"
  echo ""
  staf local respool query pool amlenMachinePool | grep Entry:.* | awk '{print $2}' | sort
  echo ""
  echo ""
  exit 1
else
  # Verify that the specified resource is valid
  resource=$1
  staf local respool query pool amlenMachinePool | grep Entry:.* | grep -q $resource
  if [ $? != 0 ] ; then
    clear
    echo ""
    echo ""
    echo "Error... \"$resource\" is not a valid resource, valid choices are:"
    echo ""
    staf local respool query pool amlenMachinePool | grep Entry:.* | awk '{print $2}' | sort 
    echo ""
    echo ""
    exit 1
  fi 

  # Verify that the specified timeout is valid
  timeOut=$2
  if ! (( $timeOut )) ; then
    echo "Error... \"$timeOut\" is not a valid timeout."
    exit 1
  fi
fi

clear
echo ""
echo ""
echo "Requesting resource \"$resource\" with timeout of \"$timeOut\" seconds (shown in milliseconds below)."
echo ""
echo "STAF Command issued  was:"
echo ""
echo "  staf local respool request pool amlenMachinePool entry $resource timeout $((timeOut*1000)) garbagecollect no"
echo ""

staf local respool request pool amlenMachinePool entry $resource timeout $((timeOut*1000)) garbagecollect no &
