#!/bin/bash

. ./svt_test.sh

ima1=${1-""}
ima2=${2-""}
operation=${3-""}
counter=${4-0}
rc=0;

if ! [ -n "$ima1" ] ;then
    echo "ERROR: $0 : $LINENO - no ima1" >> /dev/stderr ; exit 1;   
elif ! [ -n "$ima2" ] ;then
    echo "ERROR: $0 : $LINENO - no ima2" >> /dev/stderr ; exit 1;   
elif ! [ -n "$operation" ] ;then
    echo "ERROR: $0 : $LINENO - no operation" >> /dev/stderr ; exit 1;
fi

RANDOM_TEST_FILE=.tmp.svt_random_ha.test

. ./ha_test.sh $ima1 $ima2

if [ "$operation" == "svt_ha_gen_random_test" ] ;then
    svt_ha_generate_random_test $counter
elif [ "$operation" == "svt_ha_get_random_test" ] ;then
    svt_ha_get_random_test $counter
elif [ "$operation" == "svt_ha_run_random_test" ] ;then
    svt_ha_run_random_test $counter
    let rc=$?
else
    echo "ERROR: $0 : $LINENO - unknown operation: $operation"    >> /dev/stderr 
    exit 1;
fi

exit $rc;
