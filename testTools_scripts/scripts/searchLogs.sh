#! /bin/bash

#----------------------------------------------------------------------------
# searchLogs.sh: Script for verifying that messages appear or do not appear
# 		 in a log file. Should only be used with compare files of the
#		 correct format. TODO: Put in name of example file
#----------------------------------------------------------------------------

#----------------------------------------------------------------------------
# Print Usage
#----------------------------------------------------------------------------
printUsage(){

echo "Usage: searchLogs.sh <file> [binary]"
echo "file:	The name of the file that has the messages to search for.  Messages must be in the correct format"
echo "binary: Indicates that the file to be searched in binary."
}

# Check input
if [[ ($# -lt 1) ||($# -gt 2) ]]; then
	printUsage
	exit 1
elif [[ $# -eq 2 ]] ; then
   if [[ $2 != "binary" ]]; then
	printUsage
	exit 1
   fi
fi

COMPAREFILE=$1

let num_fails=0

while read line
do
	let LESS_THAN=0
	let GREATER_THAN=0
	# Take out any comments
	line=`echo "${line}" | sed 's/^[#].*//'`
	
	if [ "${line}" != "" ]
	then
		# Parse the line
		MESSAGE_ID=`echo "${line}" | cut -d ':' -f 1`
		NUM_EXPECTED=`echo "${line}" | cut -d ':' -f 2`
		LOGFILE=`echo "${line}" | cut -d ':' -f 3`
		MESSAGE=`echo "${line}" | cut -d ':' -f 4-`
	
		SIGN=`echo "${NUM_EXPECTED}" | cut -b 1`
		if [ "${SIGN}" == "<" ] ; then
			LESS_THAN=1
			NEW_NUM_EXPECTED=`echo "${NUM_EXPECTED}" | cut -c 2-`
		elif [ "${SIGN}" == ">" ] ; then
			GREATER_THAN=1
			NEW_NUM_EXPECTED=`echo "${NUM_EXPECTED}" | cut -c 2-`		
		fi

		NUM_LINES=`cat ${LOGFILE} | wc -l`
		if [[ $2 != "binary" ]] ; then
			NUM_ACTUAL=`grep -c "${MESSAGE}" ${LOGFILE}`
		else
			NUM_ACTUAL=`strings ${LOGFILE} | grep -c "${MESSAGE}"`
		fi

		if [ -e "$LOGFILE" ]
		then
			TMP=0 # do nothing
		else
			echo "Failure: file not found: $LOGFILE"
			echo "Log files in directory: " `ls -al *.log`
			let num_fails+=1
		fi

		if [[ $2 != "binary" ]] ; then
			echo "grep -c \"${MESSAGE}\" ${LOGFILE}"
		else
			echo "strings \"${LOGFILE}\" | grep -c \"${MESSAGE}"
		fi

		if [ "${NUM_ACTUAL}" == "" ]
		then
			echo "Failure: grep command failed"
			let num_fails+=1
		elif [ ${LESS_THAN} -eq 1 ] ; then
			if [ ${NEW_NUM_EXPECTED} -lt ${NUM_ACTUAL} ] ; then
				echo "Failure: Actual number of occurrences for message ${MESSAGE_ID} is ${NUM_ACTUAL} which is more than expected ${NEW_NUM_EXPECTED} "
				let num_fails+=1
			fi
		elif [ ${GREATER_THAN} -eq 1 ] ; then
			if [ ${NEW_NUM_EXPECTED} -gt ${NUM_ACTUAL} ] ; then
				echo "Failure: Actual number of occurrences for message ${MESSAGE_ID} is ${NUM_ACTUAL} which is less than expected ${NEW_NUM_EXPECTED} "
				let num_fails+=1
			fi
		elif [ ${NUM_EXPECTED} -ne ${NUM_ACTUAL} ]
		then
			if [ ${NUM_EXPECTED} -eq -1 ] && [ ${NUM_ACTUAL} -ge 1 ]
			then
				# pass if we specify 1-or-many occurances
				let num_fails+=0
			else
				echo "Failure: Actual Number of occurrences for message ${MESSAGE_ID} is ${NUM_ACTUAL}, expected ${NUM_EXPECTED}. Total lines in file: ${NUM_LINES} "
				let num_fails+=1
			fi
		fi
	fi

done < ${COMPAREFILE}

# Check to see if the compare file read failed
if [ $? != 0 ]
then
	echo "Failure: Problem reading compare file: ${COMPAREFILE}"
	echo "Compare files in directory: " `ls -al *.compare`
	let num_fails+=1
fi

if [ ${num_fails} -gt 0 ]
then
	echo "Log Search Result is Failure. Number of failures: ${num_fails}"
else
	echo "Log Search Result is Success."
fi


