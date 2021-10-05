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
# This script is created to configure Websphere Application Server clusters and servers.
# If connected to a deployment manager process:
#  Configure any number of clusters within this cell
#  Configure any number of stand alone servers within this cell
# If connected to an unmanaged process:
#  Configure any number of stand alone servers within this node
#
# /opt/IBM/WebSphereND/AppServer/bin/wsadmin.sh -lang jython -user admin -password admin
#    -f was.py was.config setup/cleanup
import os, sys, time

#------------------------------------------------------------------------------
def trace(name,pos):
    """
    Trace function to print out messages with timestamps
    """
    global DEBUG
    if (DEBUG):
        formatting_string = '[%Y %m-%d %H:%M-%S00]'
        timestamp = time.strftime(formatting_string)
        print '%s %s %s' % (timestamp,name,pos)

#------------------------------------------------------------------------------
# Resource Adapter installation and unisntallation functions
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
def installRarSingle(node):
    """
    Installs the resource adapter at the node level for a standalone server
    """
    trace('installRarSingle','Entering method for node '+node)

    # Install the .rar file to the node if it doesn't already exist
    rId = AdminConfig.getid('/Cell:'+cell+'/Node:'+node+'/J2CResourceAdapter:IBM MessageSight Resource Adapter')
    if rId == '':
        trace('installRarSingle','Installing resource adapter: '+rarFile)
        AdminTask.installResourceAdapter('[-nodeName '+node+' -rarPath '+rarFile+' -rar.name "IBM MessageSight Resource Adapter" -rar.archivePath ${CONNECTOR_INSTALL_ROOT} -rar.DeleteSourceRar false]')
    else:
        trace('installRarSingle','Resource adapter already installed on Node: '+node)
        # TODO: Update resource adapter if already installed?

    # Verify the resource adapter was installed on the node
    verify = AdminConfig.getid('/Cell:'+cell+'/Node:'+node+'/J2CResourceAdapter:IBM MessageSight Resource Adapter')
    if verify == '':
        trace('installRarSingle','Failed to install resource adapter: '+rarFile)
        global totalFails
        totalFails+=1
    else:
        trace('installRarSingle','Successfully installed resource adapter: '+rarFile)

#------------------------------------------------------------------------------
def installRarCluster(nodes,clusterName,clusterId):
    """
    Installs the resource adapter at each Node that has servers in the cluster
    """
    trace('installRarCluster','Entering method')

    # Install the .rar file to each node in the cluster
    for clusterNode in nodes:
        installRarSingle(clusterNode)

    # Add the .rar file to the cluster if it doesn't already exist
    rId = AdminConfig.getid('/Cell:'+cell+'/ServerCluster:'+clusterName+'/J2CResourceAdapter:IBM MessageSight Resource Adapter')
    if rId == '':
        rar = AdminConfig.getid("/Cell:"+cell+"/Node:"+nodes[0]+"/J2CResourceAdapter:IBM MessageSight Resource Adapter")
        clusterRar = AdminTask.copyResourceAdapter(rar, '[-scope '+clusterId+' -name "IBM MessageSight Resource Adapter" -useDeepCopy true]')
        AdminConfig.modify(clusterRar, '[[threadPoolAlias "Default"] [classpath "${CONNECTOR_INSTALL_ROOT}/IMA_RA.rar"] [name "IBM MessageSight Resource Adapter"] [isolatedClassLoader "false"] [nativepath ""] [description ""]]')

    # Verify the resource adapter was added to the cluster
    verify = AdminConfig.getid('/Cell:'+cell+'/ServerCluster:'+clusterName+'/J2CResourceAdapter:IBM MessageSight Resource Adapter')
    if verify == '':
        trace('installRarCluster','Failed to add resource adapter to cluster')
        global totalFails
        totalFails+=1
    else:
        trace('installRarCluster','Successfully added resource adapter to cluster')

#------------------------------------------------------------------------------
def deleteRar(scope):
    """
    Delete the resource adapter specified by scope 
    """
    trace('deleteRarSingle','Entering method')
    rId = AdminConfig.getid(scope)
    if rId != '':
        AdminConfig.remove(rId)

    verify = AdminConfig.getid(scope)
    if verify != '':
        trace('deleteRarSingle','Failed to uninstall resource adapter:\n'+verify)
        global totalFails
        totalFails+=1
    else:
        trace('deleteRarSingle','Successfully uninstalled resource adapter')

#------------------------------------------------------------------------------
# J2C Object creation functions
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
def createConnectionFactory(scope,cfName,cfType,host,port,index):
    """
    Creates a JMS Connection factory at the Node Scope.
    TODO: Update to J2CConnectionFactory
    """
    trace('createConnectionFactory','Entering method for cf.*.'+index)

    rId = AdminConfig.getid(scope+'J2CResourceAdapter:IBM MessageSight Resource Adapter')
    
    if 'cf.jndiName.'+index in options.keys():
        jndiName = options['cf.jndiName.'+index]
    else:
        jndiName=cfName

    cf = AdminTask.createJ2CConnectionFactory(rId, ['-name', cfName, '-jndiName', jndiName, '-connectionFactoryInterface', 'javax.jms.'+cfType])

    # Connection Pool Properties 
    connectPoolProps =  AdminConfig.showAttribute(cf,'connectionPool')
    trace ('connectPoolProps', connectPoolProps)
    properties = AdminConfig.showall(connectPoolProps).splitlines()
    trace ('properties', properties)
    print AdminConfig.showall(connectPoolProps)

    # Loop through connectPool properties for the connection factory
    updateConnectionPool="false"
    updatePoolCommand='[[ "connectionPool" ['
    for property in properties:
        trace ('property=', property)
        propertyArray = property[1:-1].split(' ')
        propertyName = propertyArray[0]
##        trace ('propertyName=', propertyName)

        if propertyName == 'connectionTimeout':
            if 'cf.connectionTimeout.'+index in options.keys():
                updateConnectionPool="true"
                propertyValue = options['cf.connectionTimeout.'+index]
                print "  %==>  NEW Connection Pool Name/Value: "+propertyName+" - "+propertyValue
                updatePoolCommand=updatePoolCommand+'[ '+propertyName+' '+propertyValue+' ]'

        elif propertyName == 'maxConnections':
            if 'cf.maximumConnections.'+index in options.keys():
                updateConnectionPool="true"
                propertyValue = options['cf.maximumConnections.'+index]
                print "  %==>  NEW Connection Pool Name/Value: "+propertyName+" - "+propertyValue
                updatePoolCommand=updatePoolCommand+'[ '+propertyName+' '+propertyValue+' ]'

        elif propertyName == 'minConnections':
            if 'cf.minimumConnections.'+index in options.keys():
                updateConnectionPool="true"
                propertyValue = options['cf.minimumConnections.'+index]
                print "  %==>  NEW Connection Pool Name/Value: "+propertyName+" - "+propertyValue
                updatePoolCommand=updatePoolCommand+'[ '+propertyName+' '+propertyValue+' ]'

        elif propertyName == 'reapTime':
            if 'cf.reapTime.'+index in options.keys():
                updateConnectionPool="true"
                propertyValue = options['cf.reapTime.'+index]
                print "  %==>  NEW Connection Pool Name/Value: "+propertyName+" - "+propertyValue
                updatePoolCommand=updatePoolCommand+'[ '+propertyName+' '+propertyValue+' ]'

        elif propertyName == 'unusedTimeout':
            if 'cf.unusedTimeout.'+index in options.keys():
                updateConnectionPool="true"
                propertyValue = options['cf.unusedTimeout.'+index]
                print "  %==>  NEW Connection Pool Name/Value: "+propertyName+" - "+propertyValue
                updatePoolCommand=updatePoolCommand+'[ '+propertyName+' '+propertyValue+' ]'

        elif propertyName == 'agedTimeout':
            if 'cf.agedTimeout.'+index in options.keys():
                updateConnectionPool="true"
                propertyValue = options['cf.agedTimeout.'+index]
                print "  %==>  NEW Connection Pool Name/Value: "+propertyName+" - "+propertyValue
                updatePoolCommand=updatePoolCommand+'[ '+propertyName+' '+propertyValue+' ]'

        elif propertyName == 'purgePolicy':
            if 'cf.purgePolicy.'+index in options.keys():
                updateConnectionPool="true"
                propertyValue = options['cf.purgePolicy.'+index]
                print "  %==>  NEW Connection Pool Name/Value: "+propertyName+" - "+propertyValue
                updatePoolCommand=updatePoolCommand+'['+propertyName+' '+propertyValue+' ]'

    if updateConnectionPool == "true":
        updatePoolCommand=updatePoolCommand+']]]'
        print "  %==>  updatePoolCommand= "+updatePoolCommand
        AdminConfig.modify(cf, updatePoolCommand )
        

    # Get the custom properties buried in the configuration
    propsList = AdminConfig.showAttribute(cf,'propertySet')
    resourceProps = AdminConfig.showAttribute(propsList,'resourceProperties')
    resourceProps = resourceProps[1:-1].split(' ')

    # Loop through the resource properties for the connection factory
    secure = options['cf.secure.'+index]

    for property in resourceProps:
        propertyName = AdminConfig.showAttribute(property,'name')
        if propertyName == 'serverHost':
            AdminConfig.modify(property,'[[confidential false] [ignore false] [name serverHost] [required false] [supportsDynamicUpdates false] [type java.lang.String] [value '+host+']]')
        elif propertyName == 'serverPort':
            AdminConfig.modify(property,'[[confidential false] [ignore false] [name serverPort] [required false] [supportsDynamicUpdates false] [type java.lang.String] [value '+port+']]')
        # elif propertyName == 'connectionProtocol':
        #    AdminConfig.modify(property,'[[name connectionProtocol] [required false] [type java.lang.String] [value tcp]]')
        # elif propertyName == 'userName':
        #    AdminConfig.modify(property,'[[name userName] [required false] [type java.lang.String] [value username]]')
        # elif propertyName == 'password':
        #    AdminConfig.modify(property,'[[name password] [required false] [type java.lang.String] [value password]]')

        # Why serverHost/serverPort/connectionProtocol/userName  .vs.  server/port/protocol/user  I had previously coded and worked?
        if propertyName == 'server':
            AdminConfig.modify(property,'[[confidential false] [ignore false] [name server] [required false] [supportsDynamicUpdates false] [type java.lang.String] [value '+a1Host+','+a2Host+']]')

        elif propertyName == 'port':
            port = options['cf.port.'+index]
            AdminConfig.modify(property,'[[confidential false] [ignore false] [name port] [required false] [supportsDynamicUpdates false] [type java.lang.String] [value '+port+']]')

        elif propertyName == 'protocol':
            if secure == 'true':
                AdminConfig.modify(property,'[[name protocol] [required false] [type java.lang.String] [value tcps]]')

        elif propertyName == 'securityConfiguration':
            if secure == 'true' and 'cf.securityConfiguration.'+index in options.keys():
                securityConfiguration = options['cf.securityConfiguration.'+index]
                AdminConfig.modify(property,'[[name securityConfiguration] [required false] [type java.lang.String] [value '+securityConfiguration+']]')

        elif propertyName == 'securitySocketFactory':
            if secure == 'true' and 'cf.securitySocketFactory.'+index in options.keys():
                securitySocketFactory = options['cf.securitySocketFactory.'+index]
                AdminConfig.modify(property,'[[name securitySocketFactory] [required false] [type java.lang.String] [value '+securitySocketFactory+']]')

        elif propertyName == 'transactionSupportLevel':
            if 'cf.tranLevel.'+index in options.keys():
                tranlevel = options['cf.tranLevel.'+index]
                AdminConfig.modify(property,'[[name transactionSupportLevel] [required false] [type java.lang.String] [value '+tranlevel+']]')

        elif propertyName == 'user':
            if 'cf.user.'+index in options.keys():
                username = options['cf.user.'+index]
                AdminConfig.modify(property,'[[name user] [required false] [type java.lang.String] [value '+username+']]')

        elif propertyName == 'password':
            if 'cf.password.'+index in options.keys():
                password = options['cf.password.'+index]
                AdminConfig.modify(property,'[[name password] [required false] [type java.lang.String] [value '+password+']]')

        elif propertyName == 'clientId':
            if 'cf.clientid.'+index in options.keys():
                clientId = options['cf.clientid.'+index]
                AdminConfig.modify(property,'[[name clientId] [required false] [type java.lang.String] [value '+clientId+']]')

        elif propertyName == 'convertMessageType':
            if 'cf.convertMessageType.'+index in options.keys():
                convMsgType = options['cf.convertMessageType.'+index]
                AdminConfig.modify(property,'[[name convertMessageType] [required false] [type java.lang.String] [value '+convMsgType+']]')

        elif propertyName == 'temporaryQueue':
            if 'cf.temporaryQueue.'+index in options.keys():
                tempQ = options['cf.temporaryQueue.'+index]
                AdminConfig.modify(property,'[[name temporaryQueue] [required false] [type java.lang.String] [value '+tempQ+']]')

        elif propertyName == 'temporaryTopic':
            if 'cf.temporaryTopic.'+index in options.keys():
                tempT = options['cf.temporaryTopic.'+index]
                AdminConfig.modify(property,'[[name temporaryTopic] [required false] [type java.lang.String] [value '+tempT+']]')

        elif propertyName == 'traceLevel':
            if 'cf.traceLevel.'+index in options.keys():
                traceLev = options['cf.traceLevel.'+index]
                AdminConfig.modify(property,'[[name traceLevel] [required false] [type java.lang.String] [value '+traceLev+']]')



    verify = AdminConfig.getid('/J2CConnectionFactory:'+cfName)
    if verify == '':
        trace('createConnectionFactory','Failed to create connection factory: '+cfName)
        global totalFails
        totalFails+=1
    else:
        trace('createConnectionFactory','Successfully created connection factory: '+cfName)

#------------------------------------------------------------------------------
def createDestination(scope,destName,destType,value,index):
    """
    Creates a JMS Destination at the Node Scope.
    """
    trace('createDestination','Entering method for dest.*.'+index)

    rId = AdminConfig.getid(scope+'J2CResourceAdapter:IBM MessageSight Resource Adapter')
    
    if 'dest.jndiName.'+index in options.keys():
        jndiName = options['dest.jndiName.'+index]
    else:
        jndiName=destName

    adminobj = AdminTask.createJ2CAdminObject(rId,['-name',destName,'-jndiName',jndiName,'-adminObjectInterface',destType])

    propsList = AdminConfig.showAttribute(adminobj,'properties')
    propsList = propsList[1:-1].split(' ')

    # Loop through the properties on the destination
    # So far, name is the only one but maybe there will be more?
    for property in propsList:
        propertyName = AdminConfig.showAttribute(property,'name')
        if propertyName == 'name':
            AdminConfig.modify(property,'[[confidential false] [ignore false] [name name] [required false] [supportsDynamicUpdates false] [type java.lang.String] [value '+value+']]')

    verify = AdminConfig.getid('/J2CAdminObject:'+destName)
    if verify == '':
        trace('createDestination','Failed to create J2C Administered Object: '+destName)
        global totalFails
        totalFails+=1
    else:
        trace('createDestination','Successfully created J2C Administered Object: '+destName)

#------------------------------------------------------------------------------
def createActivationSpec(scope,name,dest,destType,host,port,index):
    """
    Creates a JMS Activation Specification at the Node Scope.
    """
    trace('createActivationSpec','Entering method for mdb.*.'+index)

    rId = AdminConfig.getid(scope+'J2CResourceAdapter:IBM MessageSight Resource Adapter')
    
    if 'mdb.jndiName.'+index in options.keys():
        jndiName = options['mdb.jndiName.'+index]
    else:
        jndiName=name

    if rId != '':
        actSpec = AdminTask.createJ2CActivationSpec(rId,['-name',name,'-jndiName',jndiName,'-messageListenerType','javax.jms.MessageListener'])

        propsList = AdminConfig.showAttribute(actSpec, 'resourceProperties')
        propsList = propsList[1:-1].split(' ')

        # Loop through the properties and set the ones we want.
        # Set the custom properties on the activation specification
        secure = options['mdb.secure.'+index]
        for property in propsList:
            propertyName = AdminConfig.showAttribute(property,'name')
            if propertyName == 'destination':
                if 'mdb.destination.'+index in options.keys():
                    dest = options['mdb.destination.'+index]
                    AdminConfig.modify(property, '[[name "destination"] [type "java.lang.String"] [description "destination"] [value '+dest+'] [required "false"]]')

            # Seems Redundant?
            elif propertyName == 'destinationType':
                AdminConfig.modify(property, '[[name "destinationType"] [type "java.lang.String"] [description "destinationType"] [value '+destType+'] [required "true"]]')

            # Why serverHost/serverPort/connectionProtocol/userName .vs. Host/Port/Protocol/User I had previously coded and worked?
            elif propertyName == 'serverHost':
                AdminConfig.modify(property,'[[name "serverHost"] [type "java.lang.String"] [description "serverHost"] [value '+host+'] [required "false"]]')
            elif propertyName == 'serverPort':
                AdminConfig.modify(property,'[[name "serverPort"] [type "java.lang.String"] [description "serverPort"] [value '+port+'] [required "false"]]')
            #elif propertyName == 'connectionProtocol':
            #    AdminConfig.modify(property,'[[name "connectionProtocol"] [type "java.lang.String"] [description "connectionProtocol"] [value "tcp"] [required "false"]]')
            #elif propertyName == 'useJNDI':
            #    AdminConfig.modify(property,'[[name "useJNDI"] [type "java.lang.String"] [description "useJNDI"] [value "false"] [required "false"]]')
            #elif propertyName == 'userName':
            #    AdminConfig.modify(property,'[[name "userName"] [type "java.lang.String"] [description "userName"] [value "username"] [required "false"]]')
            #elif propertyName == 'password':
            #    AdminConfig.modify(property,'[[name "password"] [type "java.lang.String"] [description "password"] [value "password"] [required "false"]]')

            # Additional Parameters required by SVT
            elif propertyName == 'clientId':
                if 'mdb.clientId.'+index in options.keys():
                    clientId = options['mdb.clientId.'+index]
                    AdminConfig.modify(property,'[[name "clientId"] [type "java.lang.String"] [description "clientId"] [value "'+clientId+'"] [required "false"]]')

            elif propertyName == 'destinationLookup':
                # If mdb.destination.# is not set, then we are using JNDI.
                # Set destination JNDI name in "destinationLookup" property.
                if 'mdb.destination.'+index not in options.keys():
                    dest = options['mdb.destinationLookup.'+index]
                    AdminConfig.modify(property,'[[name "destinationLookup"] [type "java.lang.String"] [description "destinationLookup"] [value "'+dest+'"] [required "false"]]')

            elif propertyName == 'subscriptionDurability':
                if 'mdb.subscriptionDurability.'+index in options.keys():
                    durable = options['mdb.subscriptionDurability.'+index]
                    AdminConfig.modify(property,'[[name "subscriptionDurability"] [type "java.lang.String"] [value "'+durable+'"] [required "false"]]')

            elif propertyName == 'subscriptionName':
                if 'mdb.subscriptionName.'+index in options.keys():
                    subName = options['mdb.subscriptionName.'+index]
                    AdminConfig.modify(property,'[[name "subscriptionName"] [type "java.lang.String"] [value "'+subName+'"] [required "false"]]')

            elif propertyName == 'subscriptionShared':
                if 'mdb.subscriptionShared.'+index in options.keys():
                    shared = options['mdb.subscriptionShared.'+index]
                    AdminConfig.modify(property,'[[name "subscriptionShared"] [type "java.lang.String"] [value "'+shared+'"] [required "false"]]')
        
            elif propertyName == 'maxDeliveryFailures':
                if 'mdb.maxDeliveryFailures.'+index in options.keys():
                    retry = options['mdb.maxDeliveryFailures.'+index]
                    AdminConfig.modify(property,'[[name "maxDeliveryFailures"] [type "java.lang.String"] [value "'+retry+'"] [required "false"]]')

            elif propertyName == 'ignoreFailuresOnStart':
                if 'mdb.ignoreFailuresOnStart.'+index in options.keys():
                    ignoreFailuresOnStart = options['mdb.ignoreFailuresOnStart.'+index]
                    AdminConfig.modify(property,'[[name "ignoreFailuresOnStart"] [type "java.lang.String"] [value "'+ignoreFailuresOnStart+'"] [required "false"]]')

            elif propertyName == 'acknowledgeMode':
                if 'mdb.acknowledgeMode.'+index in options.keys():
                    ackMode = options['mdb.acknowledgeMode.'+index]
                    AdminConfig.modify(property,'[[name "acknowledgeMode"] [type "java.lang.String"] [value "'+ackMode+'"] [required "false"]]')

            elif propertyName == 'messageSelector':
                if 'mdb.messageSelector.'+index in options.keys():
                    selector = options['mdb.messageSelector.'+index]
                    AdminConfig.modify(property,'[[name "messageSelector"] [type "java.lang.String"] [value "'+selector+'"] [required "false"]]')

            elif propertyName == 'convertMessageType':
                if 'mdb.convertMessageType.'+index in options.keys():
                    cType = options['mdb.convertMessageType.'+index]
                    AdminConfig.modify(property,'[[name "convertMessageType"] [type "java.lang.String"] [value "'+cType+'"] [required "false"]]')

            elif propertyName == 'concurrentConsumers':
                if 'mdb.concurrentConsumers.'+index in options.keys():
                    cq = options['mdb.concurrentConsumers.'+index]
                    AdminConfig.modify(property,'[[name "concurrentConsumers"] [type "java.lang.String"] [value "'+cq+'"] [required "false"]]')

            elif propertyName == 'clientMessageCache':
                if 'mdb.clientMessageCache.'+index in options.keys():
                    cCache = options['mdb.clientMessageCache.'+index]
                    AdminConfig.modify(property,'[[name "clientMessageCache"] [type "java.lang.String"] [value "'+cCache+'"] [required "false"]]')

            elif propertyName == 'enableRollback':
                if 'mdb.enableRollback.'+index in options.keys():
                    erb = options['mdb.enableRollback.'+index]
                    AdminConfig.modify(property,'[[name "enableRollback"] [type "java.lang.String"] [value "'+erb+'"] [required "false"]]')
                
            elif propertyName == 'server':
                AdminConfig.modify(property,'[[name "server"] [type "java.lang.String"] [value '+a1Host+','+a2Host+'] [required "false"]]')

            elif propertyName == 'port':
                port = options['mdb.port.'+index]
                AdminConfig.modify(property,'[[name "port"] [type "java.lang.String"] [value '+port+'] [required "false"]]')

            elif propertyName == 'protocol':
                if secure == 'true':
                    AdminConfig.modify(property,'[[name "protocol"] [type "java.lang.String"] [value "tcps"] [required "false"]]')

            elif propertyName == 'securityConfiguration':
                if secure == 'true' and 'mdb.securityConfiguration.'+index in options.keys():
                    securityConfiguration = options['mdb.securityConfiguration.'+index]
                    AdminConfig.modify(property,'[[name securityConfiguration] [required false] [type java.lang.String] [value '+securityConfiguration+']]')

            elif propertyName == 'securitySocketFactory':
                if secure == 'true' and 'mdb.securitySocketFactory.'+index in options.keys():
                    securitySocketFactory = options['mdb.securitySocketFactory.'+index]
                    AdminConfig.modify(property,'[[name securitySocketFactory] [required false] [type java.lang.String] [value "'+securitySocketFactory+'"]]')

            elif propertyName == 'user':
                if 'mdb.user.'+index in options.keys():
                    username = options['mdb.user.'+index]
                    AdminConfig.modify(property,'[[name "user"] [type "java.lang.String"] [value "'+username+'"] [required "false"]]')

            elif propertyName == 'password':
                if 'mdb.password.'+index in options.keys():
                    password = options['mdb.password.'+index]
                    AdminConfig.modify(property,'[[name "password"] [type "java.lang.String"] [value "'+password+'"] [required "false"]]')

            elif propertyName == 'traceLevel':
                if 'mdb.traceLevel.'+index in options.keys():
                    traceLev = options['mdb.traceLevel.'+index]
                    AdminConfig.modify(property,'[[name "traceLevel"] [type "java.lang.String"] [value "'+traceLev+'"] [required "false"]]')




    verify = AdminConfig.getid('/J2CActivationSpec:'+name)
    if verify == '':
        trace('createActivationSpec','Failed to create activation specification: '+name)
        global totalFails
        totalFails+=1
    else:
        trace('createActivationSpec','Successfully created activation specification: '+name)

#------------------------------------------------------------------------------
def createAdminObjects(rScope):
    """
    Create factories, topics and queues based on the configuration file
    """
    trace('createAdminObjects','Entering method')

    #JCD - SVT NEED BOTH# host = options['a1_host']
    host = a1Host+','+a2Host

    # Create Connection Factories
    for i in range (0, int(cfCount)):
        cfName = options['cf.name.'+str(i)]
        cfType = options['cf.type.'+str(i)]
        port = options['cf.port.'+str(i)]
        createConnectionFactory(rScope,cfName,cfType,host,port,str(i))

    # Create Destinations
    for i in range (0, int(destCount)):
        dest = options['dest.name.'+str(i)]
        destType = options['dest.type.'+str(i)]
        value = options['dest.value.'+str(i)]
        createDestination(rScope,dest,destType,value,str(i))

    # Create Activation Specifications
    for i in range (0, int(mdbCount)):
        mdbName = options['mdb.name.'+str(i)]
        # If mdb.destination.# is not set, then we are using JNDI.
        # Set destination JNDI name in "destinationLookup" property.
        if 'mdb.destination.'+str(i) in options.keys():
           dest = options['mdb.destination.'+str(i)]
        else:
           dest = options['mdb.destinationLookup.'+str(i)]

        destType = options['mdb.destinationType.'+str(i)]

        port = options['mdb.port.'+str(i)]
        createActivationSpec(rScope,mdbName,dest,destType,host,port,str(i))

    trace('createAdminObjects','Exiting method')

#------------------------------------------------------------------------------
# Application installation functions
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
def installApplication(filename,servers,clusternames,myoptions,appIndex):
    """
    Install the application at the given filename to each of the 
    servers and clusters listed. 
    """
    trace('installApplication','Entering method for '+filename)
#    trace( 'servers',servers)
    trace( 'clusternames',clusternames)
    trace( 'myoptions',myoptions)
    trace( 'appIndex',appIndex)
    trace( 'appName',appName)
    trace( 'appName',options['app.name.'+str(appIndex)])

    targets = []
    for server in servers:
        targets.append("WebSphere:cell=%s,node=%s,server=%s" % (cell, server['nodename'], server['servername']))
    for clustername in clusternames:
        targets.append("WebSphere:cell=%s,cluster=%s" % (cell,clustername))

    target = "+".join( targets )
    trace('target', target)

    arglist = ['-target', target, '-appname', options['app.name.'+str(appIndex)] ]

    if myoptions != None:
        if not "-MapWebModToVH" in myoptions:
            print '  %==>  BUILD arglist for MapWebModToVH'
            arglist.extend( ['-MapWebModToVH', [['.*', '.*', 'default_host']]] )
        if not "-CtxRootForWebMod" in myoptions:
            print '  %==>  BUILD arglist for CtxRootForWebMod'
            trace( 'baseAppName',baseAppName)
            arglist.extend( ['-CtxRootForWebMod', [[ baseAppName, baseAppName+'.war,WEB-INF/web.xml', baseAppName ]]])


    if myoptions != None:
        arglist.extend(myoptions)

    print '  %==>  BEFORE AdminApp.install( filename, arglist )'
    trace('arglist',arglist)
    AdminApp.install( filename, arglist )
    print '  %==>  AFTER AdminApp.install( filename, arglist )'

#------------------------------------------------------------------------------
def findMDBDestination(beanName):
    """
    Given the name of an MDB, return the JNDI name of the destination
    specified for that bean in the configuration file.
    """
    trace('findMDBDestination','Entering method')
    for i in range(0, int(mdbCount)):
        mdbName = options['mdb.name.'+str(i)]
        if mdbName == beanName:
            return options['mdb.destJNDI.'+str(i)]
    return None

#------------------------------------------------------------------------------
def parseTask(taskName,appIndex):
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
    myoptions = [[]]
    optIter = 0

    bean = ''
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
    # Map resource references
    mapResRefToEJB = '-MapResRefToEJB'
    mapResRefToEJBArgs = parseTask('MapResRefToEJB',appIndex)
    trace('generateOptions','\nMapResRefToEJB:')
    trace('generateOptions',mapResRefToEJBArgs)

    # Map destination references
    mapMsgDestRefToEJB = '-MapMessageDestinationRefToEJB'
    mapMsgDestArgs = parseTask('MapMessageDestinationRefToEJB',appIndex)
    trace('generateOptions','\nMapMessageDestinationRefToEJB:')
    trace('generateOptions',mapMsgDestArgs)

    # Map activation specification references
    bindJndiForEJBMsgBinding = '-BindJndiForEJBMessageBinding'
    bindJndiArgs = parseTask('BindJndiForEJBMessageBinding',appIndex)
    trace('generateOptions','\nBindJndiForEJBMessageBinding:')
    trace('generateOptions',bindJndiArgs)

#JCD#    appOptions = [mapResRefToEJB, mapResRefToEJBArgs, bindJndiForEJBMsgBinding, bindJndiArgs, mapMsgDestRefToEJB, mapMsgDestArgs]
    appOptions = [mapResRefToEJB, mapResRefToEJBArgs, bindJndiForEJBMsgBinding, bindJndiArgs ]
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
    trace('startApplicationOnServer','Starting app, '+appname+' on server '+servername+', node '+nodename+', with appManager '+appManager )
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
    trace('launchApp','Entering method for app.name.'+str(appIndex) )
    appName = options['app.name.'+str(appIndex)]
    print '  %==>  appName: ' +appName

    appOptions = generateOptions(appIndex);
    installApplication(appPath, serverTargets, clusterTargets, appOptions, appIndex)

    # Verify that the application was installed before launching
    verify = AdminApp.list()
    verify = verify.splitlines()
    success = 0
    for line in verify:
        if line == appName:
            trace('launchApp','Application '+appName+' installed successfully')
            success = 1
    if success != 1:
        trace('launchApp','Application installation failed for: '+appName)
        global totalFails
        totalFails+=1
        return

    AdminConfig.save()
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

    # Verify application is running
    apps = AdminApp.list().splitlines()
    for app in apps:
        if appName == app:
            trace('launchApp','Application '+appName+' launched successfully!')
            return
    trace('launchApp','Application '+appName+' failed to launch!')
    global totalFails
    totalFails+=1

#------------------------------------------------------------------------------
def uninstallApp(thisApp):
    """
    Uninstall the test application
    """
    trace('uninstallApp','Entering method for app: '+thisApp)
    appList = AdminApp.list()
    appList = appList.splitlines()
    # trace('uninstallApp','AppList='+str(appList))
    for app in appList:
        if app == thisApp:
            AdminApp.uninstall(thisApp)

    appList = AdminApp.list()
    appList = appList.splitlines()
    for app in appList:
        if app == thisApp:
            trace('uninstallApp','Failed to uninstall application: '+thisApp)
            global totalFails
            totalFails+=1
            return
    trace('uninstallApp','Successfully uninstalled application: '+thisApp)

#------------------------------------------------------------------------------
# Main setup and cleanup functions
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
def setupCluster(type,name,id,members,nodes):
    """
    Main function for setting up of the application server
    """
    trace('setupCluster','Entering method')
    global totalFails

    # If running on a cluster, install to each node and the cluster
    installRarCluster(nodes,name,id)

    if totalFails > 0:
        AdminConfig.reset()
        trace('setupCluster','Cluster configuration failed. Resetting changes')

    rScope = '/Cell:'+cell+'/ServerCluster:'+name+'/'
    createAdminObjects(rScope)

    # Check that everything was set up
    if totalFails > 0:
        AdminConfig.reset()
        trace('setupCluster','Cluster configuration failed. Resetting changes')
    else:
        # JNDI objects created, so install and launch the application
        syncNodes(nodes)
        AdminConfig.save()
        trace('setupCluster','Cluster configuration was successful')

#------------------------------------------------------------------------------
def setupServer(type,name,node,nodeId):
    """
    Main function for setting up of the application server
    """
    trace('setupServer','Entering method')
    global totalFails

    # If running in standalone, just install the rar at Node scope
    installRarSingle(node)

    if totalFails > 0:
        AdminConfig.reset()
        trace('setupCluster','Server configuration failed. Resetting changes')

    rScope = '/Cell:'+cell+'/Node:'+node+'/'

    createAdminObjects(rScope)

    # Check that everything was set up
    if totalFails > 0:
        AdminConfig.reset()
        trace('setupServer','Server configuration failed. Resetting changes')
    else:
        # JNDI objects created, so install and launch the application
        AdminConfig.save()
        trace('setupServer','Server configuration was successful')

#------------------------------------------------------------------------------
def cleanupCluster(type,name,id,members,nodes):
    """
    Main function for cleaning up of the application server
    """
    trace('cleanupCluster','Entering method')
    # Uninstall APPs before uninstall RAR
    for i in range(0,int(appCount)):
        appName = options['app.name.'+str(i)]
        uninstallApp(appName)

    for node in nodes:
        rScope = '/Cell:'+cell+'/Node:'+node+'/J2CResourceAdapter:IBM MessageSight Resource Adapter'
        deleteRar(rScope)
    rScope = '/Cell:'+cell+'/ServerCluster:'+name+'/J2CResourceAdapter:IBM MessageSight Resource Adapter'
    deleteRar(rScope)

    global totalFails
    if totalFails > 0:
        AdminConfig.reset()
        trace('cleanupCluster','Cluster configuration cleanup failed. Resetting changes')
    else:
        AdminConfig.save()
        syncNodes(nodes)
        trace('cleanupCluster','Cluster configuration cleanup was successful')

#------------------------------------------------------------------------------
def cleanupServer(type,name,node,nodeId):
    """
    Main function for cleaning up of the application server
    """
    trace('cleanupServer','Entering method')

    for i in range(0,int(appCount)):
        appName = options['app.name.'+str(i)]
        uninstallApp(appName)

    rScope = '/Cell:'+cell+'/Node:'+node+'/J2CResourceAdapter: IBM MessageSight Resource Adapter'
    deleteRar(rScope)

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
    trace('Init','Entering method')
    clusterTargets = []
    serverTargets = []

    for i in range(0,int(clusterCount)):
        serverType = 'cluster'
        clusterName = options['cluster.name.'+str(i)]
        clusterId = AdminConfig.getid('/Cell:'+cell+'/ServerCluster:'+clusterName+'/')
        clusterMembers = AdminConfig.list('ClusterMember',clusterId).splitlines()
        nodes = getNodesInCluster(clusterId)
        if action == 'setup':
            trace('everything','setting up '+clusterName)
            setupCluster(serverType,clusterName,clusterId,clusterMembers,nodes)
            clusterTargets.append(clusterName)
        else:
            cleanupCluster(serverType,clusterName,clusterId,clusterMembers,nodes)

    for i in range(0,int(serverCount)):
        serverType = 'single'
        server = options['server.name.'+str(i)]
        node = getNodeForServer(server)
        nodeId = AdminConfig.getid('/Cell:'+cell+'/Node:'+node+'/')
        if action == 'setup':
            setupServer(serverType,server,node,nodeId)
            serverTargets.append({'servername': server, 'nodename': node})
        else:
            cleanupServer(serverType,server,node,nodeId)

    if action == 'setup':
        for i in range(0,int(appCount) ):
            launchApp(serverTargets,clusterTargets,i)

#------------------------------------------------------------------------------
# SSL Functions
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Misc
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
    Return list of (serverID,nodeName,serverName) for each server in cluster 
    """
    trace('getServerIDsForCluster','Entering method')
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
def parseConfig(filename):
    """
    Parse was configuration file for the objects to create and app to install.
    Returns dictionary objects. Ex) {'ear.name': 'testTools_JCAtests'}
    """
    trace('parseConfig','Entering method')
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
# MAIN
#------------------------------------------------------------------------------
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

# Parse the configuration file that was passed in
options = parseConfig(file)

m1Name = options['m1_name']
#jcd-SVT Not Used#  m1Host = options['m1_host']
#jcd-SVT Not Used#  m2Host = options['m2_host']
a1Host = options['a1_host']
a2Host = options['a2_host']

appCount = options['app.count']
cfCount = options['cf.count']
destCount = options['dest.count']
mdbCount = options['mdb.count']
clusterCount = options['cluster.count']
serverCount = options['server.count']

# Path to the resource adapter .rar file
rarFile = options['rarFile']

# Name of test enterprise application
baseAppName = options['ear.name']
appName = options['app.name.0']   # this will update as iterate through the app.count in launchApp() and UninstallApp() calls

# Path to EAR file which contains the EJB and WAR files
appPath = options['ear.path']

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
