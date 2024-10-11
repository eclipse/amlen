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

import sys
from server import Server, get_logger

logger = get_logger("amlen-readiness")

def ready(instance):
    p1_ha, p1_state = instance.get_ha_state(wait=True)

    if p1_ha and p1_state == "PRIMARY":
        logger.info("We are primary so check node count!")
        startup_mode = instance.get_ha_startup_mode()
        nodes = instance.get_ha_sync_nodes()
        return bool(nodes > 1 or startup_mode.lower() == "standalone")
    if p1_ha and p1_state == "STANDBY":
        logger.info("We are standby!")
        return True
    if not p1_ha and p1_state == "None":
        logger.info("We are not part of a HA pair!")
        return True
    logger.warning("Oh dear we are not primary or standby so what are we: %s", p1_state)

    nodes = instance.get_ha_sync_nodes()
    return False

if __name__ == '__main__':
    imaserver = Server("localhost", logger)
    if ready(imaserver):
        sys.exit(0)
    sys.exit(1)
