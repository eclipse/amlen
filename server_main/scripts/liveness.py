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

import sys
import json
import traceback
from server import Server, check_for_pause, get_logger

PAUSE_FILE = '/tmp/liveness-pause'
logger = get_logger("amlen-liveness")

def exit_with_condition(returncode):
    """Exits with the return code specified....
       UNLESS Liveness Probes pauses (via checkForLivenessPause() )
       If paused, always exits with a return code of 0
    """
    if check_for_pause(PAUSE_FILE,logger):
        logger.warning("Exiting with 0 due to Pause file - would have returned: %s", returncode)
        sys.exit(0)

    sys.exit(returncode)

def main():
    try:
        imaserver = Server("localhost", logger)
        status = imaserver.get_status()

        if not status:
            print('No JSON response from server status API.')
            exit_with_condition(1)
        elif 'Server' in status and 'ErrorCode' in status['Server']:
            logger.debug(json.dumps(status, indent=4))
            error_code = status['Server']['ErrorCode']
            if error_code == 0:
                logger.info('Server is alive!')
                exit_with_condition(0)
            else:
                logger.error('Server returned ErrorCode: [%s];', error_code)
                exit_with_condition(1)
        else:
            logger.debug(json.dumps(status, indent=4))
            logger.error('Server status API did not return a value for ErrorCode.')
            exit_with_condition(1)

    except Exception as ex: # pylint: disable=W0703
        logger.error('Unexpected error while requesting server status: {%s}', repr(ex))
        traceback.print_exc()
        exit_with_condition(1)

    logger.error('No error detected but could not determine server status.')
    exit_with_condition(1)

if __name__ == "__main__":
    main()
