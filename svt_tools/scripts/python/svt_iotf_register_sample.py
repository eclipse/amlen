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
import csv

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
    global logger,num_worker_threads,averageAccumulator,maxretries,apiKey,apiToken,orgId, authId,apiUrl,num_worker_threads,workQ,logQ
    if averageAccumulator is None:
        averageAccumulator = 1
    averageAccumulator= averageAccumulator+x
    return averageAccumulator/x
 
def load_devices():
    global logger,num_worker_threads,averageAccumulator,maxretries,apiKey,apiToken,orgId, authId,apiUrl,num_worker_threads,workQ,logQ,csv_file,json_file
    fileName = None
    returnList = []
    if csv_file is not None :
        with open(csv_file, newline="") as devicesFile:
            reader = csv.DictReader(devicesFile,fieldnames=("deviceId","deviceType","devicePW"))
            for row in reader:
                thisItem = iotf_device(row['deviceId'],row['deviceType'],row["devicePW"])
                thisItem.retries=0
                returnList.append(thisItem)
    elif json_file is not None: 
        with open(json_file, newline="") as devicesFile:
            reader = json.load(devicesFile)
            for row in reader:
                thisItem = iotf_device(row.deviceId,row.deviceType,row.devicePw)
                thisItem.retries=0
                returnList.append(thisItem)
    else: 
        device_list = range(10000)
        for item in device_list:
            thisItem = iotf_device(item,"test","thisisatestPassword")
            thisItem.retries=0
            returnList.append(thisItem)
    return returnList

def register_device (item):
    global logger,num_worker_threads,averageAccumulator,maxretries,apiKey,apiToken,orgId, authId,apiUrl,num_worker_threads,workQ,logQ,device_count
    devStartTime = time()

    logger.info("entered register_device")

    logMessage = ("I","completed successfully")
    authHeaders = {'id': authId, 'type': "person", 'content-type': 'application/json'}

#    if deviceType is None:
#    else:
#     url = "https://"+orgId+apiUrl + "devices/{deviceType}".format(orgId,deviceType)
#    url = apiUrl + '/%s/devices' % (orgId)
    if item.devicePw is None:
        payload= json.dumps({"type": item.deviceType, "id": item.deviceId}) 
    else:
        payload= json.dumps({"type": item.deviceType, "id": item.deviceId,"password":item.devicePw}) 

    url = "https://"+orgId+"."+apiUrl + "/devices"
    logger.info(url)
    logger.info("payload"+payload)
    try:     
        try:
            r = requests.post(url, data=payload, headers=authHeaders, auth=(apiKey,apiToken),timeout=100,verify=False)  
        except Exception as T:
            logger.exception(T)
            return
        return
        if r.status_code == 201:
            device = r.json()
            deviceReturnedId = device["id"]
            deviceAuthToken = device["password"]
            logger.debug(" * Registered device: %s:%s:%s (%s)" % (orgId, item.deviceType, deviceReturnedId, deviceAuthToken))
            logMessage = ("I"," Registered device: %s:%s:%s (%s) " % (orgId, item.deviceType, deviceReturnedId, deviceAuthToken))
        else:
            logger.critical("Error code when registering the device: %s" % (r.status_code))
            logger.critical("Headers from failed request: " + str(r.request.headers))
            logger.critical("Body from failed request: " + str(r.request.body))
            logger.critical("Response content " + r.text)
            logMessage = ("E"," Failed to register with status %s : Device info %s:%s:%s " % (r.status_code,orgId, item.deviceType, item.deviceId))        
    except Exception as E:
        logger.error("Completed post")
        logger.exception(E)
        logMessage = ("E"," Failed to register with Exception %s : Device info %s:%s:%s " % (str(E),orgId, item.deviceType,item.deviceId))
        raise E
#    print(item.deviceId," ",item.retries)
    duration = time() - devStartTime
    
    thisMsg = iotf_log(time(),item.deviceId,duration,item.retries,logMessage)
    logQ.put(thisMsg)

    if logMessage[0]=="E" :
        item.retries += 1  
        if item.retries < maxretries:
            workQ.put(item)
            logMessage = ("E","Retrying device: %s after %d of %d tries "&(item.deviceId,item.retries,maxretries))
            thisMsg = iotf_log(time(),item.deviceId,duration,item.retries,logMessage)
            logQ.put(thisMsg)
        else:
            logMessage = ("E","device: {0} Exceeded maximum retries of {1} tries.".format(item.deviceId,maxretries))
            thisMsg = iotf_log(time(),item.deviceId,duration,item.retries,logMessage)
            logger.error("Devices left on queue = {0}".format(device_count))
            device_count-=1

def worker():
    global logger,num_worker_threads,averageAccumulator,maxretries,apiKey,apiToken,orgId, authId,apiUrl,num_worker_threads,workQ,logQ
    s = requests.session()
    s.keep_alive = False
    while not workQ.empty():
        item = workQ.get()
        workTime = time()
        try: 
            logger.info ("calling register_device")
            register_device(item)
        except Exception as E:
            break
        duration = time() - workTime
        logMessage = ("I","Completed processing item {0} in thread {1} ".format(item.deviceId,str(threading.current_thread().ident) ) )
        thisMsg = iotf_log(time(),item.deviceId,duration,item.retries,logMessage)
        logQ.put(thisMsg)
        workQ.task_done()

def main():
    global logger,num_worker_threads,averageAccumulator,maxretries,apiKey,apiToken,orgId, authId,apiUrl,num_worker_threads,workQ,logQ,device_count
    startTime = time()
    logMessage = ("I","Starting main execution")
    thisMsg = iotf_log(time(),0,0,0,logMessage)
    logQ.put(thisMsg)
    url = "https://"+orgId+apiUrl + "/devices"
    logger.info(url)

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
    print ('Start time: {0} Finish time: {1}'.format( startTime,finishTime))
    print ('Created {0} devices over elapsed time: {1} '.format(device_count,elapsedTime))
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
num_worker_threads = None
workQ = None
logQ = None
device_count = 0
csv_file = None
json_file = None

if __name__ == "__main__":
    logger = logging.getLogger("register")
    logger.setLevel(logging.DEBUG)
    ch = logging.StreamHandler()
    ch.setLevel(logging.DEBUG)
    chFormatter = logging.Formatter('%(asctime)-25s %(name)-15s ' + ' %(levelname)-8s %(message)s')
    ch.setFormatter(chFormatter)
    logger.addHandler(ch)
    averageAccumulator = None
    maxretries = 5
    apiKey=os.environ.get("API_KEY").strip('"')
    apiToken=os.environ.get("API_TOKEN").strip('"')
    orgId = os.environ.get("ORG_NAME").strip('"')
    authId = os.environ.get("ORG_OWNER") #"IOTFTest"
    apiUrl = os.environ.get("REST_URL").strip('"')
    num_worker_threads = int(os.environ.get("NUM_THREADS"))
    csv_file = os.environ.get("CSV_DEVICE_FILE")
    json_file = os.environ.get("JSON_DEVICE_FILE")
#    num_worker_threads = 2
    workQ = queue.PriorityQueue()
    logQ = queue.PriorityQueue()
    main()
