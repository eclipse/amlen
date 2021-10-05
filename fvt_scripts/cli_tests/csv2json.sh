#!/bin/bash
#----------------------------------------------------------------------------
# Print Usage
#----------------------------------------------------------------------------
printUsage(){

echo "Usage: run-cli.sh -c <CSV_INPUT_FILE> [-j <JSON_OUTPUT_FILEr>]"
echo "  CVS_INPUT_FILE:   The name of the csv input file."
echo "  JSON_OUTPUT_FILE: The name of the json output file."
}

#----------------------------------------------------------------------------
# Main script starts here
#----------------------------------------------------------------------------
# Check input
if [[ ($# -lt 4) || ($# -gt 8) ]] ; then
	printUsage
	exit 1
fi

#----------------------------------------------------------------------------
# Parse Options
#----------------------------------------------------------------------------
APPLIANCE=1
SECTION="ALL"
echo "Entering $0 with $# arguments: $@"

while getopts "c:j:" option ${OPTIONS}
  do
    #echo "option=${option}  OPTARG=${OPTARG}"
    case ${option} in
    c ) CSV_INPUT_FILE=${OPTARG}
        ;;
    j ) JSON_OUTPUT_FILE=${OPTARG}
        ;;
    esac	
  done


    # Load input file into an array
    # (Do a file read did not work with the ssh statement inside the while loop.)
    saveIFS=$IFS
    IFS=$'\n'
    filecontent=( `cat "${CSV_INPUT_FILE}"` )

    # Check to see if the csv input file read failed
    if [ $? != 0 ]
    then
        echolog "Failure: Problem reading input file: ${CLI_INPUT_FILE}"
        echo "Compare files in directory: " `ls -al`
        exit 1
    fi
    # We must restore IFS because bash uses it to do its parsing!
    IFS=$saveIFS

    beginTime=$(date)

    foundSectionCount=0

    Names=${filecontent[0]}
    echo ${Names}
    Names=( ${Names//,/ } )

    json_object="{ \"Version\":\"v1\", \"Topic\": ["
    # Step through the array and call the apropriate function based on the first string in cli_action 
    for line in "${filecontent[@]:1}"
    do
        echo ${line}
        line=( ${line//,/ } )
        i=0
        json_item="{"

        for value in "${line[@]}"
        do
           #echo $i:$value
           if [[ "${Names[$i]}" != "\"ResetTime\"" && "${Names[$i]}" != "\"LastConnectedTime\"" ]] ; then          
               # if numeric do not put quotes around value
               if [[ ${value} =~ ^[0-9]+$ ]] ; then
                   json_item="${json_item}${Names[$i]}:$value,"
               else
                   json_item="${json_item}${Names[$i]}:\"$value\","
               fi
           fi
           ((i+=1))
        done

        # remove trailing ',' and add '}'
        json_item=${json_item:0:${#json_item}-1}
        json_item="${json_item}}"
        #echo ${json_item}
 
        json_object="${json_object}${json_item},"
    done

    # remove trailing ',' and add "] }"
    json_object=${json_object:0:${#json_object}-1}
    json_object="${json_object}] }"
    echo ${json_object}

    echo ${json_object} > $JSON_OUTPUT_FILE
    
