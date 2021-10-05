#!/bin/bash

#-------------------------------------------------------------
# Example: starting 58500 ssl connections and sending 10 1 1 messages on QOS 0, 1 2, repsecitively
# Note: IMPORTANT: port 16114 configured for ssl: on the appliance.
# ./RTC.17942.c.sh 100 195 10 1 1 "0 1 2" "security" "" 10.10.1.39:16114 10.10.1.40:16114
#-------------------------------------------------------------

#-------------------------------------------------------------
# Example: starting 58500 client certificate connections and sending 10 1 1 messages on QOS 0, 1 2, repsecitively
# Note: IMPORTANT: port 16113 configured for ssl: w/ client certificate on the appliance.
#./RTC.17942.c.sh 100 195 10 1 1 "0 1 2" "client_certificate" "" 10.10.1.39:16113 10.10.1.40:16113 
#-------------------------------------------------------------

#-------------------------------------------------------------
# Example: starting 8 clients for high message throughput w/ Qos=0
# Note: IMPORTANT: port 16102 configured for anything can connect
# ./RTC.17942.c.sh 8 1 1000000 na na "0" "" "" 10.10.1.39:16102 10.10.1.40:16102
#-------------------------------------------------------------

. ./svt_test.sh

ima1=${1-10.10.1.39:16102}
ima2=${2-10.10.1.40:16102}
num_clients=${3-1};
num_X_per_client=${4-1};
num_0_messages=${5-3000000};
num_1_messages=${6-300};
num_2_messages=${7-300};
now=`date +%s.%N`
name=${8-""}  # "throughput" "security"  or 
msgrate=${9-0}; # message rate - 0 means max speed
qos0_under_test=${10-""}
qos1_under_test=${11-""}
qos2_under_test=${12-""}

qos_under_test="$qos0_under_test $qos1_under_test $qos2_under_test"

echo "ima1 :$ima1"
echo "ima2 :$ima2"
echo "num_clients $num_clients"
echo "num_X_per_client $num_X_per_client"
echo "QOS under test: $qos_under_test"
echo "msgrate is $msgrate"


#echo "---------------------------------"
#echo "Kill any other mqttsample_array processes that are running"
#echo "---------------------------------"
#killall -9 mqttsample_array

get_flags(){
    local qos=$1
    local action=$2
    local flags; 
 
    if [ "$name" == "security" -o "$name" == "client_certificate" ] ;then
        echo "---------------------------------" >> /dev/stderr
        echo " INFO: - using Security flags"  >> /dev/stderr
        echo "---------------------------------" >> /dev/stderr
	    if ! [ -e ./trustStore.pem ] ;then
        	    echo "---------------------------------" >> /dev/stderr
        	    echo " ERROR: - if you want to do ssl, the proper trustStore.pem file needs to be in cwd"  >> /dev/stderr
        	    echo "---------------------------------" >> /dev/stderr
		    exit 1;
	    fi
	    if [ "$name" == "security" ] ;then
        	echo "---------------------------------" >> /dev/stderr
        	echo " INFO: - using basic ssl flags"  >> /dev/stderr
       		echo "---------------------------------" >> /dev/stderr
	
    		flags=" -s ssl://${ima1} -x keepAliveInterval=300 -x connectTimeout=300 -x reconnectWait=1 -x retryConnect=3600 -q ${qos} -t ${qos}_${now} -x verifyMsg=1 -x orderMsg=1 -x msgFile=./SMALLFILE -x statusUpdateInterval=1 -x subscribeOnConnect=1 -x publishDelayOnConnect=120 -S trustStore=trustStore.pem "  
		    if [ -n "${ima2}" ] ;then
			    flags+=" -x haURI=ssl://${ima2} "
		    fi
        
	    elif [ "$name" == "client_certificate" ] ;then
        	echo "---------------------------------" >> /dev/stderr
        	echo " INFO: - using client_certificate ssl flags"  >> /dev/stderr
       		echo "---------------------------------" >> /dev/stderr
	        for cfile in imaclient-crt.pem imaclient-key.pem ; do 
		        if ! [ -e $cfile ] ;then
        		    echo "---------------------------------" >> /dev/stderr
        		    echo " ERROR: - if you want to do client certificate ssl, the proper $cfile file needs to be in cwd"  >> /dev/stderr
        		    echo "---------------------------------" >> /dev/stderr
		    	    exit 1;
		        fi
	        done
    		flags=" -s ssl://${ima1} -x keepAliveInterval=300 -x connectTimeout=300 -x reconnectWait=1 -x retryConnect=3600 -q ${qos} -t ${qos}_${now} -x verifyMsg=1 -x orderMsg=1 -x msgFile=./SMALLFILE -x statusUpdateInterval=1 -x subscribeOnConnect=1 -x publishDelayOnConnect=120 -S trustStore=trustStore.pem -S keyStore=imaclient-crt.pem -S privateKey=imaclient-key.pem "  
		    if [ -n "${ima2}" ] ;then
			    flags+=" -x haURI=ssl://${ima2} "
		    fi
        else 
      	    echo "---------------------------------" >> /dev/stderr
       	    echo " Internal bug in script"  >> /dev/stderr
       	    echo "---------------------------------" >> /dev/stderr
		    exit 1;
        fi
    elif [ "$name" == "ldap" ] ;then
        echo "---------------------------------" >> /dev/stderr
        echo " INFO: - using ldap flags"  >> /dev/stderr
        echo "---------------------------------" >> /dev/stderr
    	flags=" -s ${ima1} -x keepAliveInterval=300 -x connectTimeout=300 -x reconnectWait=1 -x retryConnect=3600 -q ${qos} -t ${qos}_${now} -x verifyMsg=1 -x orderMsg=1 -x msgFile=./SMALLFILE -x statusUpdateInterval=1 -x subscribeOnConnect=1 -x publishDelayOnConnect=120  "  
	    if [ -n "${ima2}" ] ;then
		    flags+=" -x haURI=${ima2} "
	    fi
    elif [ "$name" == "throughput" ] ;then
        echo "---------------------------------" >> /dev/stderr
        echo " INFO: - using throughput flags"  >> /dev/stderr
        echo "---------------------------------" >> /dev/stderr
    	flags=" -s ${ima1} -x keepAliveInterval=300 -x connectTimeout=100 -x reconnectWait=1 -x retryConnect=3600 -q ${qos} -t ${qos}_${now} -x verifyMsg=1 -x orderMsg=1 -x msgFile=./SMALLFILE -x statusUpdateInterval=1 -x subscribeOnConnect=1 -x publishDelayOnConnect=120  "  
	    if [ -n "${ima2}" ] ;then
		    flags+=" -x haURI=${ima2} "
	    fi
	else
      	echo "---------------------------------" >> /dev/stderr
       	echo " ERROR: - Unsupported name flag :$name"  >> /dev/stderr
       	echo "---------------------------------" >> /dev/stderr
		exit 1;
    fi

    flags+=" -x cleanupOnConnectFails=1 "

    if [ "$action" == "SUBSCRIBE" ] ;then
        flags+=" -z $num_X_per_client -x verifyStillActive=60 "
    elif [ "$action" == "PUBLISH" ] ;then
        flags+=" -w ${msgrate} "
    else
        echo "---------------------------------" >> /dev/stderr
        echo " ERROR: - internal bug in script unsupported var : $var"  >> /dev/stderr
        echo "---------------------------------" >> /dev/stderr
        exit 1;
    fi
    

    if [ "$qos" == "0" ] ; then
        flags+=" -n ${num_0_messages} "
    elif [ "$qos" == "1" ] ; then
        flags+=" -n ${num_1_messages} -c false "
    elif [ "$qos" == "2" ] ; then
        flags+=" -n ${num_2_messages} -c false "
    else
        echo "---------------------------------" >> /dev/stderr
        echo "`date`  ERROR: - internal bug in script unsupported var : $var"  >> /dev/stderr
        echo "---------------------------------" >> /dev/stderr
	exit 1;
    fi
    echo $flags
}

for qos in $qos_under_test ; do 
    rm -rf log.s.${name}.${qos}.*
    let i=0;
    while (($i<$num_clients)); do
        echo "MQTT_C_CLIENT_TRACE= ./mqttsample_array -o log.s.${name}.${qos}.${i}.${now} -a subscribe -i ${qos}_${i}_${now}_S `get_flags $qos SUBSCRIBE` &" |sh -x > log.s.${name}.${qos}.${i}.${now}.mqtt 
        let i=$i+1;
    done
done

echo "---------------------------------"
echo "`date` Wait for processes to become active"
echo "---------------------------------"
for qos in $qos_under_test ; do 
    svt_wait_60_for_processes_to_become_active ${now} ${qos} mqttsample_array ${num_clients}
done

for qos in $qos_under_test ; do 
    echo "---------------------------------"
    echo "`date` Wait for all qos $qos clients to subscribe" 
    echo "---------------------------------"

    svt_verify_condition "All Clients Subscribed" $num_clients "log.s.${name}.${qos}.*${now}" $num_clients

    #while((1)) ; do
        #echo -n "."
        #svt_check_processes_still_active ${now} ${qos}
        #checkrc=$?
        #svt_check_processes_still_active ${now} ${qos} mqttsample_array $num_clients
        #checkrc2=$?
        #echo "grep \"All Clients Subscribed\" log.s.${name}.${qos}.*${now} 2>/dev/null | wc -l"
        #count=`grep "All Clients Subscribed" log.s.${name}.${qos}.*${now} 2>/dev/null | wc -l `
        #if (($count==$num_clients)); then
            #echo "count is $count"
            #break;   
        #else
            #echo count $count is not == to $num_clients
        #fi
        #if [ "$checkrc" != "0" ] ;then
            #echo "WARNING: breaking prematurely because no processes are still active" 
            #break;
        #fi
        #if [ "$checkrc2" != "0" ] ;then
            #echo "WARNING: breaking prematurely because an unexpected number of clients is active."
            #break;
        #fi
        #sleep 1;
    #done
done

echo "---------------------------------"
echo "`date` Complete. All clients subscribed . Start publisher" 
echo "---------------------------------"

for qos in $qos_under_test ; do 
    rm -rf log.p.${name}.${qos}.*
    let i=0;
    echo "MQTT_C_CLIENT_TRACE= ./mqttsample_array -o log.p.${name}.${qos}.1.${now} -a publish -i ${qos}_${now}_P `get_flags $qos PUBLISH` &" |sh -x> log.p.${name}.${qos}.1.${now}.mqtt

done

echo "---------------------------------"
echo "`date` Complete. All publishers started"
echo "---------------------------------"

for qos in $qos_under_test ; do 
    for var in publishers subscribers; do 
        echo "---------------------------------"
        echo "`date` Wait for all qos $qos $var to complete" 
        echo "---------------------------------"
        while((1)) ; do
            echo -n "."
            svt_check_processes_still_active ${now} 
            checkrc=$?
            if [ "$var" == "publishers" ] ;then
                count=`grep "SVT_TEST_RESULT: SUCCESS " log.p.${name}.${qos}.*${now} 2>/dev/null | wc -l `
                total=1;
            elif [ "$var" == "subscribers" ] ;then
                count=`grep "SVT_TEST_RESULT: SUCCESS " log.s.${name}.${qos}.*${now} 2>/dev/null | wc -l `
                total=$num_clients
            else
                echo "---------------------------------"
                echo " `date` ERROR: - internal bug in script unsupported var : $var" 
                echo "---------------------------------"
		        exit 1;
            fi
            
            if (($count==$total)); then
                echo "Complete - count is $count"
                break;
            fi
            if [ "$checkrc" != "0" ] ;then
                echo "ERROR: breaking prematurely because no processes are still active" 
                break;
            fi
            sleep 1;
        done
    done
done


echo "---------------------------------"
echo "`date` Cleanup - set clean to true and number of messages to zero to delete all the durable subscribers"
echo "TODO - monitor status for cleanup"
echo "---------------------------------"
my_pid=""
let total_cleanup=0
for qos in $qos_under_test ; do
    let i=0;
    while (($i<$num_clients)); do
        ./mqttsample_array -a subscribe -o log.s.${name}.${qos}.${i}.${now}.cleanup -i ${qos}_${i}_${now}_S `get_flags $qos SUBSCRIBE` -n 0 -c true > /dev/null 2>/dev/null & 
        my_pid+="$! "
        let i=$i+1;
        let total_cleanup=($total_cleanup+1);
    done
    ./mqttsample_array -o log.p.${name}.${qos}.1.${now}.cleanup -a publish -i ${qos}_${now}_P `get_flags $qos PUBLISH` -n 0 -c true > /dev/null 2>/dev/null & 
    my_pid+="$! "
    let total_cleanup=($total_cleanup+1);
done

echo "---------------------------------"
echo "`date` Start. wait for $my_pid - cleaning up $total_cleanup durable subscribers"
echo "---------------------------------"
wait $my_pid

echo "---------------------------------"
echo "`date` Done. wait for $my_pid"
echo "---------------------------------"

echo "---------------------------------"
echo "`date` Start waiting to see all cleanup process logs show final RESULT."
echo "---------------------------------"
let found_cleanup=0;
while((1)); do
    found_cleanup=`grep "SVT_TEST_RESULT: " log.*${name}.*${now}*.cleanup |wc -l`
    if (($found_cleanup>=$total_cleanup)); then
        echo "Success : found_cleanup : $found_cleanup >= $total_cleanup   "
        break;
    else
        echo "Still waiting : found_cleanup : $found_cleanup < $total_cleanup   "
    fi
    sleep 1;
done
echo "---------------------------------"
echo "`date` Done waiting to see all cleanup process logs show final RESULT."
echo "---------------------------------"

echo "---------------------------------"
echo "`date` Show netstat -tnp"
echo "---------------------------------"

netstat -tnp

echo "---------------------------------"
echo "`date` Done. All actions complete"
echo "---------------------------------"
    
exit 0;
