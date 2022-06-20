#!/bin/bash
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


#Undo the changes to ${IMA_WEBUI_APPSRV_INSTALL_PATH}/[usr/usr.org] that are performed in startWebUI.sh
#so that the rpm see and removes the files it originally created as it uninstalls

#If this is a symlink into the data dir, removal it otherwise the rpm removal will delete files from the data dir
if [ -L "${IMA_WEBUI_APPSRV_INSTALL_PATH}/usr" ] ; then
    rm "${IMA_WEBUI_APPSRV_INSTALL_PATH}/usr"
fi

#If a backup of the usr dir was made, remove it
if [ -d "${IMA_WEBUI_APPSRV_INSTALL_PATH}/usr.org" ] ; then
    rm  "${IMA_WEBUI_APPSRV_INSTALL_PATH}/usr.org"
fi

# Copy postUninstall script to either /tmp or to IMSTMPDIR location if it exists
if [ -n "${IMSTMPDIR}" ]; then
    echo "The IMSTMPDIR env var was set to ${IMSTMPDIR}."
    if [ -d "${IMSTMPDIR}" ]; then
        echo "The directory ${IMSTMPDIR} existed. Using this for temp uninstall directory."
        echo "***************************************************************************"
    else
        echo "***********************************************************************************"
        echo "The directory ${IMSTMPDIR} specified by the IMSTMPDIR variable echo does not exist."
        echo "It must exist for the uninstall to continue. Attempting to create it now."
        mkdir -p "${IMSTMPDIR}"
        if [ ! -d "${IMSTMPDIR}" ]; then
            echo "We failed to create the $IMSTMPDIR directory specified by the IMSTMPDIR var."
            echo "The WebUI rpm post-uninstall scripts will probably fail during an uninstall."
            echo "Please backup the WebUI data dir (${IMA_SVR_DATA_PATH}/webui) if it exists"
            echo "   and remove and reinstall IBMWIoTPMessageGatewayWebUI."
            echo "***********************************************************************************"
        else
            echo "Successfully created directory ${IMSTMPDIR}."
            echo "***********************************************************************************"
        fi
    fi
else
    IMSTMPDIR="/tmp"
fi

cat > "${IMSTMPDIR}"/postUninstallIBMIoTWebUI.sh << EOF
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

POSTUNFLAG=\$1

if [ -e ${IMA_SVR_DATA_PATH}/markers/install-imawebui ]
then
    #We can see a marker that some version of us (maybe with a different
    #name?) is being installed.... treat this as an upgrade
    POSTUNFLAG=1
fi

#stop legacy MessageSight service
systemctl stop IBMIoTMessageSightWebUI

#stop ISMWebUI
systemctl stop imawebui

if [ "\$POSTUNFLAG" == "0" ]
then

    # remove old name
    if [ -f /etc/systemd/system/IBMIoTMessageSightWebUI.service ]
    then
        rm -f /etc/systemd/system/IBMIoTMessageSightWebUI.service
    fi
    
    if [ -f /etc/systemd/system/imawebui.service ]
    then
        rm -f /etc/systemd/system/imawebui.service
    fi

    if [ -d ${IMA_WEBUI_APPSRV_INSTALL_PATH} ]
    then
        rm -rf ${IMA_WEBUI_APPSRV_INSTALL_PATH}
    fi

    if [ -d ${IMA_WEBUI_INSTALL_PATH} ]
    then
        rm -rf ${IMA_WEBUI_INSTALL_PATH}
    fi

    #Don't remove user data:
    #if [ -d ${IMA_SVR_DATA_PATH}/webui ]
    #then
    #    rm -rf ${IMA_SVR_DATA_PATH}/webui
    #fi

    IMA_SLAPDPID=\$(ps -ef | grep slapd | grep "${IMA_SVR_DATA_PATH}/webui/config/slapd.conf" | grep -v grep | awk '{print \$2}')

    if [ -n "\${IMA_SLAPDPID}" ]; then
        echo "Making sure the slapd process is stopped."
        kill -9 "\${IMA_SLAPDPID}"
    fi
fi
EOF

# Change file permissions
chmod +x "${IMSTMPDIR}"/postUninstallIBMIoTWebUI.sh
