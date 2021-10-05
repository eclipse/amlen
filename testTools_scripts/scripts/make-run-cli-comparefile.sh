#!/bin/bash

#----------------------------------------------------------------------------
# A simple script to help generate run-cli comparetest files
#----------------------------------------------------------------------------


#----------------------------------------------------------------------------
# Print Usage
#----------------------------------------------------------------------------
printUsage(){

echo "Usage: run-cli.sh -f <CLI_LOG_FILE> -c <CLI_COMPARE_FILE>"
echo "CLI_LOG_FILE:   The name of the log file used to generate the comparetest file"
echo "CLI_COMPARE_FILE: The name of the comparetest file we are generating"
}

#----------------------------------------------------------------------------
# Main script starts here
#----------------------------------------------------------------------------
# Check input
if [[ ($# -lt 4) || ($# -gt 4) ]] ; then
	printUsage
	exit 1
fi

#----------------------------------------------------------------------------
# Parse Options
#----------------------------------------------------------------------------
APPLIANCE=1
SECTION="ALL"
while getopts "f:c:" option ${OPTIONS}
  do
    case ${option} in
    f )
        CLI_LOG_FILE=${OPTARG}
        ;;
    c ) CLI_COMPARE_FILE=${OPTARG}
        ;;

    esac	
  done

echo "Entering $0 with $# arguments: $@" 

# Load input file into an array
# (Do a file read did not work with the ssh statement inside the while loop.)
saveIFS=$IFS
IFS=$'\n'
filecontent=( `cat "${CLI_LOG_FILE}"` )

# Check to see if the file read failed
if [ $? != 0 ]
then
	echo "Failure: Problem reading input file: ${CLI_LOG_FILE}" | tee -a ${CLI_COMPARE_FILE}
	echo "Compare files in directory: " `ls -al`
	exit 1
fi
# We must restore IFS because bash uses it to do its parsing!
IFS=$saveIFS

is_command_line=0
count=1

# Step through the array and call the apropriate function based on the first string (cli_action) 
for line in "${filecontent[@]}"
do
    if [[ "${is_command_line}" == "1" ]] ; then
        # We must first escape the quotes and square brackets
        line=${line//\"/\\\\\"}
        line=${line//\[/\\\\\[}
        line=${line//\]/\\\\\]}
        line=${line//\*/\\\\\*}

        # Can we put change substitutions back to env variables?

        echo "${count}:1:${CLI_LOG_FILE}:${line}" | tee -a ${CLI_COMPARE_FILE}
        echo "" | tee -a ${CLI_COMPARE_FILE}

        ((count+=1))
        is_command_line=0   

    # find line with CLI command.  The next line is the message being returned
    elif [[ "${line:0:3}" == "ssh" ]] ; then
        is_command_line=1
        echo "# ${line}" | tee -a ${CLI_COMPARE_FILE}
    fi
    
done

