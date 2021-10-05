#!/bin/bash

PERCENT_MESSAGES=99

for x in {1..100}; do
	echo "`date` send 1 message ($x) to all clients and wait 60 seconds"
	for n in {2..18}; do ./mqttsample_array -a publish -s 10.10.1.40:16106  -x haURI=10.10.1.39:16106 -t $n  -o log.p.$n &  done
	echo "`date` start new message pass in 60 seconds"

	sleep 60;
done
