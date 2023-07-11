#!/usr/bin/python
# Copyright (c) 2023 Contributors to the Eclipse Foundation
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

from ansible.module_utils.basic import *
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
from subprocess import Popen, PIPE, check_output
from requests.auth import HTTPBasicAuth
from ansible.module_utils.server import Server

from requests.packages.urllib3.exceptions import InsecureRequestWarning
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)

# Constants
DEFAULT_HEARTBEAT = 10

def getLogger(name="amlen-recover"):
    logging.basicConfig(filename='/tmp/amlen-recover.log', level=logging.DEBUG)
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

logger = getLogger()

def startRecovery(server):
    groupName = server.group_name
    
    payload = {"HighAvailability": {"StartupMode" : "StandAlone", "RemoteDiscoveryNIC":"none"}}

    rc, content = server.post_configuration_request(payload)
    if rc != 200:
        logger.error("Unexpected response code when modifying HA configuration for %s: %s" % (server.server_name, rc)) 
        logger.error(content)
        raise Exception("Unexpected response code when modifying HA configuration for %s: %s" % (server.server_name, rc))

    server.restart()

def stopRecovery(server,remoteServerName):
    payload = {"HighAvailability": {"StartupMode" : "AutoDetect", "RemoteDiscoveryNIC":remoteServerName}}
    rc, content = server.post_configuration_request(payload)
    if rc != 200:
        logger.error("Unexpected response code when modifying HA configuration for %s: %s" % (server.server_name, rc)) 
        logger.error(content)

if __name__ == '__main__':
    fields = {
        "server": {"required": True, "type": "str" },
        "remoteServer": {"required": True, "type": "str" },
        "command": {"required": True, "type": "str"},
        "password": {"required": True, "type": "str"},
    }

    module = AnsibleModule(argument_spec=fields)

    server = module.params['server']
    command = module.params['command']
    remoteServer = module.params['remoteServer']
    password = module.params['password']
    
    try:
        imaserver = Server(server, logger, password=password)
        if command.lower() == "start":
            startRecovery(imaserver)
        elif command.lower() == "stop":
            stopRecovery(imaserver,remoteServer)
        else:
            logger.critical("Command unknown: %s" % command)
            raise Exception("Command unknown")

    except Exception as e:
        stackTrace = traceback.format_exc()
        logger.warn("Failure during recovery: %s" % stackTrace)
        raise Exception(e)

    module.exit_json(changed=True)
