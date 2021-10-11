#!/bin/bash +x
#
#

# Source the ISMsetup file to get access to information for the remote machine
if [[ -f "../testEnv.sh"  ]]
then
    source  "../testEnv.sh"
else
    source ../scripts/ISMsetup.sh
fi

file=$1
# destfile=$file  NEED TO STRIP THE PATH from the input filename like when no $1 is passed like below
destfile="${file##*/}"

clientCRL=$2

if [[ "${file}" == "" ]] ; then
    file=../common/proxy.cfg
    destfile=imaproxy.cfg
fi

if [[ "${clientCRL}" == "" ]] ; then
    clientCRL=tls_certs/revcrl0.crl
fi

#set -x
# P1, P2, ....
if [[ $# -ge 4 ]]; then
    PX_TEMP=$4_USER
    PX_USER=$(eval echo \$${PX_TEMP})
    PX_TEMP=$4_HOST
    PX_HOST=$(eval echo \$${PX_TEMP})
    PX_TEMP=$4_PROXYROOT
    PX_PROXYROOT=$(eval echo \$${PX_TEMP})
else
    PX_USER=${P1_USER}
    PX_HOST=${P1_HOST}
    PX_PROXYROOT=${P1_PROXYROOT}
fi

if [[ "$1" == "?" ]] ; then
    echo "Usage is: "
    echo "     ./startProxy.sh proxy.cfg clientCRL-file [extra-file]"
    echo "   proxy.cfg is the config file to start the proxy with."
    echo "       if no file is specified, a file named 'proxy.cfg' is used."
    echo "   clientCRL-file if specified will also be copied to the proxy directory and to /tmp/myca.crl."
    echo "        To use DEFAULT CRL, leave off or pass "." and tls_certs/revcrl0.crl will be copied."
    echo "   extra-file if specified will also be copied to the proxy directory. ex. Use for ACL File"
    exit 0
fi

if [ -f $file ]; then
    if ssh ${PX_USER}@${PX_HOST} test -f "/opt/ibm/imaproxy/bin/imaproxy"; then
        export PROXY_RPM_INSTALL_IBM=1
    elif ssh ${PX_USER}@${PX_HOST} test -f "/usr/share/amlen-proxy/bin/imaproxy"; then
        export PROXY_RPM_INSTALL_AMLEN=1
    elif ssh ${PX_USER}@${PX_HOST} test -f "/niagara/proxy/imaproxy"; then
        export PROXY_FILE_INSTALL=1
    else
        echo "Cannot find the imaproxy binary. Giving up!"
        exit 1
    fi

    if [ -f ../lib/clientproxytest.jar ]; then
        if [ ! -d "test" ]; then
            mkdir test
        fi
        cp ../lib/clientproxytest.jar test
    else
        echo "Could not find ../lib/clientproxytest.jar"
    fi

    scp -r test ${PX_USER}@${PX_HOST}:${PX_PROXYROOT}

    if (( PROXY_RPM_INSTALL_IBM == 1 )); then
        scp -r keystore ${PX_USER}@${PX_HOST}:/var/imaproxy
    elif (( PROXY_RPM_INSTALL_AMLEN == 1 )); then
        scp -r keystore ${PX_USER}@${PX_HOST}:/var/lib/amlen-proxy
    else
        scp -r keystore ${PX_USER}@${PX_HOST}:${PX_PROXYROOT}
    fi

    scp $clientCRL ${PX_USER}@${PX_HOST}:/tmp/myca.crl
    scp tls_certs/ca_revcrl0.crl ${PX_USER}@${PX_HOST}:/tmp/myica.crl
    scp tls_certs/rca_revcrl0.crl ${PX_USER}@${PX_HOST}:/tmp/myrca.crl

    if [[ "$2" != "" ]] ; then
        scp $2 ${PX_USER}@${PX_HOST}:${PX_PROXYROOT}
    fi

    if [[ "$3" != "" ]] ; then
        scp $3 ${PX_USER}@${PX_HOST}:${PX_PROXYROOT}
    fi

    # /opt/ibm/imaproxy/sample-config/imaproxy.cfg
    #/opt/ibm/imaproxy/config/imaproxy.cfg
    #/opt/ibm/imaproxy/bin/imaproxy.cfg
    if (( PROXY_RPM_INSTALL_IBM == 1 )); then
        ssh ${PX_USER}@${PX_HOST} "systemctl stop imaproxy"
        ssh ${PX_USER}@${PX_HOST} "rm -f /var/imaproxy/proxy.cfg"
        ssh ${PX_USER}@${PX_HOST} "rm -f /var/imaproxy/imaproxy.cfg"
        ssh ${PX_USER}@${PX_HOST} "rm -f /var/imaproxy/config/imaproxy.cfg"
        ssh ${PX_USER}@${PX_HOST} "rm -f /opt/ibm/imaproxy/sample-config/imaproxy.cfg"
        ssh ${PX_USER}@${PX_HOST} "rm -f /opt/ibm/imaproxy/config/imaproxy.cfg"
        ssh ${PX_USER}@${PX_HOST} "rm -f /opt/ibm/imaproxy/bin/imaproxy.cfg"
        scp $file ${PX_USER}@${PX_HOST}:/var/imaproxy/${destfile}
        ssh ${PX_USER}@${PX_HOST} "/bin/cp -f /var/imaproxy/${destfile} /var/imaproxy/imaproxy.cfg"
        scp $file ${PX_USER}@${PX_HOST}:/var/imaproxy/config/${destfile}
        ssh ${PX_USER}@${PX_HOST} "/bin/cp -f /var/imaproxy/config/${destfile} /var/imaproxy/config/imaproxy.cfg"
        scp $file ${PX_USER}@${PX_HOST}:/opt/ibm/imaproxy/bin/${destfile}
        ssh ${PX_USER}@${PX_HOST} "/bin/cp -f /opt/ibm/imaproxy/bin/${destfile} /opt/ibm/imaproxy/bin/imaproxy.cfg"
        scp $file ${PX_USER}@${PX_HOST}:/opt/ibm/imaproxy/config/${destfile}
        ssh ${PX_USER}@${PX_HOST} "/bin/cp -f /opt/ibm/imaproxy/config/${destfile} /opt/ibm/imaproxy/config/imaproxy.cfg"
        scp $file ${PX_USER}@${PX_HOST}:/opt/ibm/imaproxy/sample-config/${destfile}
        ssh ${PX_USER}@${PX_HOST} "/bin/cp -f /opt/ibm/imaproxy/sample-config/${destfile} /opt/ibm/imaproxy/sample-config/imaproxy.cfg"
        sleep 1
        ssh ${PX_USER}@${PX_HOST} "systemctl start imaproxy"
        sleep 1
        ssh ${PX_USER}@${PX_HOST} "systemctl status imaproxy"
        ssh ${PX_USER}@${PX_HOST} "netstat -plnt"
    elif (( PROXY_RPM_INSTALL_AMLEN == 1 )); then
        ssh ${PX_USER}@${PX_HOST} "systemctl stop imaproxy"
        ssh ${PX_USER}@${PX_HOST} "rm -f /var/lib/amlen-proxy/proxy.cfg"
        ssh ${PX_USER}@${PX_HOST} "rm -f /var/lib/amlen-proxy/imaproxy.cfg"
        ssh ${PX_USER}@${PX_HOST} "rm -f /var/lib/amlen-proxy/config/imaproxy.cfg"
        ssh ${PX_USER}@${PX_HOST} "rm -f /usr/share/amlen-proxy/sample-config/imaproxy.cfg"
        ssh ${PX_USER}@${PX_HOST} "rm -f /usr/share/amlen-proxy/config/imaproxy.cfg"
        ssh ${PX_USER}@${PX_HOST} "rm -f /usr/share/amlen-proxy/bin/imaproxy.cfg"
        scp $file ${PX_USER}@${PX_HOST}:/var/lib/amlen-proxy/${destfile}
        ssh ${PX_USER}@${PX_HOST} "/bin/cp -f /var/lib/amlen-proxy/${destfile} /var/lib/amlen-proxy/imaproxy.cfg"
        scp $file ${PX_USER}@${PX_HOST}:/var/lib/amlen-proxy/config/${destfile}
        ssh ${PX_USER}@${PX_HOST} "/bin/cp -f /var/lib/amlen-proxy/config/${destfile} /var/lib/amlen-proxy/config/imaproxy.cfg"
        scp $file ${PX_USER}@${PX_HOST}:/usr/share/amlen-proxy/bin/${destfile}
        ssh ${PX_USER}@${PX_HOST} "/bin/cp -f /usr/share/amlen-proxy/bin/${destfile} /usr/share/amlen-proxy/bin/imaproxy.cfg"
        scp $file ${PX_USER}@${PX_HOST}:/usr/share/amlen-proxy/config/${destfile}
        ssh ${PX_USER}@${PX_HOST} "/bin/cp -f /usr/share/amlen-proxy/config/${destfile} /usr/share/amlen-proxy/config/imaproxy.cfg"
        scp $file ${PX_USER}@${PX_HOST}:/usr/share/amlen-proxy/sample-config/${destfile}
        ssh ${PX_USER}@${PX_HOST} "/bin/cp -f /usr/share/amlen-proxy/sample-config/${destfile} /usr/share/amlen-proxy/sample-config/imaproxy.cfg"
        sleep 1
        ssh ${PX_USER}@${PX_HOST} "systemctl start imaproxy"
        sleep 1
        ssh ${PX_USER}@${PX_HOST} "systemctl status imaproxy"
        ssh ${PX_USER}@${PX_HOST} "netstat -plnt"
    else
        scp $file ${PX_USER}@${PX_HOST}:${PX_PROXYROOT}/${destfile}
        ssh ${PX_USER}@${PX_HOST} "cd ${PX_PROXYROOT};mkdir -p config"
        ssh ${PX_USER}@${PX_HOST} "cd ${PX_PROXYROOT}; ulimit -c unlimited; LD_LIBRARY_PATH=/niagara/proxy/lib imaproxy -d $destfile" &
    fi
else
    echo "Cannot find config file '$file'."
fi
set +x
