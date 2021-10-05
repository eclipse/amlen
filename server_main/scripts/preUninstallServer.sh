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

# Copy postUninstall script in /tmp dir
if [ -n "${IMSTMPDIR}" ]; then
    if [ -d "${IMSTMPDIR}" ]; then
        cp ${IMA_SVR_INSTALL_PATH}/bin/postUninstallIBMIoTServer.sh "${IMSTMPDIR}"/
        # Change file permissions
        chmod +x "${IMSTMPDIR}"/postUninstallIBMIoTServer.sh
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
            cp ${IMA_SVR_INSTALL_PATH}/bin/postUninstallIBMIoTServer.sh "${IMSTMPDIR}"/
            # Change file permissions
            chmod +x "${IMSTMPDIR}"/postUninstallIBMIoTServer.sh
        fi
    fi
else
    cp ${IMA_SVR_INSTALL_PATH}/bin/postUninstallIBMIoTServer.sh /tmp/.
    # Change file permissions
    chmod +x /tmp/postUninstallIBMIoTServer.sh
fi
