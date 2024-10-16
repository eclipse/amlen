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
from subprocess import Popen, PIPE, check_output
from requests.auth import HTTPBasicAuth
from ansible.module_utils.server import Server

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

def deployHA(instanceA, instanceB):
    groupName = instanceA.group_name
    
    p1HA, p1State = instanceA.get_ha_state(wait=True)
    p2HA, p2State = instanceB.get_ha_state(wait=True)
        
    if p1HA and p1State == "PRIMARY":
        # A is primary - valid pair
        server1 = instanceA
        server2 = instanceB
    elif p1HA and p2State == "PRIMARY":
        # B is primary - valid pair
        server1 = instanceB
        server2 = instanceA
    else:
        if p2HA and not p1HA:
            # B has HA enabled, use it as primary 
            server1 = instanceB
            server2 = instanceA
        else:
            # Otherwise the order doesn't matter
            server1 = instanceA
            server2 = instanceB
    
    server1Down = (server1.get_status(wait=False) == None)
    server2Down = (server2.get_status(wait=False) == None)

    server1HA, server1State = server1.get_ha_state(wait=False)
    server2HA, server2State = server2.get_ha_state(wait=False)

    setupHA = False
    if ((server2Down or not server2HA) and not server1Down and 
        (not server1HA or (server1HA and server1State == 'PRIMARY'))):
        # We are switching a single-node server (or standalone HA) into HA
        # In order to do it without downtime, set up HA before
        # updating server1
        logger.info("Need to set up HA before updating original node")
        setupHA = True 

    needExtraRestart = False
    if server1Down and server2Down:
        # Both nodes are down, note to restart both nodes after deployment
        needExtraRestart = True

    # Wait for sync only if server1 node is up 
    # and HA was established before the deploy
    if not server2Down:
        isHA, state = server2.get_ha_state(wait=True)
        if not isHA or state == "UNSYNC_ERROR" or state == "UNKNOWN":
            msg = "Node %s could not synchronize (%s - %s)" % (server2.server_name, isHA, state)
            logger.warning(msg)
            if server2HA:
                logger.info("Standby lost information about HA, reestablishing it")
                setupHA = True

    if setupHA:
        logger.info("Configuring new HA pair %s and %s" % (server1.server_name, server2.server_name))
        configureHA(server1, server2, groupName)

    if needExtraRestart:
        server1.restart()
        server2.restart()

    isHA, state = server1.get_ha_state(wait=True)
    if not isHA or state == "UNSYNC_ERROR" or state == "UNKNOWN" or state == "HADISABLED":
        logger.warning("Node %s could not synchronize (%s - %s)" % (server1.server_name, isHA, state))    
    
    logger.info("Configuring existing HA pair %s and %s" % (server1.server_name, server2.server_name))
    server1.configure_username_password()
    server2.configure_username_password()
    
    configureHA(server1, server2, groupName)
    
def configureHA(server1, server2, groupName):
    # We have 4 possible situations
    # 0. There is HA and server2 is primary and server1 is standby or server1 is primary and server2 is standby
    #    Ensure the HA group name is set correctly
    # 1. There is no HA and both server1 and server2 are up
    #    a. Configure new server1 for HA with StartupMode=StandAlone and restart it
    #    b. Configure new server2 for HA and restart it
    #    c. Configure server1 with StartupMode=AutoDetect
    # 2. There is HA and server1 is primary; no standby
    #    a. Configure server2 as standby and restart it
    # 3. There is HA and server2 is primary; no standby
    #    a. Configure server1 as standby and restart it
    
    primaryInput = {"HighAvailability": {}}
    standbyInput = {"HighAvailability": {}}

    scenario = 0
    server1HA, server1State = server1.get_ha_state(wait=False)
    server2HA, server2State = server2.get_ha_state(wait=False)
    
    # If server1 has HA enabled, but is in error, unsync or hadisabled state, assume 
    # it's not a properly a properly configured HA 
    if server1HA and (server1State == "UNSYNC" or server1State == "UNSYNC_ERROR" or server1State == "HADISABLED"):
        server1HA = False

    # If server2 has HA enabled, but is in error, unsync or hadisabled state, assume
    # it's not a properly a properly configured HA
    if server2HA and (server2State == "UNSYNC" or server2State == "UNSYNC_ERROR" or server2State == "HADISABLED"):
        server2HA = False
        
    logger.info("server1 %s: HA enabled - %s, state - %s" % (server1.server_name, server1HA, server1State))
    logger.info("server2 %s: HA enabled - %s, state - %s" % (server2.server_name, server2HA, server2State))
    
    if not server1HA and not server2HA:
        scenario = 1
        
        # Put standby in maintenance mode so that it won't stop new primary from coming up
        server2.switch_to_maint_mode()

        primaryInput["HighAvailability"]={
                    "EnableHA":               True,
                    "ExternalReplicationNIC": "",
                    "RemoteDiscoveryNIC":     server2.server_name,
                    "LocalReplicationNIC":    server1.server_name,
                    "LocalDiscoveryNIC":      server1.server_name,                    
                    "ReplicationPort":        9085,
                    "Group":                  groupName,
                    "PreferredPrimary":       True,
                    "StartupMode":            "StandAlone",
                    "UseSecuredConnections":  True,
                    "HeartbeatTimeout":       DEFAULT_HEARTBEAT
        }
        
        rc, content = server1.post_configuration_request(primaryInput)
        if rc != 200:
            logger.error("Unexpected response code when configuring HA primary server %s: %s" % (server1.server_name, rc)) 
            logger.error(content)
            raise Exception("Unexpected response code when configuring HA primary server %s: %s" % (server1.server_name, rc))

        # Restart primary and wait for HA to come up properly
        server1.restart()
        if not server1.check_ha_status():
            error_text = "HA primary Amlen server %s could not be configured as standalone" % (server1.server_name)
            logger.error(error_text) 
            raise Exception(error_text)

        standbyInput["HighAvailability"]={
                    "EnableHA":               True,
                    "ExternalReplicationNIC": "",
                    "RemoteDiscoveryNIC":     server1.server_name,
                    "LocalReplicationNIC":    server2.server_name,
                    "LocalDiscoveryNIC":      server2.server_name,                    
                    "ReplicationPort":        9085,
                    "Group":                  groupName,
                    "PreferredPrimary":       False,
                    "StartupMode":            "AutoDetect",
                    "UseSecuredConnections":  True,
                    "HeartbeatTimeout":       DEFAULT_HEARTBEAT
        }

        rc, content = server2.post_configuration_request(standbyInput)
        if rc != 200:
            error_text = "Unexpected response code when configuring HA standby server %s: %s" % (server2.server_name, rc)
            logger.error(error_text) 
            logger.error(content)
            raise Exception(error_text)

        # Clear the store on new standby and switch it into production mode
        server2.clean_store()
        server2.switch_to_production_mode() 
        if not server2.check_ha_status():
            error_text = "HA standby Amlen server %s could not be configured" % (server2.server_name)
            logger.error(error_text) 
            raise Exception(error_text)
        
        # Change primary startup mode to AutoDetect
        primaryInput["HighAvailability"]={
                    "StartupMode":"AutoDetect"
        }
        rc, content = server1.post_configuration_request(primaryInput)
        if rc != 200:
            error_text = "Unexpected response code when configuring HA primary server %s: %s" % (server1.server_name, rc)
            logger.error(error_text) 
            logger.error(content)
            raise Exception(error_text)
        
    elif server1HA and not server2HA:
        scenario = 2
        
        standbyInput["HighAvailability"]={
                    "EnableHA":               True,
                    "ExternalReplicationNIC": "",
                    "RemoteDiscoveryNIC":     server1.server_name,
                    "LocalReplicationNIC":    server2.server_name,
                    "LocalDiscoveryNIC":      server2.server_name,                    
                    "ReplicationPort":        9085,
                    "Group":                  groupName,
                    "PreferredPrimary":       False,
                    "StartupMode":            "AutoDetect",
                    "UseSecuredConnections":  True,
                    "HeartbeatTimeout":       DEFAULT_HEARTBEAT
        }
        rc, content = server2.post_configuration_request(standbyInput)
        if rc != 200:
            error_text = "Unexpected response code when configuring HA standby server %s: %s" % (server2.server_name, rc)
            logger.error(error_text) 
            logger.error(content)
            raise Exception(error_text)

        # Ensure server1 node is a preferred primary and the HA group is correct
        primaryInput["HighAvailability"]={
                    "Group":                  groupName,
                    "PreferredPrimary":       True,
                    "StartupMode":            "AutoDetect",
                    "UseSecuredConnections":  True,
                    "HeartbeatTimeout":       DEFAULT_HEARTBEAT
        }
        rc, content = server1.post_configuration_request(primaryInput)
        if rc != 200:
            error_text = "Unexpected response code when configuring HA primary server %s: %s" % (server1.server_name, rc)
            logger.warn(error_text) 
            logger.warn(content)
        
        server2.clean_store()
        server2.switch_to_production_mode()

        if not server2.check_ha_status():
            logger.error("Error configuring HA for Amlen server %s" % (server2.server_name)) 
            raise Exception("Error configuring HA for Amlen server %s" % (server2.server_name))

    elif server2HA and not server1HA:
        scenario = 3
        primaryInput["HighAvailability"]={
                    "EnableHA":               True,
                    "ExternalReplicationNIC": "",
                    "RemoteDiscoveryNIC":     server2.server_name,
                    "LocalReplicationNIC":    server1.server_name,
                    "LocalDiscoveryNIC":      server1.server_name,                    
                    "ReplicationPort":        9085,
                    "Group":                  groupName,
                    "PreferredPrimary":       True,
                    "StartupMode":            "AutoDetect",
                    "UseSecuredConnections":  True,
                    "HeartbeatTimeout":       DEFAULT_HEARTBEAT
        }
        
        rc, content = server1.post_configuration_request(primaryInput)
        if rc != 200:
            error_text = "Unexpected response code when configuring HA standby server %s: %s" % (server1.server_name, rc)
            logger.error(error_text) 
            logger.error(content)
            raise Exception(error_text)

        # Ensure server2 node is no longer a preferred primary
        standbyInput["HighAvailability"]={
                    "Group":                  groupName,
                    "PreferredPrimary":       False,
                    "StartupMode":            "AutoDetect",
                    "UseSecuredConnections":  True,
                    "HeartbeatTimeout":       DEFAULT_HEARTBEAT
        }
        rc, content = server2.post_configuration_request(standbyInput)
        if rc != 200:
            error_text = "Unexpected response code when configuring HA standby server %s: %s" % (server2.server_name, rc)
            logger.warn(error_text) 
            logger.warn(content)
        
        server1.clean_store()
        
        if not server1.check_ha_status():
            error_text = "Error configuring HA for Amlen server %s" % (server1.server_name)
            logger.error(error_text) 
            raise Exception(error_text)
    else:
        scenario = 4
        # Both nodes are in HA, so make sure they have correct HA group
        updateInput = {}
        updateInput["HighAvailability"]={
                    "Group":                  groupName,
                    "StartupMode":            "AutoDetect",
                    "UseSecuredConnections":  True,
                    "HeartbeatTimeout":       DEFAULT_HEARTBEAT
        }
        rc, content = server1.post_configuration_request(updateInput)
        if rc != 200:
            error_text = "Unexpected response code when configuring HA primary server %s: %s" % (server1.server_name, rc)
            logger.warn(error_text) 
            logger.warn(content)

        rc, content = server2.post_configuration_request(updateInput)
        if rc != 200:
            error_text = "Unexpected response code when configuring HA standby server %s: %s" % (server2.server_name, rc)
            logger.warn(error_text) 
            logger.warn(content)
            
    logger.info("HA scenario %s" % (str(scenario)))

    # Wait for HA pair to synchronize
    ok = server1.check_ha_status()
    if ok == False:
        # Restart both servers and try again
        logger.warn("Initial HA synchronization failed, restarting server1")
        server1.restart(maintenance="stop")

        if server2 != None:
            logger.warn("Initial HA synchronization failed, restarting server2")
            server2.restart(maintenance="stop")

        ok = server1.check_ha_status()
        
        if ok == False:
            raise Exception("Amlen HA configuration error (%s, %s)" % (server1.server_name, server2.server_name))
    
    logger.info("HA configured properly for (%s, %s)" % (server1.server_name, server2.server_name))

def deploy(servers, path):
    if len(servers) == 1:
        servers[0].configure_stand_alone_ha(enable_ha=False)
    elif len(servers) == 2:
        deployHA(servers[0], servers[1])
    else:
        error_text = "Unexpected number of servers: %s" % (len(servers))
        logger.error(error_text)
        raise Exception(error_text)

    for server in servers:
        server.configure_file_logging(path)

if __name__ == '__main__':
    fields = {
        "servers": {"required": True, "type": "str" },
        "suffix": {"required": True, "type": "str"},
        "password": {"required": True, "type": "str"},
        "ca.crt": {"required": True, "type": "str"},
        "caname": {"required": True, "type": "str"},
        "tls.crt": {"required": True, "type": "str"},
        "tls.key": {"required": True, "type": "str"},
        "path": {"required": True, "type": "str"},
        "config": {"required": False, "type": "str"},
    }

    module = AnsibleModule(argument_spec=fields)

    servers = module.params['servers']
    suffix = module.params['suffix']
    password = module.params['password']
    cacrt = module.params['ca.crt']
    caname = module.params['caname']
    tlscrt = module.params['tls.crt']
    tlskey = module.params['tls.key']
    config = module.params['config']
    path = module.params['path']
    
    try:
        instances=[]
        names = [sub + suffix for sub in servers.split(",")]
        logger.info("Config...")
        logger.info(config)
        if config != None and config != "":
           logger.info(config)
           if config[0] == '\'':
              config = config[1:-1]
           config = json.loads(config)
        else:
           config = None
        for instance in names:
            imaserver = Server(instance, logger, password=password )
            imaserver.load_file(caname,cacrt)
            imaserver.load_file("tls.crt",tlscrt)
            imaserver.load_file("tls.key",tlskey)
            if config != None:
               imaserver.set_config(config)
            instances.append(imaserver)
        
        deploy(instances, path)
        
        # Add this service to the config map all MBGs

    except Exception as e:
        stackTrace = traceback.format_exc()
        logger.warn("Failure during deploy: %s" % stackTrace)
        raise Exception(e)

    module.exit_json(changed=False)
