# Copyright (c) 2021 Contributors to the Eclipse Foundation
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
import os
import sys
import json
import logging
import socket
from time import time, sleep
import threading
import queue
import requests

class iotf_device(object):
    def __init__(self,deviceId,deviceType,devicePw=None):
        self.retries = 0
        self.deviceId = deviceId
        self.deviceType = deviceType
        self.devicePw = devicePw
    def __lt__(self, other):
        return self.retries < other.retries

class iotf_log(object):
    def __init__(self, timestamp, deviceId, duration="", retries=0, log_message=""):
        self.timestamp = timestamp
        self.duration =duration
        self.retries =retries
        self.log_message =log_message
        self.deviceId = deviceId
    def __lt__(self,other):        
        return str(self.deviceId) < str(other.deviceId) #self.timestamp <= other.timestamp and 
    def __str__(self):
        return "{0.timestamp:<25} id: {0.deviceId:<3} : {0.log_message} retries: {0.retries} duration {0.duration}".format(self )


def avg(x):
    global averageAccumulator
    if averageAccumulator is None:
        averageAccumulator = 1
    averageAccumulator= averageAccumulator+x
    return averageAccumulator/x
 
def load_devices():
    global logger,apiKey,apiToken,orgId, authId,apiUrl, deviceType,device_count

#    if deviceType is None:
#        url = apiUrl + '/%s/devices'%orgId
#    else:
#        url = apiUrl + '/{orgId}/devices/{deviceType}'.format(orgId,deviceType)
    if deviceType is None:
        url = "https://"+orgId+"."+apiUrl + "/devices"
    else:
        url = "https://"+orgId+"."+apiUrl + "devices/{deviceType}".format(deviceType)

    authHeaders = {'id': authId, 'type' : "person", 'content-type': 'application/json'}
    device_list = []
    r = requests.get(url, headers=authHeaders, auth=(apiKey,apiToken),timeout=100,verify=False)
    print (r)
    rj = r.json()
    for dev in rj : 
        logger.debug("Device received {0}".format(dev))
        print(" device Id: {0} device type: {1}".format(dev["id"], dev["type"]))
        device_count +=1

    return device_list

def register_device (item):
    devStartTime = time()

def worker():
    global logger,workQ,logQ
    s = requests.session()
    s.keep_alive = False
    while not workQ.empty():
        item = workQ.get()
        workTime = time()
        register_device(item)
        duration = time() - workTime
        logMessage = ("I","Completed processing item {0} in thread {1} ".format(item.deviceId,str(threading.current_thread().ident) ) )
        thisMsg = iotf_log(time(),item.deviceId,duration,item.retries,logMessage)
        logQ.put(thisMsg)
        workQ.task_done()

def main():
    global logger,num_worker_threads,workQ,logQ,device_count
    startTime = time()
    logMessage = ("I","Starting main execution")
    thisMsg = iotf_log(time(),0,0,0,logMessage)
    logQ.put(thisMsg)    
    

    loadedDevices = load_devices()
    if len(loadedDevices) < 1 :
        logger.info("No items to process, exiting")
        return 

    for item in loadedDevices:
        workQ.put(item)
        device_count +=1
    logger.info("finished loading devices onto queue")

    
    for i in range(num_worker_threads):
        t = threading.Thread(target=worker)
        try:
            t.start()
            while not t.is_alive():
                0
            logger.debug("thread {0} was started".format(i))
        except E:
           logger.exception(E)

    logger.debug("Devices put on queue = {0}".format(device_count))
    logger.debug("All child threads started")
    ##  Make sure all threads are not alive
    while threading.active_count() > 1:
        0
    ##
    logger.debug("All child threads finished")

    finishTime = time()
    elapsedTime = finishTime-startTime
    while not logQ.empty():
        printItem = logQ.get()
        logQ.task_done()
        if printItem.log_message[0] == 'E':
            print(str(printItem))
            #do I need to do this loop? 
    print ('Start time: {0} Finish time: {1}'.format(startTime,finishTime))
    print ('Successfully listed {0} devices over elapsed time: {1} '.format(device_count,elapsedTime))
    print ('Average devices per second {0}'.format(device_count/elapsedTime))

logger = None
num_worker_threads = 2
averageAccumulator = None
maxretries = None
apiKey=None
apiToken=None
orgId = None
authId = None
apiUrl = None
deviceType = None
num_worker_threads = None
workQ = None
logQ = None
device_count = 0

if __name__ == "__main__":
    logger = logging.getLogger("delete")
    logger.setLevel(logging.INFO)
    ch = logging.StreamHandler()
    ch.setLevel(logging.INFO)
    chFormatter = logging.Formatter('%(asctime)-25s %(name)-15s ' + ' %(levelname)-8s %(message)s')
    ch.setFormatter(chFormatter)
    logger.addHandler(ch)
    averageAccumulator = None
    maxretries = 2
    apiKey=os.environ.get("API_KEY").strip('"')
    apiToken=os.environ.get("API_TOKEN").strip('"')
    orgId = os.environ.get("ORG_NAME")
    authId = os.environ.get("ORG_OWNER") #"IOTFTest"
    apiUrl = os.environ.get("REST_URL")
    deviceType = os.environ.get("DEVICE_TYPE")

    num_worker_threads = int(os.environ.get("NUM_THREADS"))
#    num_worker_threads = 2
    workQ = queue.PriorityQueue()
    logQ = queue.PriorityQueue()
    main()
    print("returned from main")
