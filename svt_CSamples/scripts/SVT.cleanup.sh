#!/bin/bash -x

echo "-----------------------------------------"
echo "Send sigstop (19) to all SVT processes"
echo "-----------------------------------------"
ps -ef |grep SVT | grep -v grep | grep -v cleanup  |awk '{print $2}' |xargs -i echo "kill -19 {}" |sh -x

echo "-----------------------------------------"
echo "Send sigint (2) to all mqttsample_array processes and give them some time to clean up"
echo "-----------------------------------------"
killall -2 mqttsample_array

starttime=`date +%s`
endtime=($starttime+180)

if [ "$1" == "force" ] ;then
    echo "-----------------------------------------"
    echo "$datacount: force flag present . Send sigkill (9) to all mqttsample_array processes"
    echo "-----------------------------------------"
    sleep 5
    killall -9 mqttsample_array

    echo "-----------------------------------------"
    echo "$datacount: force flag present . Send sigkill (9) to all SVT processes"
    echo "-----------------------------------------"

    ps -ef |grep SVT | grep -v grep | grep -v force | awk '{print $2}' |xargs -i echo "kill -9 {}" |sh -x

    echo "-----------------------------------------"
    echo "Completed all expected processing"
    echo "-----------------------------------------"

    ps -ef | grep SVT | grep -v grep | awk '{print $2}' |xargs -i echo "kill -9 {}" |sh -x
    
else

    let x=0;
    while((1)); do
        data=`ps -ef | grep mqttsample_array | grep -v grep`
        datacount=`echo "$data" |wc -l`
        if (($datacount>1)); then
	    curtime=`date +%s`
            if (($curtime>$endtime)); then
                echo "-----------------------------------------"
                echo "$datacount: curtime: $curtime > $endtime : endtime Send sigkill (9) to all mqttsample_array processes"
                echo "-----------------------------------------"
                killall -9 mqttsample_array
            else
                echo "$datacount: mqttsample_array is still running sleep 5. do kill -9 in 180 seconds "
            fi
            sleep 1; 
        else
            echo "$datacount: mqttsample_array no longer running"
            break;
        fi
        let x++;
    done

    echo "-----------------------------------------"
    echo "Send sigkill (9) to all SVT processes"
    echo "-----------------------------------------"
    ps -ef |grep SVT | grep -v grep | grep -v cleanup  |awk '{print $2}' |xargs -i echo "kill -9 {}" |sh -x

fi


echo "-----------------------------------------"
echo "Completed all expected processing"
echo "-----------------------------------------"

exit 0;



