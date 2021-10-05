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


def load_devices():
    global logger,maxretries,apiKey,apiToken,orgId, authId,apiUrl,workQ,logQ, deviceType

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
    if r.status_code != 200:
        logger.critical("Error code when listing the devices: {0}".format(r.status_code))
        logger.critical("Headers from failed request: {0}".format(str(r.request.headers)))
        logger.critical("Body from failed request: {0}".format(str(r.request.body)))
        logger.critical("Response content " + r.text)
        sys.exit()
    rj = r.json()
    logger.info("size of returned list is {0}".format(len(rj)))
    for dev in rj : 
        logger.info("current device returned {0}".format(dev))
        thisItem = iotf_device(dev["id"],dev["type"])
        thisItem.retries=0        
        device_list.append(thisItem)
    logger.info("finished loading list with {0}".format(len(device_list)))
    return device_list

def register_device (item):
    global logger,maxretries,apiKey,apiUrl,apiToken,orgId, authId,workQ,logQ,device_count
    devStartTime = time()

    logMessage = ("I","completed successfully")
    authHeaders = {'id': authId, 'type': "person", 'content-type': 'application/json'}

    logger.info("cleaning up device {0}".format(item.deviceId))

#    url = "https://"+orgId+apiUrl + "/devices/{0}/{1}".format(item.deviceType,item.deviceId)
#    url = apiUrl + '/{0}/devices/{1}/{2}' % (orgId,item.deviceType,item.deviceId)
    url = "https://"+orgId+"."+apiUrl + "/devices"+"/"+item.deviceType+"/"+item.deviceId
    try:     
        r = requests.delete(url,auth=(apiKey,apiToken),timeout=100,verify=False)  
        if r.status_code == 204:
            logger.debug("Deleted device: {0}:{1}:{2}".format(orgId, item.deviceType, item.deviceId))
            logMessage = ("I","Deleted device: {0}:{1}:{2}".format(orgId, item.deviceType, item.deviceId))
        else:
            logger.critical("Error code when deleting the device: {0}".format(r.status_code))
            logger.critical("Headers from failed request: {0}".format(str(r.request.headers)))
            logger.critical("Body from failed request: {0}".format(str(r.request.body)))
            logger.critical("Response content " + r.text)
            logMessage = ("E"," Failed to delete with status {0} : Device info {1}:{2}:{3} ".format(r.status_code,orgId, item.deviceType, item.deviceId))
        
    except Exception as E:
        logger.exception(E)
        logMessage = ("E"," Failed to delete with Exception %s : Device info %s:%s:%s " % (str(E),orgId, item.deviceType,item.deviceId))
#    print(item.deviceId," ",item.retries)
    duration = time() - devStartTime
    
    thisMsg = iotf_log(time(),item.deviceId,duration,item.retries,logMessage)
    logQ.put(thisMsg)

    if logMessage[0]=="E" :
        logger.error("device {0} generated error type {1} and {2}".format(item.deviceId,logMessage[0],logMessage[1]))
        item.retries += 1  
        logger.error("retries {0} for device {1} with max retries {2}".format(item.retries,item.deviceId, maxretries))
        if item.retries < maxretries:
            workQ.put(item)
            logMessage = ("R","Retrying device: {0} after {1} of {2} tries ".format(item.deviceId,item.retries,maxretries))
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
        register_device(item)
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
            logger.info("starting thread {0}".format(i))
            t.start()
            while not t.is_alive():
                0
            logger.info("thread {0} was started".format(i))
        except Exception as E:
           logger.exception(E)
    logger.info("All child threads started")

    logger.info("Devices put on queue = {0}".format(device_count))
    
    ##  Make sure all threads are not alive
    while threading.active_count() > 1:
        0
    ##
    logger.info("All child threads finished")
    
    finishTime = time()
    elapsedTime = finishTime-startTime
    while not logQ.empty():
        printItem = logQ.get()
        logQ.task_done()
        if printItem.log_message[0] == 'E':
            print(str(printItem))
            #do I need to do this loop? 
    print ('Start time: {0} Finish time: {1}'.format(startTime,finishTime))
    print ('Successfully Deleted {0} devices over elapsed time: {1} '.format(device_count,elapsedTime))
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
    ch.setLevel(logging.DEBUG)
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
