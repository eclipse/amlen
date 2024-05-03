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
import requests
import logging
import logging.handlers
import sys
import time
import paho.mqtt.client as mqtt
import os
import ssl
import argparse

from datetime import datetime
from enum import Enum

def on_connect(mqttc, obj, flags, rc):
    logger.info("Connected")
    mqttc.subscribe(top, 0)

def on_message(mqttc, obj, msg):
    logger.info("Message received on " + msg.topic + ":" + str(msg.payload))
    mqttc.disconnect()

def on_publish(mqttc, obj, mid):
    logger.info("Message Published")

def on_subscribe(mqttc, obj, mid, granted_qos):
    time.sleep(10)
    mqttc.publish(top,"hello", 0)

def on_log(mqttc, obj, level, string):
    logger.info(string)

def on_disconnect(mqttc,flags,rc):
    logger.info(f"Disconnected {flags} {rc}")

def run(host,topic,clientid,port,insecure,usetls,cacerts,certfile,keyfile,username,password):
    # Set up the logger
    global logger
    logger = logging.getLogger("hello")
    logger.setLevel(logging.DEBUG)
    global top
    top = topic

    logFile = f"/tmp/mqtt_sniff.log"
    ch = logging.handlers.RotatingFileHandler(logFile, maxBytes=10485760, backupCount=1)
    ch.setLevel(logging.DEBUG)
    chFormatter = logging.Formatter('%(asctime)-25s %(levelname)-8s %(message)s')
    ch.setFormatter(chFormatter)
    logger.addHandler(ch)

    if cacerts:
        usetls = True

    logger.info("--------------------------------")
    logger.info(f"host ............ {host}")
    logger.info(f"topic ........... {topic}")
    logger.info(f"clientid ........ {clientid}")
    logger.info(f"port ............ {port}")
    logger.info(f"insecure ........ {insecure}")
    logger.info(f"use-tls ......... {usetls}")

    if port is None:
        if usetls:
            port = 8883
        else:
            port = 1883

    try:
      mqttc = mqtt.Client(clientid,clean_session = False)
    except:
      mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, clientid,clean_session = False)

    tlsVersion = None

    if not insecure:
        cert_required = ssl.CERT_REQUIRED
    else:
        cert_required = ssl.CERT_NONE

    mqttc.tls_set(ca_certs=cacerts, certfile=certfile, keyfile=keyfile, cert_reqs=cert_required, tls_version=tlsVersion)

    if username or password:
        mqttc.username_pw_set(username, password)

    if insecure:
        mqttc.tls_insecure_set(True)

    mqttc.on_message = on_message
    mqttc.on_connect = on_connect
    mqttc.on_publish = on_publish
    mqttc.on_subscribe = on_subscribe
    mqttc.on_disconnect = on_disconnect

    keepalive=60
    mqttc.connect(host, port, keepalive)

    mqttc.loop_forever()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument('-H', '--host', required=False, default="m2m.eclipse.org")
    parser.add_argument('-t', '--topic', required=False, default="$SYS/#")
    parser.add_argument('-c', '--clientid', required=False, default=None)
    parser.add_argument('-u', '--username', required=False, default=None)
    parser.add_argument('-p', '--password', required=False, default=None)
    parser.add_argument('-P', '--port', required=False, type=int, default=None, help='Defaults to 8883 for TLS or 1883 for non-TLS')
    parser.add_argument('-s', '--use-tls', action='store_true')
    parser.add_argument('--insecure', action='store_true')
    parser.add_argument('-F', '--cacerts', required=False, default=None)
    parser.add_argument('--tls-version', required=False, default=None, help='TLS protocol version, can be one of tlsv1.2 tlsv1.1 or tlsv1\n')
    parser.add_argument('-K', '--keyfile', required=False, default=None, help='private key (for TLS communication if client certs are needed)')
    parser.add_argument('-C', '--certfile', required=False, default=None, help='cert that matches the keyfile (for TLS communication if client certs are needed)')

    args, unknown = parser.parse_known_args()

    
    run(args.host,args.topic,args.clientid,args.port,
        args.insecure,args.use_tls,args.cacerts,
        args.certfile,args.keyfile,args.username,
        args.password);

    logger.info("Test completed successfully")


