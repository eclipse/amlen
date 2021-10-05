#! /bin/bash
#-------------------------------------------------------------------------------
# prepareTestEnvParameters.sh
#
# Description:
#   This script is part of the ISM Automation Framework. 
#   This script will check for ${TESTROOT}/testEnv.sh,
#    if found assume ISM Automation running and will not generate personalized 
#    Test Env. Script ISMsetup.sh and will use the automation's testEnv.sh file.
#   If  ${TESTROOT}/testEnv.sh is not found, then this script will use (${1} or
#   ${TESTROOT}/scripts/ISMsetup.sh as the Test Env. Script and personalize 
#   ${THIS} for each machine in M_COUNT (number of machine needed).  
#   Then SCP the updated Test Env. Script to the target machine.
#
# Input Parameter:  None
#
#
# Return Code:
#   0 - All machine received a personalized Test Env. Parameter script successfully.
#   1 - One or more machines failed to receive it's Test Env. Parameter script.
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
function printUsage {
   echo " "
   echo "Usage:  ${THECMD}  [parameterfile] "
   echo " "
   echo "  parameterfile - File that contains the Test Env. Parameters. (optional)"
   echo "                  Default: ${FILE} "
   echo " "
}

#-------------------------------------------------------------------------------
function print_cmderror {
  echo " "
  echo "COMMAND FAILED:  \"$1\"  "
  echo "  Terminating Processing..."
  echo " "
  exit 1
}

#-------------------------------------------------------------------------------
function scpit {
  scp $1 $2 > /dev/null
  if [[ $? -ne 0 ]] ; then
    print_cmderror "scp $1 $2"
  fi
}



#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
# B E G I N   M A I N   P A T H 
#-------------------------------------------------------------------------------
# Set some variables needed for the script
THECMD=$0
curDir=$(dirname ${0})
SSH_ENV_FILE="environment"
SSH_ENV_STMT="echo If you see this line, you did not set M#_IBM_SDK in scripts/ISMSetup.sh and that's a problem!"
TIDYUP=1

# 'Source' the Automated Env. Setup file if it exists and don't do anything else (keep reading, this is a failsafe check)
#    prepareTestEnvParameters.sh SHOULD NOT BE CALLED DIRECTLY during OFFICIAL ISM Automation - IT IS NOT EXPECTED, 
#    Frank does this work outside the Test Case execution once per build for speed.  DO NOT REMOVE!
# Otherwise this is a User's Manual Run and ISMsetup is used to source machine environment parameters.
if [[ -f "../testEnv.sh" ]]
then
  FILE="../testEnv.sh"
else
  if [[ "${curDir}" == "." ]]
  then
    FILE="ISMsetup.sh"
    COMMON_FCNS="commonFunctions.sh"
  else
    FILE="${curDir}/ISMsetup.sh"
    COMMON_FCNS="${curDir}/commonFunctions.sh"
  fi

#set -x
  OUTFILE=${FILE}.out
  rm -f ${OUTFILE}

  # Verify the input file exists and is readable   
  if [[ ! -f ${FILE} ]]; then
    echo "${FILE} : does not exist"
    ERR=1
  elif [ ! -r ${FILE} ]; then
    echo "${FILE}: cannot read"
    ERR=1
  fi

  # Verify the output file can be written   
  touch ${OUTFILE}
  if [[ ! -w ${OUTFILE} ]]; then
    print_error "ERROR: Unable to write to log file ${OUTFILE}."
    ERR=1
  fi

  if [[ ${ERR} -eq 1 ]] ; then
    echo "Error in Input Environment: ${THECMD} $@ "
    printUsage
    exit 1
  fi
#set +x
  # Source the Test Env. Parameter File
  source ${FILE}
  source ${COMMON_FCNS}
#set -x

# LAW: Commenting out this block of code. As far as I can tell,
# all we need to do here is create a new file without the THIS line in it
# Keeping this for now, though, just in case
# Iterate through the Input Test Env. parameter file and build a generic file
# The unique THIS parameter will be added later for each machine
  #while read line    
  #do
    ## Do not process comments    
    #if [[ ! ${line} == *THIS=* && ! ${line} =~ ^[#].* ]]; then
      #echo  ${line}   >> ${OUTFILE}
      ## Can I process IMA_SDK for shh 'environment' file yet?
      #tag=`echo ${line} | cut -d "=" -f 1`   
      ## Now remove "export", leave only the tag
      #tag=`echo ${tag} | cut -d " " -f 2`    
      ## tagHead is M#, tagTail is the actual parameter being set
      #tagHead=`echo ${tag} | cut -d "_" -f 1` 
      #tagTail=`echo ${tag} | cut -d "_" -f 2-` 
      ## Separate the value from the line
      #tagValue=`echo ${line} | cut -d "=" -f 2-` 
#
    #fi
  #done <${FILE}

# LAW: Remove the THIS line if it exists and make a copy of the file called ${OUTFILE}
sed -e '/THIS=/d' ${FILE} > ${OUTFILE}

# Add the unique THIS parameter to the generic file and send to the target machine
  s=1
  while [ ${s} -le ${M_COUNT} ]
  do
    cp  ${OUTFILE} ${OUTFILE}.M${s}
    echo "export THIS=M${s}" >> ${OUTFILE}.M${s}
    # Copy the parameter file to the target machine
    userVar="M${s}_USER"
    userVal=$(eval echo \$${userVar})
    hostVar="M${s}_HOST"
    hostVal=$(eval echo \$${hostVar})
    testrootVar="M${s}_TESTROOT"
    testrootVal=$(eval echo \$${testrootVar})

    scpit ${OUTFILE}.M${s}  ${userVal}@${hostVal}:${testrootVal}/scripts/${FILE}

    # Build and Send SSH environment file to the $HOME directory of machine, make a dummy file with error msg if it was missed.
      tagSDK=$(eval echo \$M${s}_IMA_SDK)
      tagMQ_SDK=$(eval echo \$M${s}_MQ_CLIENT_SDK)

      if [[  ${tagSDK} != "" ]]
      then
        # IMA_SDK can look like:  /path:Linux:64  Need to pick apart if 'colons' exist
        SDK_PLATFORM="Linux"
        SDK_ARCH="lib64"
        # Isolate the platform
        noarch=${tagSDK%:*}
        sdkPlatform=${noarch##*:}
        #sdkPlatform=`echo ${tagSDK} | cut -d ":" -f 2 `
        # Isolate the arch
        sdkArchitecture=${tagSDK##*:}
        #sdkArchitecture=`echo ${tagSDK} | cut -d ":" -f 3 `
        # Isolate the path
        tagSDK=${tagSDK%%:*}
        #tagSDK=`echo ${tagSDK} | cut -d ":" -f 1 `
        # if there are no 'colons', what ever -f you ask for you get -f 1
        if [[ !("${sdkPlatform}" == "${tagSDK}") && !("${sdkPlatform}" == "") ]]
        then
            # actually not used yet, only building Linux clients, for future clients
            SDK_PLATFORM=${sdkPlatform}
        fi
        if [[ !("${sdkArchitecture}" == "${tagSDK}") && !("${sdkArchitecture}" == "") ]]
        then
            SDK_ARCH=${sdkArchitecture}
        fi
        
        BASEPATH=${tagSDK}/lib

        echo "PROFILEREAD=FALSE" >  ${SSH_ENV_FILE}.M${s}
        CLASSPATH=${BASEPATH}/com.ibm.micro.client.mqttv3.jar:${BASEPATH}/fscontext.jar:${BASEPATH}/imaclientjms.jar:${BASEPATH}/imaclientjms_test.jar:${BASEPATH}/ismclientws.jar:${BASEPATH}/javamqttv3samples.jar:${BASEPATH}/jms.jar:${BASEPATH}/jmssamples.jar:${BASEPATH}/porting.jar:${BASEPATH}/providerutil.jar:${testrootVal}/lib/selenium-java.jar:${testrootVal}/lib/selenium-server-standalone.jar:${testrootVal}/lib/android_webdriver_library.jar:${testrootVal}/lib/log4j.jar:${testrootVal}/lib/mqconnectivity.jar:${testrootVal}/lib/com.ibm.automation.core.jar:${testrootVal}/lib/DriverSync.jar:${testrootVal}/lib/GuiTest.jar:${testrootVal}/lib/javasample.jar:${testrootVal}/lib/jmssample.jar:${testrootVal}/lib/JMSTestDriver.jar:${testrootVal}/lib/MqttTest.jar:${BASEPATH}/org.eclipse.paho.client.mqttv3.jar:${BASEPATH}/org.eclipse.paho.mqttv5.client.jar:${BASEPATH}/org.eclipse.paho.mqttv5.common.jar:${BASEPATH}/org.eclipse.paho.sample.mqttv3app.jar:${testrootVal}/lib/svtAutomotiveTelematics.jar:${testrootVal}/lib/svt_jssamples.jar:${testrootVal}/lib/WSTestDriver.jar:${testrootVal}/lib/xrscada.jar:${testrootVal}/lib/cli-test-utils.jar:${testrootVal}/lib/commons-cli-1.2.jar:${testrootVal}/lib/commons-lang3-3.1.jar:${testrootVal}/lib/SuperCSV-1.52.jar:${testrootVal}/lib/jsch-0.1.49.jar:${testrootVal}/lib/com.ibm.jaxrs.thinclient_8.5.0.jar:${testrootVal}/lib/svt_client_tests.jar:${BASEPATH}/vertx-core-3.8.1.jar:${BASEPATH}/netty-buffer-4.1.39.Final.jar:${BASEPATH}/netty-common-4.1.39.Final.jar:${BASEPATH}/netty-transport-4.1.39.Final.jar:${BASEPATH}/netty-resolver-4.1.39.Final.jar:${BASEPATH}/netty-handler-4.1.39.Final.jar:${BASEPATH}/netty-handler-proxy-4.1.39.Final.jar
        PATH=${tagSDK}/bin:/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
        LD_LIBRARY_PATH=${tagSDK}/${SDK_ARCH}:${tagSDK}/lib

	# If the MQ_CLIENT_SDK path is provided, append it to the Env. Vars.
        if [[ "${tagMQ_SDK}" == "" ]]
        then
           echo "CLASSPATH=${CLASSPATH}" >>  ${SSH_ENV_FILE}.M${s}
           echo "PATH=${PATH}" >>  ${SSH_ENV_FILE}.M${s}
           echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}" >>  ${SSH_ENV_FILE}.M${s}
        else
           echo "CLASSPATH=${CLASSPATH}:${tagMQ_SDK}/java/lib/com.ibm.mqjms.jar:${tagMQ_SDK}/java/lib/com.ibm.mq.commonservices.jar:${tagMQ_SDK}/java/lib/com.ibm.mq.jmqi.jar:${tagMQ_SDK}/java/lib/connector.jar:${tagMQ_SDK}/java/lib/com.ibm.mq.headers.jar:${tagMQ_SDK}/java/lib/com.ibm.mqjms.jar:${tagMQ_SDK}/java/lib/dhbcore.jar:${tagMQ_SDK}/java/lib/com.ibm.mq.jar:${tagMQ_SDK}/java/lib/com.ibm.mq.pcf.jar:${tagMQ_SDK}/java/lib/jta.jar" >>  ${SSH_ENV_FILE}.M${s}
           echo "PATH=${PATH}:${tagMQ_SDK}/bin:${tagMQ_SDK}/samp/bin" >>  ${SSH_ENV_FILE}.M${s}
           echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${tagMQ_SDK}/${SDK_ARCH}" >>  ${SSH_ENV_FILE}.M${s}
           echo "C_INCLUDE_PATH=${tagMQ_SDK}/inc" >>  ${SSH_ENV_FILE}.M${s}
           echo "LIBRARY_PATH=${tagMQ_SDK}/${SDK_ARCH}" >>  ${SSH_ENV_FILE}.M${s}
        fi

      else
        echo "ERROR in ../scripts/ISMsetup.sh:  Did not specify an M${s}_IMA_SDK parameter"
        echo "${SSH_ENV_STMT}:: Missing M${s}_IMA_SDK parameter" > ${SSH_ENV_FILE}.M${s}
      fi

      if [[ ${userVal} == "root" ]]
      then
        scpit ${SSH_ENV_FILE}.M${s}  ${userVal}@${hostVal}:/${userVal}/.ssh/${SSH_ENV_FILE}
      else
        scpit ${SSH_ENV_FILE}.M${s}  ${userVal}@${hostVal}:/home/${userVal}/.ssh/${SSH_ENV_FILE}
      fi

      if [[ ${TIDYUP} == "1" ]]
      then
         rm -f ${OUTFILE}.M${s}
         rm -f ${SSH_ENV_FILE}.M${s}
       fi

    s=`expr ${s} + 1`

  done

  if [[ ${TIDYUP} == "1" ]]
  then
    rm -f ${OUTFILE}
    rm -f bitwise
  fi

  # Perform the TAG Replacement in the /common directory (use source from /common_src)
  ${curDir}/prepareReplaceCommon.sh 

fi
