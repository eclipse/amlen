#! /bin/bash
# Get the ISM Build installed on target machine
# User may wish to modify default values in parseInputs()
#
# Assumes: mount to mar145 can be made as:  /mnt/mar145
#---------------------------------------------------------------------------------

printUsage () { 
  echo "---------------------------------------------------------------------------------"
  echo " ${0} : Install a Public ISM Build!"
  echo " "
  echo "Syntax:"
  echo "  ${0}  -i[ISM SDK Install Directory]  -t[Test Install Directory] -R[Build Release] -B[Build Type] -b[Build Level Name] -x[Test build Level Name] -c[true|false] "
  echo "Inputs:  "
  echo "  -i[ISM SDK Install Directory]  Default:  '${BUILD_DIR}'"
  echo "  -t[Test Install Directory]     Default:  '${TEST_DIR}'"
  echo "  -R[Build Release]              Default:  (../release/${THIS_DEV_RELEASE}/..)  [ISM13a, IMA100, IMA13b, IMA110, IMA14a, ...]"
  echo "  -B[Build Type]                 Default:  (../release/${THIS_DEV_RELEASE}/${DEV_BUILDPATH}/..)  [PROD, DEV]"
  echo "  -b[Build Level Name]           ex:  value from /gsacache/release/${THIS_DEV_RELEASE}/${DEV_BUILDPATH}/[Build Level Name]"
  echo "  -x[Test build Level Name]      ex:  value from /gsacache/release/${THIS_TEST_RELEASE}/${TEST_BUILDPATH}/[Test build Level Name]"
  echo "  -c[true|false]                 ex:  true delete appliance, application, sdk, update, and lib dirs before copying new files"
  echo " "
  echo "---------------------------------------------------------------------------------"
  exit 1
}

#---------------------------------------------------------------------------------
parseInputs () { 

#---  Default Values  -----------
BLD_SERVER="10.10.10.10"
BUILD_DIR="/niagara"
BUILD_DIR_BAK="/niagara.bak"
TEST_DIR="/niagara/test"
A_BUILDTYPE="DEV"
DEV_BUILDPATH="development"
TEST_BUILDPATH="test"
BETA_PATH="current"
THIS_DEV_RELEASE="IMADev"
THIS_TEST_RELEASE="IMADev"
CLEAN=false
#--------------------------------

    while getopts "i:t:R:B:b:x:c:h" option ${OPTIONS}
    do
        case ${option} in
        i )
            BUILD_DIR=${OPTARG};;

        t )
            TEST_DIR=${OPTARG};;

        R )
            THIS_DEV_RELEASE=${OPTARG}
            THIS_TEST_RELEASE=${OPTARG}
            if [ ${THIS_DEV_RELEASE} == "PROXY14a" ]; then
            echo "  %==>  PROXY14a does not build Test directories -  yet PROXY14a is no longer a unique build - are you sure you want PROXY14a?"
            echo "        USING IMA14a for TEST Build ship libs"
			THIS_TEST_RELEASE="IMA14a"
            fi
            ;;

        B )
            A_BUILDTYPE=${OPTARG^^};;

        b )
            A_BUILD=${OPTARG};;

        x )
            T_BUILD=${OPTARG};;

        c )
            CLEAN=${OPTARG};;

        h )
            printUsage;;

        * )
            echo "ERROR:  Unknown option: ${option}"
            printUsage;;

        esac    
    done


}

#---------------------------------------------------------------------------------
# Start of Main
#---------------------------------------------------------------------------------
#set -x
OPTIONS="$@"
parseInputs

if [[ ! -e "${BUILD_DIR}" ]];  then  mkdir -p ${BUILD_DIR};  fi
if [[ ! -e "${TEST_DIR}" ]];  then  mkdir -p ${TEST_DIR};  fi

# Access the Public Build Directory
if [[ ! -e "/mnt/mar145" ]];  then  mkdir -p "/mnt/mar145";  fi
if [[ ! -e "/mnt/mar145/release" ]];  then  mount  ${BLD_SERVER}:/gsacache  /mnt/mar145;  fi

# Mounts have given access to the Public Dev Directory paths
if [ ${A_BUILDTYPE} == "PROD" ]; then
    DEV_BUILDPATH="production"
    TEST_BUILDPATH="prod_test"
elif [ ${A_BUILDTYPE} == "DEV" ]; then
    DEV_BUILDPATH="development"
    TEST_BUILDPATH="test"
else
    echo "BUILDTYPE of: ${A_BUILDTYPE}  is not recognized, valid values are shown in syntax that follows."
    echo " "
    printUsage;
fi


DEV_BUILD_DIR="/mnt/mar145/release/${THIS_DEV_RELEASE}/${DEV_BUILDPATH}"
TEST_BUILD_DIR="/mnt/mar145/release/${THIS_TEST_RELEASE}/${TEST_BUILDPATH}"


if [ ${TEST_DIR} != "." ]
then
  TEST_DIR=${TEST_DIR}/
fi

#set -x
dirpath=`eval echo ${BUILD_DIR} | cut -c 1`
if [[ ${dirpath} != "/" ]]
then
  BUILD_DIR=/${BUILD_DIR}
fi 
cd ${BUILD_DIR}

if [ "${A_BUILD}" == "" ]
then
  #set -x
  # -v sorts ls output by name
  ALLBUILDS=`$(eval echo ls -1v ${DEV_BUILD_DIR}/ )`
  #set +x
  for WORD in ${ALLBUILDS}
  do
    echo ${WORD}
    if [[ ${WORD} =~ [0-9]+ ]]; then
       LAST=${WORD}
    fi
  done
  read -e -i "${LAST}" -p "The available MessageSight '${THIS_DEV_RELEASE}' builds in '${A_BUILDTYPE}' are shown, pick one and press Enter:  " A_BUILD
fi

if [[ -d ${BUILD_DIR} && -d ${BUILD_DIR_BAK} ]]; then
  read -e -i "Y" -p "Backup ${BUILD_DIR}/test to ${BUILD_DIR_BAK} (y/n)?  " ANS
  if [[ ${ANS} = [Yy]* ]]; then
    echo Copying ${BUILD_DIR}/test to ${BUILD_DIR_BAK}
    cp -rpf ${BUILD_DIR}/test ${BUILD_DIR_BAK}/
  fi
fi

# LEAVE THESE TRACE SETTING UNCOMMENTED IN CASE THERE IS A COPY ERROR
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
cp -r  ${DEV_BUILD_DIR}/${A_BUILD}/*         ${BUILD_DIR}
RETVAL=${?}
set +x
if [ ${RETVAL} != 0 ]; then
   echo "HEY THERE WAS A PROBLEM WITH A PATH, probably ${DEV_BUILD_DIR}/${A_BUILD}/"
   echo "===>>> !!! CHECK AND RETRY THE COMMAND !!! <<<==== "
   exit 1
fi

cp -pf ${BUILD_DIR}/application/server_ship/bin/mqttbench ${BUILD_DIR}/application/client_ship/bin/

# If the TEST BUILD with Same Name(A_BUILD) does not exist... sometimes happens with Beta...prompt for Test Builds
if [[ "${T_BUILD}" == "" ]]; then T_BUILD=${A_BUILD}; fi
if [[ ! -e "${TEST_BUILD_DIR}/${T_BUILD}" ]]; then
  # set -x
  ALLBUILDS=`$(eval echo ls -tr ${TEST_BUILD_DIR}/ )`
  # set +x
  for WORD in ${ALLBUILDS}
  do
    echo ${WORD}
    if [[ ${WORD} =~ [0-9]+ ]]; then
       LAST=${WORD}
    fi
  done
  read -e -i "${LAST}" -p "The available TEST '${THIS_TEST_RELEASE}' builds in '${A_BUILDTYPE}' are shown, pick one and press Enter:  "  T_BUILD
fi
set -x
cp -r  ${TEST_BUILD_DIR}/${T_BUILD}/fvt/*     ${TEST_DIR}
RETVAL=${?}
cp -r  ${TEST_BUILD_DIR}/${T_BUILD}/svt/*     ${TEST_DIR}
RETVAL=$(( ${RETVAL} + ${?} ))
cp -r  ${TEST_BUILD_DIR}/${T_BUILD}/tools/*   ${TEST_DIR}
RETVAL=$(( ${RETVAL} + ${?} ))
set +x
if [ ${RETVAL} != 0 ]; then
   echo "HEY THERE WAS A PROBLEM WITH A PATH, probably ${TEST_BUILD_DIR}/${T_BUILD}/"
   echo "===>>> !!! CHECK AND RETRY THE COMMAND !!! <<<==== "
   exit 1
fi

exit 0
