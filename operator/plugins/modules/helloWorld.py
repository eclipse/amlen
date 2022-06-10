from ansible.module_utils.basic import *
import requests
import logging
import logging.handlers
import sys
import time
from datetime import datetime
from enum import Enum

# Set up the logger
logger = logging.getLogger("hello")
logger.setLevel(logging.DEBUG)

def main():
    fields = {
        "servers": {"required": True, "type": "str" },
        "suffix": {"required": True, "type": "str"},
    }

    module = AnsibleModule(argument_spec=fields)

    servers = module.params['servers']
    suffix = module.params['suffix']

    logFile = f"/tmp/hello.log"
    ch = logging.handlers.RotatingFileHandler(logFile, maxBytes=10485760, backupCount=1)
    ch.setLevel(logging.DEBUG)
    chFormatter = logging.Formatter('%(asctime)-25s %(levelname)-8s %(message)s')
    ch.setFormatter(chFormatter)
    logger.addHandler(ch)


    logger.info("--------------------------------------------------------------")
    logger.info(f"servers ........... {servers}")
    logger.info(f"suffix ........... {suffix}")
    type(sys.path)
    for path in sys.path:
       logger.info("PATH: %s" % path)
    for env in os.environ:
       logger.info("ENV: %s" % env)

    module.exit_json(changed=False)

if __name__ == '__main__':
    main()

