#!/bin/bash
# ps -eaf |grep java |gawk {'print $2'}  > /tmp/killpid
# while read LINE; do echo $LINE; kill -9 $LINE; done < /tmp/killpid
# $1 is server or list of HA
set +x
echo "Using servers $1";

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
	echo "Looking at $TEST testcase"
	cd $TEST/
	echo "Parsing"
	grep -H "" test*.out
	cd ..
done
