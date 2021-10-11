#!/bin/bash
#
# Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#

PXY_INSTALL_DIR=${IMA_PROXY_INSTALL_PATH}
PXY_DATA_DIR=${IMA_PROXY_DATA_PATH}

if [ -n "$IMSTMPDIR" ]; then
    POSTUNINST="${IMSTMPDIR}"/postUninstallProxy.sh
else
    POSTUNINST=/tmp/postUninstallProxy.sh
fi



echo > $POSTUNINST <<EOF
#!/bin/bash
#
#****************************************************************
# Copyright (c) 2018-2021 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
#
# SPDX-License-Identifier: EPL-2.0
#****************************************************************

POSTUNFLAG=$1

if [ -e ${PXY_DATA_DIR}/markers/install-imaproxy ]
then
    #We can see a marker that some version of us (maybe with a different
    #name?) is being installed.... treat this as an upgrade
    POSTUNFLAG=1
fi

#stop legacy MessageSight proxy server
systemctl stop IBMIoTMessageSightProxy

#stop proxy
systemctl stop imaproxy

if [ "\$POSTUNFLAG" == "0" ]
then

    # remove old name
    if [ -f /etc/systemd/system/IBMIoTMessageSightProxy.service ]
    then
        rm -f /etc/systemd/system/IBMIoTMessageSightProxy.service
    fi

    if [ -f /etc/systemd/system/imaproxy.service ]
    then
        rm -f /etc/systemd/system/imaproxy.service
    fi

    if [ -d ${PXY_INSTALL_DIR} ]
    then
        rm -rf ${PXY_INSTALL_DIR}
    fi
fi

EOF

chmod +x $POSTUNINST

