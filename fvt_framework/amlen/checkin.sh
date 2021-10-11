#!/bin/sh
#
# Author: Frank Colca
#
# Description:
# This script is used to release a resource back into the STAF resource pool.
# 
#
#----------------------------------------------------------------------------------
#set -x

source /staf/STAFEnv.sh STAF
if [ $# -ne 1 ] ; then
#  clear
  echo ""
  echo ""
  echo "This script is used to release a resource back into the STAF resource pool."
  echo ""
  echo ""
  echo "  $0 <resource> | all"
  echo ""
  echo "    resource = name of the resource you want to release (ie. mar111, was1, vnc2, etc)"
  echo "    all      = release "ALL" resources currently in use"
  echo ""
  echo ""
  echo "  Return Codes:"
  echo ""
  echo "    0        = release successful"
  echo "    non-zero = release failed"
  echo ""
  echo ""
  echo "Resources that are NOT currently in use:"
  echo ""
  staf local respool query pool amlenMachinePool | grep -B1 "Owner: <None>" | grep Entry:.* | awk '{print $2}' | sort
  echo ""
  echo "Resources that are currently in use:"
  echo ""
  staf local respool query pool amlenMachinePool | grep -B1 "Owner: {" | grep Entry:.* | awk '{print $2}' | sort
  echo ""
  exit 1
else
  # Verify valid request
  if [ $1 != "all" ] ; then
    # Verify that the specified resource is valid
    resource=$1
    staf local respool query pool amlenMachinePool | grep -B1 "Owner: {" | grep $resource
    if [ $? != 0 ] ; then
      echo ""
      echo ""
      echo "Invalid resource: \"$resource\" is not in use... resource that are currently in use:"
      echo ""
      staf local respool query pool amlenMachinePool | grep -B1 "Owner: {" | grep Entry:.* | awk '{print $2}' | sort
      echo ""
      echo ""
      exit 1
    fi
    clear
    echo ""
    echo ""
    echo "Releasing resource \"$resource\"."
    echo ""
    echo "STAF Command issued  was:"
    echo ""
    echo "  staf local respool release pool amlenMachinePool entry $resource force"
    echo ""

    staf local respool release pool amlenMachinePool entry $resource force

  else
    done="FALSE"
    while [ $done != "TRUE" ]
    do
      staf local respool query pool amlenMachinePool | grep -B1 "Owner: {" | grep -m1 Entry:.* > /dev/null
      if [ $? == 0 ] ; then
        resource=`staf local respool query pool amlenMachinePool | grep -B1 "Owner: {" | grep -m1 Entry:.* | awk '{print $2}'`
        echo "Releasing resource \"$resource\"."
        staf local respool release pool amlenMachinePool entry ${resource} force
      else
        done="TRUE"
      fi
    done
  fi
fi
