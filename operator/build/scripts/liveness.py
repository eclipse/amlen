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
from server import Server, checkForPause

PAUSE_FILE = '/var/messagesight/liveness-pause'
LivenessPaused = false

def getLogger(name="liveness"):
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

logger = getLogger("liveness")

def exitWithCondition(returncode):
    """Exits with the return code specified....UNLESS Liveness Probes pauses (via checkForLivenessPause() )
       If paused, always exits with a return code of 0
    """
    if checkForPause(PAUSE_FILE):
       logger.warning("Exiting with 0 due to Pause file - would have returned: "+str(returncode))
       sys.exit(0)

    sys.exit(returncode)

if __name__ == "__main__":

    try:
        imaserver = Server("localhost", logger )
        status = imaserver.getStatus()

        if not status:
            print ('No JSON response from server status API.')
            exitWithCondition(1)
        elif 'Server' in status and 'ErrorCode' in status['Server']:
            logger.debug(json.dumps(status, indent=4))
            errorCode = status['Server']['ErrorCode']
            if errorCode == 0:
                logger.info('Server is alive!')
                exitWithCondition(0)
            else:
                logger.error('Server returned ErrorCode: [{0}]'.format(errorCode))
                exitWithCondition(1)
        else:
            logger.debug(json.dumps(status, indent=4))
            logger.error('Server status API did not return a value for ErrorCode.')
            exitWithCondition(1)

    except Exception as e:
        logger.error('Unexpected error while requesting server status: '+repr(e))
        traceback.print_exc()
        exitWithCondition(1)

    logger.error('No error detected but could not determine server status.')
    exitWithCondition(1)
