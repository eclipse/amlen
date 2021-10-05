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

if [ -n "$IMSTMPDIR" ]; then
    POSTUNINST="${IMSTMPDIR}"/postUninstallBridge.sh
else
    POSTUNINST=/tmp/postUninstallBridge.sh
fi



echo > $POSTUNINST <<EOF
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

POSTUNFLAG=$1

if [ -e ${IMA_BRIDGE_DATA_PATH}/markers/install-imabridge ]
then
    #We can see a marker that some version of us (maybe with a different
    #name?) is being installed.... treat this as an upgrade
    POSTUNFLAG=1
fi

#stop legacy MessageSight bridge server
systemctl stop IBMIoTMessageSightBridge

#stop bridge
systemctl stop imabridge

if [ "\$POSTUNFLAG" == "0" ]
then

    # remove old name
    if [ -f /etc/systemd/system/IBMIoTMessageSightBridge.service ]
    then
        rm -f /etc/systemd/system/IBMIoTMessageSightBridge.service
    fi
    
    if [ -f /etc/systemd/system/imabridge.service ]
    then
        rm -f /etc/systemd/system/imabridge.service
    fi

    if [ -d ${IMA_BRIDGE_INSTALL_PATH} ]
    then
        rm -rf ${IMA_BRIDGE_INSTALL_PATH}
    fi
fi

EOF

chmod +x $POSTUNINST

