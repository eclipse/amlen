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
from requests.models import PreparedRequest
from threading import Semaphore, BoundedSemaphore

class iotf_device(object):
    def __init__(self,deviceId,deviceType=None,devicePw=None):
        self.retries = 0
        self.deviceId = deviceId
        self.deviceType = deviceType
        self.devicePw = devicePw
    def __lt__(self, other):
        return self.retries < other.retries
    def __str__(self, *args, **kwargs):
        return "deviceId : {0}, deviceType: {1} devicePw : {2}".format(self.deviceId,self.deviceType, self.devicePw)

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

def load_devices():
    global logger,num_worker_threads,maxretries,apiKey,apiToken,orgId, authId,apiUrl,num_worker_threads,workQ,logQ,csv_file,json_file,deviceCountStart,deviceCountFinish
    fileName = None
    returnList = []
    if csv_file is not None :
        with open(csv_file, newline="") as devicesFile:
            reader = csv.DictReader(devicesFile,fieldnames=("deviceId","deviceType","devicePW"))
            for row in reader:
                thisItem = iotf_device(row['deviceType'],row['deviceId'],row["devicePW"])
                thisItem.retries=0
                returnList.append(thisItem)
    elif json_file is not None: 
        with open(json_file, newline="") as devicesFile:
            reader = json.load(devicesFile)
            for row in reader:
                thisItem = iotf_device(row['deviceId'],row['deviceType'],row["devicePW"])
                thisItem.retries=0
                returnList.append(thisItem)
    else: 
        device_list = range(deviceCountStart,deviceCountFinish)
        for item in device_list:
            thisItem = iotf_device(str(item),"test","thisisatestPassword")
            thisItem.retries=0
            returnList.append(thisItem)
    return returnList

def register_device (item):
    global logger,num_worker_threads,maxretries,apiKey,apiToken,orgId,authId,apiUrl,num_worker_threads,workQ,logQ,device_count,req_device_type
    devStartTime = time()

    logger.info("entered register_device")

    logMessage = ("I","completed successfully")
    authHeaders = {'id': authId, 'type': "person", 'content-type': 'application/json'}
        

#    if deviceType is None:
#    else:
#     url = "https://"+orgId+apiUrl + "devices/{deviceType}".format(orgId,deviceType)
#    url = apiUrl + '/%s/devices' % (orgId)
    payload = []
    logger.info ("item passed in is {0}".format(item))
    logger.info ("there were {0} items passed in".format(len(item)))
#    print(str(item))
    i = 0
    try: 
        logger.info("len of item is {0}".format(len(item)))
        if len(item) >= 1:
            logger.info("len of payload is {0}".format(len(payload)))
            if len(payload) > 1:
                logger.info("clearing payoad")
                payload.clear()
                for thisDevice in item:
                    logger.info("appending to payload")
                    payload.append({"type":thisDevice.deviceType,"id": thisDevice.deviceId,"type":thisDevice.deviceType,"password":thisDevice.devicePw})
            else:
                logger.info("not clearing payload")
                for thisDevice in item:
                    logger.info("appending to payload")
                    payload.append({"type":thisDevice.deviceType,"id": thisDevice.deviceId,"type":thisDevice.deviceType,"password":thisDevice.devicePw}) 
    except Exception as T:
        logger.exception(T)
        raise E

    logger.info("payload is {0}".format(payload))

    url = "https://"+ orgId + "."+apiUrl +  "/bulk/devices/add"
    logger.info(url)
    logger.info("api key = {0} and api token = {1}".format(apiKey,apiToken))
    logger.info("there were {0} devices in the payload".format(len(payload)))
    if len(payload) > 1:
        payload = json.dumps(payload, sort_keys=True)
    startTime = time()
    logger.info("about to call url")
    try:     
        try:
            r = requests.post(url, data=payload, headers=authHeaders, auth=(apiKey,apiToken),timeout=100,verify=False)
        except Exception as T:
            logger.exception(T)
            raise T
#        return
        logger.info("Http return code is {0}".format(r.status_code))
        if r.status_code == 202 or r.status_code == 201:
            finishTime= time()
            duration = finishTime-startTime
            logMessage = ("L","Returned from rest call in {0} seconds".format(duration))
            thisMsg = iotf_log(time(),0,duration,0,logMessage)
            logQ.put(thisMsg)

            deviceList = r.json()
            print(len(deviceList))
            for device in deviceList: 
                logger.info("device = {0}".format(str(device)))
                successValue = str(device["success"])
                logger.debug("success value is '{0}' and comparison is {1}".format(successValue, successValue is "True"))
                if str(device["success"]) == "True":
                    deviceReturnedId = device["id"]
                    if device["password"] is not None:
                        deviceAuthToken = device["password"]
                    deviceReturnedType = device["type"]
                    logger.debug(" * Registered device: %s:%s:%s (%s)" % (orgId, deviceReturnedType, deviceReturnedId, deviceAuthToken))
                    logMessage = ("I"," Registered device: %s:%s:%s (%s) " % (orgId, deviceReturnedType, deviceReturnedId, deviceAuthToken))
                else: 
                    deviceReturnedId = device["id"]
                    deviceReturnedType = device["type"]
                    logger.debug(" * Failed to Register device: %s:%s:%s " % (orgId, deviceReturnedType, deviceReturnedId))
                    logMessage = ("E"," Failed to Register device: %s:%s:%s " % (orgId, deviceReturnedType, deviceReturnedId))
        else:
            logger.critical("Error code when registering the device: %s" % (r.status_code))
            logger.critical("Headers from failed request: " + str(r.request.headers))
            logger.critical("Body from failed request: '{0}' ".format(r.request.body))
            logger.critical("Response content " + r.text+" headers "+str(r.headers))
            logMessage = ("E"," Failed to register with status %s : Device info %s " % (r.status_code,orgId))        
    except Exception as E:
        logger.error("Completed post")
        logger.exception(E)
        logMessage = ("E"," Failed to register with Exception %s : Device info %s " % (str(E),orgId))
        raise E
#    print(item.deviceId," ",item.retries)
    duration = time() - devStartTime
    
    for thisDevice in item:
        thisMsg = iotf_log(time(),thisDevice.deviceId,duration,thisDevice.retries,logMessage)
        logQ.put(thisMsg)

    for thisDevice in item:
        if logMessage[0]=="E" :
            thisDevice.retries += 1  
            if thisDevice.retries < maxretries:
                workQ.put(thisDevice)
                logMessage = ("E","Retrying device: %s after %d of %d tries "%(thisDevice.deviceId,thisDevice.retries,maxretries))
                thisMsg = iotf_log(time(),thisDevice.deviceId,duration,thisDevice.retries,logMessage)
                logQ.put(thisMsg)
        else:
            logMessage = ("E","device: {0} Exceeded maximum retries of {1} tries.".format(thisDevice.deviceId,maxretries))
            thisMsg = iotf_log(time(),thisDevice.deviceId,duration,thisDevice.retries,logMessage)
            logger.error("Devices left on queue = {0}".format(device_count))
            device_count-=1
def worker():
    global logger,num_worker_threads,maxretries,apiKey,apiToken,orgId, authId,apiUrl,num_worker_threads,workQ,logQ
    s = requests.session()
    s.keep_alive = False
    deviceList = list()
    deviceList.clear()
    deviceCount = 0 
    batchSize = 10
    while not workQ.empty():
        if workQ.qsize() < batchSize :
            logger.info("queue size is {0}")
            batchSize = workQ.qsize()
        logger.info("queue size is {0}".format(workQ.qsize()))
        logger.info("batch size is {0}".format(batchSize))
        for deviceCount in range(0,batchSize) : 
            logger.info("device count is {0}".format(deviceCount))
            device = workQ.get()
            logger.info("device retrieved is {0}".format(device))
            deviceList.append(device)
            workQ.task_done()
        workTime = time()
        try: 
            register_device(deviceList)
        except Exception as E:
            logger.exception(E)
            raise E
        duration = time() - workTime
        for item in deviceList:
            logMessage = ("I","Completed processing item {0} in thread {1} ".format(item.deviceId,str(threading.current_thread().ident) ) )
            thisMsg = iotf_log(time(),item.deviceId,duration,item.retries,logMessage)
            logQ.put(thisMsg)
        deviceList.clear()   
    logger.debug("finished worker")

def main():
    global logger,num_worker_threads,maxretries,apiKey,apiToken,orgId, authId,apiUrl,num_worker_threads,workQ,logQ,device_count
    startTime = time()
    logMessage = ("I","Starting main execution")
    thisMsg = iotf_log(time(),0,0,0,logMessage)
    logQ.put(thisMsg)
#    url = "https://"+orgId+"."+apiUrl + "/device/"
#    logger.info(url)

    loadedDevices = load_devices()
    if len(loadedDevices) < 1 :
        logger.info("No items to process, exiting")
        return 

    for item in loadedDevices:
        workQ.put(item)
        device_count +=1
    logger.info("finished loading devices onto queue")
    
    logger.info("Creating {0} threads to process work".format(num_worker_threads))
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
    logger.debug("finish time is {0}, start time is {1} and elapsed time is {2}".format(finishTime,startTime,elapsedTime))
    while not logQ.empty():
        printItem = logQ.get()
        logQ.task_done()
        if printItem.log_message[0] == 'E' or printItem.log_message[0]=='L':
            print(str(printItem))
            #do I need to do this loop? 
    print ('Start time: {0} Finish time: {1}'.format( startTime,finishTime))
    print ('Average devices per second {0}'.format(device_count/elapsedTime))

logger = None
num_worker_threads = 2

maxretries = 0
apiKey=None
apiToken=None
orgId = None
authId = None
apiUrl = None
num_worker_threads = None
workQ = None
logQ = None
device_count = 0
deviceCountStart = 0
deviceCountFinish = 0
csv_file = None
json_file = None
req_device_type = None

if __name__ == "__main__":
    logger = logging.getLogger("register")
    logger.setLevel(logging.INFO)
    ch = logging.StreamHandler()
    ch.setLevel(logging.DEBUG)
    chFormatter = logging.Formatter('%(asctime)-25s %(name)-15s ' + ' %(levelname)-8s %(message)s')
    ch.setFormatter(chFormatter)
    logger.addHandler(ch)
    
    maxretries = int(5)
    apiKey=os.environ.get("API_KEY").strip('" \t')
    apiToken=os.environ.get("API_TOKEN").strip('" \t')
    logger.info("api key = {0} and api token = {1}".format(apiKey,apiToken))
    sys.exit
    orgId = os.environ.get("ORG_NAME").strip('" \t')
    authId = os.environ.get("ORG_OWNER") #"IOTFTest"
    apiUrl = os.environ.get("REST_URL").strip('" \t')
    csv_file = os.environ.get("CSV_DEVICE_FILE")
    json_file = os.environ.get("JSON_DEVICE_FILE")
    if not json_file and not csv_file :
        deviceCountStart = int(os.environ.get("FIRST_DEVICE_ID"))
        deviceCountFinish = int(os.environ.get("LAST_DEVICE_ID"))
#    print("Device Start Count {0} Device Finish Count {1}".format(deviceCountStart,deviceCountFinish))
    num_worker_threads = int(os.environ.get("NUM_THREADS"))
#    req_device_type = os.environ.get("DEVICE_TYPE")
#    print("required device type is "+req_device_type)
#    num_worker_threads = 2
    workQ = queue.PriorityQueue()
    logQ = queue.PriorityQueue()
    main()
