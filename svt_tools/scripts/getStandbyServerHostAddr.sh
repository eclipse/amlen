#!/bin/bash

##############################################################################
#
#  This script determines which appliance is the primary by calling
#  getPrimaryServerHostAddr.sh and then outputs the other appliance
#  host address as the standby
#
#  "error" is returned if the an unexpected failure occurs
#  This could occur when:
#   - unable to locate ISMsetup.sh
#   - A1_HOST and/or A2_HOST is not defined
#   - getPrimaryServerHost.sh not found
#
#  "unknown" is returned if the script is unable to determine the 
#  standby appliance address.  
#  This could occur when:
#   - getPrimaryServerHost.sh fails
#
##############################################################################

  ##############################################################################
  #
  #  setup:  Sources testEnv.sh when found or otherwise ISMsetup.sh and validates 
  #          required environment variables and scripts.  
  #          Calls error() if it fails.
  #
  ##############################################################################
  setup() {
    if [[ -f "../testEnv.sh"  ]]; then
      source  "../testEnv.sh"
    else
      if [[ -f "../scripts/ISMsetup.sh"  ]]; then
        source ../scripts/ISMsetup.sh
      elif [[ -f "/niagara/test/scripts/ISMsetup.sh"  ]]; then
        source /niagara/test/scripts/ISMsetup.sh
      else
        error
      fi
    fi
  
    if [ -z ${THIS} ]; then
      ### $THIS is needed below and not defined. ###
      error
    elif [ -z ${A1_HOST} ]; then
      ### $A1_HOST is needed below and not defined. ###
      error
    elif [ -z ${A2_HOST} ]; then
      ### $A2_HOST is needed below and not defined. ###
      error
    fi
  
    export TESTROOT=$(eval echo \${${THIS}_TESTROOT})
    
    if [[ ! -f "${TESTROOT}/scripts/getPrimaryServerHostAddr.sh"  ]]; then
      ### getPrimaryServerHostAddr.sh is needed below and not found. ###
      error
    fi
  
  }

  ##############################################################################
  #
  #  error:  prints out "error" and exits with error code 1
  #
  ##############################################################################
  error() {
    printf "error\n"
    exit 1
  }

  ##############################################################################
  #
  #  run:  Determines the primary server host address then returns the host address
  #        of standby appliance
  #      
  #        Calls error() if it fails.
  #
  ##############################################################################
  run() {
    export svt_server=unknown
  
    local PRIMARY=`${TESTROOT}/scripts/getPrimaryServerHostAddr.sh`
    if [ "${A1_HOST}" == "${PRIMARY}" ]; then
      export svt_server=${A2_HOST}
    elif [ "${A2_HOST}" == "${PRIMARY}" ]; then
      export svt_server=${A1_HOST}
    fi
  }


##############################################################################
#
#  body:  Call setup() to setup env variables and then call run() to determine 
#         the standby appliance.  Finally it prints the standby appliance host 
#         address.
#
##############################################################################

setup
run
printf "${svt_server}\n"

### end of script ###
