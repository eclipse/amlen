#/bin/bash 
#
##################################################################################
# printUsage() - prologue and HELP information for this command
# 
##################################################################################
printUsage() {
  echo " 
# ${0}
#
#   Syntax:
#      $0  [-s tcp://9.3.177.134:16102 ]  [-n ${MQTTGreenThread_spl_iterations} ]  [-h]
#
#   Inputs:
#    Optional CLI input variables
#      -s : (opt) IMAServer URI - [default - tcp://9.3.177.134:16102 ]
#      -n : (opt) Number of messages to send (iterations in SPL Beason code) [default - ${MQTTGreenThread_spl_iterations} ]
#   There are runtime Variables to set in the script!
#    see the section:  User Defined Runtime Input Variables
#
#   Outputs:
#     Streams writes a CSV file to $TESTCASE_OUTFILE
#
#   Return Codes:
#     0 - Successful and Complete
#    10 - Help was selected
#    11 - Failure in parsing of CLI options passed
#    55 - Failure in SubmitJob
#    66 - Failure in Compile
#    77 - Failure in 'cancelRunningJobs'
#    88 - Failure in 'monitorRunningJob'
#    99 - Failure in Main Line starting the Stream $STREAM_INSTANCE
# "
}

##################################################################################
# parseOptions() - parse CLI passed options, validate and set into variables
# 
##################################################################################
parseOptions(){
	while getopts "n:s:h" option ${OPTIONS}
	do
		case ${option} in
		n )
			MQTTGreenThread_spl_iterations=${OPTARG}
			;;
		s )
			TESTCASE_PARM1="MqttGreenThread.serverUri="${OPTARG}
			;;
		h )
			printUsage
                     exit 10
			;;
		* )
			echo "ERROR:  Unknown option: ${option}"
			printUsage
			exit 11
			;;
		esac	
	done

}

##################################################################################
# cancelRunningJobs() - Cancel all Running jobs in stream $STREAM_INSTANCE
# return will all jobs cancelled or ABORT with rc 77
#
# Notes:  Manipulate IFS to parse the LSJOBS output as lines of array data rather than a bunch a space delimited string elements
##################################################################################
cancelRunningJobs(){
  RUNNING_JOBS=`su - ${STREAMS_UID}  -c  "streamtool lsjob  --xheaders  -i ${STREAMS_INSTANCE}" `
  SAVE_IFS=${IFS}
  IFS=$'\n'
  for job in ${RUNNING_JOBS}
  do
    unset IFS
    JOB_ID=`echo ${job} | cut -d " " -f 1`
    su - ${STREAMS_UID}  -c  "streamtool  canceljob ${JOB_ID}  -i ${STREAMS_INSTANCE}"
  done
  IFS=${SAVE_IFS}
#set -x

  RUNNING_JOBS=`su - ${STREAMS_UID}  -c  "streamtool lsjob  --xheaders  -i ${STREAMS_INSTANCE}" `
  if [ ${#RUNNING_JOBS[*]} -gt 1 ];then
    echo "There is a problem stopping the existing running jobs. FAILED and ABORTING."
    echo ${RUNNING_JOBS}
    exit  77
  fi
#set +x
}

##################################################################################
# validateTestcaseOutput() - The $TESTCASE_OUTPUT file is verified for number of expected lines 
# All messages were received in order.
# 
# This function exits with return to invoker, or ABORTS with RC=44
##################################################################################
validateTestcaseOutput() {
  echo "Validate the log at:  ${TESTCASE_OUTFILE}"
  firsterror=0
  TOTAL_LINES=`wc -l < ${TESTCASE_OUTFILE}`
  if [ ${TOTAL_LINES} -eq ${MQTTGreenThread_spl_iterations} ];then
    echo "The log had the expected number of lines: ${MQTTGreenThread_spl_iterations};  actual lines: ${TOTAL_LINES}."
  else
    echo "ERROR:  The log DID NOT have expected number of lines, ${MQTTGreenThread_spl_iterations}; it had, ${TOTAL_LINES}. "
    firsterror=44
  fi

    linecount=0
    while read LINE
      do           
        #command           
        index=`echo ${LINE} | cut -d "," -f 1 `
        if [ ${index} -ne ${linecount} ];then
          if [ ${firsterror} -eq 0 ];then
            echo "ERROR: Some messagess were missing, duplicated, or arrived out of order."
            firsterror=44
          fi
          echo "Line # ${linecount} has message index for msg # ${index} - adjusting line count for missing, duplicate or out of order message ${index}"
          linecount=${index}
        fi
        linecount=$(( ${linecount}+1 ))
      done <${TESTCASE_OUTFILE}

  if [ ${firsterror} -ne 0 ]; then
    exit ${firsterror}
  fi
  echo "The testcase completed SUCCESSFULLY!"
  echo " "
}
##################################################################################
# monitorRunningJob() - The SPL Process does not exit when the number of expected iterations are completed, there is only $TESTCASE_OUTPUT,
# a CSV file that writes a line with each subscription receipt.  That is the only way to know when the Streams SPL is complete.
# If after ITERATION_CHECK sleep, the CURRENT_ITERATION and LAST_ITERATION index counts from TESTCASE_OUTPUT match, consider the test either
# Done or Hung waiting on messages that were lost.   Check is made to see if the desire number of iterations were received.
# This function exits with return to invoker, or ABORTS with RC=88
##################################################################################
monitorRunningJob() {
  declare LAST_ITERATION=0
  # Sleep Timeouts
  FIRST_CHECK=15
  ITERATION_CHECK=5

  echo "Monitoring file: $TESTCASE_OUTFILE  for iteration count of:  $IterationZeroIndex"
  sleep ${FIRST_CHECK}			#Give time to start writting TESTCASE_OUTFILE
  while true;
  do
    CURRENT_ITERATION=`tail -1 ${TESTCASE_OUTFILE} | cut -d',' -f 1`
    if [ "${CURRENT_ITERATION}" == "${LAST_ITERATION}" ]; then
      echo "Processing seems to have stopped at iteration:  ${CURRENT_ITERATION}"
      if [ "${CURRENT_ITERATION}" ==  "${IterationZeroIndex}" ]; then
        echo "Processing is Complete."
        echo " "
        break    
      else
        echo "Processing FAILED completion.  Check the output file on this client $(eval echo \$${THIS}_HOST) @ $TESTCASE_OUTFILE"
        exit 88
      fi
    else
      LAST_ITERATION=${CURRENT_ITERATION}
      sleep ${ITERATION_CHECK}
    fi
  done
  validateTestcaseOutput
}

#################################################################################
### MAIN LINE Starts

#set -x

##################### BEGIN:  User Defined Runtime Input Variables That can be overriden by CLI  #####################
declare TESTCASE_PARM1="MqttGreenThread.serverUri=tcp://9.3.177.134:16102"

### The Following Variable needs to match the value of
###     'iterations' in MQTTGreenThread.spl AND
###     'MESSAGES' in ism-InfoSphereStreams-01.sh
###
### -- Since the SPL Process does not exit when complete, have to monitor the outfile for completion status
### Search the TESTCASE_OUTFILE for a line to be written with that iteration number in column 1.
declare MQTTGreenThread_spl_iterations=100000

##################### END:  User Defined Runtime Input Variables That can be overriden by CLI  #####################

##################### BEGIN:  User Defined Runtime Input Variables  #####################
declare STREAMS_UID="streams"
declare STREAMS_INSTANCE="test@${STREAMS_UID}"
declare JOB_ID_FILE="/tmp/greenThread.ID"

if [ "${ISM_BUILDTYPE}" == manual ]; then
  declare PROJECT_DIR="/opt/ibm/InfoSphereStreams/myStudio/StreamsStudio/workspace2/MQTTGreenThreadProject"
else
  #Automation env. uses /niagara/test/test.env
  declare PROJECT_DIR="/home/streams/InfoSphereStreams/etc/StreamsStudio/StreamsStudio/workspace/MQTTGreenThreadProject"
fi
declare TESTCASE_ADL="${PROJECT_DIR}/output/MqttGreenThread/Distributed/MqttGreenThread.adl"
declare TESTCASE_OUTFILE="${PROJECT_DIR}/data/actual.csv"

##################### END:  User Defined Runtime Input Variables    #####################

# Parse CLI Input parameters
echo " "
echo "Parsing CLI options"
OPTIONS="$@"
parseOptions
# Subscriber Message Index starts with ZERO as first message received, not ONE. DO NOT CHANGE THIS VARIABLE adjustment!
declare IterationZeroIndex=$(( ${MQTTGreenThread_spl_iterations}-1 ))

# Verify the desired STREAMS_INSTANCE is running, if not start it
RUNNING_INSTANCES=`su - ${STREAMS_UID}  -c  "streamtool lsinstance --started" `
if [[ ${RUNNING_INSTANCES} =~ .*${STREAMS_INSTANCE}.* ]];then
  echo "Streams Instance, ${STREAMS_INSTANCE}, is running"
else
  echo "Starting Streams Instance: ${STREAMS_INSTANCE}"
  START_INSTANCE=`su - ${STREAMS_UID}  -c  "streamtool startinstance -i ${STREAMS_INSTANCE}" `
  if [[ ${START_INSTANCE} =~ .*"CDISC0003I The ${STREAMS_INSTANCE} instance was started.".* ]];then
    echo "Streams Instance, ${STREAMS_INSTANCE}, is running"
  else
    echo "Streams Instance, ${STREAMS_INSTANCE}, is FAILED TO START"
    exit 99
  fi
fi

#Verify there are no UNEXPECTED JOBS running, if there are Cancel then
cancelRunningJobs
echo "There are no active jobs running in instance: ${STREAMS_INSTANCE}"


#Start the SPL Application
SUBMIT_JOB=`su - ${STREAMS_UID} -c   "streamtool submitjob  -i ${STREAMS_INSTANCE}  --P '${TESTCASE_PARM1}'  --outfile ${JOB_ID_FILE}  ${TESTCASE_ADL}" `
if [ $? -ne 0 ]; then
  echo "ERROR Submitting job ${PROJECT_DIR}, Aborting!"
  exit 55
else
  echo "${SUBMIT_JOB}"
  echo ""
fi

monitorRunningJob

su - ${STREAMS_UID} -c   "streamtool canceljob  -i ${STREAMS_INSTANCE}  -f  ${JOB_ID_FILE} "
su - ${STREAMS_UID} -c   "streamtool stopinstance  -i ${STREAMS_INSTANCE}"

exit 0
