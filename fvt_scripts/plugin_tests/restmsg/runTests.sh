#!/bin/bash
#------------------------------------------------------------------------------
# Script for running Mocha test cases for the REST Message Plugin
# Takes 1 argument, a log file name for output from this script.
# All mocha output is put into MOCHALOG - plugin_tests/restmsg.log
#------------------------------------------------------------------------------

LOG=runTests.sh.log
MOCHALOG=${M1_TESTROOT}/plugin_tests/restmsg.log

while getopts "f:" opt; do
    case ${opt} in
    f )
        LOG=${OPTARG}
        ;;
    * )
        exit 1
        ;;
    esac
done

echo -e "Entering $0 with $# arguments: $@\n" | tee -a ${LOG}

cd ${M1_TESTROOT}/plugin_tests/restmsg

rc=1
`mocha test 1>${MOCHALOG} 2>&1`
rc=$?

cd ${M1_TESTROOT}/plugin_tests

echo "RC: ${rc}" | tee -a ${LOG}

if [ ${rc} -ne 0 ] ; then
    echo "Test result is Failure!" | tee -a ${LOG}
else
    echo "Test result is Success!" | tee -a ${LOG}
fi

exit 0
