#!/bin/bash

if [[ -f "../testEnv.sh" ]] ; then
    source "../testEnv.sh"
else
    source ../scripts/ISMsetup.sh
fi

WASAddress=`echo ${WASIP} | cut -d':' -f 1`

srvdef=`cat server.definition`

if [[ "$1" == "all" ]] ; then
    if [[ "${srvdef}" =~ "SINGLE" ]] ; then
        echo "rm -f ${WASPath}/profiles/AppSrv01/logs/server1/SystemOut.log.bz2" | ssh ${WASAddress}
    	echo "bzip2 -k -9 ${WASPath}/profiles/AppSrv01/logs/server1/SystemOut.log" | ssh ${WASAddress}
        scp ${WASAddress}:${WASPath}/profiles/AppSrv01/logs/server1/SystemOut.log.bz2 ./SystemOut.server1.log.bz2.log
        
        scp ${WASAddress}:/WAS/evilra.trace ./evilra.log
    else
        cluster=`echo $srvdef | cut -d';' -f 2-`
        IFS=$';' read -a array <<< $cluster
        for node in ${array} ; do

            echo "rm -f ${WASPath}/profiles/AppSrv01/logs/server1/SystemOut.log.bz2" | ssh ${WASAddress}
            echo "bzip2 -k -9 ${WASPath}/profiles/AppSrv01/logs/server1/SystemOut.log" | ssh ${WASAddress}
            scp ${node}:${WASPath}/profiles/AppSrv01/logs/server1/SystemOut.log.bz2 ./SystemOut.server1.log.bz2.log

            echo "rm -f ${WASPath}/profiles/AppSrv01/logs/server2/SystemOut.log.bz2" | ssh ${WASAddress}
            echo "bzip2 -k -9 ${WASPath}/profiles/AppSrv01/logs/server2/SystemOut.log" | ssh ${WASAddress}
            scp ${node}:${WASPath}/profiles/AppSrv01/logs/server2/SystemOut.log.bz2 ./SystemOut.server2.log.bz2.log
            
            echo "rm -f ${WASPath}/profiles/AppSrv01/logs/server3/SystemOut.log.bz2" | ssh ${node}
            echo "bzip2 -k -9 ${WASPath}/profiles/AppSrv01/logs/server3/SystemOut.log" | ssh ${node}
            scp ${node}:${WASPath}/profiles/AppSrv01/logs/server3/SystemOut.log.bz2 ./SystemOut.server3.log.bz2.log
            
            echo "rm -f ${WASPath}/profiles/AppSrv02/logs/server3/SystemOut.log.bz2" | ssh ${node}
            echo "bzip2 -k -9 ${WASPath}/profiles/AppSrv02/logs/server3/SystemOut.log" | ssh ${node}
            scp ${node}:${WASPath}/profiles/AppSrv02/logs/server3/SystemOut.log.bz2 ./SystemOut.server3.log.bz2.log
            
            scp ${node}:/WAS/evilra.trace ./evilra.${node}.log
        
        done
    fi

    ssh ${WASAddress} ${WASPath}/bin/wsadmin.sh -lang jython -user admin -password admin -f /WAS/resumeEndpoints.py
elif [[ "$1" == "trace" ]] ; then
    if [[ "${srvdef}" =~ "SINGLE" ]] ; then
        echo "rm -f ${WASPath}/profiles/AppSrv01/logs/server1/SystemOut.log.bz2" | ssh ${WASAddress}
    	echo "bzip2 -k -9 ${WASPath}/profiles/AppSrv01/logs/server1/SystemOut.log" | ssh ${WASAddress}
        scp ${WASAddress}:${WASPath}/profiles/AppSrv01/logs/server1/SystemOut.log.bz2 ./SystemOut.server1.log.bz2.log
        
        scp ${WASAddress}:/WAS/evilra.trace ./evilra.log
    else
        cluster=`echo $srvdef | cut -d';' -f 2-`
        IFS=$';' read -a array <<< $cluster
        for node in ${array} ; do

            echo "rm -f ${WASPath}/profiles/AppSrv01/logs/server1/SystemOut.log.bz2" | ssh ${WASAddress}
            echo "bzip2 -k -9 ${WASPath}/profiles/AppSrv01/logs/server1/SystemOut.log" | ssh ${WASAddress}
            scp ${node}:${WASPath}/profiles/AppSrv01/logs/server1/SystemOut.log.bz2 ./SystemOut.server1.log.bz2.log

            echo "rm -f ${WASPath}/profiles/AppSrv01/logs/server2/SystemOut.log.bz2" | ssh ${WASAddress}
            echo "bzip2 -k -9 ${WASPath}/profiles/AppSrv01/logs/server2/SystemOut.log" | ssh ${WASAddress}
            scp ${node}:${WASPath}/profiles/AppSrv01/logs/server2/SystemOut.log.bz2 ./SystemOut.server2.log.bz2.log
            
            echo "rm -f ${WASPath}/profiles/AppSrv01/logs/server3/SystemOut.log.bz2" | ssh ${node}
            echo "bzip2 -k -9 ${WASPath}/profiles/AppSrv01/logs/server3/SystemOut.log" | ssh ${node}
            scp ${node}:${WASPath}/profiles/AppSrv01/logs/server3/SystemOut.log.bz2 ./SystemOut.server3.log.bz2.log
            
            echo "rm -f ${WASPath}/profiles/AppSrv02/logs/server3/SystemOut.log.bz2" | ssh ${node}
            echo "bzip2 -k -9 ${WASPath}/profiles/AppSrv02/logs/server3/SystemOut.log" | ssh ${node}
            scp ${node}:${WASPath}/profiles/AppSrv02/logs/server3/SystemOut.log.bz2 ./SystemOut.server3.log.bz2.log
            
            scp ${node}:/WAS/evilra.trace ./evilra.${node}.log
        
        done
    fi
else
    ssh ${WASAddress} ${WASPath}/bin/wsadmin.sh -lang jython -user admin -password admin -f /WAS/resumeEndpoints.py
fi
