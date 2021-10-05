#!/bin/bash
#--------------------------------------------
# This script configures the client and server to launch a million MQTT C client connections.
# 
# For the simplicity of the script, multiple somewhat 
# redundant paramemters are required.
# There are 3 required parameters.
# 
# arg1: ACTION: ALL, SERVER, CLIENT, LAUNCH, or DISCONNECT
# arg2: Server 10.x: The 10.x.x.x address of the target appliance
# arg3: Optional HA argument. The x last octet of ha appliance (usually the standby appliance).
#
# Example usage: (Basic Usage)
#
# - configure the server,client,and launch the clients
# ./million_connections.sh ALL 10.10.10.10 10.10.1.40 
#
# - Disconnect all the clients.
# ./million_connections.sh LAUNCH 10.10.10.10 10.10.1.40 
#
# Example usage: (Advanced Usage)
#
# - configure just the server side for the test
# ./million_connections.sh SERVER 10.10.10.10 10.10.1.40 
#
# - configure just the client side for the test
# ./million_connections.sh CLIENT 10.10.10.10 10.10.1.40 
#
# - Just launch the clients. (assuming configuration already done)
# ./million_connections.sh LAUNCH 10.10.10.10 10.10.1.40 
#
#--------------------------------------------

if [[ -f "/niagara/test/testEnv.sh" ]] ;then
    source /niagara/test/testEnv.sh
elif [[ -f "/niagara/test/scripts/ISMsetup.sh" ]] ;then
    source /niagara/test/scripts/ISMsetup.sh
fi

. ./svt_test.sh

rc=0;

# TODO AUTOMATION uses /20 and 240
# TODO USUALLY i used /23 and 254

# - problems with this list 9, 11 ,13 , and 15 on mar516 -  not sure what that is
#SUBNET_LIST="9 11 13 15 17 19 21 23 25 27 29 31 33 35 37 39 41"
ODD_SUBNET_LIST="17 19 21 23 25 27 29 31 33 35 37 39 41 43 45 47 49 51 53 55"
EVEN_SUBNET_LIST="18 20 22 24 26 28 30 32 34 36 38 40 42 44 46 48 50 52 54 56"
#------------------------------------------------
# Wierd. I found that i needed to configure an additional interface on the
# client in order for the edges to ping ok. THus, I will add these additional
#------------------------------------------------
ODD_SUBNET_ENDS="15                                                       57 "
EVEN_SUBNET_ENDS="16                                                       58 "

CLIENT_SUBNET_LIST="$ODD_SUBNET_LIST"
CLIENT_SUBNET_ENDS="$ODD_SUBNET_ENDS"
SERVER_SUBNET_LIST="$ODD_SUBNET_LIST"
SUBNET_LIST_TOTAL=20 ; # how many items are in SUBNET_LIST
usage(){

echo "There are 3 required inputs."
echo ""
echo "arg1: ACTION           : ALL, SERVER, CLIENT, LAUNCH, or DISCONNECT"
echo "arg2: SERVER 10.x.x.x  : The 10.x.x.x address of the target appliance"
echo "arg3: HASERVER      x  : OPTIONAL: last octet of HA server"
echo "arg4: NUMCLIENTS    x  : OPTIONAL: The number of clients to launch : Default 833000"
echo "arg5: PUBRATE       x  : OPTIONAL: The publish rate  Default: 0.033"
echo "arg6: PORT          x  : OPTIONAL: The port          Default: 16102"
echo "arg7: WORKLOAD      x  : OPTIONAL: The workload name Default: connecitions "

}

#--------------------------------------------
# Required inputs.
#--------------------------------------------
ACTION=${1} ; # Must be either SERVER , CLIENT, LAUNCH or ALL, or DISCONNECT , specifying what to configure.
IMA=${2}
HA_SERVER=${3-"NULL"}
HA_END="NULL"
NUMCLIENTS=${4-833000}
PUBRATE=${5-0.033}
PORT=${6-16102}
WORKLOAD=${7-"connections"}

if [ -n "$SVT_ADMIN_IP" ] ;then
	ADMIN_CMD="ssh admin@$SVT_ADMIN_IP"
else
	ADMIN_CMD="ssh admin@$IMA"
fi

    
if [ "${A1_HOST_OS:0:4}" != "CCI_" ] ;then

	ethX=`do_find_10_dot_interface $IMA`
	rc=$?
	if [ "$rc" != "0" ] ; then
    	echo "ERROR: Could not find an interface with 10.10.[01].X network enabled on the target IMA $IMA. Exitting."
    	exit 1;
	else
    	echo "Success found ethX where 10. is enabled - $ethX"
	fi
	
	client_ethX=`ifconfig |grep -C2 10.10.[01] |grep -oE 'eth.*|br.*' | awk '{print $1}' | head -1`
	if ! echo "$client_ethX" |grep -E 'eth.*|br.*' > /dev/null ;then
    	echo "ERROR: Could not find a client interface with 10.10.[01].X network enabled on the client. Exitting."
    	exit 1;
	fi

fi

#--------------------------------------------
# Check arg1
#--------------------------------------------
if ! [  "$ACTION" == "ALL" -o "$ACTION" == "SERVER" -o "$ACTION" == "CLIENT" -o "$ACTION" == "LAUNCH" -o "$ACTION" == "DISCONNECT" ] ;then
    if ! echo "$ACTION" | grep LAUNCH > /dev/null ; then
        echo "ERROR: It seems that argument 1 is not a valid. Please specify a valid action. "
        usage;
        exit 1; 
    fi
fi

#--------------------------------------------
# Check arg2
#--------------------------------------------
regex="(10)\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})"
if [[ $IMA =~ $regex ]] ;then         
    IMA_third_octet="${BASH_REMATCH[3]}";
    IMA_END="${BASH_REMATCH[4]}";
    echo "---------------------------------------"
    echo " Last octet for server is $IMA_END"
    echo "---------------------------------------"
else

    if [ "${A1_HOST_OS:0:4}" != "CCI_" ] ; then
    	echo "ERROR: It seems that argument 2 is not a valid 10.x.x.x ip address"
    	usage;
    	exit 1;
	else
    	echo "WARN: It seems that argument 2 is not a 10.x.x.x ip address"
	fi
fi


#--------------------------------------------
# Check arg3
#--------------------------------------------
if [ "$HA_SERVER" != "NULL" ] ;then
    regex="(10)\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})"
    if [[ $HA_SERVER =~ $regex ]] ;then         
        HA_third_octet="${BASH_REMATCH[3]}";
        HA_END="${BASH_REMATCH[4]}";
        echo "---------------------------------------"
        echo " Last octet for server is $IMA_END"
        echo "---------------------------------------"
    else
    if [ ${A1_HOST_OS:0:4} != "CCI_" ] ; then
    	echo "ERROR: It seems that argument 3 is not a valid 10.x.x.x ip address"
    	usage;
    	exit 1;
	else
    	echo "WARN: It seems that argument 3 is not a 10.x.x.x ip address"
	HA_END="$HA_SERVER" ; 
	fi
    fi
    
    if [ "$IMA_third_octet" != "$HA_third_octet" ] ; then
        echo "WARNING: For this test, it is preferred that both the HA primary and backup must be on the same subnet. Please provide two appliances on same subnet."
        echo "WARNING: we will give it a try anyway, not sure if it will work."
        #exit 1;
    fi
fi
    
if [ "$IMA_third_octet" == "179" ] ; then
    echo "---------------------------------------"
    echo " For 179 servers use EVEN Subnet list "
    echo "---------------------------------------"
    SERVER_SUBNET_LIST="$EVEN_SUBNET_LIST"
fi


client_ip=`hostname -i`
#client_ip="10.10.10.10"
client_ip_third_octet=""
client_ip_last_octet=""
regex="([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})"
echo "---------------------------------------"
echo " client ip is $client_ip"
echo "---------------------------------------"
if [[ $client_ip =~ $regex ]] ;then     
    client_ip_third_octet="${BASH_REMATCH[3]}";
    client_ip_last_octet="${BASH_REMATCH[4]}";
    echo "---------------------------------------"
    echo " Third octet for client is $client_ip_third_octet"
    echo " Last octet for client is $client_ip_last_octet"
    echo "---------------------------------------"
    
else

    echo "---------------------------------------"
    echo " ERROR: Could not determine last octet of client."
    echo "---------------------------------------"
    exit 1;
fi

#--------------------------------------------
# Start processing.
#--------------------------------------------

if [ "$ACTION" == "DISCONNECT" ] ; then
    echo "---------------------------------------"
    echo " Disconnect all clients. Sending a sigint to mqttsample_array clients tells them to do a graceful shutdown."
    echo "---------------------------------------"
    killall -2 mqttsample_array

    echo "---------------------------------------"
    echo " Kill any SVT.patterns.sh scripts still running"
    echo "---------------------------------------"
    killall -9 SVT.patterns.sh

fi

if [ "$ACTION" == "ALL" -o "$ACTION" == "CLIENT" ] ; then
    echo "---------------------------------------"
    echo " This test must be run on a client system with" 
    echo " certain capabilities (As of 2.19.13 - "
    echo " mar168, mar191 have been tested to work ok ) "
    echo "---------------------------------------"
    
    sleep 3;
    
    echo "---------------------------------------"
    echo " Verify and setup the client system to run this test."
    echo "---------------------------------------"
    
    echo "---------------------------------------"
    echo " Check that ./SVT.patterns.sh script exists."
    echo "---------------------------------------"
    test -e ./SVT.patterns.sh  || { echo "ERROR: Please run this from directory where ./SVT.patterns.sh exists. Exitting."  ; exit 1; }
    
    echo "---------------------------------------"
    echo " Turn off firewall..........iptables.  "
    echo "---------------------------------------"
    /sbin/chkconfig iptables off || { echo "ERROR: Problem with /sbin/chkconfig iptables off. Exitting."  ; exit 1; }
    
    echo "---------------------------------------"
    echo " Turn off firewall..........ip6tables. "
    echo "---------------------------------------"
    /sbin/chkconfig ip6tables off || { echo "ERROR: Problem with /sbin/chkconfig ip6tables off. Exitting."  ; exit 1; }
    

    echo "---------------------------------------"
    echo " Make sure ip_conntrack module is loaded, requred for net.nf_conntrack_max=2000000 setting below"
    echo "---------------------------------------"
    if ! lsmod | grep nf_conntrack >/dev/null 2>/dev/null ; then
        modprobe -b ip_conntrack >/dev/null 2>&1
    fi
 
    echo "---------------------------------------"
    echo " Set sysctl net.nf_conntrack_max=2000000"
    echo "---------------------------------------"
    sysctl net.nf_conntrack_max=2000000 || { echo "ERROR: Problem with sysctl net.nf_conntrack_max=2000000 . Exitting."  ; exit 1; }
fi


if [ "$ACTION" == "ALL" -o "$ACTION" == "SERVER" -a "${A1_HOST_OS:0:4}" != "CCI_" ] ; then
    echo "---------------------------------------"
    echo " Verify and setup the server to run this test."
    echo "---------------------------------------"

    echo "---------------------------------------"
    echo " Check that $ethX is enabled on $IMA"
    echo "---------------------------------------"
    sync
    $ADMIN_CMD "show ethernet-interface $ethX" | grep AdminState | grep Enabled > /dev/null || { echo "ERROR: $ethX is not Enabled on $IMA1. Please configure $ethX . Exitting."  ; exit 1; }
    
    echo "---------------------------------------"
    echo " Check that $ethX is 40GbE on $IMA   -------     SKIP     -------"
    echo "---------------------------------------"
    #$ADMIN_CMD "status ethernet-interface $ethX " |grep speed:40000 > /dev/null  || { echo "ERROR: $ethX is not configured as 40GbE on $IMA1. Please configure 40GbE . Exitting."  ; exit 1; }
    
    echo "---------------------------------------"
    echo " Check that $ethX is configured with $IMA"
    echo "---------------------------------------" 
    sync
    $ADMIN_CMD "show ethernet-interface $ethX" | grep address |grep $IMA > /dev/null  || { 
	echo "ERROR: $ethX not configured with  $IMA . Exitting."  ; 
	echo "FAILING command: $ADMIN_CMD \"show ethernet-interface $ethX\" | grep address |grep $IMA > /dev/null "
	echo "INFO:  $ADMIN_CMD show ethernet-interface $ethX"
	$ADMIN_CMD "show ethernet-interface $ethX" 
	echo "INFO:  $ADMIN_CMD list ethernet-interface "
	$ADMIN_CMD "list ethernet-interface" 
	echo "INFO:  $ADMIN_CMD status ethernet-interface "
	$ADMIN_CMD "status ethernet-interface" 
	exit 1; 
	}

    
   	#echo "---------------------------------------"
   	#echo " Reset ip on $ethX on $IMA - not required."
   	#echo "---------------------------------------"
   	#$ADMIN_CMD "edit ethernet-interface $ethX; reset ip ; exit"  || { echo "ERROR: $ethX problems reset ip on $IMA . Exitting."  ; exit 1; }
   	
   	echo "---------------------------------------"
   	echo " Configure $ethX on $IMA - no error checking on this part."
   	echo "---------------------------------------"
   	#$ADMIN_CMD "edit ethernet-interface $ethX; ip ; address ${IMA}/23; exit ; exit; "
   	#for addy in  3 5 7 9 11 13 15 17 19 21 23 25 27 29 31 33 35  ; do 
   	#my_pid=""
   	#let count=0;

   	echo "---------------------------------------"
   	echo " Configure $ethX on $IMA - Save and restore existing ip addresses to preserve order (RTC 59996)"
   	echo "---------------------------------------"
   	existing_ip_address=`$ADMIN_CMD show ethernet-interface $ethX | grep address | sed "s/address//g" | \
    sed -E 's/[[:space:]]+/\n/g' | grep "\"" | sed "s/\"//g" | awk '{printf(" %s " , $1);}'`

   	ssh_cmd="edit ethernet-interface $ethX; reset ip;  "
   	ssh_cmd+=" ip ; address $existing_ip_address "
   	for addy in  $SERVER_SUBNET_LIST; do
     	echo "----------------------------------------------------"
      	echo "`date` : Configure address 10.10.$addy.${IMA_END}/23" ; 
      	echo "----------------------------------------------------"
      	#$ADMIN_CMD "edit ethernet-interface $ethX; ip ; address 10.10.$addy.${IMA_END}/23; exit ; exit; "
      	ssh_cmd+=" 10.10.$addy.${IMA_END}/23 "

      	#my_pid+="$! " 
      	#let count=$count+1;
   		#if (($count>=8)); then
    	#echo "---------------------------------------"
    	#echo "`date` Start Wait for $count configurations to complete"
    	#echo "---------------------------------------"
    	#wait $my_pid
    	#echo "---------------------------------------"
    	#echo "`date` Complete. $count configurations  complete"
    	#echo "---------------------------------------"
		#my_pid="";
		#count=0;
		#fi
    done
    ssh_cmd+=" ;  exit ; exit; "
    echo "----------------------------------------------------"
    echo "`date` : Configure address 10.10.$addy.${IMA_END}/23 - running cmd $ssh_cmd" ;
    echo "----------------------------------------------------"
    $ADMIN_CMD "$ssh_cmd"


   	#echo "---------------------------------------"
   	#echo "`date` Start Wait for $count configurations to complete"
   	#echo "---------------------------------------"
   	#wait $my_pid
 	#echo "---------------------------------------"
  	#echo "`date` Complete. $count configurations  complete"
  	#echo "---------------------------------------"
	
   	echo "---------------------------------------"
   	echo " Enable $ethX on $IMA"
   	echo "---------------------------------------"
   	$ADMIN_CMD "enable ethernet-interface $ethX "  || { echo "ERROR: could not enable $ethX on $IMA . Exitting."  ; exit 1; }

fi


if [ "$ACTION" == "ALL" -o "$ACTION" == "CLIENT" -a "${A1_HOST_OS:0:4}" != "CCI_" ] ; then
    #if [ -e $M1_TESTROOT/testEnv.sh ] ;then
    #    echo "---------------------------------------"
    #    echo " Preserve exiting automation route on client so that 10.10.3. - 10.10.7 will still work"
    #    echo "---------------------------------------"  
    #    route add -net 10.10.0.0/20 ${client_ethX}
    #else
    #    echo "---------------------------------------"
    #    echo " Cleanup any /20 configuation for manual tests"
    #    echo "---------------------------------------"  
    #    route del -net 10.10.0.0/20 ${client_ethX}
    #fi
    
    if ! route -n  |grep 10.10.0.0 | grep 255.255.254.0 > /dev/null ; then
        echo "---------------------------------------"
        echo " Warning: Attempting to Configure route properly on client, this could be dangerous"
        echo "---------------------------------------" 
        echo "---------------------------------------"
        echo " Before"
        echo "---------------------------------------" 
        route -n
    
        route add -net 10.10.0.0/23 ${client_ethX}

        echo "---------------------------------------"
        echo " After "
        echo "---------------------------------------" 
        route -n
    fi


    if [ "$client_ip_third_octet" == "179" ] ; then
    	echo "---------------------------------------"
    	echo " For 179 clients use EVEN Subnet list"
    	echo "---------------------------------------"
        CLIENT_SUBNET_LIST="$EVEN_SUBNET_LIST"
        CLIENT_SUBNET_ENDS="$EVEN_SUBNET_ENDS"
    fi

    echo "---------------------------------------"
    echo " Configure 19 virtual NICs on client"
    echo "---------------------------------------"
    for i in  $CLIENT_SUBNET_LIST ; do 
        set -x
        ifconfig ${client_ethX}:$i 10.10.$i.$client_ip_last_octet netmask 255.255.254.0 up ; 
        set +x
    done

    echo "---------------------------------------"
    echo " 6.3.2013 - Configure 2 additional virtual NICs on client, this will prevent the first or last one from failing to ping (hopefully)."
    echo "---------------------------------------"
    for i in  $CLIENT_SUBNET_ENDS ; do 
        set -x
        ifconfig ${client_ethX}:$i 10.10.$i.$client_ip_last_octet netmask 255.255.254.0 up ; 
        set +x
    done

    echo "---------------------------------------"
    echo " Wait / Check that all 19 vitual nic interfaces are up."
    echo "---------------------------------------"
    let ret=1; while (($ret>0)); do  let ret=0; for x in $SERVER_SUBNET_LIST  ; do ping -c 1 -w 1 10.10.$x.${IMA_END} ; rc=$? ; echo RC=$rc ; let ret=$ret+$rc ; done  ; echo ret is $ret; sleep 1; done

fi

#--------------------------------------------------
# log prefix (LOGP) the prefix for all log files.
#--------------------------------------------------
LOGP=$(do_get_log_prefix)


if [ "$ACTION" == "ALL" -o "$ACTION" == "LAUNCH" ] ; then

    let subscribe_client=5
    if (($NUMCLIENTS>750000)); then
        let subscribe_client=60
    elif (($NUMCLIENTS>500000)); then
        let subscribe_client=45
    elif (($NUMCLIENTS>250000)); then
        let subscribe_client=30
    elif (($NUMCLIENTS>199800)); then
        let subscribe_client=15
    elif (($NUMCLIENTS>99900)); then
        let subscribe_client=10
    elif (($NUMCLIENTS<100)); then
        let subscribe_client=1
    fi

    if [ -n "$SVT_AT_VARIATION_CLIENT_FA_MULTIPLIER" ] ;then
        #------------------------
        # Note: Tradeoff here s that more processes may connect faster. Less processes may message faster.
        #------------------------
        echo "-----------------------------------------"
        echo " SVT AUTOMATION OVERIDE: Will use overide setting for : SVT_AT_VARIATION_CLIENT_FA_MULTIPLIER: $SVT_AT_VARIATION_CLIENT_FA_MULTIPLIER instead of default: $subscribe_client"
        echo "-----------------------------------------"
        subscribe_client="${SVT_AT_VARIATION_CLIENT_FA_MULTIPLIER}"
    else
        echo "-----------------------------------------"
        echo " `date` : Using default SVT_AT_VARIATION_CLIENT_FA_MULTIPLIER: $subscribe_client "
        echo "-----------------------------------------"
    fi

    let subscribe_connections_per_client=($subscribe_client*$SUBNET_LIST_TOTAL);
    let subscribe_connections_per_client=($NUMCLIENTS/$subscribe_connections_per_client);
    if (($subscribe_connections_per_client<1)); then
        echo "---------------------------------------"
        echo " Warning: minimum number of clients was requested. Defaulting to minimum = 1"
        echo "---------------------------------------" 
        subscribe_connections_per_client=1;
    fi
    params="$subscribe_connections_per_client $subscribe_client" 
    let total_connections=($subscribe_connections_per_client*$subscribe_client*$SUBNET_LIST_TOTAL);
    echo "---------------------------------------"
    echo " fan out to $params * $SUBNET_LIST_TOTAL connections - $total_connections connections total. "
    echo "---------------------------------------" 

    if (($NUMCLIENTS<=20)); then 
        export SUBNET_LIST_TOTAL=$NUMCLIENTS # for this case not all of the subnets will be used.
    fi
    

    my_pid=""
    my_counter=0
    my_env=""
    now=`date +%s`
    for x in $SERVER_SUBNET_LIST; do
        echo "Start fan out for subnet $x"
        my_env="SVT_SUBNET_LIST_TOTAL=$SUBNET_LIST_TOTAL SVT_PATTERN_COUNTER=$my_counter "
        echo "Start fan out for subnet $x, my_env is \"$my_env\" "
        export SVT_SUBNET_LIST_TOTAL=$SUBNET_LIST_TOTAL
        export SVT_PATTERN_COUNTER=$my_counter
	
    set -x
	if [ "${A1_HOST_OS:0:4}" == "CCI_" ] ;then
		primary_ip=$IMA
		secondary_ip=$HA_SERVER
	else
		primary_ip=10.10.$x.${IMA_END}
		secondary_ip=10.10.$x.${HA_END}
	fi
        if [ "$HA_END" != "NULL" ] ; then
             ./SVT.patterns.sh $x $params 3 0 0 ${primary_ip}:${PORT} ${WORKLOAD}.$x ${secondary_ip}:${PORT} $PUBRATE $WORKLOAD > ${LOGP}.fa.${WORKLOAD}.$x.$now & 
             connections_pid=$!
        else    
             ./SVT.patterns.sh $x $params 3 0 0 ${primary_ip}:${PORT} ${WORKLOAD}.$x "NULL" $PUBRATE $WORKLOAD > ${LOGP}.fa.${WORKLOAD}.$x.$now & 

             connections_pid=$!
        fi
        my_pid+="$connections_pid "
        let my_counter=$my_counter+1
        if (($my_counter>=$NUMCLIENTS)); then 
            # -------------------------------------------
            # This logic handles requests for very low numbers of clients 1 to 20
            # -------------------------------------------
            echo "Break: Have already launched $my_counter clients, $NUMCLIENTS requested. Did not use all available subnets"
            break; # we have already launched all the requested clients without using all the subnets
        fi
	if [ "${A1_HOST_OS:0:4}" == "CCI_" ] ;then
		echo "Incrementing PORT for softlayer: PORT: $PORT"
		let PORT=$PORT+1
	fi
    set +x
    done
    echo "---------------------------------------"
    echo " Clients launched. Please monitor ${LOGP}.connections.* or web GUI for status on connections"
    echo "---------------------------------------"

    echo "---------------------------------------"
    echo " `date` Wait for clients to complete "
    echo "---------------------------------------"  
  
    wait $my_pid
    rc=$?
    echo "---------------------------------------"
    echo " `date` Done - all clients complete. rc = $rc"
    echo "---------------------------------------"  

    echo "---------------------------------------"
    echo " Completed all expected processing. Returning rc :$rc   "
    echo "---------------------------------------"
    exit $rc

fi

echo "---------------------------------------"
echo " (Default Exit) Completed all expected processing. Returning rc :$rc   "
echo "---------------------------------------"

exit 0;
