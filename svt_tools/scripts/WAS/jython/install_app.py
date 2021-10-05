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
def installApplication(filename,servers,clusternames,myoptions,appIndex):
    """
    Install the application at the given filename to each of the 
    servers and clusters listed. 
    """
    trace('installApplication','Entering method')
    trace('filename',filename)
    trace('servers',servers)
    trace('clusternames',clusternames)
##    trace('myoptions',myoptions)

    targets = []
    for server in servers:
        targets.append("WebSphere:cell=%s,node=%s,server=%s" % (cell, server['nodename'], server['servername']))
    for clustername in clusternames:
        targets.append("WebSphere:cell=%s,cluster=%s" % (cell,clustername))

    target = "+".join( targets )
    trace('target', target)
    arglist = ['-target', target, '-appname', options['app.name.'+str(appIndex)] ]
#    arglist = ['-target', target, '-appname', appName]

    if myoptions != None:
        if not "-MapWebModToVH" in myoptions:
            arglist.extend( ['-MapWebModToVH', [['.*', '.*', 'default_host']]] )
        if not "-CtxRootForWebMod" in myoptions:
            print '  %==>  BUILD arglist for CtxRootForWebMod'
            trace( 'baseAppName',baseAppName)
            arglist.extend( ['-CtxRootForWebMod', [[ baseAppName, baseAppName+'.war,WEB-INF/web.xml', baseAppName ]]])
##            trace( 'arglist', arglist)

    if myoptions != None:
        arglist.extend(myoptions)
    print '  %==>  BEFORE AdminApp.install( filename, arglist )'
    trace('filename',filename)
    trace('arglist',arglist)
    AdminApp.install( filename, arglist )
    print '  %==>  AFTER AdminApp.install( filename, arglist )'

#------------------------------------------------------------------------------
def parseTask(taskName, appIndex):
    """
    Parse the task arguments required for installing the application.
    1. MapResRefToEJB
    2. MapMessageDestinationRefToEJB
    3. BindJndiForEJBMessageBinding
    """
    trace('parseTask','Entering method for '+taskName)
    content = AdminApp.taskInfo(appPath,taskName)
    # Skip down to the actual data
    parts = content.split('The current contents of the task after running default bindings are:\n')
    if len(parts) < 2:
        return None
    bits = parts[1].splitlines()
    trace('bits:', bits)
    myoptions = [[]]
    optIter = 0
    
    for i in range(0, len(bits)):
        if bits[i] != '':
            piece = bits[i].split(': ')
            if len(piece) > 1:
                title = piece[0]
                piece = piece[1]
                trace('title:', title)
                trace('piece:', piece)

                if piece != '' and piece != 'null':
                    if title == 'Target Resource JNDI Name':
                        if taskName == 'BindJndiForEJBMessageBinding':
                            piece = options['app.asptarget.'+str(appIndex)]
                            trace('NEW asptarget piece:', piece)
                        elif taskName == 'MapResRefToEJB':
                            piece = options['app.cftarget.'+str(appIndex)]
                            trace('NEW cftarget piece:', piece)
                        else:
                            piece = piece
#                            piece = piece+'_'+m1Name
                if title == 'Destination JNDI name':
                    if taskName == 'BindJndiForEJBMessageBinding':
                        piece = options['app.appdest.'+str(appIndex)]
                        trace('NEW appdest piece:', piece)
                    else:
                    	piece = piece
                
                if title == 'ActivationSpec authentication alias':
                    if piece == 'null':
                        piece = ""
                        trace('NEW AS Auth Alias:', piece)

                myoptions[optIter].append(piece)
        else:
            optIter+=1
            myoptions.append([])
    # We end up with 1 extra empty element at the end of the list.
    # Remove it.
    myoptions.pop(optIter)
    return myoptions

#------------------------------------------------------------------------------
def generateOptions(appIndex):
    """
    Use AdminApp.taskInfo() to generate options used for installing the
    application.
    """
    trace('generateOptions','Entering method')
    appOptions = []

    # Map activation specification references
    bindJndiForEJBMsgBinding = '-BindJndiForEJBMessageBinding'
    bindJndiArgs = parseTask('BindJndiForEJBMessageBinding', appIndex)
    trace('generateOptions','\nBindJndiForEJBMessageBinding:')
##    trace('generateOptions',bindJndiArgs)
    if bindJndiForEJBMsgBinding != None:
        appOptions.append(bindJndiForEJBMsgBinding)
        appOptions.append(bindJndiArgs)

    # Map connection factory references
    mapResRefToEJB = '-MapResRefToEJB'
    bindEJBJndiArgs = parseTask('MapResRefToEJB', appIndex)
    trace('generateOptions','\nMapResRefToEJB:')
    trace('generateOptions',bindEJBJndiArgs)
    if mapResRefToEJB != None:
        appOptions.append(mapResRefToEJB)
        appOptions.append(bindEJBJndiArgs)

    return appOptions

#------------------------------------------------------------------------------
def startApplicationOnServer(appname,nodename,servername):
    """
    Start the named application on one server
    """
    trace('startApplicationOnServer','Entering method')
    # Get the application manager
    appManager = AdminControl.queryNames('cell=%s,node=%s,type=ApplicationManager,process=%s,*' %(cell,nodename,servername))
    # start the app
    trace('startApplicationOnServer','Starting app on server '+servername+', node '+nodename)
    AdminControl.invoke(appManager, 'startApplication', appname)

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
def launchApp(serverTargets,clusterTargets,appIndex):
    """
    Gets the options for installing the app, installs the app, and then
    launches the app.
    """
    trace('launchApp','Entering method')
    realAppName = options['app.name.'+str(appIndex)]
    print '  %==>  realAppName: ' +realAppName
    print '  %==>  AppIndex: '+ str(appIndex)

    appOptions = generateOptions(appIndex);
    installApplication(appPath, serverTargets, clusterTargets, appOptions, appIndex)

    # Verify that the application was installed before launching
    verify = AdminApp.list()
    verify = verify.splitlines()
    success = 0
    for line in verify:
        if line == realAppName:
            trace('launchApp','Application '+realAppName+'installed successfully')
            success = 1
    if success != 1:
        trace('launchApp','Application installation failed for: '+realAppName)
        global totalFails
        totalFails+=1
        return

    AdminConfig.save()
    if clusterCount > 0:
        syncAllNodes(clusterTargets)
        for cluster in clusterTargets:
            startApplicationOnCluster(realAppName,cluster)
    if serverCount > 0:
        for server in serverTargets:
            servername = server['servername']
            nodename = server['nodename']
            startApplicationOnServer(realAppName,nodename,servername)

    AdminConfig.save()

     # Verify application is running
    apps = AdminApp.list().splitlines()

    for app in apps:
        if realAppName == app:
#        if appName == app:
            trace('launchApp','Application '+realAppName+' launched successfully!')
#            trace('launchApp','Application '+appName+' launched successfully!')
            return
    trace('launchApp','Application '+realAppName+' failed to launch!')
#    trace('launchApp','Application '+appName+' failed to launch!')
    global totalFails
    totalFails+=1

#------------------------------------------------------------------------------
def stopAppOnServer(cellname,nodename,servername):
    """
    Stop application appName on the specified server
    """
    trace('stopAppOnServer','Entering method')

    appManager = AdminControl.queryNames('cell='+cellname+',node='+nodename+',type=ApplicationManager,process='+servername+',*')
    lineSep = system.getProperty('line.separator')

    apps = AdminControl.queryNames('cell='+cellname+',node='+nodename+',type=Application,process='+servername+',*').split(lineSep)
    for app in apps:
        appnamefromlist = AdminControl.getAttribute(app,'name')
        if appnamefromlist == appName:
            apprunning = AdminControl.completeObjectName('type=Application,name='+appnamefromlist+',*')
            if apprunning != '':
                trace('stopAppOnServer','Stopping '+appName+' on server '+servername)
                AdminControl.invoke(appManager,'stopApplication',appnamefromlist)
            else:
                trace('stopAppOnServer','Application '+appName+' is not running on server '+servername)

#------------------------------------------------------------------------------
def stopAppOnCluster(nodes,members,id):
    """
    Stop the application appName on all servers in the specified cluster
    """
    trace('stopAppOnCluster','Entering method')
    # serverID, nodename, server name
    serverIDs = getServerIDsForCluster(id)
    for server in serverIDs:
        stopAppOnServer(cell,server[1],server[2])

#------------------------------------------------------------------------------
def uninstallApp():
    """
    Uninstall the test application
    """
    trace('uninstallApp','Entering method')
    appList = AdminApp.list()
    appList = appList.splitlines()
    for app in appList:
        if app == appName:
            AdminApp.uninstall(appName)

    appList = AdminApp.list()
    appList = appList.splitlines()
    for app in appList:
        if app == appName:
            trace('uninstallApp','Failed to uninstall application: '+appName)
            global totalFails
            totalFails+=1
            return
    trace('uninstallApp','Successfully uninstalled application: '+appName)

#------------------------------------------------------------------------------
def purgeConnections():
    trace('purgeConnections','Entering method')
    cflist = AdminControl.queryNames('*:name=*,type=J2CConnectionFactory,*').splitlines()
#    cflist = AdminControl.queryNames('*:name=*_'+m1Name+'*,type=J2CConnectionFactory,*').splitlines()

    for cfname in cflist:
    	try:
            AdminControl.invoke(cfname,'pause')
            AdminControl.invoke(cfname,'purgePoolContents')
        except:
            trace('purgeConnections','Caught exception during purge')

#------------------------------------------------------------------------------
def cleanupCluster(id,members,nodes):
    """
    Main function for cleaning up of the application server
    """
    trace('cleanupCluster','Entering method')

    # Stop the app
    stopAppOnCluster(nodes,members,id)

    # Purge the connection factories
    purgeConnections()

    # uninstall the app
    uninstallApp()

    global totalFails
    if totalFails > 0:
        AdminConfig.reset()
        trace('cleanupCluster','Cluster configuration cleanup failed. Resetting changes')
    else:
        AdminConfig.save()
        syncNodes(nodes)
        trace('cleanupCluster','Cluster configuration cleanup was successful')

#------------------------------------------------------------------------------
def cleanupServer(name,node):
    """
    Main function for cleaning up of the application server
    """
    trace('cleanupServer','Entering method')

    # Stop the app
    stopAppOnServer(cell,node,name)

    # Purge the connection factories
    purgeConnections()

    # uninstall the app
    uninstallApp()

    global totalFails
    if totalFails > 0:
        AdminConfig.reset()
        trace('cleanupServer','Server configuration cleanup failed. Resetting changes')
    else:
        AdminConfig.save()
        trace('cleanupServer','Server configuration cleanup was successful')

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
        if action == 'setup':
            trace('init','setting up '+clusterName)
            clusterTargets.append(clusterName)
        else:
            cleanupCluster(clusterId,clusterMembers,nodes)

    for i in range(0,serverCount):
        serverType = 'single'
        serverId = servers[i]
        server = AdminConfig.showAttribute(serverId,'name')
        node = getNodeForServer(server)
        if action == 'setup':
            serverTargets.append({'servername': server, 'nodename': node})
        else:
            cleanupServer(server,node)

    if action == 'setup':
        for i in range(0,appCount):
            launchApp(serverTargets,clusterTargets,i)

##----------------------------------------------
## "MAIN" 
##----------------------------------------------

if (len(sys.argv) < 2 or len(sys.argv) > 3):
    print 'Usage: was was.config action [debug]'
    print 'action: setup, cleanup'
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


m1Name = options['m1_name']
a1Host = options['a1_host']

appCount = int(options['app.count'])
cfCount = options['cf.count']
destCount = options['dest.count']
mdbCount = options['mdb.count']
#print options['app.asptarget.'+str(appCount)]
clusters = []
servers = []

# Allow cluster or server lists to be defined in was.config instead of just using
# what can be found in wsadmin.
# This requires both cluster.count and server.count to be defined in was.config
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
    # If either cluster.count or server.count is in was.config, it will be
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
#appName = baseAppName+'_'+m1Name

# Path to EAR file which contains the EJB and WAR files
appPath = CYGPATH + options['ear.path']
# Name of resource adapter
raname = 'IBM MessageSight Resource Adapter'

totalFails = 0

init()

if action == 'setup':
    if totalFails > 0:
        print 'FAILURE: Setup completed with '+str(totalFails)+' errors'
    else:
        print 'SUCCESS: Setup completed with 0 errors'
elif action == 'cleanup':
    if totalFails > 0:
        print 'FAILURE: Cleanup completed with '+str(totalFails)+' errors'
    else:
        print 'SUCCESS: Cleanup completed with 0 errors'
