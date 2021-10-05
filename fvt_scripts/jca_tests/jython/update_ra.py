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
#------------------------------------------------------------------------------
# update_ra.py
#
# Script used by automation for updating the JCA resource
# adapter. This is called during automation migration runs.
#
# Return code: 0 = success, 1 = failure
#
#              success = updated the resource adapter
#              failure = did not update the resource adapter
#------------------------------------------------------------------------------

import os, sys, time, commands

#------------------------------------------------------------------------------
def trace(name,pos):
    """
    Trace function to print out messages with timestamps
    """
    global DEBUG
    if (DEBUG):
        formatting_string = '[%Y-%m-%d %H:%M:%S]'
        timestamp = time.strftime(formatting_string)
        tracefile.write('%s %s >>> %s\n' % (timestamp,name,pos))
        print('%s %s >>> %s\n' % (timestamp,name,pos))

#------------------------------------------------------------------------------
# Resource Adapter installation and unisntallation functions
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
def updateRarSingle(node):
    """
    Installs the resource adapter at the node level for a standalone server
    """
    global totalFails
    global dosave

    # Update the .rar file for the node
    raScope = '/Cell:'+cell+'/Node:'+node+'/J2CResourceAdapter:'+raname
    rId = AdminConfig.getid(raScope)
    if rId == '':
        trace('updateRarSingle','Why is the RA not already installed?')
        # What should be done here...
    else:
        trace('updateRarSingle','Updating resource adapter at scope: '+raScope)
	others = AdminTask.findOtherRAsToUpdate(rId)
	rId = AdminTask.updateRAR(rId, '[ -rarPath '+rarFile+']')
	othersArray = others.split('\n')
	for ra in othersArray:
		print ra
		AdminTask.updateRAR(ra, '[ -rarPath '+rarFile+']')

        dosave = 1
        return

    # Verify the resource adapter was installed on the node
    verify = AdminConfig.getid(raScope)
    if verify == '':
        totalFails+=1
        trace('updateRarSingle','Failed to install resource adapter: '+raScope)
    else:
        trace('updateRarSingle','Successfully installed resource adapter: '+raScope)

#------------------------------------------------------------------------------
def updateRarCluster(nodes,clusterName,clusterId):
    """
    Installs the resource adapter at each Node that has servers in the cluster
    """
    global totalFails
    global dosave
    clusterRAScope = '/Cell:'+cell+'/ServerCluster:cluster1/J2CResourceAdapter:'+raname

    # Install the .rar file to each node in the cluster
    for clusterNode in nodes:
        updateRarSingle(clusterNode)

    # Verify the resource adapter was added to the cluster
    verify = AdminConfig.getid(clusterRAScope)
    if verify == '':
        totalFails+=1
        trace('updateRarCluster','Failed to install resource adapter: '+clusterRAScope)
    else:
        trace('updateRarSingle','Successfully installed resource adapter: '+clusterRAScope)

#------------------------------------------------------------------------------
# Misc. functions
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
def syncNodes(nodes):
    """
    Synchronize changes across the nodes in the cluster
    """
    for clusterNode in nodes:
        trace('syncNodes','Sync\'ing node in cluster: '+clusterNode)
        sync = AdminControl.completeObjectName('type=NodeSync,node='+clusterNode+',*')
        AdminControl.invoke(sync,'sync')
        trace('syncNodes','Sync successful for node: '+clusterNode)

#------------------------------------------------------------------------------
def getServerIDsForCluster(clusterId):
    """
    Return list of (serverID,nodeName,serverName) for each server in cluster
    """
    serverIDList = []
    members = AdminConfig.showAttribute(clusterId,'members')
    members = members[1:-1].split(' ')
    for member in members:
        serverName = AdminConfig.showAttribute(member,'memberName')
        nodeName = AdminConfig.showAttribute(member,'nodeName')
        nodeID = AdminConfig.getid('/Cell:'+cell+'/Node:'+nodeName+'/')
        allServerIDs = AdminConfig.list('Server',nodeID).splitlines()
        for serverID in allServerIDs:
            name = AdminConfig.showAttribute(serverID,'name')
            if name == serverName:
                serverIDList.append((serverID,nodeName,serverName))
    return serverIDList

#------------------------------------------------------------------------------
def getNodesInCluster(clusterId):
    """
    Returns a list of each node within a cluster
    """
    nodes = []
    serverInfoList = getServerIDsForCluster(clusterId)
    for serverInfo in serverInfoList:
        if serverInfo[1] not in nodes:
            nodes.append(serverInfo[1])
    return nodes

#------------------------------------------------------------------------------
def getNodeForServer(server):
    """
    Returns the name of the node containing server
    """
    objName = AdminControl.completeObjectName('WebSphere:name='+server+',type=Server,*')
    node = AdminControl.getAttribute(objName,'nodeName')
    return node

#------------------------------------------------------------------------------
def stopCluster( clustername ):
    """Stop the named server cluster"""
    global totalFails
    cluster = AdminControl.completeObjectName( 'cell='+cell+',type=Cluster,name='+clustername+',*' )
    state = AdminControl.getAttribute( cluster, 'state' )
    if state != 'websphere.cluster.partial.stop' and state != 'websphere.cluster.stopped':
        AdminControl.invoke( cluster, 'stop' )
        trace('stopCluster','Stop cluster invoked')
    # Wait for it to stop
    maxwait = 150  # wait about 2.5 minutes
    count = 0
    trace('stopCluster','Waiting for cluster to stop')
    # First, we try with a plain stop
    while state != 'websphere.cluster.stopped':
        time.sleep( 30 )
        state = AdminControl.getAttribute( cluster, 'state' )
        trace('stopCluster','Current state: '+state)
        count += 1
        if count > ( maxwait / 30 ):
            trace('stopCluster','Waited 150 seconds for cluster to stop normally.')
            break
    state = AdminControl.getAttribute( cluster, 'state')
    # If plain stop wasn't enough, try to force it
    if state != 'websphere.cluster.stopped':
        trace('stopCluster','Cluster is still not stopped. Trying again with stopImmediate')
        AdminControl.invoke( cluster, 'stopImmediate')
        count = 0
        while state != 'websphere.cluster.stopped':
            time.sleep( 30 )
            state = AdminControl.getAttribute( cluster, 'state' )
            trace('stopCluster','Current state: '+state)
            count+=1
            if count > ( maxwait / 30 ):
                trace('stopCluster','Giving up')
                break
    state = AdminControl.getAttribute( cluster, 'state')
    trace('stopCluster',state)

#------------------------------------------------------------------------------
def startCluster( clustername ):
    """Start the named server cluster"""
    global totalFails
    cluster = AdminControl.completeObjectName( 'cell='+cell+',type=Cluster,name='+clustername+',*' )
    state = AdminControl.getAttribute( cluster, 'state' )
    if state != 'websphere.cluster.partial.start' and state != 'websphere.cluster.running':
        AdminControl.invoke( cluster, 'start' )
        trace('startCluster','Start cluster invoked')
    # Wait for it to start
    maxwait = 300  # wait about 5 minutes
    count = 0
    trace('startCluster','Waiting for cluster to start')
    while state != 'websphere.cluster.running':
        time.sleep( 30 )
        state = AdminControl.getAttribute( cluster, 'state' )
        trace('startCluster','Current state: '+state)
        count += 1
        if count > ( maxwait / 30 ):
            trace('startCluster','Giving up')
            break
    trace('startCluster', state)

#------------------------------------------------------------------------------
# MAIN
#------------------------------------------------------------------------------
if (len(sys.argv) != 2):
    print 'Usage: /path/to/was/bin/wsadmin.sh -lang jython -user admin -password admin -f update_ra.py /path/to/ima.jmsra.rar cluster|single'
    sys.exit(0)
else:
    rarFile = sys.argv[0]
    serverType = sys.argv[1]

DEBUG = 1
cell = AdminControl.getCell()
totalFails = 0

if 'OS' in os.environ.keys() and os.environ['OS'].find('NT') >= 0:
    CYGPATH = 'C:/cygwin'
else:
    CYGPATH = ''

if rarFile.find('evil') > 0:
    raname = 'IBM MessageSight EVIL Resource Adapter'
else:
    raname = 'IBM MessageSight Resource Adapter'

tracefile = open(CYGPATH + '/WAS/update_ra.log','w')
dosave = 0

if serverType == 'cluster':
    try:
        trace('MAIN','Server Type: Cluster')
        clusterName = 'cluster1'
        clusterId = AdminConfig.getid('/Cell:'+cell+'/ServerCluster:cluster1/')
        nodes = getNodesInCluster(clusterId)

        trace('MAIN','Action: Setup')
        stopCluster(clusterName)
        updateRarCluster(nodes,clusterName,clusterId)

        if dosave == 1:
            trace('MAIN','Changes were made to the cluster. Saving and sync\'ing now.')
            AdminConfig.save()
            syncNodes(nodes)

    finally:
        # Make sure startCluster always runs
    	startCluster(clusterName)

elif serverType == 'single':
    trace('MAIN','Server Type: Single')
    server = 'server1'
    node = getNodeForServer(server)

    updateRarSingle(node)

    if dosave == 1:
        trace('MAIN','Changes were made to the server. Saving now.')
        AdminConfig.save()

if totalFails != 0:
    trace('MAIN','FAILURE: update_ra failed')
    sys.exit(1)
else:
    trace('MAIN','SUCCESS: update_ra succeeded')
    sys.exit(0)

tracefile.close()
