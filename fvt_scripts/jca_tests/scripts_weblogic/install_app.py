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
# install_app.py - WebLogic Scripting Tool
# 
# This script is used to install the FVT test application
# into WebLogic Server 12c.
#
# NOTE: This script is currently rather hardcoded.
# 
# /opt/Oracle/Middleware/wlserver_12.1/common/bin/wlst.sh /WLS/install_app.py /WLS/test.config setup
#
#------------------------------------------------------------------------------
import sys
import time as ftime
# weblogic.time overrides time function calls, so import it with a new name.

adminUser='weblogic'
adminPassword='d0lph1ns'
adminUrl='t3://localhost:7001'

# TODO: change to get this from test.config instead
appName='testTools_JCAtests'
appPath='/WLS/testTools_JCAtests.ear'
 
#------------------------------------------------------------------------------
def wlsttrace(name,pos):
    """
    Trace function to print out messages with timestamps
    """
    formatting_string = '[%Y-%m-%d %H:%M:%S]'
    timestamp = ftime.strftime(formatting_string)
    print '%s %s >>> %s' % (timestamp,name,pos)

#------------------------------------------------------------------------------
def installApp():
    """
    Install the resource adapter
    """
    wlsttrace('installApp','Entering Method')
    app = deploy('testTools_JCAtests','/WLS/testTools_JCAtests.ear',planPath='/WLS/AppPlan.xml', createPlan='true')

#------------------------------------------------------------------------------
def uninstallApp():
    """
    Uninstall the resource adapter
    """
    wlsttrace('uninstallApp','Entering Method')
    undeploy('testTools_JCAtests')

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
        print 'Usage: wlst.sh install_app.py test.config action'
        print 'action: setup, cleanup'
        sys.exit(0)
    else:
        file   = sys.argv[1]
        action = sys.argv[2]

    options = parseConfig(file)

    connect(adminUser,adminPassword,adminUrl)

    if action == 'setup':
        installApp()

    else:
        uninstallApp()
main()
