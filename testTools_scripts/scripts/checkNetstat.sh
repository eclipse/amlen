#!/bin/bash
# checkNetstat.sh
#
# Runs netstat in busybox and check for active connections
# to the specified host ($1) on the specified port ($2).
#
# Dependencies:
# 1. The environment variable CHECK_NETSTAT_LOG 
#    must be set in order to report test results.
# 2. In AF, this script should be run using cAppDriverLog
#    so the check for "Test result is Success!" is performed
#    automatically.
#
# Usage: ./checkNetstat.sh host port
#
# Reports "Test result is Success!" on success
#         "Failure! Connections have not been closed." on failure
#
#!/bin/bash
#
# Enter busybox in order to run netstate. 
# Filter the netstat output to check for active connections with the host ($1) and port ($2)
# Write the results of the filtered netstat command to a temporary file (/tmp/userfiles/netstat_${1}_${2}.log).
#
CMD="/bin/netstat -an | /bin/grep ${1}:${2} | grep ESTABLISHED > /tmp/userfiles/netstat_${1}_${2}.log"
#RUNCMD="echo `echo ${CMD} | ssh ${A1_USER}@${A1_HOST} busybox` "
if [[ "${A1_TYPE}" == "DOCKER" ]] ; then
   RUNCMD="ssh ${A1_USER}@${A1_HOST} docker exec imaserver ${CMD}"
else
   RUNCMD="ssh ${A1_USER}@${A1_HOST} ${CMD}"
fi
echo Running command: "${CMD}"
$RUNCMD
echo $?
#
# Use scp to copy the temporary file containing the filtered netstat ouput (/tmp/userfiles/netstat_${1}_${2}.log) 
# to the runtime dirctory for the test (`pwd`).
#
testdir=`pwd`
CMD="/usr/bin/scp /tmp/userfiles/netstat_${1}_${2}.log ${M1_USER}@${M1_HOST}:${testdir}"
if [[ "${A1_TYPE}" == "DOCKER" ]] ; then
   RUNCMD="ssh ${A1_USER}@${A1_HOST} docker exec imaserver ${CMD}"
else
   RUNCMD="ssh ${A1_USER}@${A1_HOST} ${CMD}"
fi
echo Running command: "${CMD}"
$RUNCMD
echo $?
#
# Confirm there are no active connections by checking the file size of the filtered netstat output (netstat_${1}_${2}.log).
# Report failure if the file contains results.
#
if [ -s netstat_${1}_${2}.log ] ; then
    cat netstat_${1}_${2}.log >> ${CHECK_NETSTAT_LOG}
    echo Failure! Connections have not been closed. >> ${CHECK_NETSTAT_LOG}
    exit 1
fi
#
# If we get this far without exiting, report success.
#
echo Test result is Success! >> ${CHECK_NETSTAT_LOG}
