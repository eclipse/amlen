#!/bin/bash

PERCENT_MESSAGES=99
#CLIENT_COUNT=999600
CLIENT_COUNT=249900

for x in {4..100}; do
	let expected_msgs=$x*$CLIENT_COUNT
	echo "`date` send 1 message ($x) to all $CLIENT_COUNT clients and wait for all messages to be received"
	for n in {2..18}; do ./mqttsample_array -a publish -s 10.10.1.39:16102 -t $n  -o log.p.$n &  done
	
	while((1)); do
		data=`./monitor.c.client.status.update.sh SINGLE | grep "S Total" | awk '{print $7}'`
		
		if [ "$PERCENT_MESSAGES" != "" ] ;then
			k=`echo "$data $expected_msgs" | awk '{printf("%d", ($1/$2)*100 ); }'`
			if (($k>=$PERCENT_MESSAGES)); then
				echo "`date` Received >= $PERCENT_MESSAGES percent of expected messages: $data , x:$x, continuing."
				break;
			fi
		fi

		if (($data==$expected_msgs)); then
			echo "`date` Received all expected messages: $data , x:$x"	
		else
			echo "`date` Have not yet received all expected messages: $data (expected: $expected_msgs) x:$x"	
			sleep 10;
		fi	
	done
	echo "`date` start new message pass in 10 seconds"
	sleep 10;

done
