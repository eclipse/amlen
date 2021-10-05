#!/bin/bash 

. ./svt_test.sh

TEST_APP=mqttsample_array

#------------------------------------------------------
# Complete Fan-in test (with SEGFAULT workaround subscribe_wait)
#------------------------------------------------------
verify_condition() {
    local condition=$1
    local count=$2
    local file_pattern=$3
    local iteration=$4
    local myregex="[0-9]+"

    echo "File pattern is $file_pattern"
    

    if ! [[ "$count" =~ $myregex ]] ; then
        echo "ERROR: count non-numeric"
        return 1;
    fi
    k=0;
    while (($k<$count)); do
        w=`grep "WARNING" $file_pattern 2>/dev/null |wc -l`
        e=`grep "ERROR" $file_pattern 2>/dev/null  |wc -l`
        c=`ps -ef | grep $TEST_APP 2>/dev/null | grep -v grep 2>/dev/null  | wc -l`
        k=`grep "$condition" $file_pattern 2>/dev/null |wc -l`
        echo "$iteration: Waiting for condition $condition to occur $count times(current: $k) warnings($w) errors:($e) clients:($c)"
        date;
        sleep 2;
    done
    echo "Success."
    return 0;
}

#mqtt_vsa=300;

SUBFLAGS=""
PUBFLAGS=" -x publishDelayOnConnect=30 "
BASEMSG="A message from the MQTTv3 C client."
BASEFLAGS=" -x verifyMsg=1 -x statusUpdateInterval=1 -x connectTimeout=120 -x reconnectWait=1 -x retryConnect=3600 -x keepAliveInterval=600 "
subscribe_topic=${1-""};   # The desired topic
let subscribe_connections_per_client=${2-590};
subscribe_client=${3-100};
let subscribe_total=($subscribe_connections_per_client*$subscribe_client);
subscribe_count=${4-100};  # Each subscribeer subscribees this many messages (all on same topic)

if [ -n "$SVT_AT_VARIATION_MSGCNT" ] ;then
	echo "-----------------------------------------"
	echo " SVT AUTOMATION OVERIDE: Will use overide setting for MSGCNT : SVT_AT_VARIATION_MSGCNT = $SVT_AT_VARIATION_MSGCNT" 
	echo "-----------------------------------------"
	subscribe_count=${SVT_AT_VARIATION_MSGCNT}
fi

subscribe_rate=${5-0};   # The desired rate (message/second) to subscribe messages
qos=${6-0};   # The desired QoS

if [ -n "$SVT_AT_VARIATION_QOS" ] ;then
	echo "-----------------------------------------"
	echo " SVT AUTOMATION OVERIDE: Will use overide setting for QOS : SVT_AT_VARIATION_QOS = $SVT_AT_VARIATION_QOS" 
	echo "-----------------------------------------"
	qos=${SVT_AT_VARIATION_QOS}
fi
imaserver=${7-"10.10.1.87:16102"}
name=${8-"log"}
ha_imaserver=${9-""}
pubrate=${10-0.033} # Default 1 message published on each fan out pattern every 30 seconds.
if [ -n "$SVT_AT_VARIATION_PUBRATE" ] ;then
	echo "-----------------------------------------"
	echo " SVT AUTOMATION OVERIDE: Will use overide setting for PUBRATE : SVT_AT_VARIATION_PUBRATE = $SVT_AT_VARIATION_PUBRATE" 
	echo "-----------------------------------------"
	pubrate=${SVT_AT_VARIATION_PUBRATE}

    #checkvsa=`echo "$mqtt_vsa $pubrate" | awk '{ x=1/$2; y=$1/2; if(x>y) { print x*2 } else { print $1 } }'`
    #if [ "$mqtt_vsa" != "$checkvsa" ] ;then
	    #echo "-----------------------------------------"
	    #echo " SVT AUTOMATION OVERIDE: As a result of pubrate, a new verifyStillActive setting was calculated and will be used: $checkvsa "
	    #echo "-----------------------------------------"
        #mqtt_vsa=$checkvsa;
    #fi

    
fi
workload=${11-"connections"}
if [ -n "$SVT_AT_VARIATION_WORKLOAD" ] ;then
    echo "-----------------------------------------"
    echo " SVT AUTOMATION OVERIDE: Will use overide setting for WORKLOAD : SVT_AT_VARIATION_WORKLOAD = $SVT_AT_VARIATION_WORKLOAD"
    echo "-----------------------------------------"
    workload="${SVT_AT_VARIATION_WORKLOAD}"
else 
    echo "-----------------------------------------"
    echo " `date` : $0 running workload $workload"
    echo "-----------------------------------------"
fi

burst="";

if [ -n "$SVT_AT_VARIATION_BURST" ] ;then
    echo "-----------------------------------------"
    echo " SVT AUTOMATION OVERIDE: Will use overide setting for BURST : SVT_AT_VARIATION_BURST = $SVT_AT_VARIATION_BURST"
    echo "-----------------------------------------"
    burst="${SVT_AT_VARIATION_BURST}"
    PUBFLAGS+=" -x burst=$burst "
fi

now=`date +%s.%N`

if [ "$subscribe_topic" == "" ] ;then
    echo ERROR no topic
    exit 1;
fi

if [ "$imaserver" == "" ] ;then
    echo ERROR no imaserver specified as argument 7
    exit 1;
fi

if [ -n "$ha_imaserver" ]; then
    if [ "$ha_imaserver" != "NULL" ] ;then
        if [ "$workload" == "connections_client_certificate" ] ;then
	        ha_imaserver="-x haURI=ssl://$ha_imaserver"
        else	
	        ha_imaserver="-x haURI=$ha_imaserver"
        fi	
        echo "set ha_imaserver to \"$ha_imaserver\""
    else
        ha_imaserver=""
    fi
fi

automation="true"

if [ "$automation" == "false" ];   then 
    killall -9 $TEST_APP;
    ps -ef |grep valgrind | grep $TEST_APP | awk '{print $2}'  | xargs -i kill -9 {}
fi

let j=0;
rm -rf log.s.${qos}.${name}*
rm -rf log.p.${qos}.${name}*
h=`hostname`

if [   "$workload" == "connections_client_certificate" ] ;then
    BASEFLAGS+=" -s ssl://$imaserver $ha_imaserver -S trustStore=ssl/trustStore.pem -S keyStore=ssl/imaclient-crt.pem -S privateKey=ssl/imaclient-key.pem "
elif [ "$workload" == "connections_ssl" ] ;then
    BASEFLAGS+=" -s ssl://$imaserver $ha_imaserver -S trustStore=ssl/trustStore.pem "
elif [ "$workload" == "connections_ldap" ] ;then
    BASEFLAGS+=" -s $imaserver $ha_imaserver "
    PUBFLAGS+=" -u u0000001 -p imasvtest "
elif [ "$workload" == "connections_ldap_ssl" ] ;then
    BASEFLAGS+=" -s ssl://$imaserver $ha_imaserver -S trustStore=ssl/trustStore.pem "
    PUBFLAGS+=" -u u0000001 -p imasvtest "
elif [ "$workload" == "connections" ] ;then
    BASEFLAGS+=" -s $imaserver $ha_imaserver "
else 
    echo "---------------------------------"
    echo "ERROR - workload is not specified correctly or not understood: \"$workload\""
    echo "---------------------------------"
    exit 1;
fi

let x=1;
while (($x<=$subscribe_client)) ; do
    SUBFLAGS="-o log.s.${qos}.${name}.${x}.${now} -t ${subscribe_topic}.${h} -a subscribe -n $subscribe_count -w $subscribe_rate -q $qos -z $subscribe_connections_per_client -i $subscribe_topic.$x.$h -x subscribeOnConnect=1 -x verifyStillActive=30 -x userReceiveTimeout=5 -x dynamicReceiveTimeout=1  -x subscribeOnConnect=1 -x verifyPubComplete=./log.p.${qos}.${name}.${now}.complete  "
    #SUBFLAGS+=$BASEFLAGS  
    let expected_msgs=($subscribe_count*subscribe_connections_per_client)
    if [ "$qos" == "0" ] ; then
       SUBFLAGS+="-x testCriteriaPctMsg=90 "
    elif [ "$qos" == "1" ] ; then
       SUBFLAGS+="-x testCriteriaMsgCount=$expected_msgs "
    elif [ "$qos" == "2" ] ; then
       SUBFLAGS+="-x testCriteriaMsgCount=$expected_msgs "
    else
        echo "---------------------------------" >> /dev/stderr
        echo " ERROR: - internal bug in script unsupported var : $var"  >> /dev/stderr
        echo "---------------------------------" >> /dev/stderr
    fi

    if [   "$workload" == "connections_client_certificate" ] ;then
        set -v
        MQTT_C_CLIENT_TRACE= $TEST_APP ${SUBFLAGS} ${BASEFLAGS} -m "$BASEMSG" &
        set +v
    elif [ "$workload" == "connections_ssl" ] ;then
        set -v
        MQTT_C_CLIENT_TRACE= $TEST_APP ${SUBFLAGS} ${BASEFLAGS}  -m "$BASEMSG"&
        set +v
    elif [ "$workload" == "connections_ldap" ] ;then
        let ui=($x*$subscribe_connections_per_client);
        set -v
        MQTT_C_CLIENT_TRACE= $TEST_APP ${SUBFLAGS} ${BASEFLAGS} -x userIncrementer=$ui -u u -p imasvtest -m "$BASEMSG" &
        set +v
    elif [ "$workload" == "connections_ldap_ssl" ] ;then
        set -v
        MQTT_C_CLIENT_TRACE= $TEST_APP ${SUBFLAGS} ${BASEFLAGS} -x userIncrementer=$ui -u u -p imasvtest -m "$BASEMSG" &
        set +v
    elif [ "$workload" == "connections" ] ;then
        set -v
        MQTT_C_CLIENT_TRACE= $TEST_APP ${SUBFLAGS} ${BASEFLAGS}  -m "$BASEMSG" &
        set +v
    else 
        echo "---------------------------------"
        echo "ERROR - workload is not specified correctly or not understood: \"$workload\""
        echo "---------------------------------"
        exit 1;
    fi
    let x=$x+1;
done
svt_wait_60_for_processes_to_become_active ${now} ${qos} mqttsample_array ${subscribe_client}
svt_verify_condition "Connected to " $subscribe_total "log.s.${qos}.${name}.*.${now}" $subscribe_client
svt_verify_condition "Subscribed - Ready" $subscribe_total "log.s.${qos}.${name}.*.${now}" $subscribe_client
svt_verify_condition "All Clients Subscribed" $subscribe_client "log.s.${qos}.${name}.*.${now}" $subscribe_client

if [ -n "$SVT_AT_VARIATION_SUBSCRIBE_SYNC" ] ;then
    echo "---------------------------------"
    echo "`date` $0 - This process has gotten to All clients subscribed. Now wait for all other $0 processes to get to same point"
    echo "---------------------------------"
    svt_verify_condition "Condition All Clients Subscribed has been met" 20 "log.fa.*" $subscribe_client

    echo "-----------------------------------------"
    echo " SVT AUTOMATION OVERIDE: Do sync on subscribe complete with all Machines "
    echo "-----------------------------------------"
    svt_automation_sync_all_machines "${workload}.SUBSCRIBED"
else
    echo "-----------------------------------------"
    echo " SVT SYNC - Skipped SVT_AT_VARIATION_SUBSCRIBE_SYNC not set $SVT_AT_VARIATION_SUBSCRIBE_SYNC. "
    echo "-----------------------------------------"
fi

MQTT_C_CLIENT_TRACE= $TEST_APP -v -o log.p.${qos}.${name}.1.${now} ${PUBFLAGS} ${BASEFLAGS} -a publish -t ${subscribe_topic}.${h} -n $subscribe_count -w $pubrate -m "$BASEMSG" &
svt_verify_condition "SVT_TEST_RESULT: SUCCESS" 1 "log.p.${qos}.${name}.1.${now}"
touch ./log.p.${qos}.${name}.${now}.complete   # used with -x verifyPubComplete setting to show that all publishing is complete.
svt_verify_condition "SVT_TEST_RESULT: SUCCESS" $subscribe_client "log.s.${qos}.${name}.*.${now}"


echo "---------------------------------"
echo "`date` $0 - Done! Successfully completed all expected processing"
echo "---------------------------------"


exit 0;

