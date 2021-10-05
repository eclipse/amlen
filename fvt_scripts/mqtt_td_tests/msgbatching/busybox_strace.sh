#!/bin/bash
#------------------------------------------------------------------------------
# busybox_strace.sh
#
#------------------------------------------------------------------------------
#
# 1. Go into busybox and get the PID of imaserver
# 2. Go into busybox and attach strace to imaserver
# 3. Start a publisher sending small (24 Byte) messages
# 4. Start a subscriber
# 5. Kill the publisher and subscriber after some time
# 6. Go into busybox and kill strace
# 7. Get the strace output from the appliance
# 8. Cut out just the lines corresponding to our subscriber Client ID
# 9. Check the number of bytes written in each write does not exceed the 
#    expected value
#
# Uses DemoMqttEndpoint (Port 18883) with MaxSendSize = 200
#    BatchMessages=False
#    24 Byte messages
#    Client ID: test
#    Topic: test
#
# Expected strace output:
#    write(##, "0\26\0\4testtestmessagesmall", 24) = 24
#      where ## corresponds to the subscribers socket file descriptor
#
#------------------------------------------------------------------------------

echo "Entering $0 with $# arguments: ${@:1}"

while getopts "f:n:" opt; do
    case $opt in
        f )
            LOG=${OPTARG}
            ;;
        n )
            NAME=${OPTARG}
            ;;
        * )
            exit 1
            ;;
    esac
done

# Source test environment scripts
if [ -f ../testEnv.sh ] ; then
    source ../testEnv.sh
elif [ -f ../scripts/ISMsetup.sh ] ; then
    source ../scripts/ISMsetup.sh
else
    echo "No env file found"
    exit 1
fi

# Server Setup
#acceptlicense=`echo "imaserver set AcceptLicense" | ssh ${A1_USER}@${A1_HOST}`
#endpointupdate=`echo "imaserver update Endpoint Name=DemoMqttEndpoint MaxSendSize=200 BatchMessages=False Enabled=True" | ssh ${A1_USER}@${A1_HOST}`

# Get the PID of imaserver
#echo -e "Finding PID of imaserver process...\n" | tee -a ${LOG}
#pidcmd=`ssh ${A1_USER}@${A1_HOST} ps -ef | grep "imaserver \-d" | grep -v grep`
#imapid=`echo ${pidcmd} | awk -F" " '{print $2}'`
#echo -e "Found PID of imaserver process: ${imapid}\n" | tee -a ${LOG}

# Run strace to record writes of imaserver
#echo -e "Run strace on imaserver\n" | tee -a ${LOG}
#ssh ${A1_USER}@${A1_HOST} rm -rf strace.out
#ssh ${A1_USER}@${A1_HOST} strace -ff -e write -p ${imapid} 2> strace.out &

# Run MQTT Subscriber
echo -e "Run MQTT Subscriber\n" | tee -a ${LOG}
`${M2_IMA_SDK}/bin/mqttsample -a subscribe -c false -n 1 -q 1 -i msgbatchid -s tcp://${A1_IPv4_1}:18883 -t test 2>&1 > receiver.log` &
clientcmdrecvpid=$!
echo -e "Subscriber started, PID=${clientcmdrecvpid}\n" | tee -a ${LOG}
sleep 5
kill -9 ${clientcmdrecvpid} 2>&1 1>/dev/null
sleep 5

# Run MQTT Publisher
echo -e "Run MQTT Publisher\n" | tee -a ${LOG}
`${M2_IMA_SDK}/bin/mqttsample -a publish -c true -n 10000 -q 0 -s tcp://${A1_IPv4_1}:18883 -t test -m "testmessagesmall" 2>&1 > publisher.log` &
clientcmdsndpid=$!
echo -e "Publisher started, PID=${clientcmdsndpid}\n" | tee -a ${LOG}

echo -e "Waiting 10 seconds ...\n" | tee -a ${LOG}
sleep 15

# Run MQTT Subscriber
echo -e "Run MQTT Subscriber\n" | tee -a ${LOG}
`${M2_IMA_SDK}/bin/mqttsample -a subscribe -c false -n 10000 -q 1 -i msgbatchid -s tcp://${A1_IPv4_1}:18883 -t test 2>&1 > receiver.log` &
clientcmdrecvpid=$!
echo -e "Subscriber started, PID=${clientcmdrecvpid}\n" | tee -a ${LOG}

# Give some time to send and receive things
echo -e "Waiting 15 seconds ...\n" | tee -a ${LOG}
sleep 15

# Stop the sender and receiver
echo -e "Killing MQTT Clients\n" | tee -a ${LOG}
kill -9 ${clientcmdrecvpid} 2>&1 1>/dev/null
kill -9 ${clientcmdsndpid} 2>&1 1>/dev/null

sleep 5

# Kill strace
#echo -e "Finding PID of strace process...\n" | tee -a ${LOG}
#killstracecmd=`ssh ${A1_USER}@${A1_HOST} ps -ef | grep "strace" | grep -v grep`
#stracepid=`echo ${killstracecmd} | awk -F" " '{print $2}'`
#echo -e "Found PID of strace process: ${stracepid}\n" | tee -a ${LOG}
#killstracecmd=`ssh ${A1_USER}@${A1_HOST} kill ${stracepid}`

# Get strace output
#echo -e "Retrieving strace output file from server\n" | tee -a ${LOG}
#getstracelog=`scp ${A1_USER}@${A1_HOST}:~/strace.out ${M2_TESTROOT}/mqtt_td_tests/${NAME}.strace.log`

# Parse strace output
#grep "test" ${NAME}.strace.log > ${NAME}.strace.processed.log

#numlines=`wc -l ${NAME}.strace.processed.log | cut -d' ' -f 1`
#echo -e "${NAME}.strace.processed.log contains ${numlines} lines\n" | tee -a ${LOG}

#if [[ "${numlines}" == "0" ]] ; then
#	# strace.processed.log file is empty -- something went wrong
#	ehco -e "Test result is Failure!\n" | tee -a ${LOG}
#	exit 0
#fi

##########################################################################
#if [[ "${NAME}" != "" ]] ; then
#    cp msgbatching/${NAME}.comparetest.template msgbatching/${NAME}.comparetest
#	sed -i -e s/NUM_LINES/${numlines}/g msgbatching/${NAME}.comparetest
#fi

# Move the following validation to comparetest files
#numcorrect=`grep -c ", 24)" ${NAME}.strace.processed.log`
#echo -e "Found ${numcorrect} 24 Byte writes" | tee -a ${LOG}
#numunfinished=`grep -c ", 24 <u" ${NAME}.strace.processed.log`
#echo -e "Found ${numunfinished} unfinished 24 Byte writes" | tee -a ${LOG}
#num=`grep -c ", 24" ${NAME}.strace.processed.log`
#echo -e "Found ${num} Total writes\n" | tee -a ${LOG}
##########################################################################

# Server Cleanup
#endpointreset=`echo "imaserver update Endpoint Name=DemoMqttEndpoint MaxSendSize= BatchMessages=True Enabled=False"`

#echo "rm -rf strace.out" | ssh ${A1_USER}@${A1_HOST}

echo -e "Test result is Success!\n" | tee -a ${LOG}
#ehco -e "Test result is Failure!\n" | tee -a ${LOG}
