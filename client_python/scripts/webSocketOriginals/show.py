#!/usr/bin/python
# Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

# MessageSight command line interface to show configuration of an object

# Import the Message Sight Python Library
import os
path = os.path.dirname(os.path.realpath(__file__))

execfile(path + '/messageSightPythonLib.py')

#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
# Main module to process arg and send POST
def main(argv):
    adminServer = ''
    uriPath = '/ima/admin/config'

    # Parse arguments
    parser = argparse.ArgumentParser(description='Show the Configuration of an Object')
    parser.add_argument('-s', metavar='adminServerIP:port', required=True, type=str, help='adminServerIP:port, ex: 198.23.126.136:9089')
    parser.add_argument('object', metavar='ObjectType', type=str, help='Configuration object type', choices=['imaserver', 'LogAutoTransfer', 'RemoteLogServer', 'AdminLog', 'ConnectionLog', 'LogLevel', 'MQConnectivityLog', 'SecurityLog', 'LogConfig', 'CertificateProfile', 'LTPAProfile', 'OauthProfile', 'SecurityProfile', 'TrustedCertificate', 'LDAP', 'ConnectionPolicy', 'Endpoint', 'MessageHub', 'MessagingPolicy', 'Queue', 'DestinationMappingRule', 'QueueManagerConnection', 'HighAvailability', 'Logging-settings', 'TopicMonitor', 'ClusterMembership' ] )
    parser.add_argument('name', metavar='Name', type=str, nargs=argparse.REMAINDER, help='Configuration object name, (use "list.py" to see existing Names of desired ObjectType')

    args = parser.parse_args()

    adminServer = args.s
    objectType = args.object

    objectName = ""
    trace( 'len(argv)'+ str(len(sys.argv)) )
    if len(sys.argv) > 4 :
 #       if objectType != "imaserver" :
        objectName = ',"Name":"'+ args.name[0] +'"'

    # Set headers
    headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}

    # Set payload
#    payload_str = '{"Locale":"en_US","User":"admin","Action":"get","Component":"Transport","Item":"' + str(objectType) +'"'+  str(objectName) + '}'
    payload_str = '{"Locale":"en_US","User":"admin","Action":"get","Item":"' + str(objectType) +'"'+  str(objectName) + '}'
    trace( 'CMD:'+ payload_str )
    payload = json.loads(payload_str)

    # send POST request
    url = 'http://' + adminServer+uriPath

    try:
        response = requests.post(url=url,data=json.dumps(payload), headers=headers)
    except:
        print ('No response from server : "'+ adminServer +'".  CHECK the IP and PORT.' )
        sys.exit(1)
    
    # Process the response and print return information
    rc = parseResponse( 'Show', adminServer, objectType, response )

    return( int(rc) )

#-----------------------------------------------------------------------------#
# Check and invoke main
if __name__ == "__main__":
   rc = main( sys.argv[1:] )

sys.exit( rc )
