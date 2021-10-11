#!/bin/sh
#
# Author: Frank Colca
#
# Description:
# This script is used to generate a STAF Event Notice to start an automated test run.
# 
#
#----------------------------------------------------------------------------------
#set -x

if [ $# -ge 1 ] ; then
  if [ $# -ne 3 -o `echo $1 | tr 'A-Z' 'a-z'` == 'help' ] ; then
    clear
    echo ""
    echo ""
    echo "This script is used to generate a STAF Event Notice to start an ISM automated test run."
    echo ""
    echo ""
    echo "For interactive mode (script will prompt you for required input):"
    echo ""
    echo "  $0 (with no parameters)"
    echo ""
    echo ""
    echo "For non-interactive mode:"
    echo ""
    echo "  $0 <serverRelease> <bldType> <testGroup>"
    echo ""
    echo "    serverRelease = Server build release to test (IMADev OR IMA200-IFIX OR IMA500-IFIX OR IMA-MG500-IFIX OR IMA15aX OR IMAProd OR MGPROD)"
    echo "    bldType = development OR production"
    echo "    testGroup  = test group to run (ie. fvt_default)"
    echo ""
    echo "    NOTE: non-iteractive mode will determine and use the most recent build level for the"
    echo "          serverRelease specified."
    echo ""
    echo ""
    exit 1
  else

    # Verify that the specified release is valid
    serverRelease=$1
    if [ $serverRelease != "IMADev" -a $serverRelease != "IMA200-IFIX" -a $serverRelease != "IMA15a-IoT-IFIX" -a $serverRelease != "IMA15a-Proxy-IFIX" -a $serverRelease != "IMA15aX" -a $serverRelease != "IMA500-IFIX" -a $serverRelease != "IMAProd" -a $serverRelease != "IMA-MG500-IFIX" -a $serverRelease != "MGPROD" ] ; then
      echo "Error... must be an Amlen Server build release (IMADev OR IMA200-IFIX OR IMA15a-IoT-IFIX OR IMA15a-Proxy-IFIX OR IMA15aX OR IMA500-IFIX OR IMA-MG500-IFIX OR IMAProd OR MGPROD)."
      exit 1
    fi  

    pubDir=$serverRelease

    # Verify that the build type is valid
    #bldType=$2
    #if [ ! -e /gsacache/release/$pubDir/$bldType ] ; then
    #  echo "Error... \"/gsacache/release/$pubDir/$bldType\" does not exist (valid bldType are production OR development)"
    #  exit 1
    #fi  

    # Verify that the specified testGroup is valid
    testGroup=`echo $3 | tr 'A-Z' 'a-z'`
    case $testGroup in
      fvt_default)         ;;
      fvt_prod)            ;;
      fvt_request)         ;;
      fvt_debug)           ;;
      svtlong_prod)        ;;
      svt_prod)            ;;
      svt_request)         ;;
      svt_debug)           ;;
      bvt)                 ;;
      *) echo "Error... \"$testGroup\" is not a valid testGroup, use interactive mode for valid choices"
         exit 1 ;
    esac

    # Verify that the build type is development if testGroup is fvt_default or svt_default
    if [ $testGroup == "svt_default" -o $testGroup == "fvt_default" ] ; then
      if [ $bldType != "development" ] || [ $bldType != "production" ]; then
        echo "Error... (bldType=$bldType testGroup=$testGroup) bldType must be development or production for $testGroup"
        exit 1
      fi
    fi

    # Verify that the build type is production if testGroup is fvt_prod or svt_prod or svtlong_prod
    if [ $testGroup == "svt_prod" -o $testGroup == "svtlong_prod" -o $testGroup == "fvt_prod" ] ; then
      if [ $bldType != "production" ] ; then
        echo "Error... (bldType=$bldType testGroup=$testGroup) bldType must be production for $testGroup"
        exit 1
      fi
    fi

    # Verify that the build type is development if testGroup is bvt
    #if [ $testGroup == "bvt" -a $bldType != "development" ] ; then
    #  echo "Error... (bldType=$bldType testGroup=$testGroup) bldType must be development for $testGroup"
    #  exit 1
    #fi

    # determine the latest build Level for the specified release
    # this way of returning the label is broken
    # serverLabel=$(ls /gsacache/release/$pubDir/$bldType | sort -r | grep -m1 "202.....-...." | cut -b -13`    
    serverLabel=$(find /gsacache/release/$pubDir/$bldType -maxdepth 1 -exec basename {} \; | sort -r | grep -m1 -P "^20\d{6}-\d{4}")
    clientRelease=$serverRelease
    clientLabel=$serverLabel
    testRelease=$serverRelease
    testLabel=$serverLabel
  fi
else
  clear
  echo ""
  echo ""
  echo "This script is used to generate a STAF Event Notice to start an automated test run."
  echo ""

  # get and validate build release
  validInput='FALSE'
  until [ $validInput == 'TRUE' ]
  do
    echo ""
    echo "                IMA Releases                      "
    echo " (for migration, pick the release to migrate from)"
    echo "======================================================"
    echo "[a] IMADev"  
    echo "[b] IMA200-IFIX"  
    echo "[c] IMA15a-IoT-IFIX" 
    echo "[d] IMA15a-Proxy-IFIX" 
    echo "[e] IMA500-IFIX"  
    echo "[f] IMA15aX"  
    echo "[g] IMAProd"  
    echo "[h] IMA-MG500-IFIX"  
    echo "[i] MGPROD"  
    echo "======================================================"
    echo -n "Enter your choice [a-i]: "
    read releaseChoice
    case $releaseChoice in
      a) serverRelease='IMADev'            ; validInput='TRUE' ;;
      b) serverRelease='IMA200-IFIX'       ; validInput='TRUE' ;;
      c) serverRelease='IMA15a-IoT-IFIX'   ; validInput='TRUE' ;;
      d) serverRelease='IMA15a-Proxy-IFIX' ; validInput='TRUE' ;;
      e) serverRelease='IMA500-IFIX'       ; validInput='TRUE' ;;
      f) serverRelease='IMA15aX'           ; validInput='TRUE' ;;
      g) serverRelease='IMAProd'           ; validInput='TRUE' ;;
      h) serverRelease='IMA-MG500-IFIX'    ; validInput='TRUE' ;;
      i) serverRelease='MGPROD'           ; validInput='TRUE' ;;
        *) echo "Invalid Choice... \"$releaseChoice\" is not in valid choice, please select a-i";
    esac
  done

  pubDir=$serverRelease

  # get and validate build type
  validInput='FALSE'
  until [ $validInput == 'TRUE' ]
  do
    echo ""
    echo "                                   Build Types                                                        "
    echo "======================================================================================================"
    echo "[a] development         Run the Automation using a development build (built in Austin)"  
    echo "[b] production          Run the Automation using a production build (built in Hursley)"  
    echo "======================================================================================================"
    echo -n "Enter your choice [a or b]: "
    read btChoice
    case $btChoice in
      a) bldType='development'         ; validInput='TRUE' ;;
      b) bldType='production'          ; validInput='TRUE' ;;
        *) echo "Invalid Choice... \"$btChoice\" is not in valid choice, please select choice a or b";
    esac
    if ssh imapublish "[ ! -d /gsacache/release/$pubDir/$bldType ]" ; then
      echo "Error:... \"/gsacache/release/$pubDir/$bldType\" does not exist"
      exit 1
    fi
  done

  # get the desired test group to run
  validInput='FALSE'
  until [ $validInput == 'TRUE' ]
  do
    echo ""
    echo "Release      = $serverRelease"
    echo "Build Type   = $bldType"
    echo ""
    echo "                                   Test Groups                                        "
    echo "================================================================================================================================================="
    echo "[a] fvt_default          FVT - run the test group that is run against the daily build"  
    echo "[b] fvt_prod             FVT - run the FVT PRODUCTION test group that is run against the daily build"  
    echo "[c] fvt_compat           FVT - run the FVT compatability test group (IMA Server, $serverRelease, against different test/client release(s))"
    echo "[d] fvt_migrate          FVT - run the FVT migration test group (IMA Server, $serverRelease, populated then upgraded/tested with newer IMA Server"
    echo "[e] fvt_request          FVT - for special requests, runs the tests defined in the request test group"
    echo "[f] fvt_debug            FVT - for debug purposes only, runs the tests defined in the debug test group"
    echo "[g] svtlong_prod         SVT - run the SVT LONG RUN PRODUCTION test group that is run against the daily build"  
    echo "[h] svt_prod             SVT - run the SVT PRODUCTION test group that is run against the daily build"  
    echo "[i] svt_compat           SVT - run the SVT compatability test group (IMA Server, $serverRelease, against different test/client release(s))"
    echo "[j] svt_request          SVT - for special requests, runs the tests defined in the request test group"
    echo "[k] svt_debug            SVT - for debug purposes only, runs the tests defined in the debug test group"
    echo "[l] bvt                  BVT - run the BVT test group"
    echo "================================================================================================================================================="
    echo -n "Enter your choice [a-l]: "
    read tgChoice
    case $tgChoice in
      a) testGroup='fvt_default'         ; validInput='TRUE' ;;
      b) testGroup='fvt_prod'            
         if [ $bldType != 'production' ] ; then
          echo "Error... \"$testGroup\" is not a valid testGroup for build type \"$bldType\", $testGroup requires build type \"production\""
         else
           validInput='TRUE'
         fi 
         ;;
      c) testGroup='fvt_compat'          ; validInput='TRUE' ;;
      d) testGroup='fvt_migrate'         ; validInput='TRUE' ;;
      e) testGroup='fvt_request'         ; validInput='TRUE' ;;
      f) testGroup='fvt_debug'           ; validInput='TRUE' ;;
      g) testGroup='svtlong_prod'            
         if [ $bldType != 'production' ] ; then
           echo "Error... \"$testGroup\" is not a valid testGroup for build type \"$bldType\", $testGroup requires build type \"production\""
         else
           if [ $serverRelease == 'IMA100' -o $serverRelease == 'IMA100-IFIX' -o $serverRelease == 'IMA110' -o $serverRelease == 'IMA110-IFIX' ] ; then
             echo "Error... \"$testGroup\" is not a valid testGroup for release \"$serverRelease\""
           else
             validInput='TRUE'
           fi 
         fi 
         ;;
      h) testGroup='svt_prod'            
         if [ $bldType != 'production' ] ; then
          echo "Error... \"$testGroup\" is not a valid testGroup for build type \"$bldType\", $testGroup requires build type \"production\""
         else
           validInput='TRUE'
         fi 
         ;;
      i) testGroup='svt_compat'          ; validInput='TRUE' ;;
      j) testGroup='svt_request'         ; validInput='TRUE' ;;
      k) testGroup='svt_debug'           ; validInput='TRUE' ;;
      l) testGroup='bvt'                 
         if [ $bldType != 'development' ] && [ $bldType != 'production' ]; then
           echo "Error... \"$testGroup\" is not a valid testGroup for build type \"$bldType\", $testGroup requires build type \"development\" or \"production\""
         else
           validInput='TRUE'
         fi ;;
      *) echo "Invalid Choice... \"$tgChoice\" is not in valid choice, please select choice a-l";
    esac
  done

  # get and validate Server build level
  validInput='FALSE'
  until [ $validInput == 'TRUE' ]
  do
    echo ""
    echo "Release = $serverRelease"
    echo ""
    echo "Available $serverRelease Label:"
    echo "--------------------------------"
    if [ "$bldType" = "production" ]; then
      bldTypeR="centos_production"
    elif [ "$bldType" = "development" ]; then
      bldTypeR="centos_development"
    else
      echo "No recognizable build types."
      exit 2
    fi
    #ls /gsacache/release/${pubDir}/${bldTypeR | sort | grep "201.....-...." | cut -b -13
    #find /gsacache/release/${pubDir}/${bldTypeR}/ -maxdepth 1 -exec basename {} \; | sort | grep -P "^20\d{6}-\d{4}"
    ssh imapublish "find /gsacache/release/${pubDir}/${bldTypeR}/ -maxdepth 1 -exec basename {} \; | sort | grep -P '^20\d{6}-\d{4}'"
    
    echo -n "Please enter $serverRelease Label from the above list:"
    read serverLabel
    if [ ${#serverLabel} -lt 1 ] ; then
      echo "Invalid Entry... please try again"
    else
      if ssh imapublish "[ ! -d /gsacache/release/$pubDir/$bldType/$serverLabel ]" ; then
        echo "Invalid Entry... \"/gsacache/release/$pubDir/$bldType/$serverLabel\" does not exist"
      else
        validInput='TRUE'
      fi
    fi
  done

  # use server release/label unless compat or migrate testGroup (then we need to get client release/label and test release/label)
  if [ $testGroup != 'fvt_compat' -a $testGroup != 'svt_compat' -a $testGroup != 'fvt_migrate' -a $testGroup != 'svt_migrate' ] ; then
    clientRelease=$serverRelease
    clientLabel=$serverLabel
    testRelease=$serverRelease
    testLabel=$serverLabel
  else
    # get and validate release for client
    validInput='FALSE'
    until [ $validInput == 'TRUE' ]
    do
      echo ""
      echo "  Client Releases for $testGroup  "
      echo "=================================="
      echo "[a] IMADev"  
      echo "[b] IMA200-IFIX"  
      echo "[c] IMA15a-IoT-IFIX" 
      echo "[d] IMA15a-Proxy-IFIX"
      echo "[e] IMA500-IFIX"  
      echo "[f] IMA15aX"  
      echo "[g] IMAProd"  
      echo "[h] IMA-MG500-IFIX"  
      echo "[i] MGPROD"
      echo "=================================="
      echo -n "Enter your choice [a-i]: "
      read releaseChoice
      case $releaseChoice in
        a) clientRelease='IMADev'            ; validInput='TRUE' ;;
        b) clientRelease='IMA200-IFIX'       ; validInput='TRUE' ;;
        c) clientRelease='IMA15a-IoT-IFIX'   ; validInput='TRUE' ;;
        d) clientRelease='IMA15a-Proxy-IFIX' ; validInput='TRUE' ;;
        e) clientRelease='IMA500-IFIX'       ; validInput='TRUE' ;;
        f) clientRelease='IMA15aX'           ; validInput='TRUE' ;;
        g) clientRelease='IMAProd'           ; validInput='TRUE' ;;
        h) clientRelease='IMA-MG500-IFIX'    ; validInput='TRUE' ;;
        i) clientRelease='MGPROD'            ; validInput='TRUE' ;;
          *) echo "Invalid Choice... \"$releaseChoice\" is not in valid choice, please select a-i";
      esac
    done
    # get and validate Client build level
    validInput='FALSE'
    until [ $validInput == 'TRUE' ]
    do
      echo ""
      echo "Release = $clientRelease"
      echo ""
      echo "Available Client Label:"
      echo "-----------------------"
      ssh imapublish "find /gsacache/release/${clientRelease}/${bldType}/ -maxdepth 1 -exec basename {} \; | sort | grep -P '^20\d{6}-\d{4}'"
      ls /gsacache/release/$clientRelease/$bldType | sort | grep "201.....-...." | cut -b -13
      echo -n "Please enter Client Label from the above list:"
      read clientLabel
      if [ ${#clientLabel} -lt 1 ] ; then
        echo "Invalid Entry... please try again"
      else
        if ssh imapublish "[ ! -d /gsacache/release/$clientRelease/$bldType/$clientLabel ]" ; then 
            echo "Invalid Entry... \"/gsacache/release/$clientRelease/$bldType/$clientLabel\" does not exist"
        else
          validInput='TRUE'
        fi
      fi
    done
    # get and validate release for test
    validInput='FALSE'
    until [ $validInput == 'TRUE' ]
    do
      echo ""
      echo "  Test Releases for $testGroup  "
      echo "=================================="
      echo "[a] IMADev"  
      echo "[b] IMA200-IFIX"  
      echo "[c] IMA15a-IoT-IFIX" 
      echo "[d] IMA15a-Proxy-IFIX" 
      echo "[e] IMA500-IFIX"  
      echo "[f] IMA15aX"  
      echo "[g] IMAProd"  
      echo "[h] IMA-MG500-IFIX"  
      echo "[i] MGPROD"  
      echo "=================================="
      echo -n "Enter your choice [a-i]: "
      read releaseChoice
      case $releaseChoice in
        a) testRelease='IMADev'            ; validInput='TRUE' ;;
        b) testRelease='IMA200-IFIX'       ; validInput='TRUE' ;;
        c) testRelease='IMA15a-IoT-IFIX'   ; validInput='TRUE' ;;
        d) testRelease='IMA15a-Proxy-IFIX' ; validInput='TRUE' ;;
        e) testRelease='IMA500-IFIX'       ; validInput='TRUE' ;;
        f) testRelease='IMA15aX'           ; validInput='TRUE' ;;
        g) testRelease='IMA15Prod'         ; validInput='TRUE' ;;
        h) testRelease='IMA-MG500-IFIX'    ; validInput='TRUE' ;;
        i) testRelease='MGPROD'            ; validInput='TRUE' ;;
          *) echo "Invalid Choice... \"$releaseChoice\" is not in valid choice, please select a-i";
      esac
    done
    # get and validate Test build level
    validInput='FALSE'
    until [ $validInput == 'TRUE' ]
    do
      echo ""
      echo "Release = $testRelease"
      echo ""
      echo "Available Test Label:"
      echo "-----------------------"
      ls /gsacache/release/$testRelease/$bldType | sort | grep "201.....-...." | cut -b -13
      echo -n "Please enter Test Label from the above list:"
      read testLabel
      if [ ${#testLabel} -lt 1 ] ; then
        echo "Invalid Entry... please try again"
      else
        if [ ! -e /gsacache/release/$testRelease/$bldType/$testLabel ] ; then
          echo "Invalid Entry... \"/gsacache/release/$testRelease/$bldType/$testLabel\" does not exist"
        else
          validInput='TRUE'
        fi
      fi
    done
  fi
fi

source /staf/STAFEnv.sh STAF
clear
echo ""
echo ""
echo "A STAF Event Notification will be generated for:"
echo ""
echo "  Build Type     = $bldType"
echo "  Test Group     = $testGroup"
if [[ "$serverRelease" =~ "PROXY" ]] ; then
  echo "  Proxy Release  = $serverRelease @ $serverLabel"
else
  if [ $testGroup == 'fvt_migrate' ] ; then
    echo "  Migrate From   = $serverRelease @ $serverLabel"
    echo "  Migrate To     = $migrateRelease @ $migrateLabel"
  else
    echo "  Server Release = $serverRelease @ $serverLabel"
  fi
  echo "  Client Release = $clientRelease @ $clientLabel"
  echo "  Test Release   = $testRelease @ $testLabel"
fi
echo ""
echo ""
echo "STAF Event Notification was:"
echo ""
if [ $testGroup == 'fvt_migrate' ] ; then
  echo "STAF local event generate type build subtype ism property release=$serverRelease property migrateRelease=$migrateRelease property bldType=$bldType property serverLabel=$serverLabel property migrateLabel=$migrateLabel property clientRelease=$clientRelease property clientLabel=$clientLabel property testRelease=$testRelease property testLabel=$testLabel property testGroup=$testGroup property eventType=manual"  
  STAF local event generate type build subtype ism property release=$serverRelease property migrateRelease=$migrateRelease property bldType=$bldType property serverLabel=$serverLabel property migrateLabel=$migrateLabel property clientRelease=$clientRelease property clientLabel=$clientLabel property testRelease=$testRelease property testLabel=$testLabel property testGroup=$testGroup property eventType=manual property userID=bmatteso@us.ibm.com 
else
    if [ $testRelease == 'MGPROD' ] ; then
        if [ $testGroup == 'bvt' ]; then
            echo "STAF local event generate type build subtype ism property release=$serverRelease property migrateRelease=$migrateRelease property bldType=$bldType property serverLabel=$serverLabel property migrateLabel=$migrateLabel property clientRelease=$clientRelease property clientLabel=$clientLabel property testRelease=$testRelease property testLabel=$testLabel property testGroup=$testGroup property eventType=manual property buildEngine=travis property userID=bmatteso@us.ibm.com property gitRepo=NA property gitBranch=NA property gitRelease=wiotp/messagegateway"  
            STAF local event generate type build subtype ism property release=$serverRelease property migrateRelease=$migrateRelease property bldType=$bldType property serverLabel=$serverLabel property migrateLabel=$migrateLabel property clientRelease=$clientRelease property clientLabel=$clientLabel property testRelease=$testRelease property testLabel=$testLabel property testGroup=$testGroup property eventType=manual  property buildEngine=travis property userID=bmatteso@us.ibm.com property gitRepo=NA property gitBranch=NA property gitRelease=wiotp/messagegateway 
        elif [ $testGroup == 'fvt_prod' ]; then
            echo "STAF local event generate type build subtype ism property release=$serverRelease property migrateRelease=$migrateRelease property bldType=$bldType property serverLabel=$serverLabel property migrateLabel=$migrateLabel property clientRelease=$clientRelease property clientLabel=$clientLabel property testRelease=$testRelease property testLabel=$testLabel property testGroup=$testGroup property eventType=manual  property buildEngine=travis property userID=bmatteso@us.ibm.com property gitRepo=NA property gitBranch=NA property gitRelease=wiotp/messagegateway"  
            STAF local event generate type build subtype ism property release=$serverRelease property migrateRelease=$migrateRelease property bldType=$bldType property serverLabel=$serverLabel property migrateLabel=$migrateLabel property clientRelease=$clientRelease property clientLabel=$clientLabel property testRelease=$testRelease property testLabel=$testLabel property testGroup=$testGroup property eventType=manual property buildEngine=travis property userID=bmatteso@us.ibm.com property gitRepo=messagegateway property gitBranch=master property gitRelease=wiotp/messagegateway 
        else
            echo "Incorrect testGroup value: $testGroup"
        fi
    else
        echo "STAF local event generate type build subtype ism property release=$serverRelease property bldType=$bldType property serverLabel=$serverLabel property clientRelease=$clientRelease property clientLabel=$clientLabel property testRelease=$testRelease property testLabel=$testLabel property testGroup=$testGroup property eventType=manual property userID=bmatteso@us.ibm.com"  
        STAF local event generate type build subtype ism property release=$serverRelease property bldType=$bldType property serverLabel=$serverLabel property clientRelease=$clientRelease property clientLabel=$clientLabel property testRelease=$testRelease property testLabel=$testLabel property testGroup=$testGroup property eventType=manual property userID=bmatteso@us.ibm.com 
    fi
fi
exit 0
