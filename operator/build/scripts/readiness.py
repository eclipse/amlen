#!/usr/bin/python3
# Licensed Materials - Property of IBM
# 5725-S86, 5900-A0N, 5737-M66, 5900-AAA
# (C) Copyright IBM Corp. 2020 All Rights Reserved.
# US Government Users Restricted Rights - Use, duplication, or disclosure
# restricted by GSA ADP Schedule Contract with IBM Corp.

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
import server
from subprocess import Popen, PIPE, check_output, TimeoutExpired
from requests.auth import HTTPBasicAuth
from server import Server

from requests.packages.urllib3.exceptions import InsecureRequestWarning
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)

def getLogger(name="amlen-configurator"):
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

def checkForPause():
    modificationtime = None

    try:
        modificationtime = os.path.getmtime(PAUSE_FILE)
    except FileNotFoundError as e:
        logger.debug("Doing normal check as pause file doesn't exist: "+PAUSE_FILE)
    except Exception as e:
        logger.error('Error looking for for pause file modification time: '+repr(e))
        logger.error("Doing normal check as couldn't read modification time for "+PAUSE_FILE)
        modificationtime = None

    if  modificationtime is not None:
        now              = time.time()
        timediff = now - modificationtime

        if now - modificationtime > 24*60*60:
            logger.warning("Old Pause file found ignored - Should the following file be deleted? "+PAUSE_FILE)
        else:
            global Paused
            Paused=True
            logger.warning("Found Pause file - will test/log but report ok to avoid container restart ("+PAUSE_FILE+")")

def ready(instanceA):
    groupName = instanceA.groupName
    
    p1HA, p1State = instanceA.getHAState(wait=True)
        
    if p1HA and p1State == "PRIMARY":
        logger.info("We are primary so check node count!")
        nodes = instanceA.getHASyncNodes()
        if nodes > 1:
            return True
        else:
            return False
    elif p1HA and p1State == "STANDBY":
        logger.info("We are standby!")
        return True
    if not p1HA and p1State == "None":
        logger.info("We are not part of a HA pair!")
        return True
    logger.warning("Oh dear we are not primary or standby so what are we? "+p1State)

    nodes = instanceA.getHASyncNodes()
    return False
    
if __name__ == '__main__':
    imaserver = Server("localhost", logger )
    if ready(imaserver):
       sys.exit(0)
    sys.exit(1)
