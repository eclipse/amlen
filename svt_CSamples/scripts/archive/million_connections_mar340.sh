#!/bin/bash

#--------------------------------------------
# Required inputs
#--------------------------------------------
CONFIGURATION=${1-"ALL"} ; # Must be either SERVER , CLIENT, LAUNCH or ALL, or DISCONNECT , specifying what to configure.
IMA=${2-10.10.10.10}
IMAeth710net=${3-10.10.1.40}
IMAeth710net_END=${4-40}    # the last octet of the 

if [ "$CONFIGURATION" == "DISCONNECT" ] ; then
    echo "---------------------------------------"
    echo " Disconnect all clients. Sending two sigints to mqttsample_array clients tells them to do a graceful shutdown."
    echo "---------------------------------------"
    killall -2 mqttsample_array
    killall -2 mqttsample_array
    exit 0;
fi

echo "---------------------------------------"
echo " This test must be run on a client system with" 
echo " certain capabilities (As of 2.19.13 - "
echo " mar168, mar191 have been tested to work ) "
echo "---------------------------------------"

sleep 3;

if [ "$CONFIGURATION" == "ALL" -o "$CONFIGURATION" == "CLIENT" ] ; then
echo "---------------------------------------"
echo " Verify and setup the client system to run this test."
echo "---------------------------------------"

echo "---------------------------------------"
echo " Check that eth2 is 10 Gb Ethernet.    "
echo "---------------------------------------"
ethtool eth2 |grep Speed |grep 10000 > /dev/null || { echo "ERROR: Please run this from a system with eth2 Speed: 10000Mb/s. Exitting."  ; exit 1; }

echo "---------------------------------------"
echo " Check that eth2 is 10.10 network.    "
echo "---------------------------------------"
ifconfig eth2 |grep -oE "inet addr:.* "  | sed "s/Bcast.*//g" | grep 10.10 >/dev/null || { echo "ERROR: Please run this from a system eth2 on 10.10 network. Exitting."  ; exit 1; }

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


if [ "$CONFIGURATION" == "ALL" -o "$CONFIGURATION" == "SERVER" ] ; then
echo "---------------------------------------"
echo " Verify and setup the server to run this test."
echo "---------------------------------------"

echo "---------------------------------------"
echo " Check that eth7 is enabled on $IMA"
echo "---------------------------------------"
ssh admin@$IMA show ethernet-interface eth7 | grep AdminState | grep Enabled > /dev/null || { echo "ERROR: eth7 is not Enabled on $IMA1. Please configure 40GbE . Exitting."  ; exit 1; }

echo "---------------------------------------"
echo " Check that eth7 is 40GbE on $IMA"
echo "---------------------------------------"
ssh admin@$IMA status ethernet-interface eth7 |grep speed:40000 > /dev/null  || { echo "ERROR: eth7 is not configured as 40GbE on $IMA1. Please configure 40GbE . Exitting."  ; exit 1; }

echo "---------------------------------------"
echo " Check that eth7 is configured with $IMAeth710net"
echo "---------------------------------------"
ssh admin@$IMA show ethernet-interface eth7 | grep address |grep $IMAeth710net > /dev/null  || { echo "ERROR: eth7 not configured with  $IMAeth710net . Exitting."  ; exit 1; }

echo "---------------------------------------"
echo " Reset ip on eth7 on $IMA"
echo "---------------------------------------"
ssh admin@$IMA "edit ethernet-interface eth7; reset ip ; exit"  || { echo "ERROR: eth7 problems reset ip on $IMA . Exitting."  ; exit 1; }

echo "---------------------------------------"
echo " Configure eth7 on $IMA - no error checking on this part."
echo "---------------------------------------"
ssh admin@$IMA "edit ethernet-interface eth7; ip ; address ${IMAeth710net}/23; exit ; exit; "
for addy in {2..20}; do 
	echo "Configure address 10.10.$addy.${IMAeth710net_END}/23" ; 
	ssh admin@$IMA "edit ethernet-interface eth7; ip ; address 10.10.$addy.${IMAeth710net_END}/23; exit ; exit; "   
done

echo "---------------------------------------"
echo " Enable eth7 on $IMA"
echo "---------------------------------------"
ssh admin@$IMA "enable ethernet-interface eth7 "  || { echo "ERROR: could not enable eth7 on $IMA . Exitting."  ; exit 1; }

fi


if [ "$CONFIGURATION" == "ALL" -o "$CONFIGURATION" == "CLIENT" ] ; then

echo "---------------------------------------"
echo " Configure 19 virtual NICs on client"
echo "---------------------------------------"
for i in {2..20} ; do ifconfig eth2:$i 10.10.$i.168 netmask 255.255.254.0 up ; done


echo "---------------------------------------"
echo " Wait / Check that all 19 vitual nic interfaces are up."
echo "---------------------------------------"
let ret=1; while (($ret>0)); do  let ret=0; for x in {2..20} ; do ping -c 1 -w 1 10.10.$x.40 ; rc=$? ; echo RC=$rc ; let ret=$ret+$rc ; done  ; echo ret is $ret; sleep 1; done

fi

if [ "$CONFIGURATION" == "ALL" -o "$CONFIGURATION" == "LAUNCH" ] ; then

    echo "---------------------------------------"
    echo " fan out to 980*60*17 connections - 999600 connections total. Stop just before 1M "
    echo "---------------------------------------"
    for x in {2..18} ; do ./fan_out.sh $x  980 60 100 0 0 10.10.$x.40:16102 log.s.$x > log.fa.$x & done
fi

