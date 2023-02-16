#!/usr/bin/python3
# Copyright (c) 2022 Contributors to the Eclipse Foundation
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

import time
from server import Server, get_logger

# Constants
DEFAULT_HEARTBEAT = 10

logger = get_logger("amlen-monitor")

PASSWORD_FILE="/secrets/admin/adminPassword"
def get_password():
    with open(PASSWORD_FILE, encoding="utf-8") as password_file:
        return password_file.readline().rstrip()

if __name__ == '__main__':

    imaserver = Server("localhost", logger )
    current_password = get_password()
    while True :
        new_password = get_password()
        try:
            imaserver.getStatus()
        except Exception: # pylint: disable=W0703
            try:
                imaserver.update_password(new_password)
                imaserver.get_status()
                current_password = new_password
            except Exception: # pylint: disable=W0703
                imaserver.update_password(current_password)
                logger.info("Neither old or new passwords work")

        if new_password != current_password :
            logger.info("Attempting to change password")
            if imaserver.reconfigureAdminEndpoint(new_password):
                logger.info("Password change succesful")
                current_password = new_password
            else :
                logger.info("Password change not succesful")
        time.sleep(DEFAULT_HEARTBEAT)
