#!/bin/sh
#
# Author: Frank Colca
#
# Description:
# Format and display STAX logs
# 
#
#----------------------------------------------------------------------------------
#set -x

if [ $# -ne 1 ] ; then
#  clear
  echo ""
  echo ""
  echo "This script is used to format and display STAX logs."
  echo ""
  echo ""
  echo "  $0 <JobID>"
  echo ""
  echo "    JobID = STAX Job ID for the log you want to view"
  echo ""
  echo ""
  exit 1
else
  userlog="/staf/data/STAF/service/log/MACHINE/STAFServer/GLOBAL/STAX_Job_${1}_User.log"
  mainlog="/staf/data/STAF/service/log/MACHINE/STAFServer/GLOBAL/STAX_Job_${1}.log"
  if [ -f "$mainlog" ]; then
    fmtlog format logfile /staf/data/STAF/service/log/MACHINE/STAFServer/GLOBAL/STAX_Job_${1}.log newfile /root/STAXLogs/STAX_Job_${1}.txt
  else
    echo "Log does not exist: $mainlog"
    exit 2
  fi
    
  if [ -f "$userlog" ]; then
    fmtlog format logfile /staf/data/STAF/service/log/MACHINE/STAFServer/GLOBAL/STAX_Job_${1}_User.log newfile /root/STAXLogs/STAX_Job_${1}_User.txt
  else
    echo "Log does not exist: $userlog"
    exit 2
  fi

  if [ $? -ne 0 ] ; then
    echo "Error"
  else
    less STAX_Job_$1.log
    echo -n "Save file STAX_Job_$1.log? (y|n): "
    read input
    if [ $input != "y" -a $input != "Y" ] ; then
      rm -f STAX_Job_$1.log
    fi
  fi
fi
