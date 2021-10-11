#!/bin/sh
#
# Author: Frank Colca
#
# Description:
# This script is used to list/modify STAX NAMESPACE variables.
# 
#
#----------------------------------------------------------------------------------
#set -x
source /staf/STAFEnv.sh STAF

# get and validate release
endScript='FALSE'
release="none"
until [ $endScript == 'TRUE' ]
do
  clear
  echo ""
  echo ""
  echo "This script is used to list/modify STAX NAMESPACE variables"
  echo ""

  validInput='FALSE'
  until [ $validInput == 'TRUE' ]
  do
    if [ $release != 'none' ] ; then
      clear
      echo ""
      echo "Current values for release \"${release}\" are:"
      echo ""
      staf local NAMESPACE LIST NAMESPACE ${release} ONLY
      staf local NAMESPACE LIST NAMESPACE ISM | grep ${release}
    fi
    echo ""
    echo "Releases:                                                        "
    echo "======================================================================================================"
    echo "[a] IMADev            list/modify STAX NAMESPACE variables for \"IMADev\" release"  
    echo "[b] IMA200-IFIX       list/modify STAX NAMESPACE variables for \"IMA200-IFIX\" release"  
    echo "[c] IMA15a-IoT-IFIX   list/modify STAX NAMESPACE variables for \"IMA15a-IoT-IFIX\" release"
    echo "[d] IMA15a-Proxy-IFIX list/modify STAX NAMESPACE variables for \"IMA15a-Proxy-IFIX\" release"
    echo "[e] IMA500-IFIX       list/modify STAX NAMESPACE variables for \"IMA500-IFIX\" release"  
    echo "[f] IMA15aX           list/modify STAX NAMESPACE variables for \"IMA15aX\" release"  
    echo "[g] IMAProd           list/modify STAX NAMESPACE variables for \"IMAProd\" release"  
    echo "[h] IMA-MG500-IFIX    list/modify STAX NAMESPACE variables for \"IMA-MG500-IFIX\" release"  
    echo "[i] MGPROD            list/modify STAX NAMESPACE variables for \"MGPROD\" release"  
    echo "[x] exit              exit this script"  
    echo "======================================================================================================"
    echo -n "Enter your choice [a - k or x]: "
    read relChoice
    case $relChoice in
      a) release='IMADev'            ; validInput='TRUE' ;;
      b) release='IMA200-IFIX'       ; validInput='TRUE' ;;
      c) release='IMA15a-IoT-IFIX'   ; validInput='TRUE' ;;
      d) release='IMA15a-Proxy-IFIX' ; validInput='TRUE' ;;
      e) release='IMA500-IFIX'       ; validInput='TRUE' ;;
      f) release='IMA15aX'           ; validInput='TRUE' ;;
      g) release='IMAProd'           ; validInput='TRUE' ;;
      h) release='IMA-MG500-IFIX'    ; validInput='TRUE' ;;
      i) release='MGPROD'            ; validInput='TRUE' ;;
      x) exit ;;
      *) echo "Invalid Choice... \"$relChoice\" is not in valid choice, please select choice a-j or x";
    esac
  done

  clear
  echo ""
  echo "Current values for release \"${release}\" are:"
  echo ""
  staf local NAMESPACE LIST NAMESPACE ${release} ONLY
  staf local NAMESPACE LIST NAMESPACE ISM | grep ${release}

  # get and validate action
  validInput='FALSE'
  action='NONE'
  actChoice='99999'
  resetConfirm='no'
  until [ $validInput == 'TRUE' ]
  do
    validInput='TRUE'
    echo ""
    echo ""
    echo "Actions:                                                        "
    echo "=========================================================================================================================="
    echo "[1] lock         lock a test group in \"$release\"' so it will not run, BUT WILL BE QUEUED (set to IN_USE)"  
    echo "[2] unlock       unlock a test group in \"$release\"' so it will run (set to NOT_IN_USE)"  
    echo "[3] block        block a test group in \"$release\"' so NEW STAX JOBS WILL BE IMMEDIALTELY TERMINATED (set to DO_NOT_RUN)"  
    echo "[4] terminate    terminate a test group that is running for release \"$release\""  
    echo "[5] delete       delete a STAX NAMESPACE key that was set during a test run for release \"$release\""
    echo "[6] reset        reset all STAX NAMESPACE settings to the default setting for release \"$release\""  
    echo "[7] insttype     set PXE or FIRMWARE UPGRADE to install build on Appliances for release \"$release\""  
    echo "[x] return       return to main menu"
    echo "=========================================================================================================================="
    echo -n "Enter your choice [1 - 8 or x]: "
    read actChoice
    case $actChoice in
      1) action='IN_USE'     ; validInput='TRUE' ;;
      2) action='NOT_IN_USE' ; validInput='TRUE' ;;
      3) action='DO_NOT_RUN' ; validInput='TRUE' ;;
      4) action='TERMINATE'  ; validInput='TRUE' ;;
      5) action='delete'     ; validInput='TRUE' ;;
      6) echo "WARNING: This will RESET all settings to their default value (as if NO automation is currently running for ${release})" ;
         echo -n "Type \"reset\" to confirm or any other key abort: " ;
         read resetConfirm ;
         if [ ^$resetConfirm == "^reset" ] ; then
           staf local NAMESPACE DELETE NAMESPACE ${release} CONFIRM 
           staf local NAMESPACE CREATE NAMESPACE ${release} DESCRIPTION "${release} Namespace" PARENT ISM
           staf local NAMESPACE SET VAR fvt_currentJOBID=NONE VAR fvt_queuedJOBID=NONE VAR fvt_prod_queuedJOBID=NONE VAR fvt_prod_currentJOBID=NONE VAR svtlong_prod_currentJOBID=NONE VAR svtlong_prod_queuedJOBID=NONE VAR svt_prod_currentJOBID=NONE VAR svt_prod_queuedJOBID=NONE VAR fvt_InstType=PXE VAR fvt_prod_InstType=PXE VAR svtlong_prod_InstType=PXE VAR svt_prod_InstType=PXE VAR logLevel=TRACE NAMESPACE ${release} 
         fi ;;
      7) action='insttype' ; validInput='TRUE' ;;
      x) action='done'      ; validInput='TRUE' ;;
      *) echo "Invalid Choice... \"$actChoice\" is not in valid choice, please select choice a - f";  validInput='FALSE';;
    esac
  done

  if [ $action == 'IN_USE' -o $action == "NOT_IN_USE" -o $action == "DO_NOT_RUN" -o $action == "TERMINATE" ] ; then
    # get and validate test group
    validInput='FALSE'
    until [ $validInput == 'TRUE' ]
    do
      clear
      echo ""
      echo ""
      echo "The current settings for release \"$release\" are:"
      staf local NAMESPACE LIST NAMESPACE ISM | grep ${release}
      echo ""
      echo ""
      echo "Options for setting $action:                                                        "
      echo "======================================================================================================"
      echo "[1] fvt_$release           set \"fvt_$release\" to \"$action\""  
      echo "[2] fvt_prod_$release      set \"fvt_prod_$release\" to \"$action\""  
      echo "[3] svtlong_prod_$release  set \"svtlong_prod_$release\" to \"$action\""  
      echo "[4] svt_prod_$release      set \"svt_prod_$release\" to \"$action\""  
      echo "[x] return                 return to main menu"
      echo "======================================================================================================"
      echo -n "Enter your choice [1 - 5 or x]: "
      read typeChoice
      case $typeChoice in
        1) staf local NAMESPACE SET VAR fvt_$release=$action NAMESPACE ISM ; clear ; staf local NAMESPACE LIST NAMESPACE ${release};;
        2) staf local NAMESPACE SET VAR fvt_prod_$release=$action NAMESPACE ISM ; clear ; staf local NAMESPACE LIST NAMESPACE ${release};;
        3) staf local NAMESPACE SET VAR svtlong_prod_$release=$action NAMESPACE ISM ; clear ; staf local NAMESPACE LIST NAMESPACE ${release};;
        4) staf local NAMESPACE SET VAR svt_prod_$release=$action NAMESPACE ISM ; clear ; staf local NAMESPACE LIST NAMESPACE ${release};;
        x) validInput='TRUE' ;;
        *) echo "Invalid Choice... \"$typeChoice\" is not a valid choice, please select choice a - d";  validInput='FALSE';;
      esac
      staf local NAMESPACE LIST NAMESPACE ISM | grep ${release}
    done
  fi

  if [ $action == 'insttype' ] ; then
    done='FALSE'
    until [ $done == 'TRUE' ]
    do
      validInput='FALSE'
      clear
      echo ""
      echo ""
      echo "Current Settings for ${release}:                                                        "
      echo ""
      staf local NAMESPACE LIST NAMESPACE ${release} | grep "PXE\|UPGRADE"
      echo ""
      echo ""
      echo ""
      echo "Select test group to modify the $action:                                                        "
      echo "======================================================================================================"
      echo "[1] fvt_InstType           modify the install type for \"fvt_InstType\" in $release\""  
      echo "[2] fvt_prod_InstType      modify the install type for \"fvt_prod_InstType\" in $release\""  
      echo "[3] svtlong_prod_InstType  modify the install type for \"svtlong_prod_InstType\" in $release\""  
      echo "[4] svt_prod_InstType      modify the install type for \"svt_prod_InstType\" in $release\""  
      echo "[x] return                 return to main menu"
      echo "======================================================================================================"
      echo -n "Enter your choice [1 - 5 or x]: "
      read testGroup
      case $testGroup in
        1) testGroup="fvt_InstType"; validInput='TRUE' ;;
        2) testGroup="fvt_prod_InstType"; validInput='TRUE' ;;
        3) testGroup="svtlong_prod_InstType"; validInput='TRUE';;
        4) testGroup="svt_prod_InstType"; validInput='TRUE' ;;
        x) done='TRUE' ;;
        *) echo "Invalid Choice... \"$testGroup\" is not in valid choice, please select choice 1 - 4";  validInput='FALSE';;
      esac
      if [ $validInput == 'TRUE' ] ; then
        echo ""
        echo ""
        echo ""
        echo "Chose the Install type for $testGroup:                                                        "
        echo "======================================================================================================"
        echo "[1] PXE                  Set PXE install type for test group \"$testGroup\" in $release\""  
        echo "[2] Firmware Upgrade     Set Firmware Upgrade install type for test group \"$testGroup\" in $release\""  
        echo "======================================================================================================"
        echo -n "Enter your choice [1 or 2]: "
        read instType
        case $instType in
          1) staf local NAMESPACE SET VAR $testGroup=PXE NAMESPACE $release ;;
          2) staf local NAMESPACE SET VAR $testGroup=UPGRADE NAMESPACE $release ;;
          *) echo "Invalid Choice... \"$insttype\" is not in valid choice, please select choice 1 - 9";  validInput='FALSE';;
        esac
      fi
    done
  fi

  if [ $action == 'delete' ] ; then
    finished='FALSE'
    until [ $finished == 'TRUE' ]
    do
      clear
      staf local NAMESPACE LIST NAMESPACE ${release} | grep 'mar\|AF' > /dev/null
      if [ $? != '0' ] ; then 
        echo ""
        echo "There are no KEYS to delete... press Enter to continue"
        read keyStroke
        finished='TRUE'
      else
        echo ""
        echo "The following KEYS are currenlty set:"
        echo ""
        staf local NAMESPACE LIST NAMESPACE ${release} | grep 'mar\|AF\|Key\|----------------------'
        echo ""
        echo -n "Enter KEY to delete (ie. mar123 or AFfvt01) OR \"q\" when done: "
        read keyChoice
        if [ $keyChoice == 'q' -o $keyChoice == 'Q' ] ; then
          finished='TRUE'
        else
          staf local NAMESPACE DELETE VAR ${keyChoice} NAMESPACE ${release}
        fi
      fi
    done
  fi
done
exit 0
