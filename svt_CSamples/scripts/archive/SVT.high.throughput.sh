. ./svt_test.sh

do_tp_test(){
    local action=${1-"SUB"} # SUB , PUB
    local nummsg=${2-1000000}
    local qos=${3-0}
    local pubrate=${4-0}
    local clientcnt=${5-64}
    local x
    local now=`date +%s`
    local extra_pubflag=" "
    local name="47726"
    local h=`hostname -i`
    local baseflag=""
    local subflag=""
    local pubflag=""
    local launched=0

    baseflag=" -s 10.10.10.10:16102 -n $nummsg -x haURI=10.10.10.10:16102 " 
    baseflag+=" -x reconnectWait=2 -x retryConnect=360 -x verifyMsg=1 -x orderMsg=2 "
    baseflag+=" -q $qos -x keepAliveInterval=10  -x connectTimeout=10 "
    baseflag+=" -x sharedMemoryCoupling=1 "

    if [ "$nummsg" == "0" -a "$qos" == "1" -o "$qos" == "2" ] ;then
        baseflag+=" -c true "
    else
        baseflag+=" -c false "
    fi
    
    let x=0
    while(($x<$clientcnt)); do
        if [ "$action" == "SUB" ] ;then
            subflag=" -o log.s.${qos}.${name}.${x}.${now} -a subscribe -t tp.$x.$h "
            subflag+=" -x subscribeOnConnect=1 $baseflag "
            subflag+="  -i tp.s.$x.$h -x sharedMemoryKey=loop.$x -x statusUpdateInterval=1 " 
            mqttsample_array $subflag -m "Hola amigo"  &
            let launched=$launched+1
            
        elif [ "$action" == "PUB" ] ;then
            pubflag=" -o log.p.${qos}.${name}.${x}.${now} -a publish -t tp.$x.$h "
            pubflag+="  -w $pubrate -x publishDelayOnConnect=30 $baseflag "
            pubflag+="  -i tp.p.$x.$h -x sharedMemoryKey=loop.$x -x statusUpdateInterval=1 " 
            pubflag+=" -x sharedMemoryVal=1000  "
            mqttsample_array $pubflag -m "Hola amigo"  &
            let launched=$launched+1
        else
            echo "ERROR: unknown action $action "  >> /dev/stderr
            return 1;
        fi
        let x=$x+1
        #break;
    done

    svt_wait_60_for_processes_to_become_active ${now} ${qos} mqttsample_array $launched

    if [ "$action" == "SUB" ] ;then
        svt_verify_condition "Connected to " $launched "log.s.${qos}.${name}.*.${now}"
        svt_verify_condition "Subscribed - Ready" $launched "log.s.${qos}.${name}.*.${now}" 
        svt_verify_condition "All Clients Subscribed" $launched "log.s.${qos}.${name}.*.${now}" 
    elif [ "$action" == "PUB" ] ;then
        svt_verify_condition "All Clients Connected" $launched "log.p.${qos}.${name}.*.${now}" 
        svt_verify_condition "TEST_RESULT: SUCCESS" $launched "log.p.${qos}.${name}.*.${now}"
    fi
    
    return 0;
} 

