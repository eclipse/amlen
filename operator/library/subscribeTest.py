#!/usr/bin/python
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
from ansible.module_utils.basic import *
import requests
import logging
import logging.handlers
import sys
import time
import paho.mqtt.client as mqtt
import os
import ssl
import subscribeTestBasic

from datetime import datetime
from enum import Enum

# Set up the logger
fields = {
    "host": {"required": False, "type": "str", "default":"m2m.eclipse.org" },
    "topic": {"required": False, "type": "str", "default":"$SYS/#" },
    "clientid": {"required": False, "type": "str", "default":None },
    "port": {"required": False, "type": "int", "default":None },
    "insecure": {"required": False, "type":"bool", "default":False },
    "use-tls": {"required": False, "type":"bool", "default":False },
    "ca.crt": {"required": False, "type":"str"},
    "tls.crt": {"required": False, "type":"str"},
    "tls.key": {"required": False, "type":"str"},
    "password": {"required": False, "type":"str"},
    "username": {"username": False, "type":"str"}
}

module = AnsibleModule(argument_spec=fields)

host = module.params['host']
topic = module.params['topic']
clientid = module.params['clientid']
port = module.params['port']
insecure = module.params['insecure']
usetls = module.params['use-tls']
cacerts = module.params['ca.crt']
certfile = module.params['tls.crt']
keyfile = module.params['tls.key']
username = module.params['username']
password = module.params['password']

subscribeTestBasic.run(host,topic,clientid,port,insecure,usetls,cacerts,certfile,keyfile,username,password)

module.exit_json(changed=False)
