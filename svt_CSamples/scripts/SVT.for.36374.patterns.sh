#!/bin/bash 

. ./svt_test.sh

TEST_APP=mqttsample_array
SUBFLAGS=" "
PUBFLAGS=" "
EXTRAFLAGS=" "

PATTERN=fanout 

if [ -n "$SVT_AT_VARIATION_PATTERN" ] ;then
	echo "-----------------------------------------"
	echo " SVT AUTOMATION OVERIDE: Will use overide setting for SVT_AT_VARIATION_PATTERN = $SVT_AT_VARIATION_PATTERN" 
	echo "-----------------------------------------"
    PATTERN="$SVT_AT_VARIATION_PATTERN"
    
    if   [ "$PATTERN" == "fanout" ]     ; then   echo -n "";
    elif [ "$PATTERN" == "fanin" ]      ; then   echo -n "";
    else 
        echo ERROR unsupported pattern $PATTERN
        exit 1;
    fi
fi

if [ -n "$SVT_AT_VARIATION_CLEANSESSION" ] ; then
	echo "-----------------------------------------"
	echo " SVT AUTOMATION OVERIDE: Will use overide setting for SVT_AT_VARIATION_CLEANSESSION = $SVT_AT_VARIATION_CLEANSESSION" 
	echo "-----------------------------------------"
    EXTRAFLAGS+=" -c $SVT_AT_VARIATION_CLEANSESSION "
fi

if [ -n "$SVT_AT_VARIATION_TOPICCHANGE" ] ; then
	echo "-----------------------------------------"
	echo " SVT AUTOMATION OVERIDE: Will use overide setting for SVT_AT_VARIATION_TOPICCHANGE = $SVT_AT_VARIATION_TOPICCHANGE" 
	echo "-----------------------------------------"
    EXTRAFLAGS+=" -x topicChangeTest=$SVT_AT_VARIATION_TOPICCHANGE "
fi


if [ -n "$SVT_AT_VARIATION_PUB_DELAY_ON_CONNECT" ] ;then
	echo "-----------------------------------------"
	echo " SVT AUTOMATION OVERIDE: Will use overide setting for PUB_DELAY_ON_CONNECT :"
    echo "  ... SVT_AT_VARIATION_PUB_DELAY_ON_CONNECT = $SVT_AT_VARIATION_PUB_DELAY_ON_CONNECT" 
	echo "-----------------------------------------"
    PUBFLAGS=" -x publishDelayOnConnect=$SVT_AT_VARIATION_PUB_DELAY_ON_CONNECT "
else
    PUBFLAGS=" -x publishDelayOnConnect=10 "
fi

BASEMSG="A message from the MQTTv3 C client."
#BASEFLAGS=" -x warningWaitDynamic=1 -x verifyMsg=1 -x statusUpdateInterval=1 -x connectTimeout=60 -x reconnectWait=1 -x retryConnect=3600 -x keepAliveInterval=300 -x checkConnectionStatus=1 "
# TODO : need to get the checkconnectionstatus code synced up with the topicMultiTest code in mqttsample_array
BASEFLAGS=" -x warningWaitDynamic=1 -x verifyMsg=1 -x statusUpdateInterval=1 -x connectTimeout=60 -x reconnectWait=1 -x retryConnect=3600 -x keepAliveInterval=300  "
scale_topic=${1-""};   # The desired topic
let scale_connections_per_client=${2-590};
scale_client=${3-100};
let scale_total=($scale_connections_per_client*$scale_client);
scale_count=${4-100};  # Each subscribeer subscribees this many messages (all on same topic)
                          #3        * 49 *20 * 5 
    #let expected_msgs=($scale_count*scale_connections_per_client*20)

if [ -n "$SVT_AT_VARIATION_MSGCNT" ] ;then
	echo "-----------------------------------------"
	echo " SVT AUTOMATION OVERIDE: Will use overide setting for MSGCNT : SVT_AT_VARIATION_MSGCNT = $SVT_AT_VARIATION_MSGCNT" 
	echo "-----------------------------------------"
	scale_count=${SVT_AT_VARIATION_MSGCNT}
fi

scale_rate=${5-0};   # The desired rate (message/second) to subscribe messages
qos=${6-0};   # The desired QoS

if [ -n "$SVT_AT_VARIATION_QOS" ] ;then
	echo "-----------------------------------------"
	echo " SVT AUTOMATION OVERIDE: Will use overide setting for QOS : SVT_AT_VARIATION_QOS = $SVT_AT_VARIATION_QOS" 
	echo "-----------------------------------------"
	qos=${SVT_AT_VARIATION_QOS}
else 
	echo "-----------------------------------------"
	echo " SVT AUTOMATION OVERIDE: SVT_AT_VARIATION_QOS is not set"
	echo "-----------------------------------------"
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
unique=""
if [ -n "$SVT_AT_VARIATION_UNIQUE" ] ;then
    echo "-----------------------------------------"
    echo " SVT AUTOMATION OVERIDE: Will use overide setting for BURST : SVT_AT_VARIATION_UNIQUE = $SVT_AT_VARIATION_UNIQUE"
    echo "-----------------------------------------"
    unique="${SVT_AT_VARIATION_UNIQUE}."
fi 
 
    



if [ "$scale_topic" == "" ] ;then
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
        elif [ "$workload" == "connections_ldap_ssl" ] ;then
	        ha_imaserver="-x haURI=ssl://$ha_imaserver"
        elif [ "$workload" == "connections_ssl" ] ;then
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
rm -rf log.s.${qos}.${name}*
rm -rf log.p.${qos}.${name}*

let j=0;
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


#---------------------------------
# This function checks if a "quick exit" is needed.
# IF this is used it is assumed that the tester wants a "background" messaging load to be
# running while further testing is occurring.
#---------------------------------
do_quick_exit_check(){ 
    if [ -n "$SVT_AT_VARIATION_QUICK_EXIT" ] ;then
        echo "-----------------------------------------"
        echo "`date` SVT AUTOMATION OVERIDE: Do \"quick exit\" after starting publisher. It is expected that "
        echo "`date` other testing will be done while this workload continues to run, and messaging results will be evaluated later"
        echo "-----------------------------------------"
    
        echo "---------------------------------"
        echo "`date` $0 - Done! Successfully completed all expected processing - Quick Exit"
        echo "---------------------------------"
    
        exit 0;
    
    fi
}

#---------------------------------
# This function syncs up the subcribers if the the SVT_AT_VARIATION_SUBSCRIBE_SYNC environment variable was specified.
# Messaging will not continue until all subscribers are subscribed and then will sync them all up to continue at a later time.
#---------------------------------
do_subscribe_sync_check  () {
    local subscriber_count=$1

    if [ -n "$SVT_AT_VARIATION_SUBSCRIBE_SYNC" ] ;then
        echo "---------------------------------"
        echo "`date` $0 - SKIP This process has gotten to All clients subscribed. Now wait for all other $0 processes to get to same point"
        echo "---------------------------------"
        #svt_verify_condition "All Clients Subscribed" $subscriber_count "log.s.*" $scale_client
    
        #echo "-----------------------------------------"
        #echo " SVT AUTOMATION OVERIDE: Do sync on subscribe complete with all Machines "
        #echo "-----------------------------------------"
        #
        #if [ "$scale_topic" == "17" ] ;then # - FIXME: 17 means is for the subnet 17 - hardcoded also in SVT.connections.sh 
            #svt_automation_sync_all_machines "${workload}.SUBSCRIBED" 1 # - 1 means to transfer sync files for this Machine to other Machines.  17 is the master.
        #else
            #svt_automation_sync_all_machines "${workload}.SUBSCRIBED" 0 # - 0 means do not transfer sync files for other machines.  Subnet 19-55 clients are the slaves
        #fi
    else
        echo "-----------------------------------------"
        echo " SVT SYNC - Skipped SVT_AT_VARIATION_SUBSCRIBE_SYNC not set $SVT_AT_VARIATION_SUBSCRIBE_SYNC. "
        echo "-----------------------------------------"
    fi
}

#---------------------------------
# These are additional publish or subscribe flags that need to be added that are dependent on the value of x inside the current while loop
#---------------------------------
print_x_flags () {
    local x=$1
    local ui

    echo -n " $EXTRAFLAGS "

    if [   "$workload" == "connections_client_certificate" ] ;then
        echo " "
    elif [ "$workload" == "connections_ssl" ] ;then
        echo " "
    elif [ "$workload" == "connections_ldap" ] ;then
        let ui=($x*$scale_connections_per_client);
        echo " -x userIncrementer=$ui -u u -p imasvtest  "
    elif [ "$workload" == "connections_ldap_ssl" ] ;then
        echo " -x userIncrementer=$ui -u u -p imasvtest  "
    elif [ "$workload" == "connections" ] ;then
        echo " "
    else 
        echo "---------------------------------"  >> /dev/stderr
        echo "ERROR - workload is not specified correctly or not understood: \"$workload\"" >> /dev/stderr
        echo "---------------------------------" >> /dev/stderr
        exit 1;
    fi
}

print_test_criteria(){
    local expected_msgs=$1

    if [ "$qos" == "0" ] ; then
        echo " -x testCriteriaPctMsg=90 "
    elif [ "$qos" == "1" ] ; then
        echo " -x testCriteriaMsgCount=$expected_msgs "
    elif [ "$qos" == "2" ] ; then
        echo " -x testCriteriaMsgCount=$expected_msgs "
    else
        echo "---------------------------------" >> /dev/stderr
        echo " ERROR: - internal bug in script unsupported qos : $qos"  >> /dev/stderr
        echo "---------------------------------" >> /dev/stderr
        exit 1;
    fi
}

#---------------------------------
# Start a fan-out messaging pattern
#---------------------------------
do_fanout(){
    local x
    local y
    local z
    local expected_msgs

    let x=1;
    while (($x<=$scale_client)) ; do
        #TODO: take out the aaaaaaaaaaaaaaaa and x stuff below
        #TODO: support variable lenghth topic scrings instead of 1024 max in mqttsample_array and just move to using the unique string passed in

        SUBFLAGS="-o log.s.${qos}.${name}.${x}.${now} -t ${scale_topic}.${unique}${h}aa${x}aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa -a subscribe -n $scale_count -w $scale_rate -q $qos -z $scale_connections_per_client -i $scale_topic.$x.${unique}${h} -x subscribeOnConnect=1 -x verifyStillActive=30 -x userReceiveTimeout=5 -x dynamicReceiveTimeout=1  -x subscribeOnConnect=1 -x verifyPubComplete=./log.p.${qos}.${name}.${now}.complete  "
        let expected_msgs=($scale_count*scale_connections_per_client)
        SUBFLAGS+=" `print_test_criteria $expected_msgs ` "
   
        ADDNLFLAGS=`print_x_flags $x`

        set -v
        MQTT_C_CLIENT_TRACE= $TEST_APP ${SUBFLAGS} ${BASEFLAGS} ${ADDNLFLAGS} -m "$BASEMSG" &
        set +v

        let x=$x+1;
    done
    svt_wait_60_for_processes_to_become_active ${now} ${qos} mqttsample_array ${scale_client}
    svt_verify_condition "Connected to " $scale_total "log.s.${qos}.${name}.*.${now}" $scale_client
    svt_verify_condition "Subscribed - Ready" $scale_total "log.s.${qos}.${name}.*.${now}" $scale_client
    svt_verify_condition "All Clients Subscribed" $scale_client "log.s.${qos}.${name}.*.${now}" $scale_client
   
    let x=(20*$scale_client);
    do_subscribe_sync_check $x
    

    y="" 
    if [ -n "$SVT_AT_VARIATION_TOPICCHANGE" ] ; then
        y=" -x topicMultiTest=$scale_connections_per_client "
        
        let x=1 ;
        let z=($scale_connections_per_client*$scale_count);

        while (($x<=$scale_client)) ; do
        #TODO: take out the aaaaaaaaaaaaaaaa and x stuff below
            MQTT_C_CLIENT_TRACE= $TEST_APP -v -o log.p.${qos}.${name}.${x}.${now} ${y} ${PUBFLAGS} ${BASEFLAGS} -a publish -t ${scale_topic}.${unique}${h}aa${x}aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa -n $z -w $pubrate -m "$BASEMSG" &
            let x=$x+1;
        done
        
    else

        MQTT_C_CLIENT_TRACE= $TEST_APP -v -o log.p.${qos}.${name}.1.${now} $x ${PUBFLAGS} ${BASEFLAGS} -a publish -t ${scale_topic}.${unique}${h}aa${x}aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa -n $scale_count -w $pubrate -m "$BASEMSG" &

    fi

#    do_quick_exit_check
    
#    svt_verify_condition "SVT_TEST_RESULT: SUCCESS" 1 "log.p.${qos}.${name}.1.${now}"
#    touch ./log.p.${qos}.${name}.${now}.complete   # used with -x verifyPubComplete setting to show that all publishing is complete.
    svt_verify_condition "SVT_TEST_RESULT: SUCCESS" $scale_client "log.s.${qos}.${name}.*.${now}"
    
}


#---------------------------------
# Start a fan-in messaging pattern
#---------------------------------
do_fanin(){
    local x
    local mypubflags
    local expected_msgs
    
    let x=1;
    let expected_msgs=($scale_count*scale_connections_per_client*$scale_client)
    SUBFLAGS=" -o log.s.${qos}.${name}.1.${now} -t ${scale_topic}.${h} -a subscribe -n $expected_msgs -w $scale_rate -q $qos -i $scale_topic.1.$h -x subscribeOnConnect=1 -x verifyStillActive=30 -x userReceiveTimeout=5 -x dynamicReceiveTimeout=1  -x subscribeOnConnect=1 -x verifyPubComplete=./log.p.${qos}.${name}.${now}.complete  "

    SUBFLAGS+=" `print_test_criteria $expected_msgs ` "

    ADDNLFLAGS=`print_x_flags $x`

    set -v
    MQTT_C_CLIENT_TRACE= $TEST_APP ${SUBFLAGS} ${BASEFLAGS} ${ADDNLFLAGS} -m "$BASEMSG" &
    set +v

    svt_wait_60_for_processes_to_become_active ${now} ${qos} mqttsample_array 1
    svt_verify_condition "Connected to " 1 "log.s.${qos}.${name}.*.${now}" $scale_client
    svt_verify_condition "Subscribed - Ready" 1 "log.s.${qos}.${name}.*.${now}" $scale_client
    svt_verify_condition "All Clients Subscribed" 1 "log.s.${qos}.${name}.*.${now}" $scale_client
    
    do_subscribe_sync_check 20
    
    let x=1;
    while (($x<=$scale_client)) ; do
    
        mypubflags=" $PUBFLAGS -o log.p.${qos}.${name}.$x.${now} -a publish -t ${scale_topic}.${h} -z $scale_connections_per_client  -n $scale_count -w $pubrate "
    
        ADDNLFLAGS=`print_x_flags $x`

        set -v
        MQTT_C_CLIENT_TRACE= $TEST_APP ${mypubflags} ${BASEFLAGS} ${ADDNLFLAGS} -m "$BASEMSG" &
        set +v

        let x=$x+1;
    done
    
    do_quick_exit_check

    svt_verify_condition "SVT_TEST_RESULT: SUCCESS" $scale_client "log.p.${qos}.${name}.1.${now}"
    touch ./log.p.${qos}.${name}.${now}.complete   # used with -x verifyPubComplete setting to show that all publishing is complete.
    svt_verify_condition "SVT_TEST_RESULT: SUCCESS" $scale_client "log.s.${qos}.${name}.*.${now}"
    
}



#---------------------------------
# Main execution path run the selected messaging pattern
#---------------------------------

if [ "$PATTERN" == "fanout" ] ;then
    do_fanout
elif [ "$PATTERN" == "fanin" ] ;then
    do_fanin
else
    echo "---------------------------------"
    echo "`date` $0 - Done! ERROR - unexpected PATTERN $PATTERN"
    echo "---------------------------------"
    exit 1;
fi


echo "---------------------------------"
echo "`date` $0 - Done! Successfully completed all expected processing"
echo "---------------------------------"


exit 0;


