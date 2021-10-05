ima1=10.10.10.10
ima2=10.10.10.10


#local ip=$1
#local last_octet
#local regex="([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})"
#if [[ $ip =~ $regex ]] ;then
    #echo ${BASH_REMATCH[4]}
#else
    #
#fi

ssh admin@$ima1 "disable ethernet-interface eth7; edit ethernet-interface eth7 ; reset ip; exit "
ssh admin@$ima1 "edit ethernet-interface eth6 ; reset ip; ip; address 10.10.1.39/23 ; exit ; exit; enable ethernet-interface eth6 "
ssh admin@$ima1 "edit ethernet-interface ha0 ; reset ip; ip; address 10.11.1.39/23 ; exit ; exit; enable ethernet-interface ha0 "
ssh admin@$ima1 "edit ethernet-interface ha1 ; reset ip; ip; address 10.12.1.39/23 ; exit ; exit; enable ethernet-interface ha1 "


imaserver update HighAvailability "EnableHA=False" "RemoteDiscoveryNIC=10.12.1.40" "LocalDiscoveryNIC=10.12.1.39" "LocalReplicationNIC=10.11.1.39"
imaserver runmode clean_store
imaserver stop mqconnectivity 
imaserver stop 
imaserver start 
imaserver start mqconnectivity 
####################################### wait for it to get to Maintenance
imaserver runmode production
imaserver stop
imaserver start 
####################################### wait for it to get to Running 
imaserver update HighAvailability "EnableHA=True" "RemoteDiscoveryNIC=10.12.1.40" "LocalDiscoveryNIC=10.12.1.39" "LocalReplicationNIC=10.11.1.39"
imaserver stop mqconnectivity 
imaserver stop 
imaserver start 
imaserver start mqconnectivity 


ssh admin@$ima2 "disable ethernet-interface eth7; edit ethernet-interface eth7 ; reset ip; exit "
ssh admin@$ima2 "edit ethernet-interface eth6 ; reset ip; ip; address 10.10.1.40/23 ; exit ; exit; enable ethernet-interface eth6 "
ssh admin@$ima2 "edit ethernet-interface ha0 ; reset ip; ip; address 10.11.1.40/23 ; exit ; exit; enable ethernet-interface ha0"
ssh admin@$ima2 "edit ethernet-interface ha1 ; reset ip; ip; address 10.12.1.40/23 ; exit ; exit; enable ethernet-interface ha1"


imaserver update HighAvailability "EnableHA=False" "RemoteDiscoveryNIC=10.12.1.39" "LocalDiscoveryNIC=10.12.1.40" "LocalReplicationNIC=10.11.1.40" 
imaserver runmode clean_store
imaserver stop mqconnectivity 
imaserver stop 
imaserver start 
imaserver start mqconnectivity 
imaserver runmode production
imaserver stop
imaserver start 
imaserver update HighAvailability "EnableHA=True" "RemoteDiscoveryNIC=10.12.1.39" "LocalDiscoveryNIC=10.12.1.40" "LocalReplicationNIC=10.11.1.40" 
imaserver stop mqconnectivity 
imaserver stop 
imaserver start 
imaserver start mqconnectivity 


imaserver set TestLicense
user sshkey add "scp://root@10.10.10.10:~/.ssh/authorized_keys"
file get "scp://root@10.10.10.10:~/.ssh/id_sample_harness_sshkey" identity
imaserver apply identity identity
