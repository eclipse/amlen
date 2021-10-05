#!/bin/bash
# ps -eaf |grep java |gawk {'print $2'}  > /tmp/killpid
# while read LINE; do echo $LINE; kill -9 $LINE; done < /tmp/killpid
# $1 is server or list of HA
set +x
echo "Using servers $1";

#lets parse server string
OLDIFS=$IFS
IFS=,
servers=($1)
IFS=$OLDIFS
echo "Found serverA= ${servers[0]}"
echo "Found serverB= ${servers[1]}"

if [ "${servers[1]}" == "" ]; then
	echo "Running standalone tests with server ${servers[0]}"
else
	echo "Running HA tests with server pair ${servers[0]} and ${servers[1]}"
	HA=1
fi

TESTS=`ls -ld General* | gawk {'print $9'}`

#echo "Found these tests $TESTS"

EXCLUDETESTS="GeneralFailoverRegression"
for EXCLUDE in $EXCLUDETESTS
do
	TESTS=`echo $TESTS |sed "s/\b$EXCLUDE\b//g"`
done

#echo "Running these tests $TESTS"

for TEST in $TESTS 
do
	echo "starting $TEST testcase"
	cd $TEST/
	echo "cleaning files.."

	CLEANF=`find . -maxdepth 1 -type f |grep -v -e ".sh"`

	for FILE in $CLEANF
	do
		echo "Deleting $FILE"
		rm $FILE
	done

	echo "cleaning directories.."

	CLEAND=`find . -maxdepth 1 -type d |grep -vxG [\.\n]`
	for DIR in $CLEAND
	do
		echo "Deleting $DIR"
		rm -Rf $DIR
	done

	echo "invoking script"
	SCRIPTS=`find . -maxdepth 1 -type f -name "test*sh"`
	SCRIPT=($SCRIPTS)
	echo "found script 1 ${SCRIPT[0]} script 2 ${SCRIPT[1]}"
	# there should only be 2 scripts
	# check if HA and if string in script array 'contains' _HA.sh 
	if [ $HA == 1 ]; then
		if [[ "${SCRIPT[0]}" == *"_HA.sh" ]];
		then
			echo "Running HA script ${SCRIPT[0]}"
			./${SCRIPT[0]} $1
		else
			echo "Running HA script ${SCRIPT[1]}"
			./${SCRIPT[1]} $1
		fi
	else
		if [[ "${SCRIPT[0]}" != *"_HA.sh" ]]; then
			echo "Running standalone script ${SCRIPT[0]}"
			./${SCRIPT[0]} $1
		else
			echo "Running standalone script ${SCRIPT[1]}"
			./${SCRIPT[1]} $1
		fi
	fi
	cd ..
done
