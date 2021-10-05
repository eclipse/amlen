#!/bin/bash
###############################################################################
# 
# Restarting WAS servers using wsadmin doesn't seem to be very reliable.
# This script will kill the java processes.
# 

ostype=`uname`
if [[ "${ostype}" =~ "CYGWIN" ]] ; then
	suffix=".bat"
else
	suffix=".sh"
fi

WASdir=/opt/IBM/WebSphere/AppServer

srvdefinition=`cat /WAS/server.definition`
if [[ "${srvdefinition}" =~ "SINGLE" ]] ; then
    # Kill -9 server1
    pid=`ps -ef | grep java | grep server1 | awk '{ print $2 }'`
	if [[ "${pid}" != "" ]] ; then
    	kill -9 $pid
    	result=`ps -ef | grep java | grep server1 | awk '{ print \$2 }'`
    	echo ${result}
	fi
	set -x
	# Clear osgi cache and tranlog
	echo "Clearing the OSGI cache"
	${WASdir}/bin/osgiCfgInit${suffix} -profile AppSrv01
	echo "Deleting the tranlog"
	rm -rf ${WASdir}/profiles/AppSrv01/tranlog/*
	echo "Deleting the server log"
    rm -rf ${WASdir}/profiles/AppSrv01/logs/server1/*
	set +x
	
    sleep 20

    pid=`ps -ef | grep java | grep server1 | awk '{ print $2 }'`
    if [[ "${pid}" == "" ]] ; then
        echo "Starting server1"
        ${WASdir}/bin/startServer${suffix} -profileName AppSrv01 server1
    fi

elif [[ "${srvdefinition}" =~ "CLUSTER" ]] ; then
    m2=`echo $srvdefinition | cut -d';' -f 3`
    echo "M2 Address: ${m2}"

    # Running on linux
    if [[ "${suffix}" == ".sh" ]] ; then
        # Kill -9 nodeagent AppSrv01
        echo "Killing nodeagent on AppSrv01"
        n1pid=`ps -ef | grep java | grep nodeagent | awk '{ print $2 }'`
        echo ${n1pid}
        if [[ "${n1pid}" != "" ]] ; then
                kill -9 ${n1pid}
                n1result=`ps -ef | grep java | grep nodeagent | awk '{ print $2 }'`
                if [[ "${n1result}" != "" ]] ; then
                        echo "nodeagent on AppSrv01 was not killed"
                fi
        fi

        # Kill -9 nodeagent AppSrv02
        echo "Killing nodeagent on AppSrv02"
        n2pid=`ssh root@${m2} ps -ef | grep java | grep nodeagent | awk '{ print \$2 }'`
        echo ${n2pid}
        if [[ "${n2pid}" != "" ]] ; then
            n2cmd=`ssh root@${m2} kill -9 ${n2pid}`
            n2result=`ssh root@${m2} ps -ef | grep java | grep nodeagent | awk '{ print \$2 }'`
            if [[ "${n2result}" != "" ]] ; then
                echo "nodeagent on AppSrv02 was not killed"
            fi
        fi

        # Kill -9 server1
        echo "Killing server1"
        s1pid=`ps -ef | grep java | grep server1 | awk '{ print $2 }'`
        echo ${s1pid}
        kill -9 $s1pid
        if [[ "${s1pid}" != "" ]] ; then
            s1result=`ps -ef | grep java | grep server1 | awk '{ print $2 }'`
            if [[ "${s1result}" != "" ]] ; then
                echo "Server1 was not killed"
            fi
        fi

        # Kill -9 server2
        echo "Killing server2"
        s2pid=`ps -ef | grep java | grep server2 | awk '{ print $2 }'`
        echo ${s2pid}
        if [[ "${s2pid}" != "" ]] ; then
            kill -9 $s2pid
            s2result=`ps -ef | grep java | grep server2 | awk '{ print $2 }'`
            if [[ "${s2result}" != "" ]] ; then
                echo "Server2 was not killed"
            fi
        fi

        # Kill -9 server3
        echo "Killing server3"
        s3pid=`ssh root@${m2} ps -ef | grep java | grep server3 | awk '{ print \$2 }'`
        echo ${s3pid}
        if [[ "${s3pid}" != "" ]] ; then
            s3cmd=`ssh root@${m2} kill -9 ${s3pid}`
            s3result=`ssh root@${m2} ps -ef | grep java | grep server3 | awk '{ print \$2 }'`
            if [[ "${s3result}" != "" ]] ; then
                echo "Server3 was not killed"
            fi
        fi

        # Clean cache on each machine
        echo "Cleaning AppSrv01 cache"
        ${WASdir}/bin/osgiCfgInit${suffix} -profile AppSrv01
        rm -rf ${WASdir}/profiles/AppSrv01/tranlog/*
        rm -rf ${WASdir}/profiles/AppSrv01/logs/server1/*
        rm -rf ${WASdir}/profiles/AppSrv01/logs/server2/*
        echo "Cleaning AppSrv02 cache"
        ssh root@${m2} ${WASdir}/bin/osgiCfgInit${suffix} -profile AppSrv02
        ssh root@${m2} rm -rf ${WASdir}/profiles/AppSrv02/tranlog/*
        ssh root@${m2} rm -rf ${WASdir}/profiles/AppSrv02/logs/server3/*

        # the servers auto restart when killed...
        # wait and make sure they are.
        # If they aren't, then just restart them.
        sleep 20

        echo "Starting nodeagent on AppSrv01"
        ${WASdir}/bin/startNode${suffix} -profileName AppSrv01
        n1pid=`ps -ef | grep java | grep nodeagent | awk '{ print $2 }'`
        if [[ "${n1pid}" == "" ]] ; then
                echo "Failed to start nodeagent on AppSrv01"
        fi
        echo "Starting nodeagent on AppSrv02"
        ssh root@${m2} ${WASdir}/bin/startNode${suffix} -profileName AppSrv02
        n2pid=`ssh root@${m2} ps -ef | grep java | grep nodeagent | awk '{ print $2 }'`
        if [[ "${n2pid}" == "" ]] ; then
                echo "Failed to start nodeagent on AppSrv02"
        fi

        s1pid=`ps -ef | grep java | grep server1 | awk '{ print $2 }'`
        if [[ "${s1pid}" == "" ]] ; then
            echo "Starting server1"
            ${WASdir}/bin/startServer${suffix} -profileName AppSrv01 server1
        fi
        s2pid=`ps -ef | grep java | grep server2 | awk '{ print $2 }'`
        if [[ "${s2pid}" == "" ]] ; then
            echo "Starting server2"
            ${WASdir}/bin/startServer${suffix} -profileName AppSrv01 server2
        fi
        s3pid=`ssh root@${m2} ps -ef | grep java | grep server3 | awk '{ print \$2 }'`
        if [[ "${s3pid}" == "" ]] ; then
            echo "Starting server3"
            ssh root@${m2} ${WASdir}/bin/startServer${suffix} -profileName AppSrv02 server3
        fi
    else
        # windows
        ${WASdir}/bin/stopServer${suffix} -profileName AppSrv01 server1 -user admin -password admin
        ${WASdir}/bin/stopServer${suffix} -profileName AppSrv01 server2 -user admin -password admin
        ${WASdir}/bin/stopServer${suffix} -profileName AppSrv02 server3 -user admin -password admin
        ${WASdir}/bin/stopNode${suffix} -profileName AppSrv01 -user admin -password admin
        ${WASdir}/bin/stopNode${suffix} -profileName AppSrv02 -user admin -password admin
        ${WASdir}/bin/stopServer${suffix} dmgr -user admin -password admin
        ${WASdir}/bin/osgiCfgInit${suffix} -all
        ${WASdir}/bin/startServer${suffix} dmgr
        ${WASdir}/bin/startNode${suffix} -profileName AppSrv01
        ${WASdir}/bin/startNode${suffix} -profileName AppSrv02
        ${WASdir}/bin/startServer${suffix} -profileName AppSrv01 server1
        ${WASdir}/bin/startServer${suffix} -profileName AppSrv01 server2
        ${WASdir}/bin/startServer${suffix} -profileName AppSrv02 server3        
    fi
fi
