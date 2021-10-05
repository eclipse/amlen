#! /bin/bash

#----------------------------------------------------------------------------
# Script to start various test tools 
# 
# This script should be installed on the machine where the tool is to be 
# started.  The script can be passed as an argument to ssh in order to start
# the tool remotely.
#
# It is important that the only screen output of this script be the process
# id of the started process or a -1 indicating failure.  Echoing anything
# else out to the screen will compromise the ability of a calling script to
# kill the running process. A log file for the script will be created in the 
# working directory called start.log to log status and errors. 
#----------------------------------------------------------------------------

#----------------------------------------------------------------------------
# Functions
#----------------------------------------------------------------------------

#----------------------------------------------------------------------------
# Echo appends information to the log file
#----------------------------------------------------------------------------
echolog(){
	echo "$1" >> ${START_LOG}
}

#----------------------------------------------------------------------------
# Print Usage
#----------------------------------------------------------------------------
printUsage(){

echolog "Usage:	start.sh [options] " 
echolog "Options:	"
echolog " THESE Are the REQUIRED parameters"
echolog " -t : The test case tool's SubController, ex. testDriver, sync, "
echolog " -d : The Test Case directory where the test case file reside "
echolog " -f : The logfile name."
echolog ""
echolog " THESE Are the Semi-OPTIONAL Parameters, one or the other should exist"
echolog " -c : The Test Case's Configuration file for Tool's SubController "
echolog " -o : 'Other Options' enclosed in 2 sets of quotes in runScenarios.  Used when a config file might be overkill."
echolog ""
echolog "Ex:	start.sh -t toolController -d [TESTCASR_DIR] -f [LOGFILE_NAME] -c [CONFIG_FILENAME]"
echolog "or:	start.sh -t toolController -d [TESTCASR_DIR] -f [LOGFILE_NAME] '-o \'-x <param1> -y <param2>\''  "

}

#----------------------------------------------------------------------------
# Parse Options
#----------------------------------------------------------------------------
parseOptions(){
	while getopts "a:c:d:e:f:l:m:n:o:q:y:i:s:t:h" option ${OPTIONS}
	do
		case ${option} in
		a )
			COMMAND=${OPTARG}
			;;
		t )
			TOOL=${OPTARG}
			;;
		d )
			TESTCASE_DIR=${OPTARG}
			SCRIPT_DIR=$( dirname ${TESTCASE_DIR} )/scripts
			;;	
		c )
			CONFIG_FILE=${OPTARG}
			;;
		e )
			EXE_FILE=${OPTARG}
			;;
		f )
			LOG_FILE=${OPTARG}
			;;
		l )
			LOG_LEVEL=${OPTARG}
			;;
 		m )
			COMPARE_FILE=${OPTARG};;
 		n )
			NAME=${OPTARG};;
		q )
			CUENUM=${OPTARG};;
		y )
			COMP_PID=${OPTARG};;
		i )
			SOLUTION=${OPTARG};;
		s )
#                    ENVVARS="${OPTARG}"
			# Skip over '-s' and take the 3rd char as the delimiter
			# Then extract only the 'real args' of the string into TEST1
			# Finally replace the delimiter with a space 
			delimiter=`echo ${OPTARG} | cut -c '1-1' `
			TEST1=`echo ${OPTARG} | cut -c '2-' `
			ENVVARS=`echo ${TEST1} | sed "s/${delimiter}/ /g"`
                     ;;
		o )
#			OTHER_OPTS="${OPTARG}"
			# Skip over '-o' and take the 3rd char as the delimiter
			# Then extract only the 'real args' of the string into TEST1
			# Finally replace the delimiter with a space 
			delimiter=`echo ${OPTARG} | cut -c '3-3' `
			TEST1=`echo ${OPTARG} | cut -c '4-' `
			OTHER_OPTS=`echo ${TEST1} | sed "s/${delimiter}/ /g"`
			;;
		h )
			printUsage
			;;
		* )
			echolog "Unknown option: ${option}"
			printUsage
			exit 1
			;;
		esac	
	done

}

#----------------------------------------------------------------------------
# Parse Options for Sync Controller
#----------------------------------------------------------------------------
parseOptionsSync(){
#	while getopts "p:h" syncOpt ${SYNC_OPTIONS}
#	do
#		case ${syncOpt} in
#		(p)	PORT=${OPTARG}	;;
#		(h)	printUsage	;;
#		(*)	echolog "Unknown syncOpt: ${syncOpt}"
#			printUsage
#			exit 1
#			;;
#		esac	
#	done
PORT=`echo ${SYNC_OPTIONS}  | cut -d " " -f 2 `
}


#----------------------------------------------------------------------------
# Main Section
#----------------------------------------------------------------------------
#set -x
# redirecting standard error so we can debug errors
exec 2> start_error.log

# Create the logfile
##  basic START_LOG filename will be reset with Machine name after the parseOptions...I don't see this output anyway in 'start.log'
START_LOG=./start.log
# NOTE:  starts a clean START_LOG
#echo > ${START_LOG}

#echolog "Entering start.sh with $# arguments: $@..."

# If no options are given, print the usage statement and exit
if [[ $# -eq 0 ]]
then
	echolog "calling printUsage"
	printUsage
	exit 1
else

   echolog "Parsing options"
   OPTIONS="$@"
   parseOptions
   echolog "options parsed"

   # Source the ISMsetup file to get access to information about the test machines
   if [[ -f "${SCRIPT_DIR}/../testEnv.sh"  ]]
   then
     source  "${SCRIPT_DIR}/../testEnv.sh"
   else
     source ${SCRIPT_DIR}/ISMsetup.sh
   fi

   # NOTE:  Won't start a clean START_LOG using the new filename with MACHINE ID, want to see if we get other echolog output (it's lost)
   # UPDATE: The problem seemed to be that we were not providing a full path name   
   if [[ "${CONFIG_FILE}" != "" ]] ; then
      BASE_CONFIG_FILE=$(basename ${CONFIG_FILE})
   else
      BASE_CONFIG_FILE=""
   fi
   #START_LOG=${TESTCASE_DIR}/start.${THIS}${TOOL}${BASE_CONFIG_FILE}${NAME}.log
   START_LOG=${TESTCASE_DIR}/start.${THIS}${TOOL}${BASE_CONFIG_FILE}.log
   # Don't start a clean log because we can not always create a unique file name.
   #echo > ${START_LOG}
   echo "Entering start.sh with $# arguments: $@" >> ${START_LOG}
fi

# Go to the directory where this tool should be started 
if [[ "${TESTCASE_DIR}" != "" && -d ${TESTCASE_DIR} ]]
then 
	cd ${TESTCASE_DIR}
else
	MSG="Error: The directory where this tool should be started was not provided or did not exist, '${TESTCASE_DIR}'."
	echo ${MSG}
	echolog ${MSG}
	printUsage
	exit 1
fi

# Source the commonFunctions.sh script
source ${SCRIPT_DIR}/commonFunctions.sh


# Call the appropriate start function based on which tool is specified
case ${TOOL} in
	testDriver )
		# Make sure all of the required options for testDriver are set then start the driver
		if [[ "${LOG_FILE}" == "" ]]
		then
			echolog "Error: A log file must be provided to start the driver, ${TOOL}."
			printUsage
			exit 1
		elif [[ "${CONFIG_FILE}" == "" && "${OTHER_OPTS}" == "" ]]
		then
			echolog "Error: A configuration file or 'other options'  must be provided to start the driver, ${TOOL}."
			printUsage
			exit 1
		else
			 startTestDriver
		fi	
		;;
 
	cAppDriver | cAppDriverWait )
		# Make sure all of the required options for cAppDriver are set then start the driver
		if [[ "${LOG_FILE}" == "" ]]
		then
			echolog "Error: A log file must be provided to start the driver, ${TOOL}."
			printUsage
			exit 1
		elif [[ "${EXE_FILE}" == "" && -e "${EXE_FILE}"  ]]
		then
			echolog "Error: An executable file must be provided and exist, \"${EXE_FILE}\", to start the driver, ${TOOL}."
			printUsage
			exit 1
		else
			 startCAppDriver
		fi	
		;;
 
	cAppDriverRC | cAppDriverRConlyChk  | cAppDriverRConlyChkWait )
		# Make sure all of the required options for cAppDriverRC are set then start the driver
		if [[ "${LOG_FILE}" == "" ]]
		then
			echolog "Error: A log file must be provided to start the driver, ${TOOL}."
			printUsage
			exit 1
		elif [[ "${EXE_FILE}" == "" && -e "${EXE_FILE}"  ]]
		then
			echolog "Error: An executable file must be provided and exist, \"${EXE_FILE}\", to start the driver, ${TOOL}."
			printUsage
			exit 1
		else
                         if [ ${DEBUG} ];then 
                            echolog "${TOOL} ${EXE_FILE} will be executed with OPTIONS: ${OTHER_OPTS} and Env. Vars: ${ENVVARS}";
                         fi;
			 startCAppDriverRC
		fi	
		;;
 
	cAppDriverLog | cAppDriverLogWait | cAppDriverLogEnd)
		# Make sure all of the required options for cAppDriver are set then start the driver
		if [[ "${LOG_FILE}" == "" ]]
		then
			echolog "Error: A log file must be provided to start the driver, ${TOOL}."
			printUsage
			exit 1
		elif [[ "${EXE_FILE}" == "" && -e "${EXE_FILE}"  ]]
		then
			echolog "Error: An executable file must be provided and exist, \"${EXE_FILE}\", to start the driver, ${TOOL}."
			printUsage
			exit 1
		else
			 startCAppDriverLog
		fi	
		;;
 
	cAppDriverCfg | subController)
		# Make sure all of the required options for cAppDriverCfg are set then start the driver
		if [[ "${LOG_FILE}" == "" ]]
		then
			echolog "Error: A log file must be provided to start the driver, ${TOOL}."
			printUsage
			exit 1
		elif [[ "${CONFIG_FILE}" == "" && -e "${CONFIG_FILE}" ]]
		then
			echolog "Error: A configuration file must be provided, \"${CONFIG_FILE}\", to start the driver, ${TOOL}."
			printUsage
			exit 1
		else
			 startCAppDriverCfg
		fi	
		;;
 
	javaAppDriver | javaAppDriverLog | javaAppDriverWait | javaAppDriverLogWait)
		# Make sure all of the required options for javaAppDriver are set then start the driver
		if [[ "${LOG_FILE}" == "" ]]
		then
			echolog "Error: A log file must be provided to start the driver, ${TOOL}."
			printUsage
			exit 1
		elif [[ "${EXE_FILE}" == "" && -e "${EXE_FILE}"  ]]
		then
			echolog "Error: An executable file must be provided and exist, \"${EXE_FILE}\", to start the driver, ${TOOL}."
			printUsage
			exit 1
		elif [[ "${CLASSPATH}" == "" ]]
		then
			echolog "Error: The CLASSPATH environment must be defined, \"${CLASSPATH}\", to start the driver, ${TOOL}."
			exit 1
		else
			if [ "${OTHER_OPTS}" = "" ]
			then
				echolog "${TOOL} ${EXE_FILE} will be executed with Defaults and Env. Vars: ${ENVVARS}"
				startJavaAppDriver
			else
				echolog "${TOOL} ${EXE_FILE} will be executed with OPTIONS: ${OTHER_OPTS} and Env. Vars: ${ENVVARS}"
				startJavaAppDriver ${OTHER_OPTS}
			fi
		fi	
		;;

	javaAppDriverRC )
		# Make sure all of the required options for javaAppDriver are set then start the driver
		if [[ "${LOG_FILE}" == "" ]]
		then
			echolog "Error: A log file must be provided to start the driver, ${TOOL}."
			printUsage
			exit 1
		elif [[ "${EXE_FILE}" == "" && -e "${EXE_FILE}"  ]]
		then
			echolog "Error: An executable file must be provided and exist, \"${EXE_FILE}\", to start the driver, ${TOOL}."
			printUsage
			exit 1
		elif [[ "${CLASSPATH}" == "" ]]
		then
			echolog "Error: The CLASSPATH environment must be defined, \"${CLASSPATH}\", to start the driver, ${TOOL}."
			exit 1
		else
			if [ "${OTHER_OPTS}" = "" ]
			then
				echolog "${TOOL} ${EXE_FILE} will be executed with Defaults and Env. Vars: ${ENVVARS}"
				startJavaAppDriverRC
			else
				echolog "${TOOL} ${EXE_FILE} will be executed with OPTIONS: ${OTHER_OPTS} and Env. Vars: ${ENVVARS}"
				startJavaAppDriverRC ${OTHER_OPTS}
			fi
		fi	
		;;

	javaAppDriverCfg )
		# Make sure all of the required options for javaAppDriverCfg are set then start the driver
		if [[ "${LOG_FILE}" == "" ]]
		then
			echolog "Error: A log file must be provided to start the driver, ${TOOL}."
			printUsage
			exit 1
		elif [[ "${CONFIG_FILE}" == "" && -e "${CONFIG_FILE}" ]]
		then
			echolog "Error: A configuration file must be provided, \"${CONFIG_FILE}\", to start the driver, ${TOOL}."
			printUsage
			exit 1
		elif [[ "${CLASSPATH}" == "" ]]
		then
			echolog "Error: The CLASSPATH environment must be defined, \"${CLASSPATH}\", to start the driver, ${TOOL}."
			exit 1
		else
			if [ "${OTHER_OPTS}" = "" ]
			then
				echolog "${TOOL} ${CONFIG_FILE} will be executed with Defaults: Message Length=100 and Number of Messages=100 and Env. Vars: ${ENVVARS}"
				startJavaAppDriverCfg
			else
				echolog "${TOOL} ${CONFIG_FILE} will be executed with Values: Message Length and Number of Messages = ${OTHER_OPTS} and Env. Vars: ${ENVVARS}"
				startJavaAppDriverCfg ${OTHER_OPTS}
			fi
		fi	
		;;
 
	test.llm.TestServer )	
	    
		echolog "Parsing Sync options"
		SYNC_OPTIONS=${OTHER_OPTS}
		parseOptionsSync
		echolog "options Sync parsed"
		
		# Make sure all of the required options for Driver Sync are set then start test.llm.TestServer
		if [ "${LOG_FILE}" == "" -o "${PORT}" == "" ]
		then
			echolog "Error: A log file, '${LOG_FILE}', and a port, '${PORT}', must be provided in order to start the synchronization server, ${TOOL}."
			printUsage
			exit 1
		else
			 startSync
		fi
		;;
	killComponent )
		# Make sure all of the required options for killcomponent are set, then call killComponent
		if [ "${COMP_PID}" == "" ]
		then
			echolog "Error: An process id for the component to kill must be provided"
			printUsage
			exit 1
		else
			 killComponent
		fi
		;;
	searchLogsNow )
		# Make sure all of the required options for searchlogsNow are set, then call searchLogs
		if [ "${COMPARE_FILE}" == "" ]
		then
			echolog "Error: A compare file, ${COMPARE_FILE},  must be provided in order to search the logs"
			printUsage
			exit 1
		else
			 searchLogs
		fi
		;;
	searchLogs )
		# Make sure all of the required options for searchlogs are set, then call searchLogsWithCues
		if [ "${COMPARE_FILE}" == "" -o "${RUN_NUM}" == "" ]
		then
			echolog "Error: A compare file, ${COMPARE_FILE}, and a run number, ${RUN_NUM}, ex: '1' or '2' must be provided in order to search the logs"
			printUsage
			exit 1
		else
			 searchLogsWithCues
		fi
		;;
	searchLogsEnd )
		# Make sure all of the required options for searchLogsEnd are set, then call searchLogs
		if [ "${COMPARE_FILE}" == "" ]
		then
			echolog "Error: A compare file, ${COMPARE_FILE},  must be provided in order to search the logs"
			printUsage
			exit 1
		else
			 searchLogs
		fi
		;;
	runCommand )
		# Make sure all of the required options for runCommand are set, then call runCommandWithCues
		if [ "${COMMAND}" == "" -o "${CUENUM}" == "" ]
		then
			echolog "Error: A command and a cue number must be provided in order to run a command"
			printUsage
			exit 1
		else
			 runCommandWithCues
		fi
		;;
	runCommandNow )
		# Make sure all of the required options for runCommandNow are set, then call runCommand
		if [ "${COMMAND}" == "" ]
		then
			echolog "Error: A command must be provided in order to run a command"
			printUsage
			exit 1
		else
			 runCommand
		fi
		;;
	runCommandEnd )
		# Make sure all of the required options for runCommandEnd are set, then call runCommand
		if [ "${COMMAND}" == "" ]
		then
			echolog "Error: A compare file must be provided in order to search the logs"
			printUsage
			exit 1
		else
			 runCommand
		fi
		;;
	jmsDriver | jmsDriverNocheck | jmsDriverWait )
		# Make sure all of the required options for rcmsdriver are set, then call startRCMSDriver 
		if [ "${CONFIG_FILE}" == "" -o  "${LOG_FILE}" == "" -o "${NAME}" == "" ]
		then
			echolog "Error: A configuration file, a log file and a name for the CompositeAction to start must be provided in order to run the JMS driver"
			printUsage
			exit 1
		else
			 startJMSDriver
		fi
		;;

	wsDriver | wsDriverNocheck | wsDriverWait )
		# Make sure all of the required options for wsDriver are set then start the driver
		if [[ "${LOG_FILE}" == "" ]]
		then
			echolog "Error: A log file must be provided to start the driver, ${TOOL}."
			printUsage
			exit 1
		elif [ "${CONFIG_FILE}" == "" -o "${NAME}" == "" ]
		then
			echolog "Error: A configuration file and a name for the CompositeAction to start must be provided in order to run the JMS driver"
			printUsage
			exit 1
		else
			 startWSDriver
		fi	
		;;

	 * )
		echolog "Error: Invalid Tool. Tool must be set using the -t option in start.sh"
		exit 1;;
esac
	

