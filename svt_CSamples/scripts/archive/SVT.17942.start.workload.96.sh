#!/bin/bash


workload=${1-"throughput"}
workload_subname=${2-""}
art3=${3-""}
ima1=${4-10.10.1.96}
ima1_9dot=${5-"10.10.10.10"}
#ima1=${4-10.10.1.39}
#ima1_9dot=${5-"10.10.10.10"}
ima2=${6-""}
ima2_9dot=${7-"10.10.10.10"}
ima2_9dot_lastoctet="40";
insecure_port=${8-16102}
secure_port=${9-16114}
client_certificate_port=${10-16118}

if [ -n "$ima1" -a -n "$ima2" ] ;then
	ima_security="${ima1}:${secure_port} ${ima2}:${secure_port}"
	ima_client_certificate="${ima1}:${client_certificate_port} ${ima2}:${client_certificate_port}"
	ima_throughput="${ima1}:${insecure_port} ${ima2}:${insecure_port} "
	ima_many="${ima1_9dot} ${ima1} ${ima2_9dot_lastoctet}" # TODO: Currently uses port 16102 hard coded
else
	ima_security="${ima1}:${secure_port} \"\" "
	ima_client_certificate="${ima1}:${client_certificate_port} \"\" "
	ima_throughput="${ima1}:${insecure_port} \"\" "
	ima_many="${ima1_9dot} ${ima1} \"\" " # TODO: Currently uses port 16102 hard coded
fi


let x=0; while(($x<1000));do
	my_pid=""
	echo "`date` start iteration $x" 

	if [ $workload == "security" -o  $workload == "all" ] ;then
		if ! [ -n "$workload_subname" ] ; then
			workload_subname="100 195" ; #default for this workload 19.5 K  connections * 3
		fi
		echo "------------------"
		echo " Start ssl workload : ${workload_subname} *3 connections doing 100 mesages each  on Qos 0 1 2"
		echo "------------------"
		./RTC.17942.c.sh ${workload_subname} 100 100 100 "0 1 2" "security" "" ${ima_security} 1 2>>log.security.$x 1>>log.security.$x &
		security_pid=$!
		my_pid+="$security_pid "
	fi

	if [ $workload == "throughput" -o  $workload == "all" ] ;then
		#------------------
		# Start messaging workload 8 connections doing 1 M messages each
		#------------------
		#./RTC.17942.c.sh 8 1 1000000 na na "0" "" "" ${ima1}:16102 ${ima2}:16102 2>>log.throughput.$x 1>>log.throughput.$x & 
		./RTC.17942.c.sh 8 1 1000000 na na "0" "" "" ${ima_throughput} 2>>log.throughput.$x 1>>log.throughput.$x & 
		throughput_pid=$!
		my_pid+="$throughput_pid "
	fi

	if [ $workload == "connections" -o  $workload == "all" ] ;then
		if ! [ -n "$workload_subname" ] ; then
			workload_subname=833000 ; #default for this workload 833000 connections
		fi
		echo "------------------"
		echo "Start connections workload $workload_subname connections , in 17 fan out patterns , with 17 publishers each publishing 1 message every 5 seconds"
		echo "Note: should supply current Primary as ima1"
		echo "------------------"
		./RTC.17942.many.connections.sh LAUNCH ${ima_many} ${workload_subname} 0.2 ${insecure_port} 2>>log.connections.$x 1>>log.connections.$x &
		connection_pid=$!
		my_pid+="$connection_pid "
	fi

    
    if [ $workload == "ldap" ] ;then

		echo "------------------"
		echo "Start ldap workload 1024 connectins."
		echo "------------------"
          ./RTC.25629.a.39.sh 32 32 &
		ldap_pid=$!
		my_pid+="$ldap_pid "
	fi


	if [ $workload == "client_certificate" -o  $workload == "all" ] ;then
		if ! [ -n "$workload_subname" ] ; then
			workload_subname="100 195" ; #default for this workload 19.5 K  connections * 3
		fi
		echo "------------------"
		echo " Start client certificate workload: $workload_subname * 3 connections doing 100 mesages each on Qos 0 1 2, 1 msg/second "
		echo "------------------"
		./RTC.17942.c.sh ${workload_subname} 100 100 100 "0 1 2" "client_certificate" "" ${ima_client_certificate} 1 2>>log.client_certificate.$x 1>>log.client_certificate.$x &
		client_certificate_pid=$!
		my_pid+="$client_certificate_pid "
	fi

	wait $my_pid
	
	echo "`date` end iteration $x" 
	let x=$x+1; 
done


