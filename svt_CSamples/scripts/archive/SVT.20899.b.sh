ima1=10.10.1.39:16102
ima2=10.10.1.40:16102
num_clients=10;
num_0_messages=3000000;
num_1_messages=3000;
num_2_messages=3000;
now=`date +%s`

echo "---------------------------------"
echo "Kill any other mqttsample_array processes that are running"
echo "---------------------------------"
killall -9 mqttsample_array

get_flags(){
local qos=$1
local flags=" -s ${ima1} -x haURI=${ima2} -x keepAliveInterval=60 -x connectTimeout=10 -x reconnectWait=10 -x retryConnect=360 -q ${qos} -t ${qos}_${now} -x verifyMsg=1 -x orderMsg=1 -x msgFile=./SMALLFILE -x statusUpdateInterval=10 "
if [ "$qos" == "0" ] ; then
    flags+="-n ${num_0_messages} "
elif [ "$qos" == "1" ] ; then
    flags+="-n ${num_1_messages} -c false -v"
elif [ "$qos" == "2" ] ; then
    flags+="-n ${num_2_messages} -c false -v"
else
    echo "---------------------------------" >> /dev/stderr
    echo " ERROR: - internal bug in script unsupported var : $var"  >> /dev/stderr
    echo "---------------------------------" >> /dev/stderr
fi
echo $flags
}

for qos in 2 ; do 
    rm -rf log.s.$qos.*
    let i=0;
    while (($i<$num_clients)); do
        #echo "./mqttsample_array  -o log.s.${qos}.${i}.${now} -v -s ${ima1} -x haURI=${ima2} -a subscribe -q ${qos} -n ${num_messages} -x keepAliveInterval=60 -x connectTimeout=10  -x reconnectWait=10 -x retryConnect=360 -i ${qos}_${i}_${now}_S -c false -t ${qos}_${now} -x verifyMsg=1 -x orderMsg=1 -x msgFile=./SMALLFILE &" |sh -x
        echo "./mqttsample_array -o log.s.${qos}.${i}.${now} -a subscribe -i ${qos}_${i}_${now}_S `get_flags $qos` &" |sh -x
        let i=$i+1;
    done
done

for qos in 2 ; do 
    echo "---------------------------------"
    echo "Wait for all qos $qos clients to subscribe" 
    echo "---------------------------------"
    while((1)) ; do
        echo -n "."
        count=`grep "All Clients Subscribed" log.s.${qos}.* 2>/dev/null | wc -l `
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

for qos in 2 ; do 
    rm -rf log.p.${qos}.*
    let i=0;
    echo "./mqttsample_array -o log.p.${qos}.${now} -a publish -i ${qos}_${now}_P `get_flags $qos` &" |sh -x
done

echo "---------------------------------"
echo "Complete. All publishers started"
echo "---------------------------------"

for qos in 2 ; do 
    for var in publishers subscribers; do 
        echo "---------------------------------"
        echo "Wait for all qos $qos $var to complete" 
        echo "---------------------------------"
        while((1)) ; do
            echo -n "."
            if [ "$var" == "publishers" ] ;then
                count=`grep "SVT_TEST_RESULT: SUCCESS " log.p.${qos}.* 2>/dev/null | wc -l `
                total=1;
            elif [ "$var" == "subscribers" ] ;then
                count=`grep "SVT_TEST_RESULT: SUCCESS " log.s.${qos}.* 2>/dev/null | wc -l `
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
    

