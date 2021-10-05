#!/bin/bash
#ima1=10.10.1.39:16102
ima2=10.10.1.157:16102
num_clients=${1-10};
num_X_per_client=${2-10};
num_0_messages=300;
num_1_messages=300;
num_2_messages=30;
now=`date +%s`
#qos_under_test="0 1 2"
qos_under_test="2"

echo "---------------------------------"
echo "Sigint any other mqttsample_array processes that are running"
echo "---------------------------------"
killall -2 mqttsample_array

sleep 1

echo "---------------------------------"
echo "Kill any other mqttsample_array processes that are running"
echo "---------------------------------"
killall -9 mqttsample_array

rm -rf log.s.*
rm -rf log.p.*

get_flags(){
    local qos=$1
    local action=$2

    local flags=" -s ${ima2} -x keepAliveInterval=50000 -x connectTimeout=50000 -x reconnectWait=1 -x retryConnect=100 -q ${qos} -t ${qos}_${now} -x verifyMsg=1 -x orderMsg=1 -x msgFile=./SMALLFILE -x statusUpdateInterval=1 -x subscribeOnConnect=1 -x publishDelayOnConnect=120 -u u -p imasvtest -x noDisconnect=1 " 

    if [ "$action" == "SUBSCRIBE" ] ;then
        flags+=" -z $num_X_per_client "
    elif [ "$action" == "PUBLISH" ] ;then
        echo "no additional flags for publish" >> /dev/stderr
    else
        echo "---------------------------------" >> /dev/stderr
        echo " ERROR: - internal bug in script unsupported var : $var"  >> /dev/stderr
        echo "---------------------------------" >> /dev/stderr
        exit 1;
    fi
    

    if [ "$qos" == "0" ] ; then
        flags+=" -n ${num_0_messages} -v "
    elif [ "$qos" == "1" ] ; then
        flags+=" -n ${num_1_messages} -c false -v "
    elif [ "$qos" == "2" ] ; then
        flags+=" -n ${num_2_messages} -c false -v "
    else
        echo "---------------------------------" >> /dev/stderr
        echo " ERROR: - internal bug in script unsupported var : $var"  >> /dev/stderr
        echo "---------------------------------" >> /dev/stderr
    fi
    echo $flags
}

for qos in $qos_under_test ; do 
    rm -rf log.s.${qos}.*
    let i=0;
    while (($i<$num_clients)); do
        let ui=($i*$num_X_per_client);
        echo "MQTT_C_CLIENT_TRACE=ON ./mqttsample_array -x userIncrementer=$ui -o log.s.${qos}.${i}.${now} -r log.s.${qos}.${i}.${now}.persist -a subscribe -i ${qos}_${i}_${now}_S `get_flags $qos SUBSCRIBE` &" |sh -x > log.s.${qos}.${i}.${now}.mqtt 
        let i=$i+1;
    done
done

for qos in $qos_under_test ; do 
    echo "---------------------------------"
    echo "Wait for all qos $qos clients to subscribe" 
    echo "---------------------------------"
    while((1)) ; do
        echo -n "."
        echo "grep \"All Clients Subscribed\" log.s.${qos}.*${now} 2>/dev/null | wc -l"
        count=`grep "All Clients Subscribed" log.s.${qos}.*${now} 2>/dev/null | wc -l `
        if (($count==$num_clients)); then
            echo "count is $count"
            break;   
        else
        echo count $count is not == to $num_clients
        fi
        sleep 1;
    done
done

echo "---------------------------------"
echo "Complete. All clients subscribed . Start publisher" 
echo "---------------------------------"

for qos in $qos_under_test ; do 
    rm -rf log.p.${qos}.*
    let i=0;
    echo "MQTT_C_CLIENT_TRACE=ON ./mqttsample_array -o log.p.${qos}.${now} -x userIncrementer=0 -a publish -r log.p.${qos}.${now}.persist -i ${qos}_${now}_P `get_flags $qos PUBLISH` &" |sh -x> log.p.${qos}.${now}.mqtt


done

echo "---------------------------------"
echo "Complete. All publishers started"
echo "---------------------------------"

for qos in $qos_under_test ; do 
    for var in publishers subscribers; do 
        echo "---------------------------------"
        echo "Wait for all qos $qos $var to complete" 
        echo "---------------------------------"
        while((1)) ; do
            echo -n "."
            if [ "$var" == "publishers" ] ;then
                count=`grep "SVT_TEST_RESULT: SUCCESS " log.p.${qos}.*${now} 2>/dev/null | wc -l `
                total=1;
            elif [ "$var" == "subscribers" ] ;then
                count=`grep "SVT_TEST_RESULT: SUCCESS " log.s.${qos}.*${now} 2>/dev/null | wc -l `
                total=$num_clients
            else
                echo "---------------------------------"
                echo " ERROR: - internal bug in script unsupported var : $var" 
                echo "---------------------------------"
            fi
            
            if (($count==$total)); then
                echo "Complete - count is $count"
                break;
            fi
            sleep 1;
        done
    done
done

echo "---------------------------------"
echo "Cleanup - set clean to false and number of messages to zero to delete all the durable subscribers"
echo "TODO - monitor status for cleanup"
echo "---------------------------------"
for qos in $qos_under_test ; do 
    while (($i<$num_clients)); do
        echo "./mqttsample_array -a subscribe -i ${qos}_${i}_${now}_S `get_flags $qos SUBSCRIBE` -n 0 -c true &" |sh -x > /dev/null 2>/dev/null
        let i=$i+1;
    done
    echo "./mqttsample_array -a publish -i ${qos}_${now}_P `get_flags $qos PUBLISH` -n 0 -c true &" |sh -x > /dev/null 2>/dev/null
done

echo "---------------------------------"
echo "Done. All actions complete"
echo "---------------------------------"
    
exit 0;
