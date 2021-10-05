#!/bin/bash 

. ./svt_test.sh

svt_automation_env

	echo "-----------------------------------------"
    echo SVT_SUBNET_LIST_TOTAL=$SVT_SUBNET_LIST_TOTAL
    echo SVT_PATTERN_COUNTER=$SVT_PATTERN_COUNTER
	echo "-----------------------------------------"
    env | grep SVT
	echo "-----------------------------------------"

TEST_APP=mqttsample_array
SUBFLAGS=" "
PUBFLAGS=" "
MSGFILE=" "
EXTRAFLAGS=" "

PATTERN=fanout 

#--------------------------------------------------
# log prefix (LOGP) the prefix for all log files.
#--------------------------------------------------
LOGP=$(do_get_log_prefix)

if [ -n "$SVT_AT_VARIATION_VERBOSE_SUB" ] ;then
	echo "-----------------------------------------"
	echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting  " \
         "for SVT_AT_VARIATION_VERBOSE_SUB = $SVT_AT_VARIATION_VERBOSE_SUB" 
    SUBFLAGS+=" -v "
fi

if [ -n "$SVT_AT_VARIATION_VERBOSE_PUB" ] ;then
	echo "-----------------------------------------"
	echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_VERBOSE_PUB = $SVT_AT_VARIATION_VERBOSE_PUB" 
    PUBFLAGS+=" -v "
fi

if [ -n "$SVT_AT_VARIATION_PATTERN" ] ;then
	echo "-----------------------------------------"
	echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_PATTERN = $SVT_AT_VARIATION_PATTERN" 
	echo "-----------------------------------------"
    PATTERN="$SVT_AT_VARIATION_PATTERN"

fi

if [ -n "$SVT_AT_VARIATION_USERID_OVERRIDE" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
        "for USERID : SVT_AT_VARIATION_USERID_OVERRIDE = $SVT_AT_VARIATION_USERID_OVERRIDE" 
    echo "-----------------------------------------"
fi

if [ -n "$SVT_AT_VARIATION_WAITFORCOMPLETEMODE" ] ; then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_WAITFORCOMPLETEMODE = $SVT_AT_VARIATION_WAITFORCOMPLETEMODE"
    echo "-----------------------------------------"
    EXTRAFLAGS+=" -x waitForCompletionMode=$SVT_AT_VARIATION_WAITFORCOMPLETEMODE "
fi



verifyStillActive=" -x verifyStillActive=30 "
if [ -n "$SVT_AT_VARIATION_VERIFYSTILLACTIVE" ] ; then
	echo "-----------------------------------------"
	echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_VERIFYSTILLACTIVE = $SVT_AT_VARIATION_VERIFYSTILLACTIVE" 
	echo "-----------------------------------------"
    verifyStillActive=" -x verifyStillActive=$SVT_AT_VARIATION_VERIFYSTILLACTIVE "
fi

if [ -n "$SVT_AT_VARIATION_MSGFILE" ] ; then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_MSGFILE = $SVT_AT_VARIATION_MSGFILE"
    echo "-----------------------------------------"
    MSGFILE=" -x msgFile=$SVT_AT_VARIATION_MSGFILE "
fi

if [ -n "$SVT_AT_VARIATION_USLEEP" ] ; then
	echo "-----------------------------------------"
	echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_USLEEP = $SVT_AT_VARIATION_USLEEP" 
	echo "-----------------------------------------"
    EXTRAFLAGS+=" -x usleepLoop=${SVT_AT_VARIATION_USLEEP} "
fi

if [ -n "$SVT_AT_VARIATION_86WAIT4COMPLETION" ] ; then
	echo "-----------------------------------------"
	echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_86WAIT4COMPLETION = $SVT_AT_VARIATION_86WAIT4COMPLETION" 
	echo "-----------------------------------------"
    EXTRAFLAGS+=" -x 86WaitForCompletion=${SVT_AT_VARIATION_86WAIT4COMPLETION} "
fi

if [ -n "$SVT_AT_VARIATION_CLEANSESSION" ] ; then
	echo "-----------------------------------------"
	echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_CLEANSESSION = $SVT_AT_VARIATION_CLEANSESSION" 
	echo "-----------------------------------------"
    EXTRAFLAGS+=" -c $SVT_AT_VARIATION_CLEANSESSION "
fi


USERPREFIX=" -u u  " ; # "users" prefix default
if [ -n "$SVT_AT_VARIATION_USER_PREFIX_OVERRIDE" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
    "for SVT_AT_VARIATION_USER_PREFIX_OVERRIDE = $SVT_AT_VARIATION_USER_PREFIX_OVERRIDE" 
    USERPREFIX=" -u $SVT_AT_VARIATION_USER_PREFIX_OVERRIDE  " ; # Use user supplied user prefix
else
    USERPREFIX=" -u c  " ; # "users" prefix default
fi

if [ -n "$SVT_AT_VARIATION_SUBONCON" ] ; then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_SUBONCON = $SVT_AT_VARIATION_SUBONCON"
    echo "-----------------------------------------"
    EXTRAFLAGS+=" -x subscribeOnConnect=$SVT_AT_VARIATION_SUBONCON "
fi

if [ -n "$SVT_AT_VARIATION_TOPIC" ] ; then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_TOPIC = $SVT_AT_VARIATION_TOPIC"
    echo "-----------------------------------------"
    EXTRAFLAGS+=" -t $SVT_AT_VARIATION_TOPIC "
fi

if [ -n "$SVT_AT_VARIATION_SHAREDSUB_PCT_TOTAL_MSGS" ] ; then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_SHAREDSUB_PCT_TOTAL_MSGS = " \
         "$SVT_AT_VARIATION_SHAREDSUB_PCT_TOTAL_MSGS"
    echo "-----------------------------------------"
fi

if [ -n "$SVT_AT_VARIATION_SHAREDSUB" ] ; then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_SHAREDSUB = $SVT_AT_VARIATION_SHAREDSUB"
    echo "-----------------------------------------"
fi

if [ -n "$SVT_AT_VARIATION_SHAREDSUB_MULTI" ] ; then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_SHAREDSUB_MULTI = $SVT_AT_VARIATION_SHAREDSUB_MULTI" \
         " . This setting means that shared subs will be indexed, thereby increasing " \
         " the number of different shared subscriptions."
    echo "-----------------------------------------"
fi

if [ -n "$SVT_AT_VARIATION_ORDERMSG" ] ; then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_ORDERMSG = $SVT_AT_VARIATION_ORDERMSG"
    echo "-----------------------------------------"
    EXTRAFLAGS+=" -x orderMsg=$SVT_AT_VARIATION_ORDERMSG "
fi

if [ -n "$SVT_AT_VARIATION_ORDERMIN" ] ; then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_ORDERMIN = $SVT_AT_VARIATION_ORDERMIN"
    echo "-----------------------------------------"
    EXTRAFLAGS+=" -x testCriteriaOrderMin=$SVT_AT_VARIATION_ORDERMIN "
fi

if [ -n "$SVT_AT_VARIATION_PCTMSGS" ] ; then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_PCTMSGS = $SVT_AT_VARIATION_PCTMSGS"
    echo "-----------------------------------------"
    EXTRAFLAGS+=" -x testCriteriaPctMsg=$SVT_AT_VARIATION_PCTMSGS "
fi

if [ -n "$SVT_AT_VARIATION_ORDERMAX" ] ; then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_ORDERMAX = $SVT_AT_VARIATION_ORDERMAX"
    echo "-----------------------------------------"
    EXTRAFLAGS+=" -x testCriteriaOrderMax=$SVT_AT_VARIATION_ORDERMAX "
fi


if [ -n "$SVT_AT_VARIATION_VERIFYPUBCOMPLETE" -a  "$SVT_AT_VARIATION_VERIFYPUBCOMPLETE" == "0" ] ; then
        echo "-----------------------------------------"
        echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for $SVT_AT_VARIATION_VERIFYPUBCOMPLETE = " \
         "$SVT_AT_VARIATION_VERIFYPUBCOMPLETE (disabled)"
        echo "-----------------------------------------"
    EXTRAFLAGS+=" -x verifyPubComplete= "
fi

if [ -n "$SVT_AT_VARIATION_CLEANBEFORECONNECT" ] ; then
	echo "-----------------------------------------"
	echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_CLEANBEFORECONNECT = $SVT_AT_VARIATION_CLEANBEFORECONNECT" 
	echo "-----------------------------------------"
    EXTRAFLAGS+=" -x cleanupBeforeConnect=$SVT_AT_VARIATION_CLEANBEFORECONNECT "
fi

if [ -n "$SVT_AT_VARIATION_TOPICCHANGE" ] ; then
	echo "-----------------------------------------"
	echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_TOPICCHANGE = $SVT_AT_VARIATION_TOPICCHANGE" 
	echo "-----------------------------------------"
    EXTRAFLAGS+=" -x topicChangeTest=$SVT_AT_VARIATION_TOPICCHANGE "
    #### TODO: 10.4.2013 this doesn't seem to be working anymore ??  - resolved... it was due to do_set_client_topic
    ### I think it is straighted up now but probably need to check it again when i can

fi

if [ -n "$SVT_AT_VARIATION_STATUS_UPDATE" ] ; then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_STATUS_UPDATE = $SVT_AT_VARIATION_STATUS_UPDATE"
    echo "-----------------------------------------"
    EXTRAFLAGS+=" -x statusUpdateInterval=$SVT_AT_VARIATION_STATUS_UPDATE "
fi


if [ -n "$SVT_AT_VARIATION_NODISCONNECT" ] ; then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_NODISCONNECT = $SVT_AT_VARIATION_NODISCONNECT"
    echo "-----------------------------------------"
    EXTRAFLAGS+=" -x noDisconnect=$SVT_AT_VARIATION_NODISCONNECT "
fi


if [ -n "$SVT_AT_VARIATION_RECONNECTWAIT" ] ; then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_RECONNECTWAIT = $SVT_AT_VARIATION_RECONNECTWAIT"
    echo "-----------------------------------------"
    EXTRAFLAGS+=" -x reconnectWait=$SVT_AT_VARIATION_RECONNECTWAIT "
fi




if [ -n "$SVT_AT_VARIATION_PUB_DELAY_ON_CONNECT" ] ;then
	echo "-----------------------------------------"
	echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for PUB_DELAY_ON_CONNECT :"
    echo "  ... SVT_AT_VARIATION_PUB_DELAY_ON_CONNECT = $SVT_AT_VARIATION_PUB_DELAY_ON_CONNECT" 
	echo "-----------------------------------------"
    PUBFLAGS+=" -x publishDelayOnConnect=$SVT_AT_VARIATION_PUB_DELAY_ON_CONNECT "
else
    PUBFLAGS+=" -x publishDelayOnConnect=10 "
fi

if [ -n "$SVT_AT_VARIATION_UNSUBSCRIBE" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_UNSUBSCRIBE = $SVT_AT_VARIATION_UNSUBSCRIBE"
    echo "-----------------------------------------"
    # note: pass in 1 to unsubscribe upon disconnect
    SUBFLAGS+=" -x unsubscribe=$SVT_AT_VARIATION_UNSUBSCRIBE " 
fi

if [ -n "$SVT_AT_VARIATION_SUB_THROTTLE" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_SUB_THROTTLE = $SVT_AT_VARIATION_SUB_THROTTLE"
    echo "-----------------------------------------"
    SUBFLAGS+=" -x throttleFile=$SVT_AT_VARIATION_SUB_THROTTLE "
fi

if [ -n "$SVT_AT_VARIATION_PUB_THROTTLE" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_PUB_THROTTLE = $SVT_AT_VARIATION_PUB_THROTTLE"
    echo "-----------------------------------------"
    PUBFLAGS+=" -x throttleFile=$SVT_AT_VARIATION_PUB_THROTTLE "
fi

if [ -n "$SVT_AT_VARIATION_VERIFYMSG" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_VERIFYMSG = $SVT_AT_VARIATION_VERIFYMSG"
    echo "-----------------------------------------"
    EXTRAFLAGS+=" -x verifyMsg=$SVT_AT_VARIATION_VERIFYMSG "
fi

keepAliveInterval=300
if [ -n "$SVT_AT_VARIATION_KEEPALIVE" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_KEEPALIVE = $SVT_AT_VARIATION_KEEPALIVE"
    echo "-----------------------------------------"
    keepAliveInterval=$SVT_AT_VARIATION_KEEPALIVE 
fi

NOERROR=""
if [ -n "$SVT_AT_VARIATION_NOERROR" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_NOERROR = $SVT_AT_VARIATION_NOERROR"
    echo "-----------------------------------------"
    NOERROR=" -x noError=$SVT_AT_VARIATION_NOERROR "
fi


WARNWAITDYN=" -x warningWaitDynamic=1 "
if [ -n "$SVT_AT_VARIATION_WARNING_WAIT_DYNAMIC" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_WARNING_WAIT_DYNAMIC = $SVT_AT_VARIATION_WARNING_WAIT_DYNAMIC"
    echo "-----------------------------------------"
    WARNWAITDYN=" -x warningWaitDynamic=$SVT_AT_VARIATION_WARNING_WAIT_DYNAMIC "
fi

WARNWAIT=" -x warningWait=1 "
if [ -n "$SVT_AT_VARIATION_WARNING_WAIT" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_WARNING_WAIT = $SVT_AT_VARIATION_WARNING_WAIT"
    echo "-----------------------------------------"
    WARNWAIT=" -x warningWait=$SVT_AT_VARIATION_WARNING_WAIT "
fi

INJECT_DISCONNECTX=""
if [ -n "$SVT_AT_VARIATION_INJECT_DISCONNECTX" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_INJECT_DISCONNECTX = $SVT_AT_VARIATION_INJECT_DISCONNECTX"
    echo "-----------------------------------------"
    INJECT_DISCONNECTX=" -x injectDisconnectX=$SVT_AT_VARIATION_INJECT_DISCONNECTX "
fi

INJECT_DISCONNECT=""
if [ -n "$SVT_AT_VARIATION_INJECT_DISCONNECT" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_INJECT_DISCONNECT = $SVT_AT_VARIATION_INJECT_DISCONNECT"
    echo "-----------------------------------------"
    INJECT_DISCONNECT=" -x injectDisconnect=$SVT_AT_VARIATION_INJECT_DISCONNECT "
fi

CONNECTTIMEOUT="  -x connectTimeout=60 "
if [ -n "$SVT_AT_VARIATION_CONNECTTIMEOUT" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_CONNECTTIMEOUT = $SVT_AT_VARIATION_CONNECTTIMEOUT"
    echo "-----------------------------------------"
    CONNECTTIMEOUT=" -x connectTimeout=$SVT_AT_VARIATION_CONNECTTIMEOUT "
fi


BASEMSG="A message from the MQTTv3 C client."
BASEFLAGS=" $NOERROR $WARNWAIT $WARNWAITDYN  -x verifyMsg=1 -x statusUpdateInterval=1 "
BASEFLAGS+=" $INJECT_DISCONNECT $INJECT_DISCONNECTX "
BASEFLAGS+=" $CONNECTTIMEOUT -x reconnectWait=1 -x retryConnect=360000 "
BASEFLAGS+=" -x keepAliveInterval=${keepAliveInterval} -x subscribeOnReconnect=1 "

if [ -n "$SVT_AT_VARIATION_CHECK_CONN" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_CHECK_CONN = $SVT_AT_VARIATION_CHECK_CONN"
    echo "-----------------------------------------"
    BASEFLAGS+=" -x checkConnection=$SVT_AT_VARIATION_CHECK_CONN "
fi

if [ -n "$SVT_AT_VARIATION_RETAINED_MSGS" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_RETAINED_MSGS = $SVT_AT_VARIATION_RETAINED_MSGS"
    echo "-----------------------------------------"
    BASEFLAGS+=" -x retainedFlag=1 -x retainedMsgCounted=1 "
fi


scale_topic=${1-""};   # The desired topic
let scale_connections_per_client=${2-590};
scale_client=${3-100};
let scale_total=($scale_connections_per_client*$scale_client);
scale_count=${4-100};  # Each subscribeer subscribees this many messages (all on same topic)
                          #3        * 49 *20 * 5 
    #let expected_msgs=($scale_count*scale_connections_per_client*20)


if [ -n "$SVT_AT_VARIATION_MSGCNT" ] ;then
	echo "-----------------------------------------"
	echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for MSGCNT : SVT_AT_VARIATION_MSGCNT = $SVT_AT_VARIATION_MSGCNT" 
	echo "-----------------------------------------"
	scale_count=${SVT_AT_VARIATION_MSGCNT}
fi

scale_rate=${5-0};   # The desired rate (message/second) to subscribe messages
qos=${6-0};   # The desired QoS

if [ -n "$SVT_AT_VARIATION_QOS" ] ;then
	echo "-----------------------------------------"
	echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for QOS : SVT_AT_VARIATION_QOS = $SVT_AT_VARIATION_QOS" 
	echo "-----------------------------------------"
	qos=${SVT_AT_VARIATION_QOS}
fi

if [ -n "$SVT_AT_VARIATION_PUBQOS" ] ;then
	echo "-----------------------------------------"
	echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for PUBQOS : SVT_AT_VARIATION_PUBQOS = $SVT_AT_VARIATION_PUBQOS" 
	echo "-----------------------------------------"
	PUBFLAGS+=" -q $SVT_AT_VARIATION_PUBQOS "
fi
imaserver=${7-"10.10.1.87:16102"}
name=${8-"log"}
ha_imaserver=${9-""}
pubrate=${10-0.033} # Default 1 message published on each fan out pattern every 30 seconds.
if [ -n "$SVT_AT_VARIATION_PUBRATE" ] ;then
	echo "-----------------------------------------"
	echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for PUBRATE : SVT_AT_VARIATION_PUBRATE = $SVT_AT_VARIATION_PUBRATE" 
	echo "-----------------------------------------"
	pubrate=${SVT_AT_VARIATION_PUBRATE}
fi

if [ -n "$SVT_AT_VARIATION_SUBRATE" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SUBRATE : SVT_AT_VARIATION_SUBRATE = $SVT_AT_VARIATION_SUBRATE"
    echo "-----------------------------------------"
    SUBFLAGS+=" -w ${SVT_AT_VARIATION_SUBRATE} "
fi


if [ -n "$SVT_AT_VARIATION_PUBRATE_INCRA" ] ;then
	echo "-----------------------------------------"
	echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for PUBRATE : SVT_AT_VARIATION_PUBRATE_INCRA = $SVT_AT_VARIATION_PUBRATE_INCRA" 
	echo "-----------------------------------------"
    pubrate+=" -x incrPubRate=$SVT_AT_VARIATION_PUBRATE_INCRA "
fi

if [ -n "$SVT_AT_VARIATION_PUBRATE_INCRB" ] ;then
	echo "-----------------------------------------"
	echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for PUBRATE : SVT_AT_VARIATION_PUBRATE_INCRB = $SVT_AT_VARIATION_PUBRATE_INCRB" 
	echo "-----------------------------------------"
    pubrate+=" -x incrPubRateCount=$SVT_AT_VARIATION_PUBRATE_INCRB "
fi

if [ -n "$SVT_AT_VARIATION_PUBRATE_INCRC" ] ;then
	echo "-----------------------------------------"
	echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for PUBRATE : SVT_AT_VARIATION_PUBRATE_INCRC = $SVT_AT_VARIATION_PUBRATE_INCRC" 
	echo "-----------------------------------------"
    pubrate+=" -x incrPubRateInterval=$SVT_AT_VARIATION_PUBRATE_INCRC "
fi


workload=${11-"connections"}
if [ -n "$SVT_AT_VARIATION_WORKLOAD" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for WORKLOAD : SVT_AT_VARIATION_WORKLOAD = $SVT_AT_VARIATION_WORKLOAD"
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
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for BURST : SVT_AT_VARIATION_BURST = $SVT_AT_VARIATION_BURST"
    echo "-----------------------------------------"
    burst="${SVT_AT_VARIATION_BURST}"
    PUBFLAGS+=" -x burst=$burst "
fi

now=`date +%s.%N`
if [ -n "$SVT_AT_VARIATION_NOW" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for NOW : SVT_AT_VARIATION_NOW = $SVT_AT_VARIATION_NOW"
    echo "-----------------------------------------"
    if [ "${SVT_AT_VARIATION_NOW}" == "scale_topic" ] ;then
        now="$scale_topic" ; 
        # special variable allows the now parameter to match the scale_topic.
        # this is useful to make sure that each invocation of SVT.patterns.sh is able
        # to locate its group of processes and not get confused by other concurrently 
        # running SVT.pattern.sh invocations, while also having a "static" name for 
        # background processes that can be later checked to verify results. (e.g. this 
        # is used in 12.sh)
    else
        now="${SVT_AT_VARIATION_NOW}"
    fi
fi 

unique=""
if [ -n "$SVT_AT_VARIATION_UNIQUE" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for UNIQUE : SVT_AT_VARIATION_UNIQUE = $SVT_AT_VARIATION_UNIQUE"
    echo "-----------------------------------------"
    unique="/${SVT_AT_VARIATION_UNIQUE}/"
    unique_topicA="/${SVT_AT_VARIATION_UNIQUE}"
fi 

unique_clientid=""
if [ -n "$SVT_AT_VARIATION_UNIQUE_CID" ] ;then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for UNIQUE : SVT_AT_VARIATION_UNIQUE_CID = $SVT_AT_VARIATION_UNIQUE_CID"
    echo "-----------------------------------------"
    unique_clientid="${SVT_AT_VARIATION_UNIQUE_CID}"
fi 

if [ -n "$SVT_AT_VARIATION_TOPIC_MULTI_TEST" ] ; then
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for SVT_AT_VARIATION_TOPIC_MULTI_TEST = $SVT_AT_VARIATION_TOPIC_MULTI_TEST"
    if [ "$SVT_AT_VARIATION_TOPIC_MULTI_TEST" == "scale" ] ; then
        echo " `date` SVT_AT_VARIATION_TOPIC_MULTI_TEST:  Using default value "
        EXTRAFLAGS+=" -x topicMultiTest=$scale_connections_per_client "
    else
        echo " `date` SVT_AT_VARIATION_TOPIC_MULTI_TEST: Using user supplied value. "
        EXTRAFLAGS+=" -x topicMultiTest=$SVT_AT_VARIATION_TOPIC_MULTI_TEST "
    fi
    echo "-----------------------------------------"
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
        if echo "$workload" | grep "connections_client_certificate" > /dev/null 2> /dev/null ;then
	        ha_imaserver="-x haURI=ssl://$ha_imaserver"
        elif [ "$workload" == "connections_ldap_ssl" ] ;then
	        ha_imaserver="-x haURI=ssl://$ha_imaserver"
        elif [ "$workload" == "connections_ltpa_ssl" ] ;then
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

    rm -rf ${LOGP}.s.${qos}.${name}*
    rm -rf ${LOGP}.p.${qos}.${name}*
fi

let j=0;
h=`hostname -i`

if [   "$workload" == "connections_client_certificate"  ] ;then
    BASEFLAGS+=" -s ssl://$imaserver $ha_imaserver -S trustStore=ssl/trustStore.pem " 
    BASEFLAGS+=" -S keyStore=ssl/imaclient-crt.pem -S privateKey=ssl/imaclient-key.pem "
elif [   "$workload" == "connections_client_certificate_ldap" ] ;then
    BASEFLAGS+=" -s ssl://$imaserver $ha_imaserver -S trustStore=ssl/trustStore.pem "
    BASEFLAGS+=" -S keyStore=ssl/imaclient-crt.pem -S privateKey=ssl/imaclient-key.pem "
    PUBFLAGS+=" -u u0000001 -p imasvtest "
elif [   "$workload" == "connections_client_certificate_CN_fanout" ] ;then
    base_floor=$(((1000000/${M_COUNT})*(${THIS_M_NUMBER}-1)))
    base_count=$((1000000/${M_COUNT}))
    #base_floor=$(((1000000/10)*(${THIS_M_NUMBER}-1)))
    #base_count=$((1000000/10))
    base_incra=$(($base_count/$SVT_SUBNET_LIST_TOTAL))
    base_incrb=$(($base_incra*$SVT_PATTERN_COUNTER))
    base_incrc=$(($base_incra/$scale_client))
    base_dcc=$(($base_floor+$base_incrb))
    echo "-----------------------------------------------"
    echo SVT_SUBNET_LIST_TOTAL=$SVT_SUBNET_LIST_TOTAL
    echo SVT_PATTERN_COUNTER=$SVT_PATTERN_COUNTER
    echo base_floor=$base_floor
    echo base_count=$base_count
    echo base_incra=$base_incra
    echo base_incrb=$base_incrb
    echo base_incrc=$base_incrc
    echo base_dcc=$base_dcc
    echo "-----------------------------------------------"
    BASEFLAGS+=" -s ssl://$imaserver $ha_imaserver -S trustStore=ssl/trustStore.pem "
    BASEFLAGS+=" -x prependCN2Topic=1 -x dynamicClientCert=/svt.data/certificates/ -x useCNasClientID=1  "
    # Final dccIncrementer calculated in print_x_flags $(($base_dcc+($base_incrc*$x)))
elif [ "$workload" == "connections_ssl" ] ;then
    BASEFLAGS+=" -s ssl://$imaserver $ha_imaserver -S trustStore=ssl/trustStore.pem "
elif [ "$workload" == "connections_ldap" ] ;then
    BASEFLAGS+=" -s $imaserver $ha_imaserver "
    PUBFLAGS+=" -u u0000001 -p imasvtest "
elif [ "$workload" == "connections_ltpa_ssl" ] ;then
    BASEFLAGS+=" -s ssl://$imaserver $ha_imaserver -S trustStore=ssl/trustStore.pem "
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
    if [ -n "$SVT_AT_VARIATION_QUICK_EXIT" -a "$SVT_AT_VARIATION_QUICK_EXIT" == "true" ] ;then
        echo "-----------------------------------------"
        echo "`date` SVT AUTOMATION OVERIDE: Do \"quick exit\" after starting " \
            " publisher. It is expected that other testing will be done while " \
            " this workload continues to run, and messaging results will be evaluated later"
        echo "-----------------------------------------"
    
        echo "---------------------------------"
        echo "`date` $0 - Done! Successfully completed all expected processing - Quick Exit"
        echo "---------------------------------"
    
        exit 0;
    
    fi
}

#---------------------------------
# This function syncs up the subcribers if the the 
# SVT_AT_VARIATION_SYNC_CLIENTS environment variable was specified.
# Messaging will not continue until all subscribers are subscribed 
# and then will sync them all up to continue at a later time.
#---------------------------------
do_X_sync_check  () {
    local type=${1-"SUBSCRIBED"}
    local log_pattern=${2-"${LOGP}.s.*"}
    local base_condition=${3-"All Clients Subscribed"}
    local X_count=$4
    local sync_interval=${5-120}
    local condition=""

    if [ -n "$SVT_AT_VARIATION_SYNC_CLIENTS" ] ;then
        if [ "$SVT_AT_VARIATION_SYNC_CLIENTS" != "true" ] ; then
            echo "-----------------------------------------"
            echo " SVT SYNC - Skipped SVT_AT_VARIATION_SYNC_CLIENTS " \
                " not set to true $SVT_AT_VARIATION_SYNC_CLIENTS. "
            echo "-----------------------------------------"
            return 0;
        fi
    else
        echo "-----------------------------------------"
        echo " SVT SYNC - Skipped SVT_AT_VARIATION_SYNC_CLIENTS " \
            "not set $SVT_AT_VARIATION_SYNC_CLIENTS. "
        echo "-----------------------------------------"
        return 0;
    fi

    echo "---------------------------------"
    echo "`date` $0 - This process has gotten to $base_condition . " \
         "Now wait for all other $0 processes to get to same point"
    echo "---------------------------------"
    svt_verify_condition "$base_condition" $X_count "$log_pattern" $scale_client

    condition="${workload}.${type}"
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION OVERIDE: Do sync on $condition with all Machines "
    echo "-----------------------------------------"
    
    if [ "$scale_topic" == "17" ] ;then # - FIXME: 17 means is for the subnet 17 - hardcoded also in SVT.connections.sh 
        # - 1 means to transfer sync files for this Machine to other Machines.  17 is the master.
        svt_automation_sync_all_machines $condition 1 $condition "" $sync_interval 
    else
        # - 0 means do not transfer sync files for other machines.Subnet 19-55 clients are the slaves
        svt_automation_sync_all_machines $condition 0 $condition "" $sync_interval 
    fi
}

do_subscribe_sync_check  () {
    local X_count=$1
    local sync_interval=${2-120}
    do_X_sync_check "SUBSCRIBED" "${LOGP}.s.*" "All Clients Subscribed"  $X_count $sync_interval
}
do_publish_connect_sync () {
    local X_count=$1
    local sync_interval=${2-120}
    do_X_sync_check "PUBLISHING" "${LOGP}.p.*" "All Clients Connected" $X_count $sync_interval
}


#---------------------------------
# These are additional publish or subscribe flags that need to be added that 
# are dependent on the value of x inside the current while loop
#---------------------------------
print_x_flags () {
    local x=$1
    local type=$2
    local ui
    local count
    local data
    local dcc_incr

    echo -n " $EXTRAFLAGS "

    if [ -n "$SVT_AT_VARIATION_51598" ] ;then
        if [ "$type" == "publish" ] ; then
            echo -n " -x sharedMemoryKey=concurrent.51598.$x -x sharedMemoryType=1 -x sharedMemoryVal=50000 "
        elif [ "$type" == "subscribe" ] ; then
            echo -n " -x sharedMemoryKey=concurrent.51598.$x -x sharedMemoryType=1  "
        fi
    fi

    if [ -n "$SVT_AT_VARIATION_USERID_OVERRIDE" ] ;then
        echo -n " -u $SVT_AT_VARIATION_USERID_OVERRIDE -p imasvtest "
    fi

    if [   "$workload" == "connections_client_certificate" ] ;then
        echo -n " "
    elif [   "$workload" == "connections_client_certificate_ldap" ] ;then
        if ! [ -n "$SVT_AT_VARIATION_USERID_OVERRIDE" ] ;then
            let ui=($x*$scale_connections_per_client);
            echo -n " -x userIncrementer=$ui $USERPREFIX -p imasvtest  "
        fi
    elif [   "$workload" == "connections_client_certificate_CN_fanout" ] ;then
        dcc_incr=$(($base_dcc+($base_incrc*($x-1))))
        echo -n " -x dccIncrementer=$dcc_incr -x topicChangeTest=0 "  # cannot do both topicChangeTest, and topicMultiTest at same time!!!??? 
                                                                      # TODO: figure out why it was setting them both at the same time previously...
        if [ "$type" == "publish" ] ; then
            echo -n " -i p -t /sub "
        elif [ "$type" == "subscribe" ] ; then
            echo -n " -i s -t /sub "
        fi
    elif [ "$workload" == "connections_ssl" ] ;then
        echo -n  " "
    elif [ "$workload" == "connections_ldap" ] ;then
        if ! [ -n "$SVT_AT_VARIATION_USERID_OVERRIDE" ] ;then
            let ui=($x*$scale_connections_per_client);
            echo -n " -x userIncrementer=$ui $USERPREFIX -p imasvtest  "
        fi 
    elif [ "$workload" == "connections_ldap_ssl" ] ;then
        if ! [ -n "$SVT_AT_VARIATION_USERID_OVERRIDE" ] ;then
            let ui=($x*$scale_connections_per_client); # Is it possible this was causing RTC 38972?
            echo -n " -x userIncrementer=$ui $USERPREFIX -p imasvtest  "
        fi
        
    elif [ "$workload" == "connections_ltpa_ssl" ] ;then
        # TODO: only running test currently with valid ltpa tokens, can add a switch to do valid, expired , or a mix
        let count=0; 
        data=`svt_get_random_ltpa_1k_file_valid`;
        while ! [ -n "$data" -a -e "$data" ] ; do 
        	echo "---------------------------------"  >> /dev/stderr
        	echo "WARNING - problem getting ltpa file \"$data\"" >> /dev/stderr
       	    echo "---------------------------------" >> /dev/stderr
        	usleep 1000; 
        	let count=$count+1; 
        	if(($count>100)); then
				echo "---------------------------------"  >> /dev/stderr
        		echo "ERROR - Exitting. Could not get ltpa file after 100 tries \"$data\"" >> /dev/stderr
       	    	echo "---------------------------------" >> /dev/stderr
				exit 1;
			fi ; 
			data=`svt_get_random_ltpa_1k_file_valid`;
		done
        
        echo -n " -u IMA_LTPA_AUTH -x userLtpaFile=$data  "
    elif [ "$workload" == "connections" ] ;then
        echo -n  " "
    else 
        echo "---------------------------------"  >> /dev/stderr
        echo "ERROR - workload is not specified correctly or not understood: \"$workload\"" >> /dev/stderr
        echo "---------------------------------" >> /dev/stderr
        exit 1;
    fi

}

print_test_criteria(){
    local expected_msgs=$1

    if [ -n "$SVT_AT_VARIATION_PCTMSGS" ] ; then
        echo "---------------------------------" >> /dev/stderr
        echo " FYI skipping adding a test criteria as the user has already specified a percentage of messages to be received" >> /dev/stderr
        echo "---------------------------------" >> /dev/stderr
    elif [ -n "$SVT_AT_VARIATION_NO_TEST_CRITERIA" ] ; then
        echo "---------------------------------" >> /dev/stderr
        echo " FYI skipping adding a test criteria as the user declared there to be no test criteria" >> /dev/stderr
        echo " Note: this will not disable verify message criteria " >> /dev/stderr
        echo "---------------------------------" >> /dev/stderr
    elif [ "$qos" == "0" ] ; then
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
    local regex
    local unique_topic=""
    local expected_msgs
	
    if ! [ -n "$SVT_AT_VARIATION_SKIP_SUBSCRIBE" ] ;then
      if ! [ -n "$SVT_AT_VARIATION_SKIP_SUBSCRIBE_LAUNCH" ] ;then
        let x=1;
        while (($x<=$scale_client)) ; do
            #TODO: take out the aaaaaaaaaaaaaaaa and x stuff below
            #TODO: support variable lenghth topic scrings instead of 1024 max in mqttsample_array and just move to using the unique string passed in
            if [ -n "$SVT_AT_VARIATION_TOPICCHANGE" ] ; then 
                #TODO Take out these yayyyyy
                unique_topic="${unique}${x}" ;   # each subscriber to a completely different topic
            else
                unique_topic="${unique}" ; # bucnhes of subscribers share same topic
            fi 
            MYSUBFLAGS=" -o ${LOGP}.s.${qos}.${name}.${x}.${now} -t /${scale_topic}/${h}${unique_topic} -a subscribe -n $scale_count -w $scale_rate -q $qos -z $scale_connections_per_client -i $scale_topic.$x.${unique_clientid}${h} -x subscribeOnConnect=0 ${verifyStillActive} -x userReceiveTimeout=5 -x dynamicReceiveTimeout=1 -x verifyPubComplete=./${LOGP}.p.${qos}.${name}.${now}.complete ${SUBFLAGS}  "
            let expected_msgs=($scale_count*scale_connections_per_client)
            MYSUBFLAGS+=" `print_test_criteria $expected_msgs ` "
    
            ADDNLFLAGS=`print_x_flags $x "subscribe" `
#TODO take this out
#TODO take this out
#TODO take this out
#TODO take this out
   #if [ "$scale_topic" == "17" -a "$x" == "1" ] ;then

#            MQTT_C_CLIENT_TRACE=ON $TEST_APP ${BASEFLAGS} ${MYSUBFLAGS} ${ADDNLFLAGS} -m "$BASEMSG" ${MSGFILE} 1>>${LOGP}.s.${qos}.${name}.${x}.${now}.mqtt 2>> ${LOGP}.s.${qos}.${name}.${x}.${now}.mqtt &
#   else

            set -v
            #MQTT_C_CLIENT_TRACE_LEVEL=MAXIMUM MQTT_C_CLIENT_TRACE=ON  $TEST_APP ${MYSUBFLAGS} ${BASEFLAGS} ${ADDNLFLAGS} -m "$BASEMSG" ${MSGFILE} 1>>${LOGP}.s.${qos}.${name}.${x}.${now}.mqtt 2>> ${LOGP}.s.${qos}.${name}.${x}.${now}.mqtt &
            echo `date` MQTT_C_CLIENT_TRACE= $TEST_APP ${MYSUBFLAGS} ${BASEFLAGS} ${ADDNLFLAGS} -m "$BASEMSG" ${MSGFILE} 
            MQTT_C_CLIENT_TRACE= $TEST_APP ${BASEFLAGS} ${MYSUBFLAGS} ${ADDNLFLAGS} -m "$BASEMSG" ${MSGFILE} &
            set +v

#    fi
    
            let x=$x+1;
          done
          if [ "$scale_count" != "0" ] ;then
            svt_wait_60_for_processes_to_become_active ${now} ${qos} mqttsample_array ${scale_client}
          fi
        fi
       
        if [ -n "$SVT_AT_VARIATION_VERIFY_SUB_CON" -a "$SVT_AT_VARIATION_VERIFY_SUB_CON" == "false" ] ;then 
            echo "-----------------------------------------"
            echo " `date` SVT AUTOMATION OVERIDE: SVT_AT_VARIATION_VERIFY_SUB_CON: Will not verify that subscribers connected."
            echo "-----------------------------------------"
        else
            svt_verify_condition "Connected to " $scale_total "${LOGP}.s.${qos}.${name}.*.${now}" $scale_client
        fi
        if [ -n "$SVT_AT_VARIATION_VERIFY_SUB" -a "$SVT_AT_VARIATION_VERIFY_SUB" == "false" ] ;then 
            echo "-----------------------------------------"
            echo " `date` SVT AUTOMATION OVERIDE: SVT_AT_VARIATION_VERIFY_SUB: Will not verify that subscribers subscribed"
            echo "-----------------------------------------"
        else
            svt_verify_condition "Subscribed - Ready" $scale_total "${LOGP}.s.${qos}.${name}.*.${now}" $scale_client
            svt_verify_condition "All Clients Subscribed" $scale_client "${LOGP}.s.${qos}.${name}.*.${now}" $scale_client
        fi
    
        let x=(20*$scale_client);
        do_subscribe_sync_check $x
    
        if [ -n "$SVT_AT_VARIATION_SUSPEND_SUBSCRIBERS" ] ;then
            echo "-----------------------------------------"
            echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
              "for : SVT_AT_VARIATION_SUSPEND_SUBSCRIBERS = $SVT_AT_VARIATION_SUSPEND_SUBSCRIBERS"
            echo "-----------------------------------------"
            killall -19 $TEST_APP
            sleep 30;
            echo "-----------------------------------------"
            echo " `date` SVT AUTOMATION OVERIDE: continuing."
            echo "-----------------------------------------"
        fi

    fi 

    if [ -z "$SVT_AT_VARIATION_FANOUT_PUBTYPE" ] ;then
        SVT_AT_VARIATION_FANOUT_PUBTYPE="publish"
            echo "-----------------------------------------"
            echo " `date` SVT AUTOMATION : Will use default setting " \
         "for SVT_AT_VARIATION_FANOUT_PUBTYPE = $SVT_AT_VARIATION_FANOUT_PUBTYPE"
            echo "-----------------------------------------"
    else
            echo "-----------------------------------------"
            echo " `date` SVT AUTOMATION : Will use override setting " \
         "for SVT_AT_VARIATION_FANOUT_PUBTYPE = $SVT_AT_VARIATION_FANOUT_PUBTYPE"
            echo "-----------------------------------------"
    fi
    pub_type=$SVT_AT_VARIATION_FANOUT_PUBTYPE
    publish_process_count=0
    publish_process_pid=""
    if ! [ -n "$SVT_AT_VARIATION_SKIP_PUBLISH" ] ;then
      if ! [ -n "$SVT_AT_VARIATION_SKIP_PUBLISH_LAUNCH" ] ;then
        y="" 
        if [ -n "$SVT_AT_VARIATION_TOPICCHANGE" ] ; then
            y=" -x topicMultiTest=$scale_connections_per_client "
            
            let x=1 ;
            let z=($scale_connections_per_client*$scale_count);
    
	        if [ -n "$SVT_AT_VARIATION_ALTERNATE_PRIMARY_IP" ] ;then
    		    echo "-----------------------------------------"
    		    echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
                 "for : SVT_AT_VARIATION_ALTERNATE_PRIMARY_IP = $SVT_AT_VARIATION_ALTERNATE_PRIMARY_IP"
    		    echo "-----------------------------------------"
    
		        regex="([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)(.*)"
		    
		        if [[ $imaserver =~ $regex ]] ;then 
			        echo yes ${BASH_REMATCH[1]} ${BASH_REMATCH[2]}  ; 
			        imaserver=$SVT_AT_VARIATION_ALTERNATE_PRIMARY_IP${BASH_REMATCH[2]}
    			        echo "-----------------------------------------"
    			        echo " `date` SVT AUTOMATION OVERIDE: Overriding imaserver with $imaserver "
    			        echo "-----------------------------------------"
			        BASEFLAGS+=" -s $imaserver " 
		        else
    			        echo "-----------------------------------------"
			        echo "ERROR: unexpected failure with regex $regex and imaserver : $imaserver . Did not match as expected."
    			        echo "-----------------------------------------"
		        fi
	        fi 
        
            while (($x<=$scale_client)) ; do
                unique_topic="${unique}${x}" ; #publishing to a different topic for every single subscriber.
                ADDNLFLAGS=`print_x_flags $x "publish" `
#TODO - remove me
#TODO - remove me
#TODO - remove me
#TODO - remove me
#TODO - remove me
#TODO - remove me
#TODO - remove me
   #if [ "$scale_topic" == "17" -a "$x" == "1" ] ;then
            #MQTT_C_CLIENT_TRACE=ON $TEST_APP -o ${LOGP}.p.${qos}.${name}.${x}.${now} ${y} -a publish -t /${scale_topic}/${h}${unique_topic} -n $z -w $pubrate ${PUBFLAGS} ${BASEFLAGS} ${ADDNLFLAGS} -m "$BASEMSG" ${MSGFILE} 1>>${LOGP}.p.${qos}.${name}.${x}.${now}.mqtt 2>> ${LOGP}.p.${qos}.${name}.${x}.${now}.mqtt &
#else


			# note: publish_wait below. every once in a while an app does not get signal -change to file or do not reenable
            let publish_process_count=$publish_process_count+1;
            echo `date` run command MQTT_C_CLIENT_TRACE= $TEST_APP -o ${LOGP}.p.${qos}.${name}.${x}.${now} ${y} -a $pub_type -t /${scale_topic}/${h}${unique_topic} -n $z -w $pubrate ${PUBFLAGS} ${BASEFLAGS} ${ADDNLFLAGS} -m "$BASEMSG" ${MSGFILE}  
            MQTT_C_CLIENT_TRACE= $TEST_APP -o ${LOGP}.p.${qos}.${name}.${x}.${now} ${y} -a $pub_type -t /${scale_topic}/${h}${unique_topic} -n $z -w $pubrate ${PUBFLAGS} ${BASEFLAGS} ${ADDNLFLAGS} -m "$BASEMSG" ${MSGFILE}  &
			echo mqttsample_array publish process started: $! : publish_process_count= $publish_process_count
#fi
                let x=$x+1;
            done
            
        else
    
            if [ -n "$unique" ] ;then
                unique_topic="${unique}" ; # bucnhes of subscribers share same topic
            else
                unique_topic="" ; # bucnhes of subscribers share same topic
            fi
            
            set +v
            ADDNLFLAGS=" $EXTRAFLAGS "  
            let publish_process_count=$publish_process_count+1;
            echo `date` run command MQTT_C_CLIENT_TRACE= $TEST_APP -o ${LOGP}.p.${qos}.${name}.1.${now} ${PUBFLAGS} ${BASEFLAGS} ${ADDNLFLAGS} -a $pub_type -t /${scale_topic}/${h}${unique_topic} -n $scale_count -w $pubrate -m "$BASEMSG" ${MSGFILE}  
            MQTT_C_CLIENT_TRACE= $TEST_APP -o ${LOGP}.p.${qos}.${name}.1.${now} ${PUBFLAGS} ${BASEFLAGS} ${ADDNLFLAGS} -a $pub_type -t /${scale_topic}/${h}${unique_topic} -n $scale_count -w $pubrate -m "$BASEMSG" ${MSGFILE}  &
    	    echo mqttsample_array publish process started: $! : publish_process_count= $publish_process_count
            set -v
    
        fi
      fi
    fi
   
     #svt_wait_60_for_processes_to_become_active ${now} ${qos} mqttsample_array ${scale_client}

    echo publish_process_count= $publish_process_count
    echo "`date`     svt_verify_condition \"All Clients Connected\" $publish_process_count \"${LOGP}.p.${qos}.${name}.*.${now}\"" 0 mqttsample_array 0 $publish_process_count
 
    svt_verify_condition "All Clients Connected" $publish_process_count "${LOGP}.p.${qos}.${name}.*.${now}" 0 mqttsample_array 0 $publish_process_count 
        
    let x=${SVT_SUBNET_LIST_TOTAL};
    do_publish_connect_sync $x
    #
    killall -12 mqttsample_array # signal waiting publishers to continue with SIGUSR2 : TODO: change this to touching a file.
    #----------------------------------------
    
    do_quick_exit_check
        
    if ! [ -n "$SVT_AT_VARIATION_SKIP_PUBLISH" ] ;then
        svt_verify_condition "SVT_TEST_RESULT: SUCCESS" 1 "${LOGP}.p.${qos}.${name}.1.${now}"
        touch ./${LOGP}.p.${qos}.${name}.${now}.complete   # used with -x verifyPubComplete setting to show that all publishing is complete.
    else
        touch ./${LOGP}.p.${qos}.${name}.${now}.complete   # used with -x verifyPubComplete setting to show that all publishing is complete.
    fi

    if [ -z "$SVT_AT_VARIATION_SKIP_SUBSCRIBE" -o -n "$SVT_AT_VARIATION_VERIFY_SUBSCRIBE" ] ;then
        if [ -n "$SVT_AT_VARIATION_SUSPEND_SUBSCRIBERS" ] ;then
            echo "-----------------------------------------"
            echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
             "for : SVT_AT_VARIATION_SUSPEND_SUBSCRIBERS = $SVT_AT_VARIATION_SUSPEND_SUBSCRIBERS"
            echo "-----------------------------------------"
            echo "Killing all remaining apps now"
            killall -9 $TEST_APP
        fi
        if [ "$SVT_AT_VARIATION_VERIFY_SUBSCRIBE" != "false" ] ;then 
            svt_verify_condition "SVT_TEST_RESULT: SUCCESS" $scale_client "${LOGP}.s.${qos}.${name}.*.${now}"
        fi
    fi
    
}

do_verify_subs(){
    local numconnected=${1-$scale_total}
    local numsubscribed=${2-$scale_total}
    local numallsubscribed=${3-$scale_client}
    local numclients=${4-$scale_client}
    local x

    if [ "$scale_count" != "0" ] ;then
        svt_wait_60_for_processes_to_become_active ${now} ${qos} mqttsample_array ${numclients}
    fi
    
    if [ -n "$SVT_AT_VARIATION_VERIFY_SUB_CON" -a "$SVT_AT_VARIATION_VERIFY_SUB_CON" == "false" ] ;then 
        echo "-----------------------------------------"
        echo " `date` SVT AUTOMATION OVERIDE: SVT_AT_VARIATION_VERIFY_SUB_CON: Will not verify that subscribers connected."
        echo "-----------------------------------------"
    else
        svt_verify_condition "Connected to " $numconnected "${LOGP}.s.${qos}.${name}.*.${now}" $numclients
    fi
    if [ -n "$SVT_AT_VARIATION_VERIFY_SUB" -a "$SVT_AT_VARIATION_VERIFY_SUB" == "false" ] ;then 
        echo "-----------------------------------------"
        echo " `date` SVT AUTOMATION OVERIDE: SVT_AT_VARIATION_VERIFY_SUB: Will not verify that subscribers subscribed"
        echo "-----------------------------------------"
    else
        svt_verify_condition "Subscribed - Ready" $numsubscribed "${LOGP}.s.${qos}.${name}.*.${now}" $numclients
        svt_verify_condition "All Clients Subscribed" $numallsubscribed "${LOGP}.s.${qos}.${name}.*.${now}" $numclients
    fi
   
    let x=(${SVT_SUBNET_LIST_TOTAL}*$numclients);
    do_subscribe_sync_check $x
  
    if [ -n "$SVT_AT_VARIATION_SUSPEND_SUBSCRIBERS" ] ;then
        echo "-----------------------------------------"
        echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
         "for : SVT_AT_VARIATION_SUSPEND_SUBSCRIBERS = $SVT_AT_VARIATION_SUSPEND_SUBSCRIBERS"
        echo "-----------------------------------------"
        killall -19 $TEST_APP
        sleep 30;
        echo "-----------------------------------------"
        echo " `date` SVT AUTOMATION OVERIDE: continuing."
        echo "-----------------------------------------"
    fi
}
do_verify_sub_complete(){
    local numconnected=${1-$scale_total}
    local numsubscribed=${2-$scale_total}
    local numallsubscribed=${3-$scale_client}
    local numclients=${4-$scale_client}
    if [ -z "$SVT_AT_VARIATION_SKIP_SUBSCRIBE" -o -n "$SVT_AT_VARIATION_VERIFY_SUBSCRIBE" ] ;then
        if [ -n "$SVT_AT_VARIATION_SUSPEND_SUBSCRIBERS" ] ;then
            echo "-----------------------------------------"
            echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
              "for : SVT_AT_VARIATION_SUSPEND_SUBSCRIBERS = $SVT_AT_VARIATION_SUSPEND_SUBSCRIBERS"
            echo "-----------------------------------------"
            echo "Killing all remaining apps now"
            killall -9 $TEST_APP
        fi
        if [ "$SVT_AT_VARIATION_VERIFY_SUBSCRIBE" != "false" ] ;then 
            svt_verify_condition "SVT_TEST_RESULT: SUCCESS" $numclients "${LOGP}.s.${qos}.${name}.*.${now}"
        fi
    fi
}

do_create_subs(){
    local flag=${1-"fanout"}
    local x
    local expected_msgs
    local maxclients
    local zcount

    if ! [ -n "$SVT_AT_VARIATION_SKIP_SUBSCRIBE" ] ;then
        if [ "$flag" == "fanout" ] ;then
            maxclients=$scale_client
            zcount=" -z $scale_connections_per_client "
            let expected_msgs=($scale_count*scale_connections_per_client)
        else
            maxclients=1
            zcount=""
            let expected_msgs=($scale_count*scale_connections_per_client*$scale_client)
        fi  
        let x=1;
        while (($x<=$maxclients)) ; do
            MYSUBFLAGS=" -o ${LOGP}.s.${qos}.${name}.${x}.${now} -a subscribe -n $scale_count "
            MYSUBFLAGS+=" -w $scale_rate -q $qos ${zcount} "
            MYSUBFLAGS+=" -i $scale_topic.$x.${unique_clientid}${h} -x subscribeOnConnect=0 "
            #--------------------------------------------------------------------------------------------------
            # 3.6.14 : Note : w/ 999900 subs + 20 pubs (1200 processes) on mar473, we saw that 
            # userReceiveTimeout=5000 increased throughput. 
            # This seems to be because when there are many processes, a 5000 mSec timeout prevents
            # a lot of timeouts on the subscriber side due to context switching 
            #--------------------------------------------------------------------------------------------------
            MYSUBFLAGS+=" ${verifyStillActive} -x userReceiveTimeout=5000 -x dynamicReceiveTimeout=1 " 
            MYSUBFLAGS+=" -x verifyPubComplete=./${LOGP}.p.${qos}.${name}.${now}.complete ${SUBFLAGS}  "
            MYSUBFLAGS+=" `print_test_criteria $expected_msgs ` "
            ADDNLFLAGS=`print_x_flags $x "subscribe" `
            MYCMD=" MQTT_C_CLIENT_TRACE= $TEST_APP " 
            MYCMD+=${BASEFLAGS}
            MYCMD+=${MYSUBFLAGS} 
            MYCMD+=${ADDNLFLAGS} 
            if [ -n "$SVT_AT_VARIATION_SHAREDSUB_MULTI" -a "$SVT_AT_VARIATION_SHAREDSUB_MULTI" == "scale" ] ; then
                echo "Using SVT_AT_VARIATION_SHAREDSUB_MULTI setting: $SVT_AT_VARIATION_SHAREDSUB_MULTI "
                MYCMD+=" -x sharedSubMulti=$scale_connections_per_client "
            elif [ -n "$SVT_AT_VARIATION_SHAREDSUB_MULTI" ]; then
                echo "Using SVT_AT_VARIATION_SHAREDSUB_MULTI setting verbatim: $SVT_AT_VARIATION_SHAREDSUB_MULTI "
                MYCMD+=" -x sharedSubMulti=$SVT_AT_VARIATION_SHAREDSUB_MULTI "
            fi
            if [ -n "$SVT_AT_VARIATION_SHAREDSUB" -a "$SVT_AT_VARIATION_SHAREDSUB" == "scale" ] ; then
                MYCMD+=" -x sharedSubscription=${scale_topic} "
            elif [ -n "$SVT_AT_VARIATION_SHAREDSUB" ]; then
                MYCMD+=" -x sharedSubscription=$SVT_AT_VARIATION_SHAREDSUB "
            fi
            if [ -n "$SVT_AT_VARIATION_TOPIC_MULTI_TEST" -a "$flag" == "fanin" ] ; then
                 MYCMD+=" -t /${scale_topic}/${h}/# "  # many pubs to 1 subscriber fanin
                 MYCMD+=" -x topicMultiTest= "  # For the subscriber we do not want this variable set
            else
                 MYCMD+=" -t /${scale_topic}/ "  # default topic for fanout
            fi
            MYCMD+=" -m \"$BASEMSG\" ${MSGFILE} & "
            echo $MYCMD
            echo $MYCMD | sh -x
            let x=$x+1;
        done
        if [ "$flag" == "fanout" ] ;then
            do_verify_subs  # let the function do the calculation
        else
            do_verify_subs 1 1 1 1  # fanin case
        fi
    fi 
}
do_create_pubs(){
    local z
    if ! [ -n "$SVT_AT_VARIATION_SKIP_PUBLISH" ] ;then
        let z=$scale_count*$scale_connections_per_client*$scale_client
        if [ -n "$SVT_AT_VARIATION_PUB_MSGCNT_OVERRIDE" ] ;then
            echo "-----------------------------------------"
            echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
              "for : SVT_AT_VARIATION_PUB_MSGCNT_OVERRIDE = $SVT_AT_VARIATION_PUB_MSGCNT_OVERRIDE"
            echo "-----------------------------------------"
            z=$SVT_AT_VARIATION_PUB_MSGCNT_OVERRIDE
        fi
        if [ -n "$SVT_AT_VARIATION_SHAREDSUB_PCT_TOTAL_MSGS" ] ; then
            echo "-----------------------------------------"
            echo " `date` SVT AUTOMATION OVERIDE: $SVT_AT_VARIATION_SHAREDSUB_PCT_TOTAL_MSGS"
            echo " `date` old z: $z"
            let z=$z*${SVT_AT_VARIATION_SHAREDSUB_PCT_TOTAL_MSGS}/100
            echo " `date` new z: $z"
            echo "-----------------------------------------"
        fi
        ADDNLFLAGS=`print_x_flags $x "publish" `
        MYCMD="MQTT_C_CLIENT_TRACE= $TEST_APP -o ${LOGP}.p.${qos}.${name}.1.${now} "
        MYCMD+=" -n $z -w $pubrate "
        MYCMD+=" ${PUBFLAGS} ${BASEFLAGS} ${ADDNLFLAGS} -a publish " 
        #MYCMD+=" -t /${scale_topic}/${h} " 
        MYCMD+=" -t /${scale_topic}/ " 
        MYCMD+=" -m \"$BASEMSG\"  ${MSGFILE} & "
        echo $MYCMD
        echo $MYCMD | sh -x
        svt_verify_condition "SVT_TEST_RESULT: SUCCESS" 1 "${LOGP}.p.${qos}.${name}.1.${now}"
        #next file is used with -x verifyPubComplete setting : all publishing is complete.
        touch ./${LOGP}.p.${qos}.${name}.${now}.complete   
    else
        #next file is used with -x verifyPubComplete setting : all publishing is complete.
        touch ./${LOGP}.p.${qos}.${name}.${now}.complete  
    fi
}
do_create_subs2(){
    local flag=${1-"fanin"}
    local x
    local z
    local expected_msgs
    local maxclients
    local zcount

    if ! [ -n "$SVT_AT_VARIATION_SKIP_SUBSCRIBE" ] ;then
        if [ "$flag" == "fanout" ] ;then
            maxclients=$scale_client
            zcount=" -z $scale_connections_per_client "
            let expected_msgs=($scale_count*scale_connections_per_client)
            let z=$scale_count
        else
            maxclients=1
            zcount=""
            #let expected_msgs=($scale_count*scale_connections_per_client*$scale_client/$M_COUNT)
            #let z=$scale_count*$scale_connections_per_client*$scale_client/$M_COUNT
            let expected_msgs=($scale_count*scale_connections_per_client*$scale_client)
            let z=$scale_count*$scale_connections_per_client*$scale_client
        fi  
        if [ -n "$SVT_AT_VARIATION_SHAREDSUB_PCT_TOTAL_MSGS" ] ; then
            #echo "-----------------------------------------"
            #echo -n " `date` SVT AUTOMATION OVERIDE: $SVT_AT_VARIATION_SHAREDSUB_PCT_TOTAL_MSGS"
            #echo "not yet implemented for fanin"
            #echo "-----------------------------------------"
            echo "-----------------------------------------"
            echo " `date` SVT AUTOMATION OVERIDE: $SVT_AT_VARIATION_SHAREDSUB_PCT_TOTAL_MSGS"
            echo " `date` old z: $z"
            let z=$z*${SVT_AT_VARIATION_SHAREDSUB_PCT_TOTAL_MSGS}/100
            echo " `date` new z: $z"
            echo "-----------------------------------------"
            #let expected_msgs=$expected_msgs*${SVT_AT_VARIATION_SHAREDSUB_PCT_TOTAL_MSGS}/100
        fi
        let x=1;
        while (($x<=$maxclients)) ; do
            MYSUBFLAGS=" -o ${LOGP}.s.${qos}.${name}.${x}.${now} -a subscribe -n $z "
            MYSUBFLAGS+=" -w $scale_rate -q $qos ${zcount} "
            MYSUBFLAGS+=" -i s.$scale_topic.$x.${unique_clientid}${h} -x subscribeOnConnect=0 "
            #--------------------------------------------------------------------------------------------------
            # 3.6.14 : Note : w/ 999900 subs + 20 pubs (1200 processes) on mar473, we saw that 
            # userReceiveTimeout=5000 increased throughput. 
            # This seems to be because when there are many processes, a 5000 mSec timeout prevents
            # a lot of timeouts on the subscriber side due to context switching 
            #--------------------------------------------------------------------------------------------------
            MYSUBFLAGS+=" ${verifyStillActive} -x userReceiveTimeout=5000 -x dynamicReceiveTimeout=1 " 
            MYSUBFLAGS+=" -x verifyPubComplete=./${LOGP}.p.${qos}.${name}.${now}.complete ${SUBFLAGS}  "
            MYSUBFLAGS+=" `print_test_criteria $expected_msgs ` "
            ADDNLFLAGS=`print_x_flags $x "subscribe" `
            #if [ "$scale_topic" == "17" -a "$x" == "1" ] ;then
                #MYCMD=" MQTT_C_CLIENT_TRACE=ON $TEST_APP " 
            #else
                MYCMD=" MQTT_C_CLIENT_TRACE= $TEST_APP " 
            #fi
            MYCMD+=${BASEFLAGS}
            MYCMD+=${MYSUBFLAGS} 
            MYCMD+=${ADDNLFLAGS} 
            if [ -n "$SVT_AT_VARIATION_SHAREDSUB_MULTI" -a "$SVT_AT_VARIATION_SHAREDSUB_MULTI" == "scale" ] ; then
                echo "Using SVT_AT_VARIATION_SHAREDSUB_MULTI setting: $SVT_AT_VARIATION_SHAREDSUB_MULTI "
                MYCMD+=" -x sharedSubMulti=$scale_connections_per_client "
            elif [ -n "$SVT_AT_VARIATION_SHAREDSUB_MULTI" ]; then
                echo "Using SVT_AT_VARIATION_SHAREDSUB_MULTI setting verbatim: $SVT_AT_VARIATION_SHAREDSUB_MULTI "
                MYCMD+=" -x sharedSubMulti=$SVT_AT_VARIATION_SHAREDSUB_MULTI "
            fi
            if [ -n "$SVT_AT_VARIATION_SHAREDSUB" -a "$SVT_AT_VARIATION_SHAREDSUB" == "scale" ] ; then
                MYCMD+=" -x sharedSubscription=${scale_topic} "
            elif [ -n "$SVT_AT_VARIATION_SHAREDSUB" ]; then
                MYCMD+=" -x sharedSubscription=$SVT_AT_VARIATION_SHAREDSUB "
            fi
            if [ -n "$SVT_AT_VARIATION_TOPIC_MULTI_TEST" -a "$flag" == "fanin" ] ; then
                 MYCMD+=" -t /${scale_topic}/# "  # many pubs to 1 subscriber fanin
                 MYCMD+=" -x topicMultiTest= "  # For the subscriber we do not want this variable set
            else
                 MYCMD+=" -t /${scale_topic}/${h} "  # default topic for fanout
            fi
            #if [ "$scale_topic" == "17" -a "$x" == "1" ] ;then
                #MYCMD+=" -m \"$BASEMSG\" ${MSGFILE} > ${LOGP}.s.${qos}.${name}.${x}.${now}.mqtt & "
            #else
                MYCMD+=" -m \"$BASEMSG : $scale_topic\" ${MSGFILE} & "
            #fi
            echo $MYCMD
            echo $MYCMD | sh -x
            let x=$x+1;
        done
        if [ "$flag" == "fanout" ] ;then
            do_verify_subs  # let the function do the calculation
        else
            do_verify_subs 1 1 1 1  # fanin case
        fi
    fi 
}
do_create_pubs2(){
    local flag=${1-"fanin"}
    local x
    local z
    local maxclients
    local zcount

    if ! [ -n "$SVT_AT_VARIATION_SKIP_PUBLISH" ] ;then
        if [ "$flag" == "fanout" ] ;then
            maxclients=1
            zcount=""
            let z=$scale_count*$scale_connections_per_client*$scale_client
        else
            maxclients=$scale_client
            zcount=" -z $scale_connections_per_client "
            let z=$scale_count
        fi  
        if [ -n "$SVT_AT_VARIATION_PUB_MSGCNT_OVERRIDE" ] ;then
            echo "-----------------------------------------"
            echo " `date` SVT AUTOMATION OVERIDE: Will use overide setting " \
            "for : SVT_AT_VARIATION_PUB_MSGCNT_OVERRIDE = $SVT_AT_VARIATION_PUB_MSGCNT_OVERRIDE"
            echo "-----------------------------------------"
            z=$SVT_AT_VARIATION_PUB_MSGCNT_OVERRIDE
        fi
        if [ -n "$SVT_AT_VARIATION_SHAREDSUB_PCT_TOTAL_MSGS" ] ; then
            echo "-----------------------------------------"
            echo " `date` SVT AUTOMATION OVERIDE: $SVT_AT_VARIATION_SHAREDSUB_PCT_TOTAL_MSGS"
            echo " `date` old z: $z"
            let z=$z*${SVT_AT_VARIATION_SHAREDSUB_PCT_TOTAL_MSGS}/100
            echo " `date` new z: $z"
            echo "-----------------------------------------"
        fi
        let x=1;
        while (($x<=$maxclients)) ; do
            ADDNLFLAGS=`print_x_flags $x "publish" `
            MYCMD="MQTT_C_CLIENT_TRACE= $TEST_APP -o ${LOGP}.p.${qos}.${name}.$x.${now} "
            MYCMD+=" -i p.$scale_topic.$x.${unique_clientid}${h} "
            MYCMD+=" -n $z $zcount -w $pubrate "
            #MYCMD+=" ${PUBFLAGS} ${BASEFLAGS} ${ADDNLFLAGS} -a publish_wait " 
            MYCMD+=" ${PUBFLAGS} ${BASEFLAGS} ${ADDNLFLAGS} -a publish " 
            MYCMD+=" -t /${scale_topic}/${h} " 
            MYCMD+=" -m \"$BASEMSG : $scale_topic\"  ${MSGFILE} & "
            echo $MYCMD
            echo $MYCMD | sh -x
            let x=$x+1;
        done

        echo "--------------------------------------------"
        echo "`date` : Continuing with svt_verify_condition on publishers " \
         "for condition All Clients Connected"
        echo "--------------------------------------------"  

        #----------------------------------------
        # The following lines will align the publishers to all publish at the same time..
        # Unfortuneately this can overwhelm the client and cause kernel: BUG soft lockup ....
        #
        # Note: to reenable this you also need to turn on "publish_wait" above, as the action.
        #----------------------------------------
        #svt_verify_condition "All Clients Connected" $maxclients "${LOGP}.p.${qos}.${name}.*.${now}" 
        #
        #        let x=(${SVT_SUBNET_LIST_TOTAL}*$maxclients);
        #        do_publish_connect_sync $x
        #
        #        killall -12 mqttsample_array # signal waiting publishers to continue with SIGUSR2 : TODO: change this to touching a file.
        #----------------------------------------

        do_quick_exit_check

        svt_verify_condition "SVT_TEST_RESULT: SUCCESS" $maxclients "${LOGP}.p.${qos}.${name}.*.${now}"
        #next file is used with -x verifyPubComplete setting : all publishing is complete.
        touch ./${LOGP}.p.${qos}.${name}.${now}.complete   
    else
        #next file is used with -x verifyPubComplete setting : all publishing is complete.
        touch ./${LOGP}.p.${qos}.${name}.${now}.complete  
    fi
}

#---------------------------------
# Start a fan-out shared messaging pattern
#---------------------------------
do_fanout_shared(){

    do_create_subs	

    do_create_pubs
    
    do_quick_exit_check
        
    do_verify_sub_complete
}


#---------------------------------
# Start a fan-out shared messaging pattern
#---------------------------------
do_fanin_shared(){

    do_create_subs2	

    do_create_pubs2
    
    do_quick_exit_check
        
    do_verify_sub_complete 1 1 1 1
}


#---------------------------------
# Start a fan-in messaging pattern
#---------------------------------
do_fanin(){
    local x
    local mypubflags
    local expected_msgs
    local unique_topic

    if [ -n "$unique" ] ;then
        unique_topic="${unique}" ; # bucnhes of subscribers share same topic
    else
        unique_topic="" ; # bucnhes of subscribers share same topic
    fi

    if ! [ -n "$SVT_AT_VARIATION_SKIP_SUBSCRIBE" ] ;then
    
        let x=1;
        let expected_msgs=($scale_count*scale_connections_per_client*$scale_client)
        MYSUBFLAGS=" -o ${LOGP}.s.${qos}.${name}.1.${now} -t /${scale_topic}/${h}${unique_topic} -a subscribe -n $expected_msgs -w $scale_rate -q $qos -i $scale_topic.1.$h -x subscribeOnConnect=1 ${verifyStillActive} -x userReceiveTimeout=5 -x dynamicReceiveTimeout=1  -x subscribeOnConnect=1 -x verifyPubComplete=./${LOGP}.p.${qos}.${name}.${now}.complete ${SUBFLAGS} "
    
        MYSUBFLAGS+=" `print_test_criteria $expected_msgs ` "
    
        ADDNLFLAGS=`print_x_flags $x "subscribe" `
    
        if [ -n "$SVT_AT_VARIATION_TOPICCHANGE" ] ; then
            if [ "$SVT_AT_VARIATION_TOPICCHANGE" == "1" ] ;then
                ADDNLFLAGS+=" -x topicChangeTest=0 " ; # actually we don't want this enabled... what we really want is....
                ADDNLFLAGS+=" -t /${scale_topic}/${h}/#" # what we really want is to be subscribed to /#
            fi
        fi
    
        set -v
        MQTT_C_CLIENT_TRACE= $TEST_APP ${MYSUBFLAGS} ${BASEFLAGS} ${ADDNLFLAGS} -m "$BASEMSG" ${MSGFILE}  &
        set +v
    
        if [ "$scale_count" != "0" ] ;then
            svt_wait_60_for_processes_to_become_active ${now} ${qos} mqttsample_array 1
        fi
        svt_verify_condition "Connected to " 1 "${LOGP}.s.${qos}.${name}.*.${now}" $scale_client
        svt_verify_condition "Subscribed - Ready" 1 "${LOGP}.s.${qos}.${name}.*.${now}" $scale_client
        svt_verify_condition "All Clients Subscribed" 1 "${LOGP}.s.${qos}.${name}.*.${now}" $scale_client
        
        do_subscribe_sync_check 20 20
    fi

    if ! [ -n "$SVT_AT_VARIATION_SKIP_PUBLISH" ] ;then
        
        let x=1;
        while (($x<=$scale_client)) ; do
        
            if [ -n "$SVT_AT_VARIATION_TOPICCHANGE" ] ; then 
                #TODO Take out these yayyyyy
                unique_topic="${unique}${x}" ;   # each subscriber to a completely different topic
            else
                if [ -n "$unique" ] ;then
                    unique_topic="${unique}" ; # bucnhes of subscribers share same topic
                else
                    unique_topic="" ; # bucnhes of subscribers share same topic
                fi
            fi 
    
            mypubflags=" $PUBFLAGS -o ${LOGP}.p.${qos}.${name}.$x.${now} -a publish_wait -t /${scale_topic}/${h}${unique_topic} -z $scale_connections_per_client  -n $scale_count -w $pubrate -i $scale_topic.$x.$h"

            ADDNLFLAGS=`print_x_flags $x "publish" `
        
            if [ "$SVT_AT_VARIATION_69925_VAL" == "LIMIT_NUM_MSG" ] ;then
                echo "`date` -----------------------------"
                echo "`date` SVT_AT_VARIATION_69925_VAL OVERRIDE: special override for 17.sh test. "
                echo "`date` limit message number to what actually was published + buffer"
                echo "`date` -----------------------------"
                max=`grep -h actual_msgs *.log log.* | awk '{print $2}' | sort -nr  | head -n 1`
                echo "`date` -----------------------------"
                echo "`date` SVT_AT_VARIATION_69925_VAL OVERRIDE: max is $max scale_connections_per_client is $scale_connections_per_client"
                echo "`date` -----------------------------"
                actual=`echo $max $scale_connections_per_client | awk '{printf("%d", ($1/$2)*3.0);}'`
                ADDNLFLAGS+=" -n $actual "
                echo "`date` -----------------------------"
                echo "`date` SVT_AT_VARIATION_69925_VAL OVERRIDE: limitted to -n $actual "
                echo "`date` -----------------------------"
            fi

    
            set -v
            echo "Running command: MQTT_C_CLIENT_TRACE= $TEST_APP ${mypubflags} ${BASEFLAGS} ${ADDNLFLAGS} -m \"$BASEMSG\" ${MSGFILE}  &"
            MQTT_C_CLIENT_TRACE= $TEST_APP ${mypubflags} ${BASEFLAGS} ${ADDNLFLAGS} -m "$BASEMSG" ${MSGFILE}  &
            set +v
    
            let x=$x+1;
        done
    
        echo "--------------------------------------------"   
        echo "`date` : Continuing with svt_verify_condition on publishers " \
         "for condition All Clients Connected"  
        echo "--------------------------------------------"   
    
        svt_verify_condition "All Clients Connected" $scale_client "${LOGP}.p.${qos}.${name}.*.${now}" $scale_client
   
		if (($SVT_SUBNET_LIST_TOTAL<20)); then 
			let x=(${SVT_SUBNET_LIST_TOTAL}*$scale_client);
		else
        	let x=(20*$scale_client);
		fi
        do_publish_connect_sync $x
    
        killall -12 mqttsample_array # signal waiting publishers to continue with SIGUSR2 : TODO: change this to touching a file.
    
        do_quick_exit_check

        svt_verify_condition "SVT_TEST_RESULT: SUCCESS" $scale_client "${LOGP}.p.${qos}.${name}.1.${now}"
    fi

    touch ./${LOGP}.p.${qos}.${name}.${now}.complete   # used with -x verifyPubComplete setting to show that all publishing is complete.

    if ! [ "$SVT_AT_VARIATION_SKIP_SUBSCRIBE" ] ;then
        svt_verify_condition "SVT_TEST_RESULT: SUCCESS" $scale_client "${LOGP}.s.${qos}.${name}.*.${now}"
    fi
    
}

do_connblast(){
    let x=1;
    while (($x<=$scale_client)) ; do

        mypubflags=" $PUBFLAGS -o ${LOGP}.p.${qos}.${name}.$x.${now} -a publish_wait -t ${scale_topic}/${h}${unique_topic} -z $scale_connections_per_client -w $pubrate -n 0 "

        ADDNLFLAGS=`print_x_flags $x `

        set -v
        MQTT_C_CLIENT_TRACE= $TEST_APP ${mypubflags} ${BASEFLAGS} ${ADDNLFLAGS} -m "$BASEMSG" ${MSGFILE} &
        set +v

        let x=$x+1;
    done
    svt_verify_condition "All Clients Connected" $scale_client "${LOGP}.p.${qos}.${name}.*.${now}" $scale_client
    let x=(20*$scale_client);
    do_publish_connect_sync $x
    killall -12 mqttsample_array # signal waiting publishers to continue with SIGUSR2 : TODO: change this to touching a file.
}



do_connblast(){
    let x=1;
    while (($x<=$scale_client)) ; do

        mypubflags=" $PUBFLAGS -o ${LOGP}.p.${qos}.${name}.$x.${now} -a publish_wait -t ${scale_topic}/${h}${unique_topic} -z $scale_connections_per_client -w $pubrate -n 0 "

        ADDNLFLAGS=`print_x_flags $x `

        set -v
        MQTT_C_CLIENT_TRACE= $TEST_APP ${mypubflags} ${BASEFLAGS} ${ADDNLFLAGS} -m "$BASEMSG" ${MSGFILE} &
        set +v

        let x=$x+1;
    done
    svt_verify_condition "All Clients Connected" $scale_client "${LOGP}.p.${qos}.${name}.*.${now}" $scale_client
    let x=(20*$scale_client);
    do_publish_connect_sync $x
    killall -12 mqttsample_array # signal waiting publishers to continue with SIGUSR2 : TODO: change this to touching a file.
}





#---------------------------------
# Main execution path run the selected messaging pattern
#---------------------------------

if [ "$PATTERN" == "fanout" ] ;then
    do_fanout
elif [ "$PATTERN" == "fanout_shared" ] ;then
    do_fanout_shared
elif [ "$PATTERN" == "fanin_shared" ] ;then
    do_fanin_shared
elif [ "$PATTERN" == "fanin" ] ;then
    do_fanin
elif [ "$PATTERN" == "connblast" ] ;then
    do_connblast
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


