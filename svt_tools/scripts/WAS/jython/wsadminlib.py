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
import os, sys, time
import java.lang.System as system

#------------------------------------------------------------------------------
# Misc functions
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
def syncAllNodes(clusters):
    """
    Synchronize changes across nodes for each cluster
    """
    trace('syncAllNodes','Entering method')

    for cluster in clusters:
        id = AdminConfig.getid('/Cell:'+cell+'/ServerCluster:'+cluster+'/')
        nodes = getNodesInCluster(id)
        for clusterNode in nodes:
            sync = AdminControl.completeObjectName('type=NodeSync,node='+clusterNode+',*')
            trace('syncAllNodes','Synchronizing node: '+clusterNode)
            AdminControl.invoke(sync,'sync')

#------------------------------------------------------------------------------
def syncNodes(nodes):
    """
    Synchronize changes across the nodes in the cluster
    """
    trace('syncNodes','Entering method')

    for clusterNode in nodes:
        sync = AdminControl.completeObjectName('type=NodeSync,node='+clusterNode+',*')
        trace('syncNodes','Synchronizing node: '+clusterNode)
        AdminControl.invoke(sync,'sync')

#------------------------------------------------------------------------------
def getServerIDsForCluster(clusterId):
    """
    Return list of (serverID,nodeName,serverName) for each server in clusterId
    """
    trace('getServerIDsForCluster','Entering method')
    trace('clusterID',clusterId)

    serverIDList = []
    members = AdminConfig.showAttribute(clusterId,'members')
    members = members[1:-1].split(' ')
    for member in members:
        serverName = AdminConfig.showAttribute(member,'memberName')
        trace('serverName',serverName)
        nodeName = AdminConfig.showAttribute(member,'nodeName')
        trace('nodeName',nodeName)
        nodeID = AdminConfig.getid('/Cell:'+cell+'/Node:'+nodeName+'/')
        allServerIDs = AdminConfig.list('Server',nodeID).splitlines()
        for serverID in allServerIDs:
            name = AdminConfig.showAttribute(serverID,'name')
#            trace('serverID',serverID)
#            trace('serverID-name',name)
            if name == serverName:
                serverIDList.append((serverID,nodeName,serverName))
    return serverIDList

#------------------------------------------------------------------------------
def getNodesInCluster(clusterId):
    """
    Returns a list of each node within clusterId
    """
    trace('getNodesInCluster','Entering method')

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
def countClusters():
    """
    Count the number of ServerCluster's
    """
    trace('countClusters','Entering method')
    global clusters

    clusters = AdminConfig.list('ServerCluster').split()
    trace('countClusters','Found '+str(len(clusters))+' server clusters.')
    return len(clusters)

#------------------------------------------------------------------------------
def countServers():
    """
    Count the number of Application Servers
    """
    trace('countServers','Entering method')
    global servers

    servers = AdminConfig.list('Server').split()
    count = len(servers)
    # Don't count dmgr or nodeagent servers
    for server in servers:
        if server.count('dmgr') > 0:
            count-=1
            servers.remove(server)
        if server.count('nodeagent') > 0:
            count-=1
            servers.remove(server)
    trace('countServers','Found '+str(count)+' application servers.')
    return count

#------------------------------------------------------------------------------
def parseConfig(filename):
    """
    Parse was configuration file for the objects to create and app to install.
    Returns dictionary objects. Ex) {'ear.name': 'testTools_JCAtests'}
    """
    trace('parseConfig','Entering method on file:' + filename)
    COMMENT_CHAR = '#'
    OPTION_CHAR =  '='
    options = {}
    f = open(filename,'r')
    data = f.read()
    f.close()
    data = data.split("\n")
    for line in data:
        if COMMENT_CHAR in line:
            line, comment = line.split(COMMENT_CHAR, 1)
        if OPTION_CHAR in line:
            option, value = line.split(OPTION_CHAR, 1)
            option = option.strip()
            value = value.strip()
            options[option] = value
    return options

#------------------------------------------------------------------------------
def stopCluster( clustername ):
    """
    Stop the named server cluster
    """

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
    """
    Start the named server cluster
    """

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
