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
# wlst.py - WebLogic Scripting Tool
# 
# This script is used to install and configure the Resource Adapter inside 
# of WebLogic Server 12c.
#
# NOTE: This script is currently rather hardcoded.
#
# /opt/Oracle/Middleware/wlserver_12.1/common/bin/wlst.sh /WLS/wlst.py /WLS/test.config setup
# 
#------------------------------------------------------------------------------
import sys
import time as ftime
# weblogic.time overrides time function calls, so import it with a new name.

adminUser='weblogic'
adminPassword='d0lph1ns'
adminUrl='t3://localhost:7001'

# TODO: don't hardcode these?
appName='ima.jmsra'
appPath='/WLS/ima.jmsra.rar'
evilAppName='ima.evilra'
evilAppPath='/WLS/ima.evilra.rar'
 
moduleOverrideName=appName+'.rar'
moduleDescriptorName='META-INF/weblogic-ra.xml'

#------------------------------------------------------------------------------
def wlsttrace(name,pos):
    """
    Trace function to print out messages with timestamps
    """
    formatting_string = '[%Y-%m-%d %H:%M:%S]'
    timestamp = ftime.strftime(formatting_string)
    print '%s %s >>> %s' % (timestamp,name,pos)

#------------------------------------------------------------------------------
def makeVariable(wlstPlan, name, value, xpath, origin='planbased'):
    """
    Create the Deployment Plan variable and variable assignment for property
    specified by name.
    
    xpath = weblogic-ra.xml element for this variable
    origin = always 'planbased'
    """
    variableAssignment = wlstPlan.createVariableAssignment( name, moduleOverrideName, moduleDescriptorName )
    variableAssignment.setXpath( xpath )
    variableAssignment.setOrigin( origin )
    wlstPlan.createVariable( name, value )

#------------------------------------------------------------------------------
def createObjects(myPlan, options):
    wlsttrace('createObjects','Entering Method')

    # Create javax.jms.ConnectionFactory JMS_BASE_CF
    cfCount = int(options['cf.count'])
    print cfCount
    for i in range(0, cfCount):
        cfJNDI=options['cf.name.'+str(i)]
        cfInterface=options['cf.type.'+str(i)]
        cfInstance='ConnectionInstance_'+cfJNDI+'_JNDIName_'+str(i)

        makeVariable(myPlan, cfInstance, cfJNDI, '/weblogic-connector/outbound-resource-adapter/connection-definition-group/[connection-factory-interface="javax.jms.'+cfInterface+'"]/connection-instance/[jndi-name="'+cfJNDI+'"]/jndi-name')
        setCFprops(myPlan, options, cfJNDI, cfInterface, i)
        wlsttrace('createObjects','Created ConnectionFactory: ' + cfJNDI)

    # Create javax.jms.Topic replyTopic
    aoCount = int(options['dest.count'])
    print aoCount
    for i in range(0, aoCount):
        aoJNDI=options['dest.name.'+str(i)]
        aoInterface=options['dest.type.'+str(i)]
        aoInstance='AdminObjectInstance_'+aoJNDI+'_JNDIName_'+str(i)

        if aoInterface == 'javax.jms.Topic':
            aoClass = 'com.ibm.ima.jms.impl.ImaTopic'
        else:
            aoClass = 'com.ibm.ima.jms.impl.ImaQueue'

        makeVariable(myPlan, aoInstance, aoJNDI, '/weblogic-connector/admin-objects/admin-object-group/[admin-object-class="'+aoClass+'",admin-object-interface="'+aoInterface+'"]/admin-object-instance/[jndi-name="'+aoJNDI+'"]/jndi-name')
        setAOprops(myPlan, options, aoJNDI, aoInterface, aoClass, i)
        wlsttrace('createObjects','Created AdminObject: ' + aoJNDI)

#------------------------------------------------------------------------------
def setCFprops(myPlan, options, cfJNDI, cfInterface, index):
    """
    Set the properties for the specified connection factory instance
    """
    wlsttrace('setCFprops','Entering Method')

    # property names in test.config do not map directly to property names in the resource adapter.
    # e.g. clientid vs clientId
    # securitySocketFactory and securityConfiguration are left out as these configurations don't
    # apply to WebLogic -- although it may be worth configuring them anyways for negative tests.
    cfPropsList={'port': 'port','clientid': 'clientId','secure': 'protocol','user': 'user','password': 'password','tranLevel': 'transactionSupportLevel'}

    # Set the server property for the connection factory
    serverName='ConfigProperty_server_Name_'+str(index)
    serverValue='ConfigProperty_server_Value_'+str(index)
    server=options['a1_host']+','+options['a2_host']

    makeVariable(myPlan, serverValue, server, '/weblogic-connector/outbound-resource-adapter/connection-definition-group/[connection-factory-interface="javax.jms.'+cfInterface+'"]/connection-instance/[jndi-name="'+cfJNDI+'"]/connection-properties/properties/property/[name="server"]/value')

    # Go through the remaining properties
    # prop = test.config name (clientid)
    # propConfigName = full test.config name (cf.clientid.0)
    # propName = actual prop name (clientId)
    # Yes, this was designed poorly. See TODO above!
    for prop in cfPropsList.keys():
        propConfigName='cf.'+prop+'.'+str(index)
        if propConfigName in options.keys():
            propName=cfPropsList[prop]
            valueId='ConfigProperty_'+propName+'_Value_'+str(index)
            value=options[propConfigName]
            
            # In weblogic, we don't have SSL Configuration profiles.
            # In WAS, we set secure=true, and then set securitySocketFactory and securityConfiguration
            # For WebLogic, just ignore those 2 properties and if secure == true, set protocol = tcps.
            if propName == 'protocol':
                if value == 'true':
            	    value='tcps'
            	else:
            	    value='tcp'
            	    
            makeVariable(myPlan, valueId, value, '/weblogic-connector/outbound-resource-adapter/connection-definition-group/[connection-factory-interface="javax.jms.'+cfInterface+'"]/connection-instance/[jndi-name="'+cfJNDI+'"]/connection-properties/properties/property/[name="'+propName+'"]/value')

#------------------------------------------------------------------------------
def setAOprops(myPlan, options, aoJNDI, aoInterface, aoClass, index):
    """
    Set the properties for the specified admin object instance
    """
    wlsttrace('setAOprops','Entering Method')

    # The only property to set for an admin object is 'name'
    adminObjectValue=options['dest.value.'+str(index)]
    adminObjectConfigValue='ConfigProperty_name_Value_'+str(index)

    makeVariable(myPlan, adminObjectConfigValue, adminObjectValue, '/weblogic-connector/admin-objects/admin-object-group/[admin-object-class="'+aoClass+'",admin-object-interface="'+aoInterface+'"]/admin-object-instance/[jndi-name="'+aoJNDI+'"]/properties/property/[name="name"]/value')

#------------------------------------------------------------------------------
def parseConfig(filename):
    """
    Parse was configuration file for the objects to create and app to install.
    Returns dictionary objects. Ex) {'ear.name': 'testTools_JCAtests'}
    """
    wlsttrace('parseConfig','Entering method')
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
def main():
    if len(sys.argv) != 3:
        print 'Usage: wlst.sh wlst.py test.config action'
        print 'action: setup, cleanup'
        sys.exit(0)
    else:
        file   = sys.argv[1]
        action = sys.argv[2]

    options = parseConfig(file)

    connect(adminUser,adminPassword,adminUrl)

    if action == 'setup':

        edit()
        try:
            startEdit()
            
            # Get the deployment plan, Plan.xml, which contains all variable assignments
            # for the resource adapter configuration.
            planPath = get('/AppDeployments/'+appName+'/PlanPath')
            wlsttrace('planPath:', planPath)
            
            myPlan=loadApplication(appPath, planPath)
            
            wlsttrace('main','Beginning plan changes')

            # Create connection factories and administered objects.
            createObjects(myPlan, options)

            wlsttrace('main','Done changing plan')

            myPlan.save();
            save();
            activate(block='true');
        except:
            stopEdit('y')
            wlsttrace('main','Caught Exception')

    else:
        print 'TODO: do some cleanup work?'
main()
