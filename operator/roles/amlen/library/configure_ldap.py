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
from ansible.module_utils.server import Server

from requests.packages.urllib3.exceptions import InsecureRequestWarning
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)

# Constants
DEFAULT_HEARTBEAT = 10

def getLogger(name="amlen-configurator"):
    logging.basicConfig(filename='/tmp/configure_ldap.log', level=logging.DEBUG)
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

def deployLDAP(instanceA, instanceB, certificate):
    groupName = instanceA.groupName
    
    p1HA, p1State = instanceA.getHAState(wait=True)
        
    if not p1HA or (p1HA and p1State == "PRIMARY"):
        # A is primary - valid pair
        server = instanceA
    elif instanceB != None:
        p2HA, p2State = instanceB.getHAState(wait=True)
        if p1HA and p2State == "PRIMARY":
        # B is primary - valid pair
            server = instanceB
        else:
            server = None
    else:
        server = None

    if server == None:
        logger.info("Can't find primary")
        error_text = "Can't find primary"
        logger.error(error_text)
        raise Exception(error_text)
    
    serverDown = (server.getStatus(wait=False) == None)
    serverHA, serverState = server.getHAState(wait=False)

    if serverDown:
        logger.info("Server Down")
        error_text = "Server Down"
        logger.error(error_text)
        raise Exception(error_text)
    primaryInput = {"LDAP": {
                                "CacheTimeout":           10,
                                "NestedGroupSearch":      False,
                                "BindDN":                 "cn=admin,dc=amleninternal,dc=auth",
                                "BindPassword":           "adminpassword",
                                "Enabled":                True,
                                "GroupCacheTimeout":      300,
                                "UserSuffix":             "ou=users,dc=amleninternal,dc=auth",
                                "URL":                    "ldap://ldap-service:1389",
                                "GroupIdMap":             "*:cn",
                                "GroupMemberIdMap":       "uniqueMember",
                                "UserIdMap":              "*:cn",
                                "BaseDN":                 "dc=amleninternal,dc=auth",
                                "GroupSuffix":            "ou=groups,dc=amleninternal,dc=auth",
                                "MaxConnections":         100,
                                "IgnoreCase":             True,
                                "EnableCache":            True,
                                "Timeout":                30,
                                "Certificate":            "ldap.pem",
                                "CheckServerCert":        "TrustStore",
                                "Verify":                 True,
                                "Overwrite":              True
                            }
                   }
       
    server.uploadFile({'file': ("ldap.pem",certificate)})
    rc, content = server.postConfigurationRequest(primaryInput)
    if rc != 200:
        logger.error("Unexpected response code when configuring LDAP %s: %s" % (server.serverName, rc)) 
        logger.error(content)
        raise Exception("Unexpected response code when configuring LDAP %s: %s" % (server.serverName, rc))
    primaryInput["LDAP"]["Verify"] = False
    rc, content = server.postConfigurationRequest(primaryInput)
    if rc != 200:
        logger.error("Unexpected response code when configuring LDAP %s: %s" % (server.serverName, rc)) 
        logger.error(content)
        raise Exception("Unexpected response code when configuring LDAP %s: %s" % (server.serverName, rc))

if __name__ == '__main__':
    fields = {
        "servers": {"required": True, "type": "str" },
        "suffix": {"required": True, "type": "str"},
        "password": {"required": True, "type": "str"},
        "cert": {"required": True, "type": "str"},
    }

    module = AnsibleModule(argument_spec=fields)

    servers = module.params['servers']
    suffix = module.params['suffix']
    password = module.params['password']
    cert = module.params['cert']
    
    try:
        instances=[]
        names = [sub + suffix for sub in servers.split(",")]
        for instance in names:
            imaserver = Server(instance, logger )
            imaserver.setCredentials(password)
            instances.append(imaserver)
        
        if len(instances) == 1:
            deployLDAP(instances[0],None,cert)
        elif len(instances) == 2:
            deployLDAP(instances[0], instances[1],cert)
        else:
            error_text = "Unexpected number of servers: %s" % (len(instances))
            logger.error(error_text)
            raise Exception(error_text)
        
        # Add this service to the config map all MBGs

    except Exception as e:
        stackTrace = traceback.format_exc()
        logger.warn("Failure during deploy: %s" % stackTrace)
        raise Exception(e)

    module.exit_json(changed=False)
