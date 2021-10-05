#/bin/bash 
############################################################################
#  Assumes:
# [M*_TESTROOT]/testEnv.sh (automation) or [M*_TESTROOT]/scripts/ISMsetup.sh  have been SOURCED
#
#--  config-usage()  -------------------------------------------------------
config-usage() {
  if [[ "${MSG_MODE}" != "-h"*  &&  "${MSG_MODE}" != "-H"*  &&  "${MSG_MODE}" != "-?"* ]]; then
     echo " "
     echo "$0  ${PARAMS}     -->passed invalid parameters <--"
  fi

  echo " "
  echo "SYNTAX: "
  echo "  $0  [ queue|topic ]  [ setup|cleanup ]" 
  echo "where: "
  echo "   MSG_MODE:"
  echo "      'queue'    Creates/Deletes the QUEUE/TOPICs (Deletes even if not empty)"
  echo "      'topic'    Creates/Deletes the QUEUE/TOPICs (Deletes even if not empty)"
  echo "   ACTION:"
  echo "      'setup'    Creates the QUEUE/TOPICs, installs RA, J2C objects and Ent. App into WAS"
  echo "      'cleanup'  Deletes the QUEUE/TOPICs (even if not empty), uninstalls RA, J2C objects and Ent. App from WAS"
  echo " "
  echo "examples: "
  echo "  $0  queue  setup"
  echo "  $0  topic  setup"
  echo "  $0  queue  cleanup"
  echo "  $0  topic  cleanup"
  echo " "
  echo "RC: "
  echo " 0   - Successful"
  echo " 100 - Help due to input validation failure"
  echo " 200 - Setup Failure"
  echo " 300 - Cleanup Failure"
  echo " 1, 2, or 9 from status-imaserver() indeterminate." 
  echo " "
  rc=100
  echo "config-usage.RC="${rc}
  return ${rc}
}

#***************************************************************************
#--  setup()  --------------------------------------------------------------
setup() {
#set -x
   if [ "${MSG_MODE}" == "queue" ]; then
      echo "Create IMA Queues and WAS Resources"
      `./manualrunMakeQUEUE  ${PRIMARY} 0 4 487 >  manualrunMakeQUEUE.log 2>&1`
      ((rc+=$?))
      `./manualrunMakeQUEUE  ${PRIMARY} 0 4 488 >> manualrunMakeQUEUE.log 2>&1`
      ((rc+=$?))
      `./manualrunMakeQUEUE  ${PRIMARY} 0 4 491 >> manualrunMakeQUEUE.log 2>&1`
      ((rc+=$?))
      `./manualrunMakeQUEUE  ${PRIMARY} 0 4 499 >> manualrunMakeQUEUE.log 2>&1`
      ((rc+=$?))
   else
      # Actually topics are created on the fly and don't need to preexisting to install WAS pieces, like Queues
      echo "Create IMA Topics and WAS Resources"
#     `./manualrunMakeTopic  ${PRIMARY} 0 4 487`
#   ((rc+=$?))
   fi

   `./manualrunMakeMsgHub ${PRIMARY} > manualrunMakeMsgHub.log 2>&1`
set -x
   `cp ${WAS_TESTROOT}/jython/was.config-JCD  ${WAS_TESTROOT}/jython/was.config`
   ((rc+=$?))
   `${WAS_TESTROOT}/was.sh  -a prepare  -f was.log > was.setup.log 2>&1`
   ((rc+=$?))
   `${WAS_TESTROOT}/was.sh  -a setup  -f was.log >> was.setup.log 2>&1`
   ((rc+=$?))

   echo "setup.rc ="${rc}
set +x
   return ${rc}
}

#***************************************************************************
#--  cleanup()  ------------------------------------------------------------
cleanup() {
set -x
   `ssh admin@${PRIMARY} "datetime get" > was.cleanup.log 2>&1`
   `${WAS_TESTROOT}/was.sh  -a cleanup  -f was.log >> was.cleanup.log 2>&1`
   ((rc+=$?))
   `ssh admin@${PRIMARY} "datetime get" >> was.cleanup.log 2>&1`

   if [ "${MSG_MODE}" == "queue" ]; then
      echo "Remove IMA Queues and WAS Resources"
      `ssh admin@${PRIMARY} "datetime get" > manualrunDeleteQUEUE.log 2>&1`
      `./manualrunDeleteQUEUE  ${PRIMARY} 0 4 487 >>  manualrunDeleteQUEUE.log 2>&1`
      ((rc+=$?))
      `ssh admin@${PRIMARY} "datetime get" >> manualrunDeleteQUEUE.log 2>&1`
      `./manualrunDeleteQUEUE  ${PRIMARY} 0 4 488 >> manualrunDeleteQUEUE.log 2>&1`
      ((rc+=$?))
      `./manualrunDeleteQUEUE  ${PRIMARY} 0 4 491 >> manualrunDeleteQUEUE.log 2>&1`
      ((rc+=$?))
      `./manualrunDeleteQUEUE  ${PRIMARY} 0 4 499 >> manualrunDeleteQUEUE.log 2>&1`
      ((rc+=$?))
   else
      echo "Remove IMA Topics and WAS Resources"
      `./manualrunDeleteTOPIC  ${PRIMARY} 0 4 487 >  manualrunDeleteTOPIC.log 2>&1`
      ((rc+=$?))
      `./manualrunDeleteTOPIC  ${PRIMARY} 0 4 488 >> manualrunDeleteTOPIC.log 2>&1`
      ((rc+=$?))
      `./manualrunDeleteTOPIC  ${PRIMARY} 0 4 491 >> manualrunDeleteTOPIC.log 2>&1`
      ((rc+=$?))
      `./manualrunDeleteTOPIC  ${PRIMARY} 0 4 499 >> manualrunDeleteTOPIC.log 2>&1`
      ((rc+=$?))
   fi

   echo "cleanup.rc ="${rc}
set +x
   return ${rc}
}

#***************************************************************************
# Note:  This method will EXIT if RC /= 0
#--  validate_input()  -----------------------------------------------------
validate_input() {
   echo "Validating Input parameters: ${PARAMS} "

   if [ ${PARAMS_COUNT} -ne 2 ]; then
      echo "ERROR processing input, TWO and only 2 are expected!!"
      rc=100
   fi

   if [[ "${MSG_MODE}" != "queue" && "${MSG_MODE}" != "topic" ]]; then
      echo "ERROR processing input!!  MSG_MODE: '${MSG_MODE}' is unknown."
      rc=100
   fi

   if [[ "${ACTION}" != "setup" && "${ACTION}" != "cleanup" ]]; then
      echo "ERROR processing input!!  Action: '${ACTION}' is unknown"
      rc=100
   fi
   if [ ${rc} -ne 0 ]; then
      config-usage
      exit ${rc}
   fi

   echo "validate_input.rc ="${rc}
   return ${rc}
}

#***************************************************************************

############################################################################
#  Main
############################################################################
source manualrun_HA_JMS

declare -i rc=0
declare PARAMS_COUNT="$#"
declare PARAMS="$@"
declare MSG_MODE="$1"
declare ACTION="$2"
set -x
WAS_TESTROOT=$(eval echo \$${THIS}_TESTROOT)
WAS_TESTROOT="${WAS_TESTROOT}/scripts/WAS"
set +x
echo "WAS_ROOT=${WAS_TESTROOT}"

validate_input;    ### Only returns if RC is "0" - Successful

server_list;
rc=$?

# if the IMAServer Status was able to find PRIMARY, then able to process the ACTION
if [ ${rc} -eq 0 ]; then

   if [ "${ACTION}" == "setup" ]; then
      setup
      rc=$?
   else
      cleanup
      rc=$?
   fi
else
   echo "ERROR:  FAILED on 'status-imaserver' returned rc=${rc} - unable to perform ACTION=${ACTION} "
fi

exit $rc
