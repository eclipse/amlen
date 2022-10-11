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

import socket
import subprocess
import time
import sys
import os
import json
import requests
import logging
import traceback
import re
import base64
import yaml
import argparse
from subprocess import Popen, PIPE, check_output, TimeoutExpired
from requests.auth import HTTPBasicAuth
from server import Server

from requests.packages.urllib3.exceptions import InsecureRequestWarning
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)

# Constants
DEFAULT_HEARTBEAT = 10

def getLogger(name="amlen-configurator"):
    logging.basicConfig(filename='/tmp/configure.log', level=logging.DEBUG)
    logger = logging.getLogger(name)
    logger.setLevel(logging.INFO)

    # Only add the handlers once!
    if logger.handlers == []:
        ch = logging.StreamHandler()
        ch.setLevel(logging.INFO)

        chFormatter = logging.Formatter('%(asctime)-25s %(name)-50s %(threadName)-16s %(levelname)-8s %(message)s')
        ch.setFormatter(chFormatter)

        logger.addHandler(ch)

    return logger

logger = getLogger("amlen-configurator")

PASSWORD_FILE="/secrets/adminpassword/password"
def getPassword():
    with open(PASSWORD_FILE) as f:
        return f.readline().rstrip()

if __name__ == '__main__':
    
    imaserver = Server("localhost", logger )
    currentPassword = getPassword()
    while (True) : 
        newPassword = getPassword()
        try:
            imaserver.getStatus()
        except Exception as e:
            try:
                imaserver.updatePassword(newPassword)
                imaserver.getStatus()
                currentPassword = newPassword
            except Exception as e:
                imaserver.updatePassword(currentPassword)
                logger.info("Neither old or new passwords work")              
 
        if newPassword != currentPassword :
            logger.info("Attempting to change password")
            if imaserver.reconfigureAdminEndpoint(newPassword):
                logger.info("Password change succesful")
                currentPassword = newPassword
            else :
                logger.info("Password change not succesful")
        time.sleep(DEFAULT_HEARTBEAT)
