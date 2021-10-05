#!/bin/bash
#-----------------------------------------------
# This test is for 28K producer to 1 consumer fan in pattern with qos2 messages
# Message ordering verification can be done with a post processing command described below.
#-----------------------------------------------

#-----------------------------------------------
# The interesting thing about this test is that I was using a mounted ram disk
# I was able to achieve 1000 msgs/sec with no wait for completion and 2800 publishers.
# 
# The mounted ram disk speeded up the logging considerably
#
#  mount -o size=16G -t tmpfs none /mnt/logs
#
# The post processing on the logs in the mounted ram disk was also very quick.
#
#-----------------------------------------------
# To verify the order of messages:
#-----------------------------------------------
#
# 1. cd /mnt/logs
#
# Generate list of all unique client_id s that were used to publish to the same topic
# 2. client_id_list=`cat log.s.28x1  | grep -oE 'ORDER.*' | awk -F ':' '{print $2}' | sort -u` 
#
# Check the log.s.28x1 output file for each client id and make sure that 10 messages were received in order. Print OK if true, else print ERROR. filter out all the OKs and only watch screen for  . dots to indicate progress and confirm that no ERROR message is printed. If ERROR printed investiagate log.s.28x1 for that client id to figure out why.
# First created a modified file with only the data we are looking at, this will make things run faster.
# 3a.  grep -oE 'ORDER.*' log.s.28x1 | awk -F ':' '{printf("%s:%s:%s:\n", $1,$2,$3);}' > log.s.modified 
# Then check for the results: Note this command below is hardcoded to look for the in order pattern of 12345678910, if you change the number of messages then the pattern must change.
# 3. echo "$client_id_list" | xargs -i echo " echo -n \"checking {} \" ; dat=\$(grep -oE \":{}:.*\" log.s.modified | awk -F : '{printf(\"%s\",\$3);}') ; echo -n \$dat ; if [ \"\$dat\" != \"12345678910\" ]; then echo \" ERROR\"; else echo \" OK \" ; fi ; echo -n "." >> /dev/stderr  "  |sh   | grep -v OK
#-----------------------------------------------


. ./svt_test.sh

#./SVT.cleanup.sh

rm -rf /mnt/logs/log.s.*
rm -rf /mnt/logs/log.p.*

echo "`date` start subscriber"

#28x1 
mqttsample_array -o /mnt/logs/log.s.28x1 -t /28x1/# -a subscribe -n 280000 -w 0 -q 2 -i 28x1.s -x subscribeOnConnect=1 -x verifyStillActive=1800 -x userReceiveTimeout=5 -x dynamicReceiveTimeout=1 -x subscribeOnConnect=1 -x testCriteriaMsgCount=280000 -x warningWaitDynamic=1 -x verifyMsg=1 -x statusUpdateInterval=1 -x connectTimeout=60 -x reconnectWait=1 -x retryConnect=3600 -x keepAliveInterval=300 -s 10.10.17.125:16102 -x haURI=10.10.17.126:16102 -c false -x statusUpdateInterval=1 -m "A message from the MQTTv3 C client." -x orderMsg=2 -v  &

sleep 3

svt_verify_condition "All Clients Connected" 1 "/mnt/logs/log.s.*" 

svt_verify_condition "All Clients Subscribed" 1 "/mnt/logs/log.s.*" 


for v in {1..280} ; do
    echo "`date` start publisher $v"
     #mqttsample_array -x publishDelayOnConnect=1 -q 2 -o /mnt/logs/log.p.28x1.$v -a publish_wait -t /28x1/$v -z 1 -n 10 -w 1000 -x warningWaitDynamic=1 -x verifyMsg=1 -x statusUpdateInterval=1 -x connectTimeout=60 -x reconnectWait=1 -x retryConnect=3600 -x keepAliveInterval=300 -s 10.10.17.125:17772 -x haURI=10.10.17.126:17772 -c false -x statusUpdateInterval=1 -m "A message from the MQTTv3 C client." -x orderMsg=2 -z 100 -x 86WaitForCompletion=1  -x topicMultiTest=100 -v &
     mqttsample_array -x publishDelayOnConnect=1 -q 2 -o /mnt/logs/log.p.28x1.$v -a publish_wait -t /28x1/$v -z 1 -n 10 -w 1000 -x warningWaitDynamic=1 -x verifyMsg=1 -x statusUpdateInterval=1 -x connectTimeout=60 -x reconnectWait=1 -x retryConnect=3600 -x keepAliveInterval=300 -s 10.10.10.10:17772 -x haURI=10.10.10.10:17772 -c false -x statusUpdateInterval=1 -m "A message from the MQTTv3 C client." -x orderMsg=2 -z 100 -x 86WaitForCompletion=1  -x topicMultiTest=100 -v &

done

svt_verify_condition "All Clients Connected" 28 "/mnt/logs/log.p.*" 


killall -12 mqttsample_array # signal waiting publishers to continue with SIGUSR2 : TODO: change this to touching a file.



#-x verifyPubComplete=./log.p.2.connections.17.1383074083.229831053.complete -x testCriteriaMsgCount=50000 -x warningWaitDynamic=1 -x verifyMsg=1 -x statusUpdateInterval=1 -x connectTimeout=60 -x reconnectWait=1 -x retryConnect=3600 -x keepAliveInterval=300 -s 10.10.17.125:17772 -x haURI=10.10.17.126:17772 -c false -x statusUpdateInterval=1 -m A message from the MQTTv3 C client.

#head log.s.2.connections.17.1.1383074083.229831053 
#2013-09-29 14:14:43.343692 CMDLINE: mqttsample_array -o log.s.2.connections.17.1.1383074083.229831053 -t /17/mar473/T2_. -a subscribe -n 50000 -w 0 -q 2 -i 17.1.mar473 -x subscribeOnConnect=1 -x verifyStillActive=1800 -x userReceiveTimeout=5 -x dynamicReceiveTimeout=1 -x subscribeOnConnect=1 -x verifyPubComplete=./log.p.2.connections.17.1383074083.229831053.complete -x testCriteriaMsgCount=5000000 -x warningWaitDynamic=1 -x verifyMsg=1 -x statusUpdateInterval=1 -x connectTimeout=60 -x reconnectWait=1 -x retryConnect=3600 -x keepAliveInterval=300 -s 10.10.17.125:17772 -x haURI=10.10.17.126:17772 -c false -x statusUpdateInterval=1 -m A message from the MQTTv3 C client. 
#2013-09-29 14:14:43.346050 0: Using Primary HA Client @ 10.10.17.125:17772
#2013-09-29 14:14:43.347232 0: MQTTClient_connect rc: -1 seconds: 0.001161 connectionCounter: 0 connectionFailure: 0
#
##root@mar473 /niagara/test/svt_cmqtt> head log.p.2.connections.17.1.1383074083.229831053 
#2013-09-29 14:15:21.058004 CMDLINE: mqttsample_array -x publishDelayOnConnect=1 -q 2 -o log.p.2.connections.17.1.1383074083.229831053 -a publish_wait -t /17/mar473/T2_. -z 1 -n 1000000 -w 1000 -x warningWaitDynamic=1 -x verifyMsg=1 -x statusUpdateInterval=1 -x connectTimeout=60 -x reconnectWait=1 -x retryConnect=3600 -x keepAliveInterval=300 -s 10.10.17.125:17772 -x haURI=10.10.17.126:17772 -c false -x statusUpdateInterval=1 -m A message from the MQTTv3 C client. 
##
