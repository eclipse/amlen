#!/usr/bin/bash
PRELAUNCH=0

SROOT=/home/changeme/Development/IMA15a/sroot
BROOT=/home/changeme/Development/IMA15a/broot

IMAINSTDIR=/opt/ibm/imaserver
IMAINSTBACKUPDIR=${IMAINSTDIR}/backup
IMADATADIR=/var/messagesight

CP="/usr/bin/cp -f --preserve=all"

if [ -d ${SROOT}/gcc-5.2 ]
then
    ${CP} -r ${SROOT}/gcc-5.2 /opt/gcc-5.2
fi

# Update binaries from the output tree
if [ ! -d "${IMAINSTBACKUPDIR}" ]
then
    mkdir -p "${IMAINSTBACKUPDIR}/debug"
    ${CP} -r ${IMAINSTDIR}/bin ${IMAINSTBACKUPDIR}
    ${CP} -r ${IMAINSTDIR}/lib64 ${IMAINSTBACKUPDIR}
    ${CP} -r ${IMAINSTDIR}/debug/bin ${IMAINSTBACKUPDIR}/debug
    ${CP} -r ${IMAINSTDIR}/debug/lib ${IMAINSTBACKUPDIR}/debug
fi

# Update binaries & some scripts from the output and source trees
if [ ${PRELAUNCH} -eq 1 ] || [ ${PRELAUNCH} -eq 2 ] || [ ${PRELAUNCH} -eq 3 ]
then
    mkdir -p "${IMAINSTDIR}/prod/bin"
    mkdir -p "${IMAINSTDIR}/prod/lib"
    ${CP} ${BROOT}/server_ship/bin/* ${IMAINSTDIR}/prod/bin
    ${CP} ${BROOT}/server_ship/lib/* ${IMAINSTDIR}/prod/lib
    ${CP} ${BROOT}/server_ship/debug/bin/* ${IMAINSTDIR}/debug/bin
    ${CP} ${BROOT}/server_ship/debug/lib/* ${IMAINSTDIR}/debug/lib
    ${CP} ${SROOT}/server_main/scripts/startServer_docker.sh ${IMAINSTDIR}/prod/bin/.
    perl -pi -e 's/\r\n$/\n/g' ${IMAINSTDIR}/prod/bin/startServer_docker.sh
    chmod u+x ${IMAINSTDIR}/prod/bin/startServer_docker.sh
    ${CP} ${SROOT}/server_main/scripts/startMQCBridge.sh ${IMAINSTDIR}/prod/bin/.
    perl -pi -e 's/\r\n$/\n/g' ${IMAINSTDIR}/prod/bin/startMQCBridge.sh
    chmod u+x ${IMAINSTDIR}/prod/bin/startMQCBridge.sh
    ${CP} ${IMAINSTDIR}/prod/lib/lib*icu* /lib64
    ${CP} ${IMAINSTDIR}/debug/lib/lib*icu* /lib64
fi

# Update binaries with our production versions
if [ ${PRELAUNCH} -eq 1 ]
then
    ${CP} ${IMAINSTDIR}/prod/bin/* ${IMAINSTDIR}/bin
    ${CP} ${IMAINSTDIR}/prod/lib/* ${IMAINSTDIR}/lib64
fi

# Update binaries and server_dynamic.cfg from the source tree
if [ ${PRELAUNCH} -eq 2 ]
then
    ${CP} ${IMAINSTDIR}/prod/bin/* ${IMAINSTDIR}/bin
    ${CP} ${IMAINSTDIR}/prod/lib/* ${IMAINSTDIR}/lib64
    ${CP} ${SROOT}/server_main/config/server_dynamic.cfg ${IMAINSTDIR}/config
    mkdir -p ${IMADATADIR}/config >/dev/null 2>&1 3>&1
    ${CP} ${SROOT}/server_main/config/server_dynamic.cfg ${IMADATADIR}/config
fi

# Update binaries with our debug versions
if [ ${PRELAUNCH} -eq 3 ]
then
    ${CP} ${IMAINSTDIR}/debug/bin/* ${IMAINSTDIR}/bin
    ${CP} ${IMAINSTDIR}/debug/lib/* ${IMAINSTDIR}/lib64
fi

# Copy binaries from backup
if [ ${PRELAUNCH} -eq 4 ]
then
    ${CP} -r ${IMAINSTBACKUPDIR}/bin/* ${IMAINSTDIR}/bin
    ${CP} -r ${IMAINSTBACKUPDIR}/lib64/* ${IMAINSTDIR}/lib64
    ${CP} -r ${IMAINSTBACKUPDIR}/debug/bin/* ${IMAINSTDIR}/debug/bin
    ${CP} -r ${IMAINSTBACKUPDIR}/debug/lib/* ${IMAINSTDIR}/debug/lib
fi

export ASAN_OPTIONS="log_path=/var/messagesight/diag/cores"
export LD_LIBRARY_PATH=${IMAINSTDIR}/lib64

sed -i "/HA.AllowSingleNIC/d" /opt/ibm/imaserver/config/server.cfg
echo "HA.AllowSingleNIC = 1" >>/opt/ibm/imaserver/config/server.cfg

${IMAINSTDIR}/bin/startServer.sh
#sleep 10000000
