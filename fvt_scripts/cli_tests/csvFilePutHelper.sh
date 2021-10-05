#! /bin/bash
# CSV File Put Helper
#set -x


while getopts ":f:c:s:d:" option ${OPTIONS}
  do
    case ${option} in
    f) LOG_FILE=${OPTARG}
        ;;
    c) CSV_FILE=${OPTARG}
        ;;
    s) SERVER=${OPTARG}
        ;;
    d) DESTINATION=${OPTARG}
        ;;

    esac	
  done

# Get name of csv file on Appliance from run_cli.sh log file after running the following command:
#   stat Memory SubType=History Duration=100 FilePrefix=stat_memory_003
SERVER_CSV_FILE=`cat ${CSV_FILE}`
SERVER_CSV_FILE=${SERVER_CSV_FILE##*(}
SERVER_CSV_FILE=${SERVER_CSV_FILE%%)*}

# Copy file from Appliance
ssh ${SERVER} file put ${SERVER_CSV_FILE} scp://${DESTINATION}

# Delete file on Appliance
ssh ${SERVER} file delete ${SERVER_CSV_FILE}

# Log Success/Failure message so the CAppDriverLog can determine if we were successfull
if [[ "$?" == "0" ]] ; then
    echo -e "\nTest result is Success!" >> $LOG_FILE
else
    echo -e "\nTest result is Failure!" >> $LOG_FILE
fi
