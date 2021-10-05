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
def parseObjArgs( type, obj_args ):
    trace( '...Entering: parseObjArgs on args: '+ str(obj_args) )
#    json_args = json.dumps( obj_args, separators=(', ', '=') )
#    print( json_args )
    json_str = ""
    for item in obj_args:
        name, value = item.split( '=', 1 )
        trace( 'Name: '+ name +' , Value: '+ value )
        json_str = json_str +', "'+ name +'":"'+ value +'"'
    if type == 'ConnectionPolicy' :
        json_str = json_str +',"PolicyType":"Connection"'
        if '"ClientID":' not in json_str :
            json_str = json_str +',"ClientID":"*"'
    elif type == 'MessagingPolicy' :
        json_str = json_str +',"PolicyType":"Messaging"'
        if '"Destination":' not in json_str :
            json_str = json_str +',"Destination":"*"'
        if '"DestinationType":' not in json_str :
            json_str = json_str +',"DestinationType":"Topic"'
        if '"ClientID":' not in json_str :
            json_str = json_str +',"ClientID":"*"'
        if '"ActionList":' not in json_str :
            json_str = json_str +',"ActionList":"Publish,Subscribe"'

    trace( 'JSON_STR: '+ json_str )
    return ( json_str )    


#------------------------------------------------------------------------------
# Main module to process arg and send POST
#------------------------------------------------------------------------------
def main(argv):
    adminServer = ''
    uriPath = '/ima/admin/config'

    # Parse arguments
    parser = argparse.ArgumentParser(description='Create an ObjectType Configuration')
    parser.add_argument('-s', metavar='adminServerIP:port', required=True, type=str, help='adminServerIP:port, ex: 198.23.126.136:9089')
    parser.add_argument('object', metavar='ObjectType', type=str, help='Configuration object type', choices=['LogAutoTransfer', 'RemoteLogServer' , 'CertificateProfile' , 'LTPAProfile' , 'OauthProfile' , 'SecurityProfile' , 'LDAP' , 'ConnectionPolicy' , 'Endpoint' , 'MessageHub' , 'MessagingPolicy' , 'Queue' , 'DestinationMappingRule' , 'QueueManagerConnection' , 'TopicMonitor' ] )
    parser.add_argument('name', metavar='Name', type=str, nargs="?", help='Configuration object name, (use "list.py" to see existing Names of desired ObjectType')
    parser.add_argument('obj_args', help='context specific name=value pairs based on ObjectType', nargs=argparse.REMAINDER )

    args = parser.parse_args()

    adminServer = args.s
    objectType = args.object
    objectName = 'None'
    
    trace('  --  args.name '+str(args.name) )
    if args.name is not None :
        objectName = args.name
        if objectType == 'LDAP':
            args.obj_args.insert(0, args.name )
            objectName = 'None'
        
    # Parse the rest of the Args and build the JSON string
    trace('  --  args.obj_args '+str(args.obj_args) )
    if args.obj_args :
        json_args = parseObjArgs( objectType, args.obj_args )
    else :
        json_args = ''

    # Set headers
    headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}

    # Set payload
    # CMD: { "Action":"set","User":"root","Component":"Transport","Item":"Endpoint","Type":"composite","Name":"DemoEndpoint2","Port":"9999","MessageHub":"DemoHub","ConnectionPolicies":"DemoConnectionPolicy","MessagingPolicies":"DemoMessagingPolicy,DemoMPForSharedSub","Security":"False","UID":"autoGenerate","Enabled":"True","Protocol":"JMS,MQTT","Interface":"all","MaxMessageSize":"1024KB","BatchMessages":"True" }
    if objectType == 'LDAP' :
        payload_str = '{"Locale":"en_US","User":"admin","Action":"set","Type":"composite","Item":"' + str(objectType) + '","Name":"None"'+ json_args +'}'
    else :
        payload_str = '{"Locale":"en_US","User":"admin","Action":"set","Type":"composite","Item":"' + str(objectType) + '","Name":"' + str(objectName) + '"'+ json_args +'}'

    payload = json.loads(payload_str)
    trace( 'CMD:  '+ payload_str )
    
    # send POST request
    url = 'http://' + adminServer+uriPath

    try:
        response = requests.post(url=url,data=json.dumps(payload), headers=headers)
    except:
        print('No response from server : "'+ adminServer +'".  CHECK the IP and PORT.' )
        sys.exit(1)
    
    # Process the response and print return information
    rc = parseResponse( 'Create', adminServer, objectType, response )

    return( int(rc) )        

#-----------------------------------------------------------------------------#
# Check and invoke main
if __name__ == "__main__":
   rc = main( sys.argv[1:] )

sys.exit( rc )
