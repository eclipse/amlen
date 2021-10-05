#!/bin/bash


rm -rf log.archive.mdbmessages.*

do_test(){
    local parm1=${1-100}
    local parm2=${2-100}
    local parm3=${3-1}
    local success
    local success2

#------------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------------
# success2 is needed because sometimes this happens (expected 2000 below, but it never printed out)
#------------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------------
#ID: SUB_e68f1b12_88c4_4468_ | Connected | RESPONSE/RUNNER068aa39af0bd0004d/# | Msgs: 1999
#ID: PUB_a2ff9490_2cb4_4868_ | Finished | REQUESTTOPIC | Msgs: 2000
# LOG  : Unsubscribing subscriber
# LOG  : All susbcribers are complete.
# LOG  : Endgame is complete.
#
#************************************************************************
#*
#* Tearing down
#*
#************************************************************************
#
# LOG  : FINISHED
# LOG  : Requested to close down monitor.
#------------------------------------------------------------------------------------------------



    local pids
    local waitpid
    local startt
    local curt

    rm -rf log.mdbmessages.*
    let x=0
    waitpid=""
    while(($x<$parm1)) ; do
#        echo "java -jar mdbmessages.jar 10.10.1.125 10.10.1.126 16102 $parm2 0 $parm3 1>>log.mdbmessages.$x 2>>log.mdbmessages.$x  &"
        java -jar mdbmessages.jar 10.10.1.125 10.10.1.126 16102 $parm2 0 $parm3 1>>log.mdbmessages.$x 2>>log.mdbmessages.$x  &
        waitpid+=" $! "
        #echo -n "$x: "
        #cat /proc/meminfo |grep MemFree
        let x=$x+1
    done

    # ------------------------------------
    # Note cannot wait for process to finish because of this issue:  Instead we wait for all messages to be recieved and from that point
    # if things don't clean up in 60 seconds assume that things are done.
    # ------------------------------------
    # 
    #
    #LOG  :
    #ID: SUB_4600914d_99e0_40f0_ | Connected | RESPONSE/RUNNER0ca21f24f09e7004b/# | Msgs: 987
    #ID: PUB_5d8f431d_7580_4a2a_ | Connected | REQUESTTOPIC | Msgs: 987
    #LOG  : Starting the endgame!
    #LOG  :
    #ID: SUB_4600914d_99e0_40f0_ | Connected | RESPONSE/RUNNER0ca21f24f09e7004b/# | Msgs: 1000
    #ID: PUB_5d8f431d_7580_4a2a_ | Finished | REQUESTTOPIC | Msgs: 1000
    #LOG  : Unsubscribing subscriber
    #LOG  : SUB_4600914d_99e0_40f0_: Failed to unsubscribe!
    #LOG  : SUB_4600914d_99e0_40f0_: Connection lost with cause "Connection lost" Reason code 32109" Cause "java.net.SocketException: Connection reset". Reattempting to connect...
    #Connection lost (32109) - java.net.SocketException: Connection reset
        #at org.eclipse.paho.client.mqttv3.internal.CommsReceiver.run(CommsReceiver.java:138)
        #at java.lang.Thread.run(Thread.java:777)
    #Caused by: java.net.SocketException: Connection reset
        #at java.net.SocketInputStream.read(SocketInputStream.java:200)
        #at java.net.SocketInputStream.read(SocketInputStream.java:132)
        #at java.net.SocketInputStream.read(SocketInputStream.java:214)
        #at java.io.DataInputStream.readByte(DataInputStream.java:276)
        #at org.eclipse.paho.client.mqttv3.internal.wire.MqttInputStream.readMqttWireMessage(MqttInputStream.java:51)
        #at org.eclipse.paho.client.mqttv3.internal.CommsReceiver.run(CommsReceiver.java:100)
        #... 1 more
    #LOG  : SUB_4600914d_99e0_40f0_: Publisher set as BAD!!!
    #LOG  : SUB_4600914d_99e0_40f0_: Connection lost with cause "Unable to connect to server" Reason code 32103" Cause "java.net.ConnectException: Connection refused". Reattempting to connect...
    #LOG  : SUB_4600914d_99e0_40f0_: Connection successful
    #LOG  : SUB_4600914d_99e0_40f0_: Connection successful
    #
    #<< hanging here>>
    #
    # ------------------------------------
    #echo "`date` wait $waitpid"
    #wait $waitpid

    echo "`date` waiting for $parm2 messages to be received $parm1 times"
    #success=`grep SUB log.mdbmessages.* | grep SUB_ | grep "Msgs: ${parm2}" |wc -l`
    success=`find log.mdbmessages.* | xargs -i echo "cat {} | grep SUB_ | grep \"Msgs: ${parm2}\" | head -1 " |sh |wc -l`

    while(($success!=$parm1)); do
        success=`find log.mdbmessages.* | xargs -i echo "cat {} | grep SUB_ | grep \"Msgs: ${parm2}\" | head -1 " | sh | wc -l`
        success2=`find log.mdbmessages.* | xargs -i echo "cat {} | grep SUB_ | grep \"LOG  : All susbcribers are complete.\" | head -1 " | sh | wc -l`
        if (($success2>$success)); then
            echo "`date` .  WARNING: Overriding success:$success with success2:$success2"
            success=$success2
        fi

        sleep 5
    done

    #echo "`date` waiting for $parm2 messages to be received"
    
    pids=`ps $waitpid | grep java |wc -l`
    echo "`date` waiting for processes to complete: $pids"
    startt=`date +%s`
    while(($pids>0)); do
        curt=`date +%s`
        let totalt=($curt-$startt)
        if (($totalt>60)); then 
            echo "`date` .  Issuing Kill to $waitpid"
            kill -9 $waitpid
        fi
        pids=`ps $waitpid | grep java |wc -l`
        sleep 1
        echo "`date` waiting for processes to complete: $pids"
    done
 

    success=`find log.mdbmessages.* | xargs -i echo "cat {} | grep SUB_ | grep \"Msgs: ${parm2}\" | head -1 " | sh |wc -l`
        success2=`find log.mdbmessages.* | xargs -i echo "cat {} | grep SUB_ | grep \"LOG  : All susbcribers are complete.\" | head -1 " | sh | wc -l`
        if (($success2>$success)); then
            echo "`date` .  WARNING: Overriding success:$success with success2:$success2"
            success=$success2
        fi

    echo "`date` .  Success. All processes have completed. Receved messages found:$success times. (expected:$parm1)"
    
   # success=`grep FINISHED log.mdbmessages.* | wc -l ` 
    if [ "$success" == "$parm1" ] ;then
        echo "`date` Success $success"
        return 0;
    else
        echo "`date` Failure $success"
        return 1;
    fi
    
    return 1; 
}

let passcount=0
let failcount=0
let delsubcount=0
let iteration=1
while((1)); do
    do_test 10 2000 1
    rc=$?
    if [ "$rc" == "0" ]; then
        let passcount=$passcount+1
    else 
        let failcount=$failcount+1
    fi

    #if /niagara/test/scripts/svt-delsubs.sh 10.10.1.125  | grep "imaserver delete Subscription" > /dev/null; then
        #echo "`date` ERROR: had to delete sub on .20"
        #let delsubcount=$delsubcount+1
    #fi
    #if /niagara/test/scripts/svt-delsubs.sh 10.10.1.126  | grep "imaserver delete Subscription" > /dev/null; then
        #echo "`date` ERROR: had to delete sub on .91"
        #let delsubcount=$delsubcount+1
    #fi

#    /niagara/test/scripts/svt-delsubs.sh 10.10.1.126  2>/dev/null 1>/dev/null
#    /niagara/test/scripts/svt-delsubs.sh 10.10.1.125  2>/dev/null 1>/dev/null

    echo "#############################################################"
    echo "`date` RESULT iteration:$iteration: passcount:$passcount failcount:$failcount "
    echo "#############################################################"
    #echo "`date` RESULT iteration:$iteration: passcount:$passcount failcount:$failcount " #delsubcount:$delsubcount "
    
    tar -czvf log.archive.mdbmessages.$iteration.`date +%s`.tgz log.mdbmessages.*

#echo "`date` : iteration:$iteration Wait for failover to finish this iteration"
#while((1)); do
    #if [ -e log.failover.${iteration} ] ;then
        #echo "found  log.failover.${iteration} exists"
        #break;
    #fi
#done

echo "`date`  : iteration:$iteration Done."

    let iteration=$iteration+1
    
done



#-------------------------------------------------------
# New issue to handle w/ 100 concurrent:  ADD These two and report an error?
#-------------------------------------------------------
#
#root@mar473 /niagara/test/svt_cmqtt> grep "Giving up waiting for subscribers" *
#log.mdbmessages.32: WARN : Giving up waiting for subscribers to complete.
#log.mdbmessages.55: WARN : Giving up waiting for subscribers to complete.
#log.mdbmessages.72: WARN : Giving up waiting for subscribers to complete.
#log.mdbmessages.74: WARN : Giving up waiting for subscribers to complete.
#log.mdbmessages.84: WARN : Giving up waiting for subscribers to complete.
#log.mdbmessages.91: WARN : Giving up waiting for subscribers to complete.
#
#root@mar473 /niagara/test/svt_cmqtt> grep "Giving up waiting for subscribers" * |wc
      #6      60     444
#
#root@mar473 /niagara/test/svt_cmqtt> grep "Msgs: 1000" * |wc
  #17537  157835 1509140
#
#root@mar473 /niagara/test/svt_cmqtt> grep "Msgs: 1000" log.mdbmessages.* |wc
  #17535  157815 1508947
#
#root@mar473 /niagara/test/svt_cmqtt> grep "Msgs: 1000^Clog.mdbmessages.* |wc
#
#root@mar473 /niagara/test/svt_cmqtt> vi log.mdbmessages.0
#
#root@mar473 /niagara/test/svt_cmqtt> grep "Msgs: 1000" log.mdbmessages.* | grep SUB|wc
     #94     846   10236
#
#root@mar473 /niagara/test/svt_cmqtt> grep "Msgs: 1000" log.mdbmessages.* | grep SUB|wc -l
#94

