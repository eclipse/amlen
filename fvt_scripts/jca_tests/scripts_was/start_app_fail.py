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
# start_app_fail.py
#
# ...
#------------------------------------------------------------------------------
import os, sys, time
import java.lang.System as system

#------------------------------------------------------------------------------
def trace(name,pos):
    """
    Trace function to print out messages with timestamps
    """
    global DEBUG
    if (DEBUG):
        formatting_string = '[%Y-%m-%d %H:%M:%S]'
        timestamp = time.strftime(formatting_string)
        print '%s %s >>> %s' % (timestamp,name,pos)

#------------------------------------------------------------------------------
# Application installation functions
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
def startApplicationOnServer(appname,nodename,servername):
    """
    Start the named application on one server
    """
    trace('startApplicationOnServer','Entering method')
    # Get the application manager
    appManager = AdminControl.queryNames('cell=%s,node=%s,type=ApplicationManager,process=%s,*' %(cell,nodename,servername))
    # start the app
    try:
        trace('startApplicationOnServer','Starting app on server '+servername+', node '+nodename)
        AdminControl.invoke(appManager, 'startApplication', appname)
    except:
        trace('startApplicationOnServer','Failed to start the application as expected')
        return
    trace('startApplicationOnServer','FAILURE: Successfully launched application')
    global totalFails
    totalFails+=1

#------------------------------------------------------------------------------
def startApplicationOnCluster(appname,clustername):
    """
    Start the named application on all servers in named cluster
    """
    trace('startApplicationOnCluster','Entering method')
    clusterId = AdminConfig.getid('/Cell:'+cell+'/ServerCluster:'+clustername+'/')
    clusterMembers = AdminConfig.list('ClusterMember',clusterId).splitlines()
    for member in clusterMembers:
        nodename = AdminConfig.showAttribute(member,'nodeName')
        servername = AdminConfig.showAttribute(member,'memberName')
        startApplicationOnServer(appname,nodename,servername)

#------------------------------------------------------------------------------
def launchApp(serverTargets,clusterTargets):
    """
    Gets the options for installing the app, installs the app, and then
    launches the app.
    """
    trace('launchApp','Entering method')

    if clusterCount > 0:
        syncAllNodes(clusterTargets)
        for cluster in clusterTargets:
            startApplicationOnCluster(appName,cluster)
    if serverCount > 0:
        for server in serverTargets:
            servername = server['servername']
            nodename = server['nodename']
            startApplicationOnServer(appName,nodename,servername)

    AdminConfig.save()

#------------------------------------------------------------------------------
def init():
    """
    Run setup or cleanup for each cluster and server in the config file
    """
    trace('init','Entering method')
    global servers
    global clusters
    clusterTargets = []
    serverTargets = []

    for i in range(0,clusterCount):
        serverType = 'cluster'
        clusterId = clusters[i]
        clusterName = AdminConfig.showAttribute(clusterId,'name')
        clusterMembers = AdminConfig.list('ClusterMember',clusterId).splitlines()
        nodes = getNodesInCluster(clusterId)
        trace('init','setting up '+clusterName)
        clusterTargets.append(clusterName)
        
    for i in range(0,serverCount):
        serverType = 'single'
        serverId = servers[i]
        server = AdminConfig.showAttribute(serverId,'name')
        node = getNodeForServer(server)
        serverTargets.append({'servername': server, 'nodename': node})

    launchApp(serverTargets,clusterTargets)

##----------------------------------------------
## "MAIN" 
##----------------------------------------------

if (len(sys.argv) < 2 or len(sys.argv) > 3):
    print 'Usage: start_app_fail.py fail.config [debug]'
    print 'debug: 1 = on, default = off'
    sys.exit(0)
else:
    file   = sys.argv[0]
    action = sys.argv[1]
    if len(sys.argv) > 2:
        DEBUG = 1
    else:
        DEBUG = 0
    
cell = AdminControl.getCell()

if 'OS' in os.environ.keys() and os.environ['OS'].find('NT') >= 0:
    CYGPATH = 'C:/cygwin'
else:
    CYGPATH = ''

execfile(CYGPATH+'/WAS/wsadminlib.py')
# Parse the configuration file that was passed in
options = parseConfig(file)


a1Host = options['a1_host']

cfCount = options['cf.count']
destCount = options['dest.count']
mdbCount = options['mdb.count']

clusters = []
servers = []

# Allow cluster or server lists to be defined in test.config instead of just using
# what can be found in wsadmin.
# This requires both cluster.count and server.count to be defined in test.config
if 'cluster.count' in options.keys() and 'server.count' in options.keys():
    clusterCount = int(options['cluster.count'])
    for i in range (0,clusterCount):
        name = options['cluster.name.'+str(i)]
        id = AdminConfig.getid('/Cell:'+cell+'/ServerCluster:'+name)
        clusters.append(id)
    serverCount = int(options['server.count'])
    for i in range (0,serverCount):
        servername = options['server.name.'+str(i)]
        servernode = options['server.node.'+str(i)]
        id = AdminConfig.getid('/Cell:'+cell+'/Node:'+servernode+'/Server:'+servername)
        servers.append(id)

else:
    # This will either set up:
    #    - any number of clusters, or
    #    - any number of individual servers
    #
    # For the individual servers, they may be in an ND install
    # or standalone install.
    # We only use 1 cluster or 1 standalone server for FVT automation
    # 
    # If either cluster.count or server.count is in test.config, it will be
    # ignored in this case.
    #
    # Figure out if we are in dmgr cell or not
    wsadminNode = AdminControl.getNode().count('CellManager')
    if wsadminNode == 1:
        # Count how many clusters there are if we are connected to a managed node
        clusterCount = countClusters()
        if clusterCount > 0:
            # If we have clusters, set servers to 0
            serverCount = 0
        else:
            # If we have no clusters, count how many servers there are
            serverCount = countServers()
    else:
        # Unmanaged node, will only be servers
        serverCount = countServers()
        clusterCount = 0

# Name of test enterprise application
baseAppName = options['ear.name']
appName = baseAppName
# Path to EAR file which contains the EJB and WAR files
appPath = CYGPATH + options['ear.path']
# Name of resource adapter
raname = 'IBM MessageSight Resource Adapter'

totalFails = 0

init()

if totalFails > 0:
    print 'FAILURE: Setup completed with '+str(totalFails)+' errors'
else:
    print 'SUCCESS: Setup completed with 0 errors'
