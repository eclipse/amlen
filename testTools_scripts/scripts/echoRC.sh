#! /bin/bash
#set -x 

echo "Entering $0 with $# arguments: $@"

# If no options are given, print the usage statement and exit
if [[ $# -eq 0 ]]
then
	echo "Usage: $0 <executable> [options]"
	exit 1
else

#   $1 $2 $3 $4 $5 $6 $7 $8 $9 $10
   $@
   echo "RC=$?"

fi



