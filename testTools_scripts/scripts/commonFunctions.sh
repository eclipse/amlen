#!/bin/bash

#***********************************************************************#
# CommonFunctions.sh                                                    #
#                                                                       #
# This script defines common functions for automated ISM testcases,     #
# This file should be sourced from ismAutoSTUB.sh or run-scenarios.sh   #
#                                                                       #
#***********************************************************************#


#-----------------------------------------------------------------------#
#  calculateTime                                                        #
#                                                                       #
#  Calculate the elapsed time for executing test scenarios              #
#  where:                                                               #
#        ${beginTime} is defined                                        #
#        ${endTime} is defined                                          #
#  output:                                                              #
#        User can use the values of the hr, min, and sec variables      #
#        to display the time elapsed from beginTime to endTime.         #
#-----------------------------------------------------------------------#

function calculateTime {
        # LAW:  08/16/12: Updated this function so that it runs faster on Windows
        function removeLeadingZero {
            case ${1} in
                0?) return ${1#0};;
                 *) return ${1};;
            esac
        }
        # Note: Date +%s is not supported on Solaris
        # Get each part of the begin and end times.
        # The date looks like this to start with Day Mon DD hh:mm:ss Zone YYYY
        # These commands chop it up so that we are saving only the DD and hh:mm:ss parts
        _noDay=${beginTime#* }
        _noMon=${_noDay#* }
        # Remove leading space for the case where there was 2 spaces (Tue Sep  4 15:15:15 CDT 2012) 
        _noMon=${_noMon# }
        _d1=${_noMon%% *}
        _noDate=${_noMon#* }
        _begin=${_noDate%% *}
        _noDay=${endTime#* }
        # Remove leading space for the case where there was 2 spaces (Tue Sep  4 15:15:15 CDT 2012) 
        _noMon=${_noDay#* }
        _noMon=${_noMon# }
        _d2=${_noMon%% *}
        _noDate=${_noMon#* }
        _end=${_noDate%% *}

        # Process begin
        # Remove the mm:ss from the time
        _h1=${_begin%%:*}
        removeLeadingZero ${_h1}
        _h1=$?

        # Remove the hh: and :ss from the time
        _m1_tmp=${_begin#*:}
        _m1=${_m1_tmp%%:*}
        removeLeadingZero ${_m1}
        _m1=$?
        
        # Remove the hh:mm: from the time
        _s1=${_begin##*:}
        removeLeadingZero ${_s1}
        _s1=$?

        #_h1=$(echo ${beginTime} | cut -f4 -d' ' | cut -f1 -d':' | sed 's/^0//')
        #_m1=$(echo ${beginTime} | cut -f4 -d' ' | cut -f2 -d':' | sed 's/^0//')
        #_s1=$(echo ${beginTime} | cut -f4 -d' ' | cut -f3 -d':' | sed 's/^0//')

        # Process end
        # Remove the mm:ss from the time
        _h2=${_end%%:*}
        removeLeadingZero ${_h2}
        _h2=$?

        # Remove the hh: and :ss from the time
        _m2_tmp=${_end#*:}
        _m2=${_m2_tmp%%:*}
        removeLeadingZero ${_m2}
        _m2=$?
        
        # Remove the hh:mm: from the time
        _s2=${_end##*:}
        removeLeadingZero ${_s2}
        _s2=$?

        #_h2=$(echo ${endTime} | cut -f4 -d' ' | cut -f1 -d':' | sed 's/^0//')
        #_m2=$(echo ${endTime} | cut -f4 -d' ' | cut -f2 -d':' | sed 's/^0//')
        #_s2=$(echo ${endTime} | cut -f4 -d' ' | cut -f3 -d':' | sed 's/^0//')

        #_d1=$(echo ${beginTime} | cut -f3 -d' ') 
        #_d2=$(echo ${endTime} | cut -f3 -d' ')
        # Handle going past midnight (only once)
        # If the begin and end dates are not the same then add 24 hours to the end time 
        # Use string compare since some OS's use commas in the date "Tue, Feb 04, 2014 1:52:33 PM" vs "Tue Feb 4 2014 1:52:33 PM"
        if [[ ${_d1} != ${_d2} ]] ; then
           ((_h2+=24))
        fi
        ((_ttl_s1=_h1*60*60+_m1*60+_s1))
        ((_ttl_s2=_h2*60*60+_m2*60+_s2))
        ((eclipse_s=_ttl_s2-_ttl_s1))
        ((sec=eclipse_s%60))
        ((min=(eclipse_s/60)%60))
        ((hr=eclipse_s/3600))

        # If the result is a single digit, then add a leading zero for readability
        case ${hr} in
            ?) hr=0${hr};;
        esac
        case ${min} in
            ?) min=0${min};;
        esac
        case ${sec} in
            ?) sec=0${sec};;
        esac
        #hr=$(echo ${hr} | sed 's/^.$/0&/')
        #min=$(echo ${min} | sed 's/^.$/0&/')
        #sec=$(echo ${sec} | sed 's/^.$/0&/')
}


#-----------------------------------------------------------------------#
#  get_llmd_stat                                                        #
#  where:                                                               #
#          ${llmd_ip} is defined and an IP address llmd "allow"
#          ${1} is llmd controlport or if not provided
#                      llmd controlport is 16102 by default	
#
#-----------------------------------------------------------------------#

function get_llmd_stat {

    if [[ -z $1 ]]; then
        llmd_ctl_p="16102"
    else
        llmd_ctl_p=$1
    fi

    if [[ ${choice} = "-ls" ]] ; then
        ssh ${remotebox_user}@${remotebox_host} "cd ${remote_tcdir}; ../bin/upd_cmd_xml.sh stat"
        ssh ${remotebox_user}@${remotebox_host} "cd ${remote_tcdir}; llmdval -i [${llmd_ip}]:${llmd_ctl_p} -f llmd_cmdstat.xml"  2>&1 > /dev/null
        scp ${remotebox_user}@${remotebox_host}:${remote_tcdir}/llmd_cmdstat.log .
    else
        if [[ -z ${curDir} ]]; then
          curDir=$(dirname $0)
        fi
        ${curDir}/upd_cmd_xml.sh stat
        llmdval -i [${llmd_ip}]:${llmd_ctl_p} -f llmd_cmdstat.xml 2>&1 > /dev/null
    fi

}

#-----------------------------------------------------------------------#
# getRunningProcessID
# 
# This function checks if a process is running.
# where:
#	${1} is the process id to check
# output:
#	If the process is running, echoes out the process id
#	If the process is not running, echoes out -1
#
#-----------------------------------------------------------------------#
function getRunningProcessID {

	local process_id=$1

	# Use ps and grep for the process id to see if the process is running
	if [ `ps -ef | grep -v grep | grep -c " ${process_id} "` -gt 0 ]
	then
        	echo ${process_id}
	else
        	echo "-1"
	fi
}


#-----------------------------------------------------------------------#
# deleteStores
#
# This function deletes the message stores that are used in the specified configuration file.
# Be careful when calling this function if multiple LLMDs are running in the test. Similar 
# to the corresponding functionality in llmd-start.sh
# where:
#	${CONFIG_FILE}: REQUIRED. The name of the LLMD configuration file where the message stores are specified.
#-----------------------------------------------------------------------#
# function deleteStores { 
	# # Parse message store directory names from the llmd configuration file
	# dirs=`grep -i "^[ \t]*Directory" ${CONFIG_FILE} | cut -d '=' -f 2`

	# if [[ ! -z ${dirs} ]]; then
	   	# # Remove the message store directories
   		# rm -fr ${dirs}
      		# echo "$?"
   	# fi
# }

#-----------------------------------------------------------------------#
# createStores
#
# This function creates the message stores that are used in the specified configuration file.
# Similar to the corresponding functionality in llmd-start.sh
# where:
#	${CONFIG_FILE}: REQUIRED. The name of the LLMD configuration file where the message stores are specified.
#-----------------------------------------------------------------------#
# function createStores { 
	# # Parse message store directory names from the llmd configuration file
	# dirs=`grep -i "^[ \t]*Directory" ${CONFIG_FILE} | cut -d '=' -f 2`


	# if [[ ! -z ${dirs} ]]; then
		# # Create the message store directories
		# mkdir -p ${dirs}
		# echo "$?"
        # fi
# }

#-----------------------------------------------------------------------#
#
#-----------------------------------------------------------------------#
 function getJavaVersion { 
  java -version 2> java.out >&1
  set +x
# THIS NEEDS TO THE SSHed TO GET RIGHT VALUE...
  javaVersionTag=`grep 'java version' java.out`
  # Is java installed - assume yes if string is found
  if [[ "${javaVersionTag}" == java?version* ]]
  then
    JAVA_VERSION=`echo ${javaVersionTag} | cut -d " " -f "3-"`
    JAVA_VERSION=`echo ${JAVA_VERSION} | cut -d '"' -f "2"`

    ibmTrue=`grep -c IBM java.out`
    if [ "${ibmTrue}" -ge "1" ]
    then
      JAVA_PROVIDER="IBM"
    else
      JAVA_PROVIDER="SUN"
    fi

    arch64True=`grep -c 64 java.out`
    if [ "${arch64True}" -ge "1" ]
    then
      JAVA_ARCH="64"
    else
      JAVA_ARCH="32"
    fi
    JAVA_ID="${JAVA_PROVIDER}_${JAVA_ARCH}-${JAVA_VERSION}"

  else
    JAVA_VERSION="NO JAVA INSTALLED"
    JAVA_ID="NO JAVA INSTALLED"
  fi

  if    [[ ${JAVA_ID} == "IBM_32-1.5.0" ]]; then JAVA_PATH="/opt/ibm/java2-i386-50/jre/bin/"
  elif  [[ ${JAVA_ID} == "IBM_64-1.5.0" ]]; then JAVA_PATH="/opt/ibm/java2-x86_64-50/jre/bin/"
  elif  [[ ${JAVA_ID} == "IBM_32-1.6.0" ]]; then JAVA_PATH="/opt/ibm/java-i386-60/jre/bin/"
  elif  [[ ${JAVA_ID} == "IBM_32-1.7.0" ]]; then JAVA_PATH="/opt/ibm/java-i386-70/jre/bin/"
  elif  [[ ${JAVA_ID} == "IBM_64-1.6.0" ]]; then JAVA_PATH="/opt/ibm/java-x86_64-60/jre/bin/"
  elif  [[ ${JAVA_ID} == "IBM_64-1.7.0" ]]; then JAVA_PATH="/opt/ibm/java-x86_64-70/jre/bin/"
  elif  [[ ${JAVA_ID} == "SUN_32-1.5.0_15" ]]; then JAVA_PATH="/usr/java/jre1.5.0_15_32bit/bin/"
  elif  [[ ${JAVA_ID} == "SUN_64-1.5.0_15" ]]; then JAVA_PATH="/usr/java/jre1.5.0_15_64bit/bin/"
  elif  [[ ${JAVA_ID} == "SUN_32-1.6.0_11" ]]; then JAVA_PATH="/usr/java/jre1.6.0_11_32bit/bin/"
  elif  [[ ${JAVA_ID} == "SUN_64-1.6.0_11" ]]; then JAVA_PATH="/usr/java/jre1.6.0_11_64bit/bin/"
  elif  [[ ${JAVA_ID} == "SUN_32-1.7.0" ]]; then JAVA_PATH="/usr/java/jre1.7.0_32bit/bin/"
  elif  [[ ${JAVA_ID} == "SUN_64-1.7.0" ]]; then JAVA_PATH="/usr/java/jre1.7.0_64bit/bin/"
  fi

  echo "This is:  JAVA_ID: ${JAVA_ID} with JAVA_PATH ${JAVA_PATH}"
  rm -f java.out

}

#-----------------------------------------------------------------------#
# startTestDriver
#
# This function starts the ${EXE_FILE}
# where:
#	${CONFIG_FILE}:	REQUIRED. The name of the xml configuration file.
#	${LOG_FILE}:	REQUIRED. The name of the log file for the driver output.
#	${OTHER_OPTS}:	OPTIONAL. Any other options (like -l 9 for the log level).
# output:
#	The only screen output MUST be the output from the call to getRunningProcessID; otherwise, testcase failures are likely.
#
#-----------------------------------------------------------------------#
function startTestDriver {
   l_testdriver="../scripts/testDriver.sh"
   td_desc="testDriver ${CONFIG_FILE} ${OTHER_OPTS} will be launched with addtional Env. Vars: ${ENVVARS}."

   echo " " >> ${LOG_FILE}
   echo "----------------------- Beginning of testDriver -----------------------" >> ${LOG_FILE}
   echo ${td_desc} >> ${LOG_FILE}
   echo " " >> ${LOG_FILE}

   # Start the test Driver
   if [ "${ENVVARS}" != "" ]; then
     export ${ENVVARS}
   fi
   SCREEN_OUTFILE=`echo ${LOG_FILE} | sed "s/\.log/\.screenout\.log/"`

   ${l_testdriver} ${CONFIG_FILE} ${OTHER_OPTS} -f ${LOG_FILE} 1> ${SCREEN_OUTFILE} 2>&1 &
	
   DRIVER_PID=$!
   echo `getRunningProcessID ${DRIVER_PID}`
}


#-----------------------------------------------------------------------#
# startCAppDriver
#
# This function starts the C Application Driver
# where:
#   ${EXE_FILE}:    REQUIRED. The name of the "C" executable file to run
#   ${LOG_FILE}:	REQUIRED. The name of the log file for the driver output.
#   ${OTHER_OPTS}:	OPTIONAL. Any other options (like -l 9 for the log level).
# output:
#	The only screen output MUST be the output from the call to getRunningProcessID; otherwise, testcase failures are likely.
#
#-----------------------------------------------------------------------#
function startCAppDriver {
   td_desc="@startCAppDriver == ${EXE_FILE}  ${OTHER_OPTS} will be launched with addtional Env. Vars: ${ENVVARS}."

   echo " " >> ${LOG_FILE}
   echo "----------------------- Beginning of ${EXE_FILE} -----------------------" >> ${LOG_FILE}
   echo ${td_desc} >> ${LOG_FILE}
   echo " " >> ${LOG_FILE}

   # Start the C App Driver
   if [ "${ENVVARS}" != "" ]; then
     export ${ENVVARS}
   fi

   SCREEN_OUTFILE=`echo ${LOG_FILE} | sed "s/\.log/\.screenout\.log/"`

   ${EXE_FILE}  ${OTHER_OPTS}                1> ${SCREEN_OUTFILE} 2>&1 &
#	${EXE_FILE}  ${OTHER_OPTS} -f ${LOG_FILE} 1> ${SCREEN_OUTFILE} 2>&1 &

   DRIVER_PID=$!
   pidcheck=`getRunningProcessID ${DRIVER_PID}`
   echo "The process id is reported as: ${pidcheck}" >> ${LOG_FILE}
   # If more than one firefox process is started, the newer one may eventually run under the older process
   # In this case, getRunningProcessID will return -1 because the original process will be gone. When this 
   # happens, try to assign it the process id for the firefox process that is still running.
   if [ "${pidcheck}" == "-1" ]
   then
       case ${EXE_FILE} in
         firefox*)
            ostype=`uname`
            case ${ostype} in
                CYGWIN*)
                    ps -efW | grep firefox >> ${LOG_FILE}
                    PIDVAR=`ps -efW | grep firefox`
                    ;;
                *)
                    ps -f -C firefox | grep firefox  >> ${LOG_FILE}
                    PIDVAR=`ps -f -C firefox | grep firefox`
                    ;;
            esac
            PIDVAR=`echo ${PIDVAR}`
            PIDVAR=${PIDVAR#* }
            PIDVAR=${PIDVAR%% *}
            if [ "${PIDVAR}" != "" ]
            then
                pidcheck=${PIDVAR}
            else
                pidcheck="-1"
            fi
            echo "The new process id is: ${pidcheck}" >> ${LOG_FILE}
            ;;
       esac
   fi
   echo ${pidcheck}
}

#-----------------------------------------------------------------------#
# startCAppDriverRC
#
# This function starts the C Application Driver, execs RC is echoed in screenout.log and checked later.
# where:
#   ${EXE_FILE}:    REQUIRED. The name of the "C" executable file to run
#   ${LOG_FILE}:	REQUIRED. The name of the log file for the driver output.
#   ${OTHER_OPTS}:	OPTIONAL. Any other options (like -l 9 for the log level).
# output:
#	The only screen output MUST be the output from the call to getRunningProcessID; otherwise, testcase failures are likely.
#
#-----------------------------------------------------------------------#
function startCAppDriverRC {
   td_desc="@startCAppDriverRC == ${EXE_FILE}  ${OTHER_OPTS} will be launched with addtional Env. Vars: ${ENVVARS}."

   echo " " >> ${LOG_FILE}
   echo "----------------------- Beginning of ${EXE_FILE} -----------------------" >> ${LOG_FILE}
   echo ${td_desc} >> ${LOG_FILE}
   echo " " >> ${LOG_FILE}

   # Start the C App Driver
   if [ "${ENVVARS}" != "" ]; then
     export ${ENVVARS}
   fi

   SCREEN_OUTFILE=`echo ${LOG_FILE} | sed "s/\.log/\.screenout\.log/"`

   ../scripts/echoRC.sh ${EXE_FILE}  ${OTHER_OPTS}                1> ${SCREEN_OUTFILE} 2>&1 &
#	${EXE_FILE}  ${OTHER_OPTS} -f ${LOG_FILE} 1> ${SCREEN_OUTFILE} 2>&1 &

   DRIVER_PID=$!
   pidcheck=`getRunningProcessID ${DRIVER_PID}`
   echo "The process id is reported as: ${pidcheck}" >> ${LOG_FILE}
   # If more than one firefox process is started, the newer one may eventually run under the older process
   # In this case, getRunningProcessID will return -1 because the original process will be gone. When this 
   # happens, try to assign it the process id for the firefox process that is still running.
   if [ "${pidcheck}" == "-1" ]
   then
       case ${EXE_FILE} in
         firefox*)
            ostype=`uname`
            case ${ostype} in
                CYGWIN*)
                    ps -efW | grep firefox >> ${LOG_FILE}
                    PIDVAR=`ps -efW | grep firefox`
                    ;;
                *)
                    ps -f -C firefox | grep firefox  >> ${LOG_FILE}
                    PIDVAR=`ps -f -C firefox | grep firefox`
                    ;;
            esac
            PIDVAR=`echo ${PIDVAR}`
            PIDVAR=${PIDVAR#* }
            PIDVAR=${PIDVAR%% *}
            if [ "${PIDVAR}" != "" ]
            then
                pidcheck=${PIDVAR}
            else
                pidcheck="-1"
            fi
            echo "The new process id is: ${pidcheck}" >> ${LOG_FILE}
            ;;
       esac
   fi
   echo ${pidcheck}
}

#-----------------------------------------------------------------------#
# startCAppDriverLog
#
# This function starts the C Application Driver where LOGFILE will be checked for SUCCESS.

# where:
#   ${EXE_FILE}:    REQUIRED. The name of the "C" executable file to run
#   ${LOG_FILE}:	REQUIRED. The name of the log file for the driver output.
#   ${OTHER_OPTS}:	OPTIONAL. Any other options (like -l 9 for the log level).
# output:
#	The only screen output MUST be the output from the call to getRunningProcessID; otherwise, testcase failures are likely.
#
#-----------------------------------------------------------------------#
function startCAppDriverLog {
   td_desc="${EXE_FILE}  ${OTHER_OPTS} will be launched with addtional Env. Vars: ${ENVVARS}."

   echo " " >> ${LOG_FILE}
   echo "----------------------- Beginning of ${EXE_FILE} -----------------------" >> ${LOG_FILE}
   echo ${td_desc} >> ${LOG_FILE}
   echo " " >> ${LOG_FILE}

   # Start the C App Driver
   if [ "${ENVVARS}" != "" ]; then
     export ${ENVVARS}
   fi
   SCREEN_OUTFILE=`echo ${LOG_FILE} | sed "s/\.log/\.screenout\.log/"`

   ${EXE_FILE}  ${OTHER_OPTS} -f ${LOG_FILE} 1> ${SCREEN_OUTFILE} 2>&1 &

   DRIVER_PID=$!
   echo `getRunningProcessID ${DRIVER_PID}`
}

#-----------------------------------------------------------------------#
# startCAppDriverCfg  or subController
#
# This function starts the C App Driver with Configuration file
# where:
#	${CONFIG_FILE}:	REQUIRED. The name of the xml configuration file.
#	${LOG_FILE}:	REQUIRED. The name of the log file for the driver output.
# output:
#	The only screen output MUST be the output from the call to getRunningProcessID; otherwise, testcase failures are likely.
#
#-----------------------------------------------------------------------#
function startCAppDriverCfg {

   l_testDriver=${EXE_FILE}
   td_desc="${l_testDriver} ${CONFIG_FILE} ${OTHER_OPTS} will be launched with addtional Env. Vars: ${ENVVARS}."

   echo " " >> ${LOG_FILE}
   echo "----------------------- Beginning of ${l_testDriver} -----------------------" >> ${LOG_FILE}
   echo ${td_desc} >> ${LOG_FILE}
   echo " " >> ${LOG_FILE}

   # Start the subController, Need to pass the Env Vars to subController, not set them here
#   if [ "${ENVVARS}" != "" ]; then
#     export ${ENVVARS}
#   fi
   SCREEN_OUTFILE=`echo ${LOG_FILE} | sed "s/\.log/\.screenout\.log/"`

   if [ "${DEBUG}" == "ON" ]
   then
      echo    ${l_testDriver} ${CONFIG_FILE} ${LOG_FILE} ${ENVVARS}     1> ${SCREEN_OUTFILE} 2>&1 
   fi

#   ${l_testDriver} ${CONFIG_FILE}                 1> ${SCREEN_OUTFILE} 2>&1 &
   ${l_testDriver} ${CONFIG_FILE} ${LOG_FILE} ${ENVVARS}     1> ${SCREEN_OUTFILE} 2>&1 &
	
   DRIVER_PID=$!
   echo `getRunningProcessID ${DRIVER_PID}`
}

#-----------------------------------------------------------------------#
# startJavaAppDriver
#
# This function starts the Java Application Driver
# where:
#   ${EXE_FILE}:    REQUIRED. The name of the "C" executable file to run
#   ${LOG_FILE}:    REQUIRED. The name of the log file for the driver output.
#   ${OTHER_OPTS}:  OPTIONAL. Any other options (like -l 9 for the log level).
# output:
#	The only screen output MUST be the output from the call to getRunningProcessID; otherwise, testcase failures are likely.
#   A special !!COMMA!! replacement is needed for those rare cases a comma is 
#   required as input on a parameter to a Java program. Earlier scripts parse
#	using a comma, so it has to be modified here.
#
#-----------------------------------------------------------------------#
function startJavaAppDriver {
   td_desc="${EXE_FILE}  ${OTHER_OPTS} will be launched with additional Env. Vars:  ${ENVVARS}."
   
   OTHER_OPTS=`echo ${OTHER_OPTS} | sed 's/!!COMMA!!/,/'`
   

   echo " " >> ${LOG_FILE}
   echo "----------------------- Beginning of ${EXE_FILE} -----------------------" >> ${LOG_FILE}
   echo ${td_desc} >> ${LOG_FILE}
   echo " " >> ${LOG_FILE}

   # Start the Java App Driver
   if [ "${ENVVARS}" != "" ]; then
      if [ "${ENVVARS:0:9}" = "JVM_ARGS=" ]; then
         export "${ENVVARS}"
      else  
         export ${ENVVARS}
      fi
   fi
   SCREEN_OUTFILE=`echo ${LOG_FILE} | sed "s/\.log/\.screenout\.log/"`

#   echo "`java ${JVM_ARGS} -cp ${CLASSPATH} ${EXE_FILE}  ${OTHER_OPTS}  1> ${SCREEN_OUTFILE} 2>&1` " &
   java ${JVM_ARGS} -cp ${CLASSPATH} ${EXE_FILE} ${OTHER_OPTS} 1> ${SCREEN_OUTFILE} 2>&1 &

   DRIVER_PID=$!
   echo `getRunningProcessID ${DRIVER_PID}`
}

#-----------------------------------------------------------------------#
# startJavaAppDriverRC
#
# This function starts the Java Application Driver using echoRC.sh so that the Return Code is saved in *.screenout.log
# where:
#   ${EXE_FILE}:    REQUIRED. The name of the "C" executable file to run
#   ${LOG_FILE}:    REQUIRED. The name of the log file for the driver output.
#   ${OTHER_OPTS}:  OPTIONAL. Any other options (like -l 9 for the log level).
# output:
#	The only screen output MUST be the output from the call to getRunningProcessID; otherwise, testcase failures are likely.
#
#-----------------------------------------------------------------------#
function startJavaAppDriverRC {
   td_desc="${EXE_FILE}  ${OTHER_OPTS} will be launched with additional Env. Vars:  ${ENVVARS}."

   echo " " >> ${LOG_FILE}
   echo "----------------------- Beginning of ${EXE_FILE} -----------------------" >> ${LOG_FILE}
   echo ${td_desc} >> ${LOG_FILE}
   echo " " >> ${LOG_FILE}

   # Start the Java App Driver
   if [ "${ENVVARS}" != "" ]; then
      if [ "${ENVVARS:0:9}" = "JVM_ARGS=" ]; then
         export "${ENVVARS}"
      else  
         export ${ENVVARS}
      fi
   fi
   SCREEN_OUTFILE=`echo ${LOG_FILE} | sed "s/\.log/\.screenout\.log/"`

../scripts/echoRC.sh java ${JVM_ARGS} -cp ${CLASSPATH} ${EXE_FILE} ${OTHER_OPTS} 1> ${SCREEN_OUTFILE} 2>&1 &

   DRIVER_PID=$!
   echo `getRunningProcessID ${DRIVER_PID}`
}

#-----------------------------------------------------------------------#
# startJavaAppDriverCfg
#
# This function starts the Java App Driver with Configuration file
# where:
#   ${CONFIG_FILE}: REQUIRED. The name of the xml configuration file.
#   ${LOG_FILE}:    REQUIRED. The name of the log file for the driver output.
# output:
#	The only screen output MUST be the output from the call to getRunningProcessID; otherwise, testcase failures are likely.
#
#-----------------------------------------------------------------------#
function startJavaAppDriverCfg {
   l_testDriver="READ THE EXE_FILE from the CONFIG_FILE"
   td_desc="${l_testDriver} ${CONFIG_FILE} will be launched with additional Env. Vars:  ${ENVVARS}."

   echo " " >> ${LOG_FILE}
   echo "----------------------- Beginning of ${l_testDriver} -----------------------" >> ${LOG_FILE}
   echo ${td_desc} >> ${LOG_FILE}
   echo " " >> ${LOG_FILE}

   # Start the Java App Driver
   if [ "${ENVVARS}" != "" ]; then
     export ${ENVVARS}
   fi
   SCREEN_OUTFILE=`echo ${LOG_FILE} | sed "s/\.log/\.screenout\.log/"`

#   ${l_testDriver} ${CONFIG_FILE}                1> ${SCREEN_OUTFILE} 2>&1 &
   java -cp ${CLASSPATH} ${l_testDriver} ${CONFIG_FILE} 1> ${SCREEN_OUTFILE} 2>&1 &

   DRIVER_PID=$!
   echo `getRunningProcessID ${DRIVER_PID}`
}

#-----------------------------------------------------------------------#
# startSync
#
# This function starts the synchronization server (DriverSync) 
# where:
#	${LOG_FILE}:	REQUIRED. The name of the log file for the sync server output
#	${PORT}:	REQUIRED. The name of the port to start the synchronization server on 
# output:
#	The only screen output MUST be the output from the call to getRunningProcessID; otherwise, testcase failures are likely.
#
#-----------------------------------------------------------------------#
function startSync {
	# Start the synchronization server
        java test.llm.TestServer ${PORT} 1> ${LOG_FILE} 2>&1 &

	SYNC_PID=$!
	MAX_TRIES=3

	# See if the process is running
	# If not, sleep 2 seconds then retry up to MAX_TRIES number of tries before failing	
	__is_running=`getRunningProcessID ${SYNC_PID}`
	let tries=1
	while [[ ${__is_running} -eq -1 && ${tries} < ${MAX_TRIES} ]]
	do
		sleep 2
		let tries=${tries}+1
		__is_running=`getRunningProcessID ${SYNC_PID}`
	done
	echo ${__is_running}
 
        

}
	

#---------------------------------------------------------------------------
# killComponent
#
# This function kills a specified component, using the COMP_INDEX set when
# the component is first started. 
#
# The primary usage of this function is for RCMS tests, where often one member.
# of a tier must be killed (ungracefully) to verify that failover behavior
# works as expected.  This can be preceded by the sleep component in
# run_scenarios to configure the exact time during execution that the desired
# component will be killed.
#
# The process id returned by this function may not be very reliable when killing
# off processes at the end of a test, but the likelyhood of these processes hanging
# seems slim.  
# input:
#	${COMP_PID}:	REQUIRED. The PID of the component to kill.
# output:
#	The only screen output MUST be the output from the call to getRunningProcessID; otherwise, testcase failures are likely.
#
#---------------------------------------------------------------------------
function killComponent {

	if [[ "" =  "${CUENUM}" ]]
	then
		kill -9 ${COMP_PID} 1>/dev/null 2>&1
		compkill_pid=$!
		echo `getRunningProcessID ${compkill_pid}`
	else
		COMMAND="kill -9 ${COMP_PID}"
		runCommandWithCues
	fi
}

#---------------------------------------------------------------------------
# searchLogsWithCues
#
# This function searches through log files for specific messages, and compares
# the number of times each message occurs with the number of times that it is
# expected to occur. The script will only search the log files that are on
# the machine where it is called, so there may be a need to have two separate 
# compare files, depending on which log files should be checked.
#
# The number of times that a message is expected to occur, along with the
# message text, and other information is specified in a file that is passed 
# to this function. The convention for files like this is to use a .compare
# extension.
# 
# This function can be used to search the log files at a specific point in 
# a test by means of the DriverSync tool, similarly to the other "WithCues" 
# functions.
# 
# To specify the point in the test when the logs should be searched, add a 
# SyncAction into the test's XML configuration file for the LLM Driver that 
# sets a variable called cue_wait to 1.  This will be an indication that the 
# logs should then be searched. After the call to search the logs has been 
# has completed, this function will cause the cue_set variable to be set to 1,
# so another SyncAction can be added in the test's XML configuration file 
# after the first one that waits for the cue_set variable to be 1 before
# moving on with the test.
# 
# where:
#	${COMPARE_FILE}: REQUIRED. The file that contains the information for the log search.
#	${RUN_NUM}: REQUIRED. Must be either 1 or 2. Allows up to two different searchLogsWithCues to be running at the same time. One to run on machine 1 and one on machine 2.
# output:
#	The only screen output MUST be the output from the call to getRunningProcessID; otherwise, testcase failures are likely
# A file is created in the working directory called ${COMPARE_FILE}.log that will contain the results of the test.
#	A script is created on the machine called temp_searchLogs_script.sh.  Once the test has ended, this script can be removed.
#
#---------------------------------------------------------------------------
function searchLogsWithCues {

	# Set the log file name for searchLogs.
	# Only use the basename of the compare file,
	# so that the log will be written to the working directory.
	COMPARE_LOG=`echo ${COMPARE_FILE} | awk -F'/' '{ print $NF }'`.log

	# Create script
	echo "#! /bin/bash" > temp_searchlogs_script.sh
	echo "testDriver searchlogs_control.xml -n cue_wait${RUN_NUM} 1> /dev/null 2>&1" >> temp_searchlogs_script.sh
	echo "../bin/searchLogs.sh ${COMPARE_FILE} 1> ${COMPARE_LOG} 2>&1" >> temp_searchlogs_script.sh
	echo "testDriver searchlogs_control.xml -n cue_set${RUN_NUM} 1> /dev/null 2>&1" >> temp_searchlogs_script.sh
	
	chmod +x temp_searchlogs_script.sh
	./temp_searchlogs_script.sh 1>/dev/null 2>&1 &
	searchlogs_pid=$!
	echo `getRunningProcessID ${searchlogs_pid}`

}

#---------------------------------------------------------------------------
# searchLogs
#
# This function searches through log files for specific messages, and compares
# the number of times each message occurs with the number of times that it is
# expected to occur. The script will only search the log files that are on
# the machine where it is called, so there may be a need to have two separate 
# compare files, depending on which log files should be checked.
#
# The number of times that a message is expected to occur, along with the
# message text, and other information is specified in a file that is passed 
# to this function. The convention for files like this is to use a .compare
# extension.
# 
# This function can be used to search the log files at the time that it is
# called (such as at the end of the test).
# 
# where:
#	${COMPARE_FILE}: REQUIRED. The file that contains the information for the log search.
# output:
#	The only screen output MUST be the output from the call to getRunningProcessID; otherwise, testcase failures are likely
# A file is created in the working directory called ${COMPARE_FILE}.log that will contain the results of the test.
#
#---------------------------------------------------------------------------
function searchLogs {
	# Set the log file name for searchLogs.
	# Only use the basename of the compare file,
	# so that the log will be written to the working directory.
	COMPARE_LOG=`echo ${COMPARE_FILE} | awk -F'/' '{ print $NF }'`.log
	../scripts/searchLogs.sh ${COMPARE_FILE} 1> ${COMPARE_LOG} 2>&1

}

#---------------------------------------------------------------------------
# setSavePassedLogs
#
# This function is used to set/unset an environment variable that determines whether
# or not logs from PASSED executions are saved.  The default behavior is to only
# save the logs from execution runs that had at least one FAIL (e.g. 'setSavePassedLogs off').
#
# where:
#  ${1}: setting REQUIRED. One of {on, off}.  All runs after "on" is set will save passed logs.
#  ${2}: logfile REQUIRED. The logfile to which we output a message reporting the setting or "NONE".
#---------------------------------------------------------------------------
function setSavePassedLogs {
  SETTING=$1
  LOGFILE=$2

  if [[ ($# -ne 2) ]] ; then
    echo "Incorrect number of arguments.  setSavePassedLogs will not execute."
    exit 1;
  fi
  
  if [[ "${SETTING}" == "on" ]] ; then
    diff=1
  else
    diff=-1
  fi

  if [[ "${save_passed_logs}" = "" ]]; then
    save_passed_logs=0
  fi

  val=$( expr ${save_passed_logs} + ${diff} )
  export save_passed_logs=${val}
  if [[ ! ${LOGFILE} == "NONE" ]];  then
    echo "save_passed_logs = ${save_passed_logs}" | tee -a ${LOGFILE}
  fi
}

#---------------------------------------------------------------------------
# getSavePassedLogs
#
# This accessor function outputs the value of the environment variable defined by setSavePassedLogs.
# If setSavePassedLogs was never called, the value will be "off"
#
# This function should be used as follows:
#   ex. if [[ $(getSavePassedLogs) == "on" ]] ; then ...
#---------------------------------------------------------------------------
function getSavePassedLogs {
  if [[ ${save_passed_logs} -gt 0 ]] ; then
    echo "on"
  else
    echo "off"
  fi
}

#---------------------------------------------------------------------------
# getSavePassedLogsCmd
#
# This function outputs the command used to set the "save passed logs" variable.
# This command can be used to replicate the current value of the environment variable 
# on a remote machine.
#---------------------------------------------------------------------------
function getSavePassedLogsCmd {
  echo "export save_passed_logs=${save_passed_logs}"
}

#---------------------------------------------------------------------------
# setComponentSleep
#
# This function is used to set an environment variable that specifies the  
# amount of time to sleep in between starting components. After the tests have 
# completed, this function should be called again with the parameter "default"
# to reset the variable back to the default sleep time for other tests.
#
# where:
#  ${1}: setting REQUIRED. An integer specifying the number of seconds to sleep or "default".
#	
#  ${2}: logfile REQUIRED. The logfile to which we output a message reporting the setting or "NONE".
#---------------------------------------------------------------------------
function setComponentSleep {
  SETTING=$1
  LOGFILE=$2

  if [[ ($# -ne 2) ]] ; then
    echo "Incorrect number of arguments.  setComponentSleep will not execute."
    exit 1;
  fi
  
  if [[ "${SETTING}" == "default" ]] ; then
    unset component_sleep
  else
    export component_sleep=${SETTING}
  fi

  if [[ ! ${LOGFILE} == "NONE" ]];  then
    echo "component_sleep = ${component_sleep}" | tee -a ${LOGFILE}
  fi
}



#---------------------------------------------------------------------------
# setStopImaserverTimeout
#
# This function is used to set an environment variable that specifies the  
# amount of time for run-scenarios to wait for imaserver to stop. After the tests have 
# completed, this function should be called again with the parameter "default"
# to reset the variable back to the default vaule for other tests.
#
# where:
#  ${1}: setting REQUIRED. An integer specifying the number of seconds to wait or "default".
#	
#  ${2}: logfile REQUIRED. The logfile to which we output a message reporting the setting or "NONE".
#---------------------------------------------------------------------------
function setStopImaserverTimeout {
  SETTING=$1
  LOGFILE=$2

  if [[ ($# -ne 2) ]] ; then
    echo "Incorrect number of arguments.  setStopImaserverTimeout will not execute."
    exit 1;
  fi
  
  if [[ "${SETTING}" == "default" ]] ; then
    unset stop_imaserver_timeout 
  else
    export stop_imaserver_timeout=${SETTING}
  fi

  if [[ ! ${LOGFILE} == "NONE" ]];  then
    echo "stop_imaserver_timeout = ${stop_imaserver_timeout}" | tee -a ${LOGFILE}
  fi
}


#---------------------------------------------------------------------------
# setStartImaserverTimeout
#
# This function is used to set an environment variable that specifies the  
# amount of time for run-scenarios to wait for imaserver to start. After the tests have 
# completed, this function should be called again with the parameter "default"
# to reset the variable back to the default vaule for other tests.
#
# where:
#  ${1}: setting REQUIRED. An integer specifying the number of seconds to wait or "default".
#	
#  ${2}: logfile REQUIRED. The logfile to which we output a message reporting the setting or "NONE".
#---------------------------------------------------------------------------
function setStartImaserverTimeout {
  SETTING=$1
  LOGFILE=$2

  if [[ ($# -ne 2) ]] ; then
    echo "Incorrect number of arguments.  setStartImaserverTimeout will not execute."
    exit 1;
  fi
  
  if [[ "${SETTING}" == "default" ]] ; then
    unset start_imaserver_timeout 
  else
    export start_imaserver_timeout=${SETTING}
  fi

  if [[ ! ${LOGFILE} == "NONE" ]];  then
    echo "start_imaserver_timeout = ${start_imaserver_timeout}" | tee -a ${LOGFILE}
  fi
}


#-----------------------------------------------------------------------
# supportSharedMemory
#
# This function determines if the OS supports shared memory
#-----------------------------------------------------------------------
function supportSharedMemory {

    llmd_host=${llmd_user}@${llmd_ip}
    if [[ -z ${llmd_host} ]]; then
      echo "FAILED:  A userid@hostname could not be found.  Use parameter"
      echo "         to specify the userid@hostname."
      echo "         $1: userid@hostname"
    fi

  if [[ ${llmd_ip} == ${localbox_ip} ]] ; then
    ostype=$(uname)
    is_zLinux=$(uname -m | grep s390 | wc -l)
  else
    ostype=$(ssh ${llmd_host} "uname")
    is_zLinux=$(ssh ${llmd_host} "uname -m" | grep s390 | wc -l)
  fi
  if [[ (${is_zLinux} -ne 0) ]] || [[ (${ostype} = "HP-UX") ]] ; then
    echo "off"
  else
    echo "on"
  fi 

}

#---------------------------------------------------------------------------
# runCommandWithCues
#
# This function runs the specified command at a time coordinated by the Sync Driver
# 
# To specify the point in the test when the command should run, add a 
# SyncAction into the test's XML configuration file for the LLM Driver that 
# sets a variable called cue_wait to ${CUE_NUM}.  This will be an indication that the 
# command should then be executed. After the command has completed, this function 
# will cause the cue_set variable to be set to ${CUE_NUM}, so another SyncAction can be 
# added in the test's XML configuration file after the first one that waits for
# the cue_set variable to be 1 before moving on with the test. If the command returns 
# an exit code other than 0, a testcase failure will be reported.
# 
# where:
#	${COMMAND}: REQUIRED. The command that should be executed. Keep in mind, this can be a script. No parameters should be specified here, but if parameters are required it is recommended to create a script that calls the commmand, and call the script from this function.
#	${CUENUM}: REQUIRED. The number that the cue_wait and cue_set variables will be set to. 
# output:
#	The only screen output MUST be the output from the call to getRunningProcessID; otherwise, testcase failures are likely
#	Any output that the command generates will be redirected to a file called ${COMMAND}.${CUENUM}.log	
#
#	A script is created on the machine called temp_command_script.sh.  Once the test has ended, this script can be removed.
#
#---------------------------------------------------------------------------
function runCommandWithCues {

	# First, replace the CUENUM_REPLACE with the number passed in as CUENUM
    #//    sed "s/CUENUM_REPLACE/${CUENUM}/" runcommand_control.xml > runcommand_control_gen_${CUENUM}.xml	 

	# Set up a log name using just the base name of the command
	CHECK_RESULT=`echo ${COMMAND} | wc -w`
	if [  ${CHECK_RESULT} == 1 ]
	then
	    if [ -f ${COMMAND} ]
	    then
		    LOGNAME=`echo ${COMMAND} | awk -F'/' '{ print $NF }'`
	    else
		   LOGNAME=${COMMAND}
		fi
	else
		LOGNAME=`echo ${COMMAND} | cut -d" " -f 1`
	fi
	if [ "${SOLUTION}" == "" ]
	then
		SOLUTION="tmrefcount"
	fi

	# Create script
	echo "#! /bin/bash" > temp_command_${CUENUM}_script.sh
	echo "java test.llm.TestClient ${SYNCPORT} -n ${M1_IPv4_1} -s ${SOLUTION} -u 3 cue_wait ${CUENUM} 1> /dev/null 2>&1" >> temp_command_${CUENUM}_script.sh
	echo "${COMMAND} 1> ${LOGNAME}.${CUENUM}.log 2>&1" >> temp_command_${CUENUM}_script.sh
	echo "echo \$? > temp_runcmd_${CUENUM}.rc" >> temp_command_${CUENUM}_script.sh
	echo "java test.llm.TestClient ${SYNCPORT} -n ${M1_IPv4_1} -s ${SOLUTION} -u 2 cue_set ${CUENUM} 1> /dev/null 2>&1" >> temp_command_${CUENUM}_script.sh
	
       if [ "${ENVVARS}" != "" ]; then
  	  export ${ENVVARS}
       fi
	chmod +x temp_command_${CUENUM}_script.sh
	./temp_command_${CUENUM}_script.sh 1>/dev/null 2>&1 &
	command_pid=$!
	echo `getRunningProcessID ${command_pid}`

}

#---------------------------------------------------------------------------
# runCommand
#
# This function runs the specified command immediately open execution
# 
# where:
#	${COMMAND}: REQUIRED. The command that should be executed. Keep in mind, this can be a script.  No parameters should be specified here, but if parameters are required it is recommended to create a script that calls the commmand, and call the script from this function.  This command string will have all consecutive underscore characters replaced by whitespace before execution.  This is useful for passing arguments, e.g. "COMMAND=script.sh__arg1__arg2"
# output:
#	The only screen output MUST be the output from the call to getRunningProcessID; otherwise, testcase failures are likely
#	Any output that the command generates will be redirected to a file called ${COMMAND}.log
#
#---------------------------------------------------------------------------
function runCommand {

	# Set up a log name using just the base name of the command
	RUNCOMMAND=`echo ${COMMAND} | sed 's/__/ /g'`
       if [ "${ENVVARS}" != "" ]; then
  	  export ${ENVVARS}
       fi
	    if [ -f ${COMMAND} ]
	    then
		    LOGNAME=`echo ${COMMAND} | awk -F'/' '{ print $NF }'`
	    else
		   LOGNAME=${COMMAND}
		fi

	./${RUNCOMMAND} 1> ${LOGNAME}.log 2>&1
	echo $? > ${LOGNAME}.rc

	command_pid=$!
	echo `getRunningProcessID ${command_pid}`
}


#-----------------------------------------------------------------------#
# startJMSDriver
#
# This function starts the JMSDriver
# where:
#	${CONFIG_FILE}:  REQUIRED. The name of the xml configuration file.
#	${NAME}:         REQUIRED. The member name of the compositeAction to start. Often, it's something like: rmdt1 or rmdr1
#	${LOG_FILE}:     REQUIRED. The name of the log file for the driver output.
#	${OTHER_OPTS}:   OPTIONAL. Any other options (like -l 9 for the log level).
# output:
#	The only screen output MUST be the output from the call to getRunningProcessID; otherwise, testcase failures are likely.
#
#-----------------------------------------------------------------------#
function startJMSDriver {

    SCREEN_OUTFILE=`echo ${LOG_FILE} | sed "s/\.log/\.screenout\.log/"`
    echo "Start of JMSDriver screenout.log file." > $SCREEN_OUTFILE
    # Verify java environment has been set on the local machine
    if [[ ($(java 2>&1 | grep Usage | wc -l) -eq 0) ]] ; then
        echo "Can't locate java program on the local machine. Please make sure the PATH variable contains the path where java is located." >> $SCREEN_OUTFILE
        echo "The current PATH variable setting is:" >> $SCREEN_OUTFILE
        echo $PATH >> $SCREEN_OUTFILE
        exit 1
    fi

    # Verify the classpath on the local machine is set correctly
#    f1=$(echo ${CLASSPATH} | grep providerutil.jar | wc -l)
#    f2=$(echo ${CLASSPATH} | grep fscontext.jar | wc -l)
#    f3=$(echo ${CLASSPATH} | grep imaclientjms.jar | wc -l)
#    f4=$(echo ${CLASSPATH} | grep JMSTestDriver.jar | wc -l)
#    if [[ (${f1} -ne 1) || (${f2} -ne 1) || (${f3} -ne 1) || (${f4} -ne 1)  ]] ; then
#        echo "Some of providerutil.jar, fscontext.jar, imaclientjms.jar and JMSTestDriver.jar"
#        echo "cannot be found from the CLASSPATH on the local machine."
#        echo "Please make sure the CLASSPATH variable been set correctly to match your test environment."
#        echo "The current CLASSPATH setting is:"
#        echo $CLASSPATH
#        exit 1
#     fi

    # Start the JMS Driver
    if [ "${ENVVARS}" != "" ]; then
      export ${ENVVARS}
    fi
    
    CLASS=com.ibm.ima.jms.test.JMSTestDriver
    l_jmsdriver="java ${BITFLAG} -cp ${CLASSPATH} ${CLASS}"

    # Take out any underscores and put in spaces in the other options
    OTHER_OPTS=`echo ${OTHER_OPTS} | sed 's/_/ /'`
    NAME_STR="-n ${NAME}"
    if [[ "${NAME}" == "ALL" ]] ; then
        NAME_STR=""
    fi
    echo ${l_jmsdriver} ${CONFIG_FILE} ${NAME_STR} ${OTHER_OPTS} -f ${LOG_FILE} >> ${SCREEN_OUTFILE}
    ${l_jmsdriver} ${CONFIG_FILE} ${NAME_STR} ${OTHER_OPTS} -f ${LOG_FILE} 1>> ${SCREEN_OUTFILE} 2>&1 &
       
    JMSDRIVER_PID=$!
    echo `getRunningProcessID ${JMSDRIVER_PID}`
}

#-----------------------------------------------------------------------#
# startWSDriver
#
# This function starts the WSDriver
# where:
#	${CONFIG_FILE}:  REQUIRED. The name of the xml configuration file.
#	${NAME}:         REQUIRED. The member name of the compositeAction to start. Often, it's something like: rmdt1 or rmdr1
#	${LOG_FILE}:     REQUIRED. The name of the log file for the driver output.
#	${OTHER_OPTS}:   OPTIONAL. Any other options (like -l 9 for the log level).
# output:
#	The only screen output MUST be the output from the call to getRunningProcessID; otherwise, testcase failures are likely.
#
#-----------------------------------------------------------------------#
function startWSDriver {

    # Verify java environment has been set on the local machine
    if [[ ($(java 2>&1 | grep Usage | wc -l) -eq 0) ]] ; then
        echo "Can't locate java program on the local machine. Please make sure the PATH variable contains the path where java is located."
        echo "The current PATH variable setting is:"
        echo $PATH
        exit 1
    fi

    CLASS=com.ibm.ism.ws.test.WSTestDriver

    # Start the WS test Driver
    if [ "${ENVVARS}" != "" ]; then
      export ${ENVVARS}
    fi

    l_wsdriver="java ${BITFLAG} -cp ${CLASSPATH} ${CLASS}"
    SCREEN_OUTFILE=`echo ${LOG_FILE} | sed "s/\.log/\.screenout\.log/"`
    # Take out any underscores and put in spaces in the other options
    OTHER_OPTS=`echo ${OTHER_OPTS} | sed 's/_/ /'`
        NAME_STR="-n ${NAME}"
        if [[ "${NAME}" == "ALL" ]] ; then
            NAME_STR=""
        fi
 
    echo ${l_wsdriver} ${CONFIG_FILE} ${NAME_STR} ${OTHER_OPTS} -f ${LOG_FILE} > ${SCREEN_OUTFILE}
    ${l_wsdriver} ${CONFIG_FILE} ${NAME_STR} ${OTHER_OPTS} -f ${LOG_FILE} 1>> ${SCREEN_OUTFILE} 2>&1 &
	
    DRIVER_PID=$!
    echo `getRunningProcessID ${DRIVER_PID}`
}




#-------------------------------------
# Test template Functions
#
# Use these functions to have comparetest files automatically generated and transferred to the 
# necessary Mx machines. This saves time of having to create and maintain comparetest files by hand.
#
#  Usage: Using the Test template is a 5 step process outlined below, but it saves you time because you no longer have to maintain separate comparetest files.
# 
# 1. To use this helper interface the user must call test_template_initialize_test with the name of the test.
# Exmaple:
#  xml[${n}]="atelm_01_00"
#   test_template_initialize_test "${xml[${n}]}"
#
# 2. To use this helper interface the user must then specify compare test strings, counts (optional)  or metrics to add.
# Example: The user specified that the string "SVTVehicleScale Success" must be found in the output for component 2 with 1 count). 
#          The user also specifies to generate metrics "v1" for that output.
#   component[2]=javaAppDriver,m1,"-e|svt.scale.vehicle.SVTVehicleScale,-o|${A1_IPv4_1}|16102|1|0|90|0|10|1"
#   test_template_compare_string[2]="SVTVehicleScale Success"
#   test_template_compare_count[2]="1"
#   test_template_metrics_v1[2]="1";
# 
# 4. To use this helper interface, the user must define the run order using a new method (instead of the old method)
# Example: instead of doing components[${n}]="${component[1]} ${component[11]} ${component[5]} .... the user does it the new way:
#  test_template_runorder="1 11 5 6 7 2 3 4 "
#
# 5. To use this helper interface, finally the user must call the finalize function
# Example: Below the user calls the function test_template_finalize_test to create compare test files , scp them to other machines, and generate the components[${n}] line.
#   test_template_finalize_test
#
#
# NOTE: As of 11.26.12, the test template functions do not support tests that start "subcontrollers". For those types of tests use the normal methods.
#
#-------------------------------------

if [ -z $test_template_prefix ] ;then
    test_template_prefix="noname_00_";
fi

test_template_set_prefix(){
    test_template_prefix=${1-"noname_00_"}
}


#-------------------------------------
# Test template Functions::  test_template_transfer_m_files
#
# This next function scps files based on the machine number input and filename input
# All files input are expected to be in the current working directory and will be
# transferred to the same directory on the remote machine. The same directory must
# already exist on the remote machine or there will be an error from scp.
#-------------------------------------
test_template_transfer_m_files() {
    local machine=$1
    local file=$2
    local hn; # hostname
    local rc;

    hn=$(eval echo M${machine}_HOST)
    if [ -z $hn ]; then hn=$(eval echo M${machine}_IPv4); fi

    echo "---------------------------------"
    echo "scp -r ${file} ${!hn}:`pwd`/${file} ...  "
    echo "---------------------------------"

    scp -r ${file} ${!hn}:`pwd`/${file}
    rc=$?

    echo "---------------------------------"
    echo " scp rc = $rc"
    echo "---------------------------------"
}


#-------------------------------------
# Test template Functions::  test_template_transfer_compare_files
#
# This next function scps .comparetest files to the Machines Mx that need them.
# Currently it is only intended to be called from another test_template function
#-------------------------------------
test_template_transfer_compare_files() {
    local index=2; 
    if [ "${test_template_compare_files[1]}" == "" ] ; then
        return;  # If not set then it means there are no files to transfer .
    fi
    while (($index<=${test_template_compare_files[1]})); do
        if ! [ -z "${test_template_compare_files[${index}]}" ] ; then
             test_template_transfer_m_files ${index} ${test_template_compare_files[${index}]}
        fi
        let index=$index+1;
    done
}

#-------------------------------------
# Test template Functions::  test_template_add_to_comparetest
#
# This next function adds a line to a compare test file, and records
# if that comparetest file needs to later be scp transferred to other
# machines.
#-------------------------------------
test_template_add_to_comparetest_full() {
    local test_id="$1" # should be == to ${xml[${n}]}
    local compare_machine=$2
    local compare_count=$3
    local compare_search_file=$4
    local compare_string="$5"
    local compare_line=1;
    local compare_file="${test_id}_m${compare_machine}.comparetest"
    #---------------------------------------
    # Make sure position 1 of the test_template_compare_files array is not blank
    #---------------------------------------
    if [ "${test_template_compare_files[1]}" == "" ] ; then
        test_template_compare_files[1]=${compare_machine}
    fi
    if [ -e $compare_file ] ; then
        current_line=`cat $compare_file | wc -l`         
    else
        current_line=1;
    fi
    
    #---------------------------------------
    # Add to compare specification to compare file for this machine.
    #---------------------------------------
    echo "${current_line}:${compare_count}:${compare_search_file}:${compare_string}" >> ${compare_file}

    #---------------------------------------
    # For all machines except machine 1 a transfer is needed, store the machines > 1 that need an scp transfer.
    #---------------------------------------
    if [ "${compare_machine}" != "1" ] ; then 
        test_template_compare_files[${compare_machine}]=$compare_file;
    fi

    #---------------------------------------
    # The first index holds the largest machine that a compare file was created for.
    #---------------------------------------
    if ((${test_template_compare_files[1]}<${compare_machine})) ; then
        test_template_compare_files[1]=${compare_machine}
    fi

}
test_template_add_to_comparetest() {
    local test_id="$1" # should be == to ${xml[${n}]}
    local compare_string="$2"
    local compare_count=$3
    local compare_driver=$4
    local compare_app=$5
    local compare_machine=$6
    local compare_runorder=$7
    local compare_comp=$8

    local compare_search_file

    compare_app=`basename $compare_app`
    
    if [[ ($# -ne 8) ]] ; then
      echo "Incorrect number of arguments.  test_template_add_to_comparetest will not execute."
      exit 1;
    fi

    if [ -z "${test_template_search_file[${compare_comp}]}" ] ; then
      compare_search_file=${test_id}-${compare_driver}-${compare_app}-M${compare_machine}-${compare_runorder}.screenout.log
    else
      compare_search_file=${test_template_search_file[${compare_comp}]}.log
    fi
    #---------------------------------------
    # Add to compare specification to compare file for this machine.
    #---------------------------------------
    test_template_add_to_comparetest_full ${test_id} ${compare_machine} ${compare_count} ${compare_search_file} "${compare_string}" 
    #test_template_add_to_comparetest_full ${test_id} ${compare_machine} ${compare_count} ${test_id}-${compare_driver}-${compare_app}-M${compare_machine}-${compare_runorder}.screenout.log "${compare_string}" 
   
}

#-------------------------------------
# Test template Functions::  test_template_initialize_test
#
# Required input: $1 - must be called with the value of ${xml[${n}]}
#
# This function must be called to initialize a test that is following the
# test template process to reduce the manual maintenance of comparetest files.
#-------------------------------------
test_template_initialize_test() { 
    local test_id="$1" ; # should be == to ${xml[${n}]}
    unset test_template_compare_string
    unset test_template_search_file
    unset test_template_compare_count
    unset test_template_compare_files
    unset test_template_metrics_v1
    unset test_template_runorder
    unset test_template_logfile
    unset component
    echo "---------------------------------"
    echo "rm -rf $1_m*.comparetest"
    echo "---------------------------------"
    rm -rf $1_m*.comparetest
    test_template_logfile=${test_id}.test_template.log
}

#-------------------------------------
# Test template Functions::  test_template_finalize_test
#
# This function must be called to finalize a test that is following the
# test template process to reduce the manual maintenance of comparetest files.
# 
# This function builds the final components[$n] string, 
# Creates comparetest files
# Scp's created comparetest files to necessary machines
# Adds svt_metrics processing if desired to the end of the components[$n] string,
#-------------------------------------
test_template_finalize_test() {
    local comp;
    local tmp_count=1;
    local current_runorder=1;
    local current_machine="";
    local current_app="";
    local current_app_full="";
    local current_options_full="";
    local current_env_var="";
    local current_driver="";
    local orig_ifs;
    local end="";
    local unexpected_count=0;
    local line="";
    components[${n}]=""
    echo started processing for test id  ${xml[${n}]}

    if [ -z "${test_template_runorder}" ] ; then
        test_template_runorder=$(eval echo {1..${#component[@]}})
    fi

    echo processing ${test_template_runorder[$@]}

    for comp in ${test_template_runorder} ; do 
        echo "Processing $comp, from ${component[${comp}]}"
        current_machine="";
        current_app="";
        current_app_full="";
        current_options_full="";
        current_driver="";
        components[${n}]+="${component[${comp}]} "
        let unexpected_count=0;

        #IFS=, ; for w in ${component[3]} ; do  echo $w; done
        #javaAppDriver
        #m2
        #-e|svt.scale.vehicle.SVTVehicleScale
        #-o||16102|1|0|90|0|10|1
  
        orig_ifs=$IFS 
        tmp_count=1;
        IFS=, ; for word in ${component[${comp}]} ; do  
            if (($tmp_count==1)) ; then
                current_driver="$word";
            elif (($tmp_count==2)); then
                current_machine=`echo $word |sed 's/m//g' `;
            elif (($tmp_count==3)) ; then
                current_app_full="$word"
            elif (($tmp_count==4)) ; then
                current_options_full="$word";
            elif (($tmp_count==5)) ; then
                current_env_var="$word";
            elif (($tmp_count>=6)) ; then
               echo "WARNING: Processing comp: Unexpected tmp_count = $tmp_count"
                let unexpected_count=$unexpected_count+1;
                break;
            fi
            let tmp_count=$tmp_count+1;
        done

        tmp_count=1;

        IFS="|" ; for word in ${current_app_full}  ; do
            if (($tmp_count==1)) ; then
               if [ "$word" != "-e" ] ;then  
                    let unexpected_count=$unexpected_count+1;
               fi
            elif (($tmp_count==2)) ; then
               current_app="$word"
            elif (($tmp_count>=3)) ; then
               echo "WARNING: Processing current app full: Unexpected  tmp_count = $tmp_count"
               let unexpected_count=$unexpected_count+1;
               break;
            fi 
            let tmp_count=$tmp_count+1;
        done

        IFS="$orig_ifs"

        echo "current_driver is $current_driver"
        echo "current_machine is $current_machine"
        echo "current_app is $current_app"
        echo "current_runorder is $current_runorder"

        if [ -n "${test_template_compare_string[${comp}]}" ]  ; then
            echo "there are compare test strings to process for $comp, from ${component[${comp}]}"
            if (($unexpected_count>0)); then
                echo "WARNING: There were unexpected errors while parsing the comp $comp. It is possible that the line format is not supported. Please investigate."
            fi
            #-----------------------------------------
            # 01.03.2013: The loop below adds support multiple compare strings for a single component. 
            # TODO: Currently only supports one compare count, for multiple compare strings (expecting usually 1)
            # If in future a different count is needed for each compare string then that would need to be added.
            #
            # To use this functionality the user must add additional compare strings with the following command
            # syntax, the key part being the $'\n which is used to add in the newline character in between each compare string.
            # 
            # test_template_compare_string[X]+=`echo -e "\n<ADDITIONAL COMPARE STRING>"`
            #
            # Example: (example of adding two additional compare strings, for component 5)
            # test_template_compare_string[5]="Intialized ok."
            # test_template_compare_string[5]+=$'\nReceived 30 messages.'
            # test_template_compare_string[5]+=$'\nTest Completed.'
            #-----------------------------------------

            while read -r line; do 
                echo "Processing compare string \"$line\""
                if [ -n "${test_template_compare_count[${comp}]}" ]  ; then
                    test_template_add_to_comparetest "${xml[${n}]}" "$line" ${test_template_compare_count[${comp}]} ${current_driver} ${current_app} ${current_machine} ${current_runorder} ${comp}
                else
                    test_template_add_to_comparetest "${xml[${n}]}" "$line" 1 ${current_driver} ${current_app} ${current_machine} ${current_runorder} ${comp}
                fi
            done <<< "${test_template_compare_string[${comp}]}"
        fi

        if [ -n "${test_template_metrics_v1[${comp}]}" ]  ; then
            end+="runCommandEnd,m${current_machine},svt-metrics.sh,-s|SVT_TEXT=finished__|SVT_FILE=${xml[${n}]}-${current_driver}-${current_app}-M${current_machine}-${current_runorder}.screenout.log "
        fi
        let current_runorder=$current_runorder+1;
    done
    components[${n}]+="$end"

    test_template_transfer_compare_files; 
    
    echo "components of n $n is ${components[${n}]}"
}


#-------------------------------------
# Test template Functions::  test_template_singlecommand
#
# Parameters: 
#    index         Test scenario number, typically passed in as $n 
#    label         Testcase label
#    description   Testcase description
#    driver        Value such as cAppDriver used by underlying automated test environment
#    machine       Value such as m1, m2, ... where the testcase will be executed
#    timeout       Maximum seconds the testcase will run 
#    command       Command and options (delimited by a space) to be executed
#    result        Expected output string if command executes successfuly
#    
# PREREQ:  ISMsetup.sh is required to define automated test environment
#
# This function is called to run a simple single command test.
# 
# This function calls the test_template... api's to populate and generate
# necessary arrays and comparetest files and scp's them to the correct machine.
#
# Example:  
#    test_template_singlecommand $n cli_webui_test_1 "check port 9087" 10 cAppDriver m1 "openssl s_client -connect ${A1_IPv4_1}:9087" CONNECTED
#
#-------------------------------------
test_template_singlecommand() {
  local cont=1
  if [ ! $# == 8 ]; then
    echo test_template_singlecommand ERROR invalid number of parameters: $#
    cont=0;
  else
    local index=$1
    local label=$2
    local desc="$3"
    local driver=$4
    local machine=$5
    local timeout=$6
    local command
    local options
    read -r command options <<< "$7"
    local result="$8"

    if [ ${machine} == m* ]; then
       echo test_template_singlecommand ERROR invalid parameter machine: $machine
       cont=0;
    fi
  fi

  if [ $cont == 0 ]; then
    echo Usage:  test_template_singlecommand index label description driver machine timeout command result
  else
    xml[$index]="$label"
    test_template_initialize_test "${xml[$index]}"
    scenario[$index]="${xml[$index]} - "$desc
    timeouts[$index]=$timeout
    component[1]=$driver,$machine,"-e|${command// /|},-o|${options// /|}"
    component[2]=searchLogsEnd,$machine,${xml[$index]}_$machine.comparetest,9
    test_template_compare_string[1]="$result"
    test_template_runorder="1 2"
    test_template_finalize_test
  fi
}

#-------------------------------------
# Test template Functions::  test_template_add_test_single
#
# This function will add a test that runs on a single machine as 
# specified in the input paramemeters
#-------------------------------------
test_template_add_test_single(){ 
	local description=${1-""}
	local tool=${2-""}
	local machine=${3-""}
	local command=${4-""}
	local a_timeout=${5-1200}
	local pipe_separated_env=${6-""}
	local compare_string=${7-"TEST_RESULT: SUCCESS"}
	local prefix=${8-"$test_template_prefix"}
	echo "processing prefix $prefix"
	echo "processing description $description"
	echo "processing tool $tool"
	echo "processing machine $machine"
	echo "processing command $command"
	echo "processing a_timeout $a_timeout"
	echo "processing pipe_separated_env $pipe_separated_env"
	echo "processing compare_string $compare_string"
	xml[${n}]="${prefix}`awk 'BEGIN {printf("%02d",'${n}'); }'`"
	scenario[${n}]="${xml[${n}]} - ${description}"
	test_template_initialize_test "${xml[${n}]}"
	timeouts[${n}]=${a_timeout}
	component[1]=${tool},${machine},"${command},-s|AF_AUTOMATION_TEST=${xml[${n}]}|AF_AUTOMATION_TEST_N=${n}|${pipe_separated_env}"
	test_template_compare_string[1]="$compare_string"
	echo "processing component 1 ${component[1]}"

    if [ -n "$compare_string" ] ;then
        component[2]=searchLogsEnd,${machine},${xml[${n}]}_${machine}.comparetest,9
	    test_template_runorder="1 2"
    else
	    test_template_runorder="1"
    fi 
	test_template_finalize_test
	((n+=1))
}

#-------------------------------------
# Test template Functions::  test_template_add_test_all_Ms
#
# This function will add a test that runs on all machines one at
# a time (serially) on all available "M" clients
#-------------------------------------
test_template_add_test_all_Ms(){ 
	local description=${1-""}
	local partial_command=${2-""}
	local a_timeout=${3-600}
	local pipe_separated_env=${4-""}
	local compare_string=${5-"TEST_RESULT: SUCCESS"}
	local prefix=${6-"$test_template_prefix"}
	local v=1;
	while (($v<=${M_COUNT})); do
		test_template_add_test_single "$description - M${v}" cAppDriver m${v} $partial_command $a_timeout "${pipe_separated_env}" "${compare_string}" $prefix
		let v=$v+1;
	done
}
#-------------------------------------
# Test template Functions::  test_template_add_test_all_M_concurrent
#
# This function will add a test that runs concurrently (in parallel)
# on all available "M" clients
#-------------------------------------
test_template_add_test_all_M_concurrent(){ 
	local description=${1-""}
	local partial_command=${2-""}
	local a_timeout=${3-1200}
	local pipe_separated_env=${4-""}
	local compare_string=${5-"TEST_RESULT: SUCCESS"}
	local prefix=${6-"$test_template_prefix"}
	local v=1;
	local k=1;
	local j=1;
	xml[${n}]="${prefix}`awk 'BEGIN {printf("%02d",'${n}'); }'`"
	scenario[${n}]="${xml[${n}]} - ${description}"
	test_template_initialize_test "${xml[${n}]}"
	timeouts[${n}]=${a_timeout}
	test_template_runorder=""
	while (($v<=${M_COUNT})) ; do
		component[${v}]=cAppDriver,m${v},"${partial_command},-s|AF_AUTOMATION_TEST=${xml[${n}]}|AF_AUTOMATION_TEST_N=${n}|${pipe_separated_env}"
		test_template_compare_string[${v}]="$compare_string"
		test_template_runorder+="${v} "
		let v=$v+1;
	done
    let j=($v-1);
    let v=1;
    if [ -n "$compare_string" ] ;then
	    while (($v<=${M_COUNT})) ; do
            let k=($j+$v);
            component[$k]=searchLogsEnd,m${v},${xml[${n}]}_m${v}.comparetest,9
		    test_template_runorder+="${k} "
		    let v=$v+1;
        done
    fi 

	test_template_finalize_test
	((n+=1))
}


test_template_env(){
  local force=${1-""} # set to any string or true to force it
  if [ -z "$THIS" -o -z "$M_COUNT" -o -z "$A_COUNT" -o -z "$AF_AUTOMATION_ENV" -o -n "$force" ] ; then  # if key vars not set, then the attempt to source automation environment
    if [[ -f "/niagara/test/testEnv.sh" ]] ;then
        # Official ISM Automation machine assumed
        source /niagara/test/testEnv.sh
        export AF_AUTOMATION_ENV="AUTOMATION"
    elif [[ -f "/niagara/test/scripts/ISMsetup.sh" ]] ;then
        export AF_AUTOMATION_ENV="SANDBOX"
        # Manual Runs need to build Customized ISMSetup for THIS param, SSH environment file and Tag Replacement
        source /niagara/test/scripts/ISMsetup.sh
        #../scripts/prepareTestEnvParameters.sh
        #source ../scripts/ISMsetup.sh
        ntpdate changeme.example.com >/dev/null 2>/dev/null
    else
        echo "ERROR - automation environment not available"
    fi
  fi

  THIS_M_NUMBER=`echo $THIS | sed "s/M//g"`
} 

test_template_systems_list(){
    local v=1
    test_template_env
    { while (($v<=${M_COUNT})); do
        echo "M${v}_HOST"
        let v=$v+1;
    done ; }  |  xargs -I repstr echo "echo \$repstr" 2> /dev/null | sh 
}

test_template_add_test_sleep(){
    local seconds=$1
	local prefix=${2-"$test_template_prefix"}

	xml[${n}]="${prefix}`awk 'BEGIN {printf("%02d",'${n}'); }'`"
	scenario[${n}]="${xml[${n}]} - sleep $seconds so that processing can occurr"
    let timeouts[${n}]=($seconds*2);
    component[1]=sleep,$seconds
    components[${n}]="${component[1]} "
    
    ((n+=1))
    
}

#-------------------------------------
# Test template Functions::  test_template_format_search_component
#
# This function formats the component[] entry that references the comparetest file generated in test_template_finalize_test
#-------------------------------------
test_template_format_search_component(){
    local test_machine=${1-"m1"}
    local test_command=${2-"searchLogsEnd"}
    printf "%s,%s,%s_%s.comparetest,9\n" ${test_command} ${test_machine} ${xml[${n}]} ${test_machine}
}


svt_get_ltpa_token(){
    local uid=${1-"u0000001"};
    local password=${2-"imasvtest"};
    local wasserveruri=${3-"https://10.10.10.10:9443/snoop"}
    local tmpout=${4-"./.tmp.ltpa"}
    local errorcount=${5-0}
    local regex=""
    local data=""

    rm -rf $tmpout*
    curl -s -k -c $tmpout $wasserveruri -u $uid:$password -o ${tmpout}.connect
    #curl -s -k -c $tmpout $wasserveruri -u $uid:$password >/dev/null
    #curl -s -k -b $tmpout $wasserveruri -o ${tmpout}.data

    #---------------------
    # one way to extract ltpa token... no error handling this way
    #---------------------
    #curl -s -k -b $tmpout $wasserveruri -o ${tmpout}.data
    #cat ${tmpout}.data |grep LtpaToken2 | grep Cookie | sed "s/.*<td>LtpaToken2=//" | sed "s/<\/td>.*//"

    #---------------------
    # another way to extract ltpa token , also with a little better error handling.
    #---------------------
    #curl -s -k -b $tmpout $wasserveruri -o ${tmpout}.data
    #sync;
    #data=`cat ${tmpout}` 
    #regex="(.*LtpaToken2[[:space:]])(.*)[[:space:]]*"
    #if [[ $data =~ $regex ]] ;then 
    #    echo ${BASH_REMATCH[2]}; 
    #    return 0;
    #else
    #    echo "Error: could not get LTPA token for $uid $password $wasserveruri " >> /dev/stderr
    #    return 1;
    #fi
    
    #---------------------
    # another way to extract ltpa token , also with a little better error handling, and without any filesystem direct involvment
    #---------------------
    data=`curl -s -k -b $tmpout $wasserveruri `
    regex="(.*LtpaToken2=)(.*)(</td></tr>.*)(HTTPS Information:)(.*)"
    if [[ $data =~ $regex ]] ;then 
        echo ${BASH_REMATCH[2]}; 
        return 0;
    else
        if (($errorcount>10)); then
            echo "Error: could not get LTPA token for $uid $password $wasserveruri " >> /dev/stderr
            echo "data is $data"
            echo "$data" > $tmpout.data
            exit 1;
        else
            let errorcount=$errorcount+1
            echo "WARNING: $errorcount: Could not get LTPA token for $uid $password $wasserveruri " >> /dev/stderr
            svt_get_ltpa_token $uid $password $wasserveruri $tmpout $errorcount      
            return $?;
        fi
    fi
}


svt_get_ltpa_token_bunch(){
    local start=${1-0}
    local count=${2-100}
    local prefix=${3-"u"}
    local password=${4-"imasvtest"};
    local wasserveruri=${5-"https://10.10.10.10:9443/snoop"}
    local tmpout=${6-"./.tmp.ltpa.${start}"}
    local out=${7-`awk 'BEGIN {printf("db.flat.'${prefix}'%07d", '$start'); exit 0;}'`} 
    local end;
    let end=($start+$count);

    echo "" | awk ' BEGIN {printf(" . ./commonFunctions.sh \n"); } 
                    END { x='$start'; while(x<'$end') { 
                    printf ("echo -n \"u%07d \"; svt_get_ltpa_token '$prefix'%07d '$password' '$wasserveruri' '$tmpout' \n ",x, x); x=x+1; } } ' | sh > $out

}


#---------------------------------
# Return a random set of 1K LTPA tokens that are X (valid or expired)
# and selected by prefix "u" "c" (users or cars)"
#---------------------------------
svt_get_random_ltpa_1k_file_X(){
    local X=${1-"valid"} ; #valid or expired
    local prefix=${2-"u"}
    local errorcount=${3-100}
    local myfile="";

    if [ -z ${M1_TESTROOT} ] ; then
        echo "WARNING: sourcing test_template_env, this should have already been done by the user. " >> /dev/stderr
        test_template_env force
    fi

    if ! [ -e /svt.data/ltpa.tokens/${X}/.tmp.filelist ] ;then
        find  /svt.data/ltpa.tokens/${X}/db.flat.${prefix}*.split.* > /svt.data/ltpa.tokens/${X}/.tmp.filelist
    fi
    myfile=`cat  /svt.data/ltpa.tokens/${X}/.tmp.filelist | sort -R | head -1`

    if [ -n "$myfile" -a -e "$myfile" ] ;then
        echo "$myfile"
        return 0;
    else
        let errorcount=$errorcount-1;
        if (($errorcount<=0)); then
            echo "ERROR: svt_get_random_ltpa_1k_file_X failed to retrieve a file after 100 tries: $myfile "
            echo "ERROR: svt_get_random_ltpa_1k_file_X failed to retrieve a file after 100 tries: $myfile " >> /dev/stderr
            return 1;
        else
            echo "WARNING: $errorcount svt_get_random_ltpa_1k_file_X failed to retrieve a file : $myfile " >> /dev/stderr
            myfile=`svt_get_random_ltpa_1k_file_X $X $prefix $errorcount`
            return $?; 
        fi
    fi
    
    echo "ERROR: svt_get_random_ltpa_1k_file_X unexpected code path"
    echo "ERROR: svt_get_random_ltpa_1k_file_X unexpected code path" >> /dev/stderr
    return 1;
}
svt_get_random_ltpa_1k_file_expired(){
    local prefix=${1-"u"}
    svt_get_random_ltpa_1k_file_X "expired" $prefix
    return $?
}
svt_get_random_ltpa_1k_file_valid(){
    local prefix=${1-"u"}
    svt_get_random_ltpa_1k_file_X "valid" $prefix
    return $?
}
svt_get_random_ltpa_key_valid(){
    svt_get_random_ltpa_1k_file_valid | xargs -I repstr cat repstr | sort -R | head -1
}
svt_get_random_ltpa_key_expired(){
    svt_get_random_ltpa_1k_file_expired | xargs -I repstr cat repstr | sort -R | head -1
}


svt_get_cached_ltpa_token () { # currently only implemented for uxxxxxxx users
    local user=${1-1}
    local type=${2-"valid"}
    local x=$user
    local y
    let y=($x/10000)
    local z
    let z=($y*10000)
    local a
    let a=($x-$z)
    local b
    let b=($a/1000)
    local u=`echo $user |awk '{printf("u%07d", $1);}'`
    local t=`echo $z |awk '{printf("%07d", $1);}'`
    local myfile="/svt.data/ltpa.tokens/${type}/db.flat.u${t}.split.0${b}"
    
    grep $u $myfile 
}

RANDOM=`date +%s`;
#-------------------------------------------------------
# This can be used to return a pseudo random true or
# false with the a 1 in <chance> value of being true
#-------------------------------------------------------
svt_check_random_truth(){
    local chance=${1-2} # chance value == 2 means 50% or 1 in 2 chance of returning true;
    local upper=32767 ; # upper limit of what is returned by RANDOM
    local var;
    local randvar=$RANDOM
    let var=(${randvar}%${chance})

    #echo "rv:$randvar var:$var chance:$chance"

    if [ "$var" == "0" ] ;then
        return 0;  # randomly returned true based on 1 in <chance>
    else
        return 1;  # did not return true
    fi

}

