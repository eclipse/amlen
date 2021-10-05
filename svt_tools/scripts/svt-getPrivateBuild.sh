#! /bin/bash
# Get the ISM Build installed on the target machine
# User may wish to modify default values in parseInputs()
#
# Assumes: mount to mar145:/home can be made as:  /mnt/mar145_HOME
#---------------------------------------------------------------------------------

printUsage () { 
  echo "---------------------------------------------------------------------------------"
  echo " ${0} : Install a Private ISM Build!"
  echo " "
  echo "Syntax:"
  echo "  ${0}  -s[Source Directory] -i[ISM SDK Install Directory]  -t[Test Install Directory] -u[user] "
  echo "Inputs:  "
  echo "  -i[ISM SDK Install Directory]  Default:  '${BUILD_DIR}'"
  echo "  -s[Source Directory]           Default:  '${SOURCE_DIR_PATH}' appended to end ${SOURCE_DIR_STEM}"
  echo "  -t[Test Install Directory]     Default:  '${TEST_DIR}'"
  echo "  -u[user]                       Default:  '${USER}'"
  echo "  -c[true|false]                 ex:  true delete appliance, application, sdk, update, and lib dirs before copying new files"
  echo " "
  echo "---------------------------------------------------------------------------------"
  exit 1
}

#---------------------------------------------------------------------------------
parseInputs () { 

#---  Default Values  -----------
BUILD_DIR="/niagara"
BUILD_DIR_BAK="/niagara.bak"
TEST_DIR="/niagara/test"
USER=""
# Default valuse for SOURCE_DIR_STEM, could be out of sync if USER is passed on CLI  (Leave here for HELP SYNTAX, reset below)
SOURCE_DIR_STEM="/mnt/mar145_HOME/${USER}/"
#SOURCE_DIR_PATH="/temp/workspace"
SOURCE_DIR_PATH="/workspace"
BLD_SERVER="10.10.10.10"
CLEAN=false
#--------------------------------

	while getopts "i:s:t:u:c:h" option ${OPTIONS}
	do
		case ${option} in
		i )
			BUILD_DIR=${OPTARG};;

		s )
			SOURCE_DIR_PATH=${OPTARG};;

		t )
			TEST_DIR=${OPTARG};;

		u )
			USER=${OPTARG};;

                c )
                        CLEAN=${OPTARG};;

		h )
			printUsage;;

		* )
			echo "ERROR:  Unknown option: ${option}"
			printUsage;;
		esac	
	done
# If USER is changed need to reset the SOURCE_DIR_STEM value, yet need a default value above if printUsage() called.
SOURCE_DIR_STEM="/mnt/mar145_HOME/${USER}/"

}

#---------------------------------------------------------------------------------
# Start of Main
#---------------------------------------------------------------------------------

OPTIONS="$@"
parseInputs

if [[ ! -e "${BUILD_DIR}" ]];  then  mkdir -p ${BUILD_DIR};  fi
if [[ ! -e "${TEST_DIR}" ]];  then  mkdir -p ${TEST_DIR};  fi

# Access the Private Build Directory
if [[ ! -e "/mnt/mar145_HOME" ]];  then  mkdir -p "/mnt/mar145_HOME";  fi
if [[ ! -e "/mnt/mar145_HOME/$USER" ]];  then  mount  ${BLD_SERVER}:/home  /mnt/mar145_HOME;  fi
if [[ ! -e "/mnt/mar145_HOME/$USER" ]];  
then
  echo "ERROR:  The USER, ${USER}, does not exist @ /mnt/mar145_HOME"
  exit
fi

# Mounts have given access to the Public Dev and Private Test Directory paths
#DEV_BUILD_DIR="/mnt/mar145_HOME/${USER}/temp/workspace"
DEV_BUILD_DIR=${SOURCE_DIR_STEM}${SOURCE_DIR_PATH}
#TEST_BUILD_DIR="/mnt/mar145_HOME/${USER}/temp/workspace"
TEST_BUILD_DIR=${SOURCE_DIR_STEM}${SOURCE_DIR_PATH}


if [ ${TEST_DIR} != "." ]
then
  TEST_DIR=${TEST_DIR}/
fi

set -x
dirpath=`eval echo ${BUILD_DIR} | cut -c 1`
if [[ ${dirpath} != "/" ]]
then
  BUILD_DIR=/${BUILD_DIR}
fi 
cd ${BUILD_DIR}

if [[ -d ${BUILD_DIR} && -d ${BUILD_DIR_BAK} ]]; then
  read -e -i "Y" -p "Backup ${BUILD_DIR}/test to ${BUILD_DIR_BAK} (y/n)?  " ANS
  if [[ ${ANS} = [Yy]* ]]; then
    echo Copying ${BUILD_DIR}/test to ${BUILD_DIR_BAK}
    cp -rpf ${BUILD_DIR}/test ${BUILD_DIR_BAK}/
  fi
fi
set -x
if [[ "${CLEAN}" == "true" ]]; then
  if [[ -d ${BUILD_DIR} ]]; then
    rm -rf ${BUILD_DIR}/appliance/*
    rm -rf ${BUILD_DIR}/application/*
    rm -rf ${BUILD_DIR}/sdk/*
    rm -rf ${BUILD_DIR}/update/*
  fi

  if [[ -d ${TEST_DIR} ]]; then
    rm -rf ${TEST_DIR}/lib/*
  fi
fi
cp -r  ${DEV_BUILD_DIR}/server_ship/*      ${BUILD_DIR}/application/server_ship/
RETVAL=${?}
set +x
if [ ${RETVAL} != 0 ]; then
   echo "HEY THERE WAS A PROBLEM WITH A PATH, probably ${DEV_BUILD_DIR}/server_ship/"
   echo "===>>> !!! CHECK AND RETRY THE COMMAND !!! <<<==== "
   exit 1
fi
cp -r  ${DEV_BUILD_DIR}/client_ship/*      ${BUILD_DIR}/application/client_ship/
RETVAL=${?}
if [ ${RETVAL} != 0 ]; then
   echo "HEY THERE WAS A PROBLEM WITH A PATH, probably ${DEV_BUILD_DIR}/client_ship/"
   echo "===>>> !!! CHECK AND RETRY THE COMMAND !!! <<<==== "
   exit 1
fi
set -x
cp -r  ${TEST_BUILD_DIR}/svt_ship/*        ${TEST_DIR}
RETVAL=${?}
cp -r  ${TEST_BUILD_DIR}/fvt_ship/*        ${TEST_DIR}
RETVAL=$(( ${RETVAL} + ${?} ))
cp -r  ${TEST_BUILD_DIR}/testTools_ship/*  ${TEST_DIR}
RETVAL=$(( ${RETVAL} + ${?} ))
set +x
if [ ${RETVAL} != 0 ]; then
   echo "HEY THERE WAS A PROBLEM WITH A PATH, probably ${TEST_BUILD_DIR}/*_ship/"
   echo "===>>> !!! CHECK AND RETRY THE COMMAND !!! <<<==== "
   exit 1
fi

exit 0
