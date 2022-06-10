from ansible.module_utils.basic import *
import requests
import logging
import logging.handlers
import sys
import time
import paho.mqtt.client as mqtt
import os
import ssl

from datetime import datetime
from enum import Enum

# Set up the logger
logger = logging.getLogger("hello")
logger.setLevel(logging.DEBUG)
fields = {
    "host": {"required": False, "type": "str", "default":"m2m.eclipse.org" },
    "topic": {"required": False, "type": "str", "default":"$SYS/#" },
    "clientid": {"required": False, "type": "str", "default":None },
    "port": {"required": False, "type": "int", "default":None },
    "insecure": {"required": False, "type":"bool", "default":False },
    "use-tls": {"required": False, "type":"bool", "default":False },
    "ca.crt": {"required": False, "type":"str"},
    "tls.crt": {"required": False, "type":"str"},
    "tls.key": {"required": False, "type":"str"}
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

def on_connect(mqttc, obj, flags, rc):
    mqttc.subscribe(topic, 0)

def on_message(mqttc, obj, msg):
    logger.info("Message received on " + msg.topic + ":" + str(msg.payload))
    module.exit_json(changed=False)

def on_publish(mqttc, obj, mid):
    logger.info("Message Published")

def on_subscribe(mqttc, obj, mid, granted_qos):
    time.sleep(10)
    mqttc.publish(topic,"hello", 0, retain=True)

def on_log(mqttc, obj, level, string):
    logger.info(string)

logFile = f"/tmp/hello.log"
ch = logging.handlers.RotatingFileHandler(logFile, maxBytes=10485760, backupCount=1)
ch.setLevel(logging.DEBUG)
chFormatter = logging.Formatter('%(asctime)-25s %(levelname)-8s %(message)s')
ch.setFormatter(chFormatter)
logger.addHandler(ch)

logger.info("--------------------------------")
logger.info(f"host ............ {host}")
logger.info(f"topic ........... {topic}")
logger.info(f"clientid ........ {clientid}")
logger.info(f"port ............ {port}")
logger.info(f"insecure ........ {insecure}")
logger.info(f"use-tls ......... {usetls}")

if cacerts:
  usetls = True

if port is None:
  if usetls:
    port = 8883
  else:
    port = 1883

mqttc = mqtt.Client(clientid,clean_session = False)

tlsVersion = None

if not insecure:
    cert_required = ssl.CERT_REQUIRED
else:
    cert_required = ssl.CERT_NONE

mqttc.tls_set(ca_certs=cacerts, certfile=certfile, keyfile=keyfile, cert_reqs=cert_required, tls_version=tlsVersion)

if insecure:
    mqttc.tls_insecure_set(True)

mqttc.on_message = on_message
mqttc.on_connect = on_connect
mqttc.on_publish = on_publish
mqttc.on_subscribe = on_subscribe

keepalive=60
mqttc.connect(host, port, keepalive)

mqttc.loop_forever()

