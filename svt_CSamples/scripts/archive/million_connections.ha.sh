#!/bin/bash
#--------------------------------------------
# This script configures the client and server to launch a million MQTT C client connections.
# 
# For the simplicity of the script, multiple somewhat 
# redundant paramemters are required.
# There are 3 required parameters.
# 
# arg1: ACTION: ALL, SERVER, CLIENT, LAUNCH, or DISCONNECT
# arg2: Server 9.x: The 9.x.x.x address of the target appliance
# arg3: Server 10.x: The 10.x.x.x address of the target appliance
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

usage(){

echo "There are 3 required inputs."
echo ""
echo "arg1: ACTION      : ALL, SERVER, CLIENT, LAUNCH, or DISCONNECT"
echo "arg2: SERVER 9.x  : The 9.x.x.x address of the target appliance"
echo "arg3: SERVER 10.x : The 10.x.x.x address of the target appliance"

}

#--------------------------------------------
# Required inputs.
#--------------------------------------------
ACTION=${1} ; # Must be either SERVER , CLIENT, LAUNCH or ALL, or DISCONNECT , specifying what to configure.
IMA=${2}
IMAeth610net=${3}

HA_END="39"

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
regex="(9)\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})"
if ! [[ $IMA =~ $regex ]] ;then         
    echo "ERROR: It seems that argument 2 is not a valid 9.x.x.x ip address"
    usage;
    exit 1;
fi

#--------------------------------------------
# Check arg3
#--------------------------------------------
regex="(10)\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})"
if [[ $IMAeth610net =~ $regex ]] ;then         
    IMAeth610net_END="${BASH_REMATCH[4]}";
    echo "---------------------------------------"
    echo " Last octet for server is $IMAeth610net_END"
    echo "---------------------------------------"
else
    echo "ERROR: It seems that argument 3 is not a valid 10.x.x.x ip address"
    usage;
    exit 1;
fi

client_ip=`hostname -i`
client_ip_last_octet=""
regex="([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})"
echo "---------------------------------------"
echo " client ip is $client_ip"
echo "---------------------------------------"
if [[ $client_ip =~ $regex ]] ;then         
    client_ip_last_octet="${BASH_REMATCH[4]}";
    echo "---------------------------------------"
    echo " Last octet for client is $client_ip_last_octet"
    echo "---------------------------------------"
    
    echo hiya
else

    echo what

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
    echo " Kill any fan_out.sh scripts still running"
    echo "---------------------------------------"
    killall -9 fan_out.sh

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
    echo " Check that eth0 is 10 Gb Ethernet. SKIP    "
    echo "---------------------------------------"
    #ethtool eth0 |grep Speed |grep 10000 > /dev/null || { echo "ERROR: Please run this from a system with eth0 Speed: 10000Mb/s. Exitting."  ; exit 1; }
    
    echo "---------------------------------------"
    echo " Check that eth0 is 10.10 network.  SKIP  "
    echo "---------------------------------------"
    #ifconfig eth0 |grep -oE "inet addr:.* "  | sed "s/Bcast.*//g" | grep 10.10 >/dev/null || { echo "ERROR: Please run this from a system eth0 on 10.10 network. Exitting."  ; exit 1; }
    
    echo "---------------------------------------"
    echo " Check that ./fan_out.sh script exists."
    echo "---------------------------------------"
    test -e ./fan_out.sh  || { echo "ERROR: Please run this from directory where ./fan_out.sh exists. Exitting."  ; exit 1; }
    
    echo "---------------------------------------"
    echo " Turn off firewall..........iptables.  "
    echo "---------------------------------------"
    /sbin/chkconfig iptables off || { echo "ERROR: Problem with /sbin/chkconfig iptables off. Exitting."  ; exit 1; }
    
    echo "---------------------------------------"
    echo " Turn off firewall..........ip6tables. "
    echo "---------------------------------------"
    /sbin/chkconfig ip6tables off || { echo "ERROR: Problem with /sbin/chkconfig ip6tables off. Exitting."  ; exit 1; }
    
    echo "---------------------------------------"
    echo " Set sysctl net.nf_conntrack_max=2000000"
    echo "---------------------------------------"
    sysctl net.nf_conntrack_max=2000000 || { echo "ERROR: Problem with sysctl net.nf_conntrack_max=2000000 . Exitting."  ; exit 1; }
fi


if [ "$ACTION" == "ALL" -o "$ACTION" == "SERVER" ] ; then
    echo "---------------------------------------"
    echo " Verify and setup the server to run this test."
    echo "---------------------------------------"

    echo "---------------------------------------"
    echo " Check that eth6 is enabled on $IMA"
    echo "---------------------------------------"
    ssh admin@$IMA show ethernet-interface eth6 | grep AdminState | grep Enabled > /dev/null || { echo "ERROR: eth6 is not Enabled on $IMA1. Please configure 40GbE . Exitting."  ; exit 1; }
    
    echo "---------------------------------------"
    echo " Check that eth6 is 40GbE on $IMA   -------     SKIP     -------"
    echo "---------------------------------------"
    #ssh admin@$IMA status ethernet-interface eth6 |grep speed:40000 > /dev/null  || { echo "ERROR: eth6 is not configured as 40GbE on $IMA1. Please configure 40GbE . Exitting."  ; exit 1; }
    
    echo "---------------------------------------"
    echo " Check that eth6 is configured with $IMAeth610net"
    echo "---------------------------------------"
    ssh admin@$IMA show ethernet-interface eth6 | grep address |grep $IMAeth610net > /dev/null  || { echo "ERROR: eth6 not configured with  $IMAeth610net . Exitting."  ; exit 1; }
    
    echo "---------------------------------------"
    echo " Reset ip on eth6 on $IMA"
    echo "---------------------------------------"
    ssh admin@$IMA "edit ethernet-interface eth6; reset ip ; exit"  || { echo "ERROR: eth6 problems reset ip on $IMA . Exitting."  ; exit 1; }
    
    echo "---------------------------------------"
    echo " Configure eth6 on $IMA - no error checking on this part."
    echo "---------------------------------------"
    ssh admin@$IMA "edit ethernet-interface eth6; ip ; address ${IMAeth610net}/23; exit ; exit; "
    for addy in {2..20}; do 
       echo "Configure address 10.10.$addy.${IMAeth610net_END}/23" ; 
       ssh admin@$IMA "edit ethernet-interface eth6; ip ; address 10.10.$addy.${IMAeth610net_END}/23; exit ; exit; "   
    done

    echo "---------------------------------------"
    echo " Enable eth6 on $IMA"
    echo "---------------------------------------"
    ssh admin@$IMA "enable ethernet-interface eth6 "  || { echo "ERROR: could not enable eth6 on $IMA . Exitting."  ; exit 1; }

fi


if [ "$ACTION" == "ALL" -o "$ACTION" == "CLIENT" ] ; then

    echo "---------------------------------------"
    echo " Configure 19 virtual NICs on client"
    echo "---------------------------------------"
    for i in {2..20} ; do ifconfig eth0:$i 10.10.$i.$client_ip_last_octet netmask 255.255.254.0 up ; done


    echo "---------------------------------------"
    echo " Wait / Check that all 19 vitual nic interfaces are up."
    echo "---------------------------------------"
    let ret=1; while (($ret>0)); do  let ret=0; for x in {2..20} ; do ping -c 1 -w 1 10.10.$x.${IMAeth610net_END} ; rc=$? ; echo RC=$rc ; let ret=$ret+$rc ; done  ; echo ret is $ret; sleep 1; done

fi

if [ "$ACTION" == "ALL" -o "$ACTION" == "LAUNCH" ] ; then

    echo "---------------------------------------"
    echo " fan out to 980*60*17 connections - 999600 connections total. Stop just before 1M "
    echo "---------------------------------------"

    for x in {2..18} ; do
	 ./fan_out.sh $x  980 60 100 0 0 10.10.$x.${IMAeth610net_END}:16106 log.s.$x 10.10.$x.${HA_END}:16106 > log.fa.$x & 
    done

    echo "---------------------------------------"
    echo " Clients launched. Please monitor log.fa.* or web GUI for status on connections"
    echo "---------------------------------------"
fi


echo "---------------------------------------"
echo " Completed all expected processing.    "
echo "---------------------------------------"
