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
# install_ra.py - WebLogic Scripting Tool
# 
# This script is used to install and configure the Resource Adapter inside 
# of WebLogic Server 12c.
#
# NOTE: This script is currently rather hardcoded.
#
# /opt/Oracle/Middleware/wlserver_12.1/common/bin/wlst.sh /WLS/install_ra.py /WLS/ima.jmsra.rar setup
# /opt/Oracle/Middleware/wlserver_12.1/common/bin/wlst.sh /WLS/install_ra.py /WLS/ima.evilra.rar setup
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
appPlan='/WLS/Plan.xml'
evilAppName='ima.evilra'
evilAppPlan='/WLS/EvilPlan.xml'
 
#moduleOverrideName=appName+'.rar'
moduleDescriptorName='META-INF/weblogic-ra.xml'

# Weblogic specific resource adapter properties
# JNDI name must be set in order to perform inbound communication
jndiProperty='WebLogicConnector_JNDIName_12345'
appAccess='WeblogicConnector_EnableAccessOutsideApp_12345'
globalClasses='WeblogicConnector_EnableGlobalAccessToClasses_12345'
nativeLibDir='WeblogicConnector_NativeLibdir_12345'
maxRequests='ConnectorWorkManager_MaxConcurrentLongRunningRequests_12345'
traceLevel='ConfigProperty_defaultTraceLevel_Value_12345'

#------------------------------------------------------------------------------
def wlsttrace(name,pos):
    """
    Trace function to print out messages with timestamps
    """
    formatting_string = '[%Y-%m-%d %H:%M:%S]'
    timestamp = ftime.strftime(formatting_string)
    print '%s %s >>> %s' % (timestamp,name,pos)

#------------------------------------------------------------------------------
def makeVariable(jndiname, wlstPlan, name, value, xpath, origin='planbased'):
    """
    Create the Deployment Plan variable and variable assignment for property
    specified by name.
    
    xpath = weblogic-ra.xml element for this variable
    origin = always 'planbased'
    """
    moduleOverrideName=jndiname+'.rar'
    variableAssignment = wlstPlan.createVariableAssignment( name, moduleOverrideName, moduleDescriptorName )
    variableAssignment.setXpath( xpath )
    variableAssignment.setOrigin( origin )
    wlstPlan.createVariable( name, value )

#------------------------------------------------------------------------------
def installRA(name, path, planpath):
    """
    Install the resource adapter
    """
    wlsttrace('installRA','Entering Method')
    ra = deploy(name, path, planPath=planpath, createPlan='true')

#------------------------------------------------------------------------------
def uninstallRA(appname):
    """
    Uninstall the resource adapter
    """
    wlsttrace('uninstallRA','Entering Method')
    undeploy(appname)

#------------------------------------------------------------------------------
def setRAVariables(myPlan, jndiname):
    """
    Set the WebLogic specific properties for the Resource Adapter
    """
    wlsttrace('setRAVariables','Entering Method')
    makeVariable(jndiname, myPlan, jndiProperty, jndiname, '/weblogic-connector/jndi-name')
    makeVariable(jndiname, myPlan, appAccess, 'true', '/weblogic-connector/enable-access-outside-app')
    makeVariable(jndiname, myPlan, globalClasses, 'true', '/weblogic-connector/enable-global-access-to-classes')
    makeVariable(jndiname, myPlan, nativeLibDir, '/WLS/lib/', '/weblogic-connector/native-libdir')
    makeVariable(jndiname, myPlan, maxRequests, '100', '/weblogic-connector/connector-work-manager/max-concurrent-long-running-requests')
    if jndiname == 'ima.jmsra':
        makeVariable(jndiname, myPlan, traceLevel, '9', '/weblogic-connector/properties/property/[name="defaultTraceLevel"]/value')

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
        print 'Usage: wlst.sh install_ra.py /WLS/ima.jmsra.rar action'
        print 'action: setup, cleanup'
        sys.exit(0)
    else:
        rarname   = sys.argv[1]
        action    = sys.argv[2]

    connect(adminUser,adminPassword,adminUrl)

    if action == 'setup':
    	if rarname.find('evil') != -1:
    	    installRA('ima.evilra',rarname,evilAppPlan)
    	    
            edit()
            try:
                startEdit()

                evilPlanPath = get('AppDeployments/'+evilAppName+'/PlanPath')
                wlsttrace('evilPlanPath:',evilPlanPath)
                myEvilPlan=loadApplication(rarname, evilPlanPath)

                # Set weblogic specific properties including the JNDI name 
                # for the resource adapter.
                setRAVariables(myEvilPlan, 'ima.evilra')
                
                # create TheEvilCon
                evilCFinstance='ConnectionInstance_TheEvilCon_JNDIName_1234567'
                variableAssignment = myEvilPlan.createVariableAssignment( evilCFinstance, evilAppName+'.rar', moduleDescriptorName )
                variableAssignment.setXpath( '/weblogic-connector/outbound-resource-adapter/connection-definition-group/[connection-factory-interface="javax.resource.cci.ConnectionFactory"]/connection-instance/[jndi-name="TheEvilCon"]/jndi-name' )
                variableAssignment.setOrigin( 'planbased' )
                myEvilPlan.createVariable( evilCFinstance, 'TheEvilCon' )

                myEvilPlan.save();
                save();
                activate(block='true');

            except:
                stopEdit('y')
                wlsttrace('main','Caught Exception')
    	else:
            installRA('ima.jmsra',rarname,appPlan)

            edit()
            try:
                startEdit()
            
                # Get the deployment plan, Plan.xml, which contains all variable assignments
                # for the resource adapter configuration.
                planPath = get('/AppDeployments/'+appName+'/PlanPath')
                wlsttrace('planPath:', planPath)
            
                myPlan=loadApplication(rarname, planPath)
            
                wlsttrace('main','Beginning plan changes')

                # Set weblogic specific properties including the JNDI name 
                # for the resource adapter.
                setRAVariables(myPlan, 'ima.jmsra')

                wlsttrace('main','Done changing plan')

                myPlan.save();
                save();
                activate(block='true');
            except:
                stopEdit('y')
                wlsttrace('main','Caught Exception')

    else:
        if rarname.find('evil') != -1:
            uninstallRA(evilAppName)
        else:
            uninstallRA(appName)
main()
