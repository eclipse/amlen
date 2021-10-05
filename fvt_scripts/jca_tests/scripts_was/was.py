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
# was.py
#
# This script is created to configure Websphere Application Server clusters and servers.
# If connected to a deployment manager process:
#  Configure any number of clusters within this cell
#  Configure any number of stand alone servers within this cell
# If connected to an unmanaged process:
#  Configure any number of stand alone servers within this node
#
# /opt/IBM/WebSphereND/AppServer/bin/wsadmin.sh -lang jython -user admin -password admin
#    -f was.py test.config setup/cleanup
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
# J2C Object creation functions
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
def createAdminObjects(rScope):
    """
    Create factories, topics and queues based on the configuration file
    """
    trace('createAdminObjects','Entering method')

    host = options['a1_host']
    rId = rScope+'J2CResourceAdapter:IBM MessageSight Resource Adapter'

    # Create Connection Factories
    for i in range (0, int(cfCount)):
        createObject(rId,'J2CConnectionFactory',str(i))

    # Create Destinations
    for i in range (0, int(destCount)):
        createObject(rId,'J2CAdminObject',str(i))

    # Create Activation Specifications
    for i in range (0, int(mdbCount)):
        createObject(rId,'J2CActivationSpec',str(i))

    trace('createAdminObjects','Exiting method')

#------------------------------------------------------------------------------
def createObject(rScope,type,index):
    """
    Creates the object of the specified type
    """
    trace('createObject','Entering method')

    global totalFails
    global clusterId

    rId = AdminConfig.getid(rScope)
    name = ''

    if type == 'J2CConnectionFactory':
        name = options['cf.name.'+index]
        cfType = options['cf.type.'+index]
        cf = AdminTask.createJ2CConnectionFactory(rId, ['-name', name, '-jndiName', name, '-connectionFactoryInterface', 'javax.jms.' + cfType])
        setCFprops(cf,index)

    elif type == 'J2CAdminObject':
        name = options['dest.name.'+index]
        destType = options['dest.type.'+index]
        adminobj = AdminTask.createJ2CAdminObject(rId,['-name',name,'-jndiName',name,'-adminObjectInterface',destType])
        setDestprops(adminobj,index)

    elif type == 'J2CActivationSpec':
        name = options['mdb.name.'+index]
        # If we are creating an activation specification with a clientID
        # and we are in a cluster, then create the AS at each server scope
        if 'mdb.clientid.'+index in options.keys() and clusterId != '' and 'mdb.ignoreFailuresOnStart.'+index not in options.keys():
            createActSpecPerServer(name,index)
        # Otherwise, just create it at the node scope
        else:
            actSpec = AdminTask.createJ2CActivationSpec(rId,['-name',name,'-jndiName',name,'-messageListenerType','javax.jms.MessageListener'])
            setASprops(actSpec,index)

    verify = AdminConfig.getid('/'+type+':'+name)
    if verify == '':
        trace('createObject','Failed to create '+type+': '+name)
        totalFails+=1
    else:
        trace('createObject','Successfully created '+type+': '+name)

#------------------------------------------------------------------------------
def createActSpecPerServer(name,index):
    """
    This function is only used in a clustered environment.
    If an activation specification uses a clientID, then we need to create it
    at each server scope to avoid clientID conflicts.
    """
    trace('createActSpecPerServer','Entering method')
    global clusterId
    serverlist = getServerIDsForCluster(clusterId)

    id = 0
    for server in serverlist:
        rId = AdminConfig.getid('/Cell:'+cell+'/Node:'+server[1]+'/Server:'+server[2]+'/J2CResourceAdapter:IBM MessageSight Resource Adapter')
        actSpec = AdminTask.createJ2CActivationSpec(rId,['-name',name,'-jndiName',name,'-messageListenerType','javax.jms.MessageListener'])
        setASprops(actSpec,index)
        setASClientId(actSpec,index,id)
        id+=1

        if actSpec == '':
            trace('createActSpecPerServer','Failed to create J2CActivationSpec: '+name)
        else:
            trace('createActSpecPerServer','Successfully created J2cActivationSpec: '+name)

#------------------------------------------------------------------------------
def setASClientId(actSpec,index,i):
    """
    This function is only used in a clustered environment.
    The clientID for each activation specification will be identical 
    except for an .# appended at the end.
    """
    propsList = AdminConfig.showAttribute(actSpec, 'resourceProperties')
    properties = propsList[1:-1].split(' ')

    # Loop through the properties and set the ones we want.
    # Set the custom properties on the activation specification
    secure = options['mdb.secure.'+index]
    for property in properties:
        propertyName = AdminConfig.showAttribute(property,'name')
        if propertyName == 'clientId':
            clientId = options['mdb.clientid.'+index]
            AdminConfig.modify(property,'[[name "clientId"] [type "java.lang.String"] [description "clientId"] [value "'+clientId+'.'+str(i)+'"] [required "false"]]')

#------------------------------------------------------------------------------
def setCFprops(cf,index):
    propsList = AdminConfig.showAttribute(cf,'propertySet')

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
    
    
    resourceProps = AdminConfig.showAttribute(propsList,'resourceProperties')
    properties = resourceProps[1:-1].split(' ')

    # Loop through the resource properties for the connection factory
    secure = options['cf.secure.'+index]
    for property in properties:
        propertyName = AdminConfig.showAttribute(property,'name')
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

#------------------------------------------------------------------------------
def setDestprops(adminobj,index):
    propsList = AdminConfig.showAttribute(adminobj,'properties')
    properties = propsList[1:-1].split(' ')

    # Loop through the properties on the destination
    # So far, name is the only one but maybe there will be more?
    for property in properties:
        propertyName = AdminConfig.showAttribute(property,'name')
        if propertyName == 'name':
            value = options['dest.value.'+index]
            AdminConfig.modify(property,'[[confidential false] [ignore false] [name name] [required false] [supportsDynamicUpdates false] [type java.lang.String] [value '+value+']]')

#------------------------------------------------------------------------------
def setASprops(actSpec,index):
    propsList = AdminConfig.showAttribute(actSpec, 'resourceProperties')
    properties = propsList[1:-1].split(' ')

    # Loop through the properties and set the ones we want.
    # Set the custom properties on the activation specification
    secure = options['mdb.secure.'+index]
    for property in properties:
        propertyName = AdminConfig.showAttribute(property,'name')
        if propertyName == 'destination':
            # If we aren't using JNDI, then mdb.dest.# must be set.
            # Set complete destination name in "destination" property.
            if 'mdb.dest.'+index in options.keys():
                dest = options['mdb.dest.'+index]
                AdminConfig.modify(property, '[[name "destination"] [type "java.lang.String"] [value '+dest+'] [required "true"]]')

        elif propertyName == 'destinationType':
            destType = options['mdb.type.'+index]
            AdminConfig.modify(property, '[[name "destinationType"] [type "java.lang.String"] [value '+destType+'] [required "true"]]')

        elif propertyName == 'clientId':
            if 'mdb.clientid.'+index in options.keys():
                clientId = options['mdb.clientid.'+index]
                AdminConfig.modify(property,'[[name "clientId"] [type "java.lang.String"] [description "clientId"] [value "'+clientId+'"] [required "false"]]')

        elif propertyName == 'destinationLookup':
            # If mdb.dest.# is not set, then we are using JNDI.
            # Set destination JNDI name in "destinationLookup" property.
            if 'mdb.dest.'+index not in options.keys():
                dest = options['mdb.destJNDI.'+index]
                AdminConfig.modify(property,'[[name "destinationLookup"] [type "java.lang.String"] [description "destinationLookup"] [value "'+dest+'"] [required "false"]]')

        elif propertyName == 'subscriptionDurability':
            if 'mdb.durable.'+index in options.keys():
                durable = options['mdb.durable.'+index]
                AdminConfig.modify(property,'[[name "subscriptionDurability"] [type "java.lang.String"] [value "'+durable+'"] [required "false"]]')

        elif propertyName == 'subscriptionName':
            if 'mdb.subscriptionName.'+index in options.keys():
                subName = options['mdb.subscriptionName.'+index]
                AdminConfig.modify(property,'[[name "subscriptionName"] [type "java.lang.String"] [value "'+subName+'"] [required "false"]]')

        elif propertyName == 'subscriptionShared':
            if 'mdb.shared.'+index in options.keys():
                shared = options['mdb.shared.'+index]
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
            if 'mdb.ackMode.'+index in options.keys():
                ackMode = options['mdb.ackMode.'+index]
                AdminConfig.modify(property,'[[name "acknowledgeMode"] [type "java.lang.String"] [value "'+ackMode+'"] [required "false"]]')

        elif propertyName == 'messageSelector':
            if 'mdb.selector.'+index in options.keys():
                selector = options['mdb.selector.'+index]
                AdminConfig.modify(property,'[[name "messageSelector"] [type "java.lang.String"] [value "'+selector+'"] [required "false"]]')

        elif propertyName == 'convertMessageType':
            if 'mdb.convertMessageType.'+index in options.keys():
                cType = options['mdb.convertMessageType.'+index]
                AdminConfig.modify(property,'[[name "convertMessageType"] [type "java.lang.String"] [value "'+cType+'"] [required "false"]]')

        elif propertyName == 'concurrentConsumers':
            if 'mdb.concurrentConsumers.'+index in options.keys():
                cq = options['mdb.concurrentConsumers.'+index]
                AdminConfig.modify(property,'[[name "concurrentConsumers"] [type "java.lang.String"] [value "'+cq+'"] [required "false"]]')

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

#------------------------------------------------------------------------------
def deleteAdminObjects():
    """
    Delete all connection factories, admin objects and activation specs
    named in the config file
    """
    trace('deleteAdminObjects','Entering method')
    global clusterId

    for i in range (0,int(cfCount)):
        cfname = options['cf.name.'+str(i)]
        deleteObject(cfname,'J2CConnectionFactory')

    for i in range (0,int(destCount)):
        aoname = options['dest.name.'+str(i)]
        deleteObject(aoname,'J2CAdminObject')

    for i in range (0,int(mdbCount)):
        mdbname = options['mdb.name.'+str(i)]
        # If the activation spec has a clientID and we are in a cluster,
        # then delete the activation spec on each server
        if 'mdb.clientid.'+str(i) in options.keys() and clusterId != '':
            deleteASperServer(mdbname)
        else:
            deleteObject(mdbname,'J2CActivationSpec')

    trace('deleteAdminObjects','Exiting method')

#------------------------------------------------------------------------------
def deleteASperServer(name):
    """
    This function only applies to a clustered environment.
    If an activation specification has a clientID, then it was created at
    each server scope to avoid clientID conflicts.
    Delete the activation spec at each server scope.
    """
    trace('deleteASperServer','Entering method')
    aslist = AdminConfig.getid('/J2CActivationSpec:'+name).splitlines()

    for actSpec in aslist:
        AdminConfig.remove(actSpec)

    verify = AdminConfig.getid('/J2CActivationSpec:'+name)
    if verify != '':
        trace('deleteASperServer','Failed to delete J2CActivationSpec: '+name)
    else:
        trace('deleteASperServer','Sucessfully deleted J2CActivationSpec: '+name)

#------------------------------------------------------------------------------
def deleteObject(objname,objtype):
    """
    Delete the object specified by objname of type objtype
    """
    trace('deleteObject','Entering method')
    global totalFails

    obj = AdminConfig.getid('/'+objtype+':'+objname)
    if obj != '':
        AdminConfig.remove(obj)

    verify = AdminConfig.getid('/'+objtype+':'+objname)
    if verify != '':
        trace('deleteObject','Failed to delete object '+objname+' of type '+objtype)
        totalFails+=1
    else:
        trace('deleteObject','Successfully deleted object '+objname+' of type '+objtype)

#------------------------------------------------------------------------------
# Main setup and cleanup functions
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
def setup(type,name):
    """
    Main function for setting up of the application server
    """
    trace('setup','Entering method')
    global totalFails

    if type == 'cluster':
        rScope = '/Cell:'+cell+'/ServerCluster:'+name+'/'
    elif type == 'single':
        rScope = '/Cell:'+cell+'/Node:'+name+'/'

    createAdminObjects(rScope)

    # Check that everything was set up
    if totalFails > 0:
        trace('setup','Server configuration failed. Resetting changes')
    else:
        trace('setup','Server configuration was successful')

#------------------------------------------------------------------------------
def cleanup():
    """
    Main function for cleaning up of the application server
    """
    trace('cleanup','Entering method')

    deleteAdminObjects()

    global totalFails
    if totalFails > 0:
        trace('cleanup','Server configuration cleanup failed. Resetting changes')
    else:
        trace('cleanup','Server configuration cleanup was successful')

#------------------------------------------------------------------------------
def init():
    """
    Run setup or cleanup for each cluster and server in the config file
    """
    trace('init','Entering method')
    global servers
    global clusters
    global clusterId
    global totalFails

    for i in range(0,clusterCount):
        clusterId = clusters[i]
        clusterName = AdminConfig.showAttribute(clusterId,'name')
        nodes = getNodesInCluster(clusterId)
        if action == 'setup':
            setup('cluster',clusterName)
        else:
            cleanup()

        if totalFails > 0:
            AdminConfig.reset()
        else:
            syncNodes(nodes)
            AdminConfig.save()

    for i in range(0,serverCount):
        serverId = servers[i]
        server = AdminConfig.showAttribute(serverId,'name')
        node = getNodeForServer(server)
        nodeId = AdminConfig.getid('/Cell:'+cell+'/Node:'+node+'/')
        if action == 'setup':
            setup('single',node)
        else:
            cleanup()
        if totalFails > 0:
            AdminConfig.reset()
        else:
            AdminConfig.save()

#------------------------------------------------------------------------------
# MAIN
#------------------------------------------------------------------------------
if (len(sys.argv) < 2 or len(sys.argv) > 3):
    print 'Usage: was test.config action [debug]'
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

execfile(CYGPATH + '/WAS/wsadminlib.py')

# Parse the configuration file that was passed in
options = parseConfig(file)


a1Host = options['a1_host']
a2Host = options['a2_host']

cfCount = options['cf.count']
destCount = options['dest.count']
mdbCount = options['mdb.count']

clusters = []
servers = []
clusterId = ''

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
