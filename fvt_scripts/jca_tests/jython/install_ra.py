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
# install_ra.py
# 
#Script used by automation for installing and uninstalling the JCA resource
# adapter. This is called at the start of an automation run.
# 
# Return code: 0 = success, 1 = failure
# For cleanup, success = deleted objects or objects didn't exist
#              failure = objects were not deleted
#
# For setup,   success = installed the resource adapter
#              failure = resource adapter was already installed or failed
#                        installation
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
def installRarSingle(node):
    """
    Installs the resource adapter at the node level for a standalone server
    """
    global totalFails
    global dosave

    # Install the .rar file to the node if it doesn't already exist
    raScope = '/Cell:'+cell+'/Node:'+node+'/J2CResourceAdapter:'+raname
    rId = AdminConfig.getid(raScope)
    if rId == '':
        rId = AdminTask.installResourceAdapter('[-nodeName '+node+' -rarPath '+rarFile+' -rar.name "'+raname+'" -rar.archivePath ${CONNECTOR_INSTALL_ROOT} -rar.classpath /WAS/JMSTestDriver.jar -rar.DeleteSourceRar false]')

        rIdprops = AdminConfig.showAttribute(rId, 'propertySet')
        rIdResourceProps = AdminConfig.showAttribute(rIdprops, 'resourceProperties')
        rIdResourceProps = rIdResourceProps[1:-1].split(' ')

        if rarFile.find('ima.jmsra') > 0:
            trcLvl = rIdResourceProps[0]
            AdminConfig.modify(trcLvl,'[[name defaultTraceLevel] [required false] [type java.lang.String] [value 8]]')
            trcFile = rIdResourceProps[2]
            AdminConfig.modify(trcFile,'[[name traceFile] [required false] [type java.lang.String] [value /WAS/ra.trace]]')

        nodeId = AdminConfig.getid('/Cell:'+cell+'/Node:'+node)
        serversInNode = AdminConfig.list('Server',nodeId).splitlines()
        # Install the .rar file to each server in the node
        for server in serversInNode:
            rId = AdminConfig.getid(raScope)
            serverRar = AdminTask.copyResourceAdapter(rId, '[-scope '+server+' -name "'+raname+'" -useDeepCopy true]')
            if serverRar != '':
                trace('installRarSingle','Successfully installed resource adapter to server')
                if rarFile.find('ima.jmsra') > 0:
                    rIdprops = AdminConfig.showAttribute(serverRar, 'propertySet')
                    rIdResourceProps = AdminConfig.showAttribute(rIdprops, 'resourceProperties')
                    rIdResourceProps = rIdResourceProps[1:-1].split(' ')
                    trcLvl = rIdResourceProps[0]
                    AdminConfig.modify(trcLvl,'[[name defaultTraceLevel] [required false] [type java.lang.String] [value 8]]')
                    trcFile = rIdResourceProps[2]
                    AdminConfig.modify(trcFile,'[[name traceFile] [required false] [type java.lang.String] [value /WAS/ra.trace]]')

        dosave = 1
    else:
        trace('installRarSingle','Resource adapter already exists at scope: '+raScope)
        totalFails+=1
        return

    # Verify the resource adapter was installed on the node
    verify = AdminConfig.getid(raScope)
    if verify == '':
        totalFails+=1
        trace('installRarSingle','Failed to install resource adapter: '+raScope)
    else:
        trace('installRarSingle','Successfully installed resource adapter: '+raScope)

#------------------------------------------------------------------------------
def installRarCluster(nodes,clusterName,clusterId):
    """
    Installs the resource adapter at each Node that has servers in the cluster
    """
    global totalFails
    global dosave
    clusterRAScope = '/Cell:'+cell+'/ServerCluster:cluster1/J2CResourceAdapter:'+raname

    # Install the .rar file to each node in the cluster
    for clusterNode in nodes:
        installRarSingle(clusterNode)

    # Add the .rar file to the cluster if it doesn't already exist
    rId = AdminConfig.getid(clusterRAScope)
    if rId == '':
        rar = AdminConfig.getid("/Cell:"+cell+"/Node:"+nodes[0]+"/J2CResourceAdapter:"+raname)
        clusterRar = AdminTask.copyResourceAdapter(rar, '[-scope '+clusterId+' -name "'+raname+'" -useDeepCopy true]')
        AdminConfig.modify(clusterRar, '[[threadPoolAlias "Default"] [classpath "' +  CYGPATH + '/WAS/JMSTestDriver.jar"] [name "'+raname+'"] [isolatedClassLoader "false"] [nativepath ""] [description ""]]')
        dosave = 1

    else:
        trace('installRarCluster','Resource adapter already exists at scope: '+clusterRAScope)
        totalFails+=1
        return

    # Verify the resource adapter was added to the cluster
    verify = AdminConfig.getid(clusterRAScope)
    if verify == '':
        totalFails+=1
        trace('installRarCluster','Failed to install resource adapter: '+clusterRAScope)
    else:
        trace('installRarSingle','Successfully installed resource adapter: '+clusterRAScope)

#------------------------------------------------------------------------------
def deleteRar():
    """
    Delete all IBM MessageSight Resource Adapter's
    """
    global totalFails
    global dosave

    rarlist = AdminConfig.list('J2CResourceAdapter','*'+raname+'*').splitlines()
    for rar in rarlist:
        AdminConfig.remove(rar)

    verify = AdminConfig.list('J2CResourceAdapter','*'+raname+'*')
    if verify != '':
        totalFails+=1
        trace('deleteRar','Failed to delete resource adapters')
    else:
        trace('deleteRar','Successfully deleted '+raname)
        dosave = 1

#------------------------------------------------------------------------------
# Setup of ima.evilra.rar configuration objects for testing
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
def configureEvilRA():
    """
    Create J2CConnectionFactory 'TheEvilCon' from the ima.evilra.rar resource
    adapter.
    """
    trace('configureEvilRA','Creating J2CConnectionFactory "TheEvilCon"')

    # Create the TheEvilCon Connection Factory for Evil Resource Adapter
    if serverType == 'cluster':
        rScope = '/Cell:'+cell+'/ServerCluster:cluster1/'
    elif serverType == 'single':
        rScope = '/Cell:'+cell+'/Node:'+node+'/'
    rId = AdminConfig.getid(rScope+'J2CResourceAdapter:IBM MessageSight EVIL Resource Adapter')
    evilcf = AdminTask.createJ2CConnectionFactory(rId, ['-name', 'TheEvilCon', '-jndiName', 'TheEvilCon', '-connectionFactoryInterface', 'javax.resource.cci.ConnectionFactory'])

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
if (len(sys.argv) != 3):
    print 'Usage: /path/to/was/bin/wsadmin.sh -lang jython -user admin -password admin -f install_ra.py /path/to/ima.jmsra.rar cluster|single setup|cleanup'
    sys.exit(0)
else:
    rarFile = sys.argv[0]
    serverType = sys.argv[1]
    action = sys.argv[2]

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

tracefile = open(CYGPATH + '/WAS/install_ra.'+action+'.log','w')
dosave = 0

if serverType == 'cluster':
    trace('MAIN','Server Type: Cluster')
    clusterName = 'cluster1'
    clusterId = AdminConfig.getid('/Cell:'+cell+'/ServerCluster:cluster1/')
    nodes = getNodesInCluster(clusterId)

    if action == 'setup':
        trace('MAIN','Action: Setup')
        installRarCluster(nodes,clusterName,clusterId)

        if rarFile.find('evil') > 0:
            configureEvilRA()

        if dosave == 1:
            trace('MAIN','Changes were made to the cluster. Saving and sync\'ing now.')
            AdminConfig.save()
            syncNodes(nodes)

    else:
        trace('MAIN','Action: Cleanup')

        deleteRar()

        if dosave == 1:
            trace('MAIN','Changes were made to the cluster. Saving and sync\'ing now.')
            AdminConfig.save()
            syncNodes(nodes)
            
        stopCluster(clusterName)
        startCluster(clusterName)

elif serverType == 'single':
    server = 'server1'
    node = getNodeForServer(server)

    if action == 'setup':
        installRarSingle(node)

        if rarFile.find('evil') > 0:
            configureEvilRA()

    else:
        deleteRar()

    if dosave == 1:
        trace('MAIN','Changes were made to the server. Saving now.')
        AdminConfig.save()

if totalFails != 0:
    trace('MAIN','FAILURE: '+action+' failed')
    sys.exit(1)
else:
    trace('MAIN','SUCCESS: '+action+' succeeded')
    sys.exit(0)

tracefile.close()
