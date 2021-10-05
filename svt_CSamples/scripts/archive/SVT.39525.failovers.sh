#!/bin/bash

. ./ha_test.sh

rm -rf log.failover.*

let iteration=1;

while ((1)); do
echo "##################################################################"
#failover=`echo "svt_ha_test_X_disrupt_server_on_Y KILL PRIMARY
#svt_ha_test_X_disrupt_server_on_Y STOP PRIMARY
failover=`echo " svt_ha_test_X_disrupt_server_on_Y DEVICE PRIMARY" | sort -R | head -1 `

echo "`date` : iteration:$iteration Starting  a failover :$failover."

svt_ha_test_print_current_status 1>> log.failover.$iteration 2>>log.failover.$iteration
$failover 1>> log.failover.$iteration 2>>log.failover.$iteration
echo "`date` : iteration:$iteration Done with failover :$failover."
svt_ha_test_print_current_status 1>> log.failover.$iteration 2>>log.failover.$iteration

if exist_server_core ; then
    echo "`date` : iteration:$iteration stopping due to core file found"
    break;
fi
echo "`date` : iteration:$iteration Completed failover test. "
echo "`date` : iteration:$iteration Syncrhonize server time."
synchronize_server_time
echo "`date` : iteration:$iteration Sleep 120 before startin next test"
sleep 120
echo "`date`  : iteration:$iteration Done sleeping."

#echo "`date` : iteration:$iteration Wait for MDB to finish this iteration"
#while((1)); do
    #if [ -e log.archive.mdbmessages.${iteration}.* ] ;then
        #echo "found that log.archive.mdbmessages.${iteration}.* exists"
        #break;
    #fi
#done
echo "`date`  : iteration:$iteration Done."


let iteration=$iteration+1;

done

# story 30999 not supported : svt_ha_test_X_disrupt_server_on_Y SIGSEGV PRIMARY
