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

# MessageSight command line interface to show statistics of an object

# Import the Message Sight Python Library
import os
path = os.path.dirname(os.path.realpath(__file__))

execfile(path + '/messageSightPythonLib.py')

#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
def parseObjArgs( obj_args ):
    trace( '...Entering: parseObjArgs on args: '+ str(obj_args) )
#    json_args = json.dumps( obj_args, separators=(', ', '=') )
#    print( json_args )
    json_str = ""
    for item in obj_args:
        if '=' in item:
           name, value = item.split( '=', 1 )
           trace( 'Name: '+ name +' , Value: '+ value )
           json_str = json_str +', "'+ name +'":"'+ value +'"'
        else:
           json_str = json_str +', "Name":"'+ item +'"'
    trace( 'JSON_STR: '+ json_str )
    return ( json_str )    

    
#------------------------------------------------------------------------------
# Main module to process arg and send POST
#------------------------------------------------------------------------------
def main(argv):
    adminServer = ''
    uriPath = '/ima/monitoring'

    # Parse arguments
    parser = argparse.ArgumentParser(description='Show the Configuration of an Object')
    parser.add_argument('-s', metavar='adminServerIP:port', required=True, type=str, help='adminServerIP:port, ex: 198.23.126.136:9089')
    parser.add_argument('object', metavar='ObjectType', type=str, help='Configuration object type', choices=['LogAutoTransfer', 'engine', 'Endpoint', 'Queue', 'DestinationMappingRule', 'MQTTClient', 'Subscription', 'Connection', 'Memory', 'Server', 'Store', 'Topic', 'Transaction', 'MessagingChannel' ])
#    parser.add_argument('name', metavar='Name', type=str, help='Configuration object name, (use "list.py" to see existing Names of desired ObjectType')
    parser.add_argument('obj_args', help='context specific name=value pairs based on ObjectType', nargs=argparse.REMAINDER )

    args = parser.parse_args()

    adminServer = args.s
    objectType = args.object
#    objectName = args.name

    # Parse the rest of the Args and build the JSON string
    json_args = parseObjArgs( args.obj_args )

    # Set headers
    headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}

    # Set payload
    payload_str = '{ "Locale":"en_US","User":"admin"'+ json_args 
    
    # CMD: { "Action":"Endpoint","User":"root","Locale":"en_US","SubType":"Current","StatType":"ActiveConnections","Duration":"1800" }
    if objectType == 'Endpoint' :
        payload_str = payload_str +    ',"Action":"'+ objectType +'"'
        if 'SubType' not in payload_str :
            payload_str = payload_str +',"SubType":"Current"'
        if 'StatType' not in payload_str :
            payload_str = payload_str +',"StatType":"ActiveConnections"'
        if 'Duration' not in payload_str :
            payload_str = payload_str +',"Duration":"1800"'
    
    # CMD: { "Action":"Connection","User":"root","Locale":"en_US","StatType":"NewestConnection" }
    elif objectType == 'Connection' :
        payload_str = payload_str +    ',"Action":"'+ objectType +'"'
        if 'StatType' not in payload_str :
            payload_str = payload_str +',"StatType":"NewestConnection"'

    # CMD: { "Action":"DestinationMappingRule","User":"root","Locale":"en_US","Name":"*","RuleType":"Any","Association":"*","StatType":"CommitCount","ResultCount":"25" }
    elif objectType == 'DestinationMappingRule' :
        payload_str = payload_str +    ',"Action":"'+ objectType +'"'
        if 'Name' not in payload_str :
            payload_str = payload_str +',"Name":"*"'
        if 'RuleType' not in payload_str :
            payload_str = payload_str +',"RuleType":"Any"'
        if 'Association' not in payload_str :
            payload_str = payload_str +',"Association":"*"'
        if 'StatType' not in payload_str :
            payload_str = payload_str +',"StatType":"CommitCount"'
        if 'ResultCount' not in payload_str :
            payload_str = payload_str +',"ResultCount":"25"'

    # CMD: { "Action":"Memory","User":"root","Locale":"en_US","SubType":"Current","StatType":"MemoryTotalBytes","Duration":"1800" }
    elif objectType == 'Memory' :
        payload_str = payload_str +    ',"Action":"'+ objectType +'"'
        if 'SubType' not in payload_str :
            payload_str = payload_str +',"SubType":"Current"'
        if 'StatType' not in payload_str :
            payload_str = payload_str +',"StatType":"MemoryTotalBytes"'
        if 'Duration' not in payload_str :
            payload_str = payload_str +',"Duration":"1800"'

    # CMD: { "Action":"MQTTClient","User":"root","Locale":"en_US","ClientID":"*","StatType":"LastConnectedTimeOldest","ResultCount":"25" }
    elif objectType == 'MQTTClient' :
        payload_str = payload_str +    ',"Action":"'+ objectType +'"'
        if 'ClientID' not in payload_str :
            payload_str = payload_str +',"ClientID":"*"'
        if 'StatType' not in payload_str :
            payload_str = payload_str +',"StatType":"LastConnectedTimeOldest"'
        if 'ResultCount' not in payload_str :
            payload_str = payload_str +',"ResultCount":"25"'

    # CMD: { "Action":"Queue","User":"root","Locale":"en_US","Name":"*","StatType":"BufferedMsgsHighest","ResultCount":"25" }
    elif objectType == 'Queue' :
        payload_str = payload_str +    ',"Action":"'+ objectType +'"'
        if 'Name' not in payload_str :
            payload_str = payload_str +',"Name":"*"'
        if 'StatType' not in payload_str :
            payload_str = payload_str +',"StatType":"BufferedMsgsHighest"'
        if 'ResultCount' not in payload_str :
            payload_str = payload_str +',"ResultCount":"25"'
            
    # CMD: { "Action":"Connection","User":"root","Option":"Volume" }
    elif objectType == 'Server' :
        payload_str = payload_str +    ',"Action":"Connection","Option":"Volume"'
        # if 'Option' not in payload_str :
            # payload_str = payload_str +',"Option":"Volume"'
            
    #  CMD: { "Action":"Store","User":"root","Locale":"en_US","SubType":"Current","StatType":"StoreDiskUsagePct","Duration":"1800" }
    elif objectType == 'Store' :
        payload_str = payload_str +    ',"Action":"'+ objectType +'"'
        if 'SubType' not in payload_str :
            payload_str = payload_str +',"SubType":"Current"'
        if 'StatType' not in payload_str :
            payload_str = payload_str +',"StatType":"StoreDiskUsagePct"'
        if 'Duration' not in payload_str :
            payload_str = payload_str +',"Duration":"1800"'

    # CMD: { "Action":"Subscription","User":"root","Locale":"en_US","SubName":"*","TopicString":"*","ClientID":"*","MessagingPolicy":"*","SubType":"All","StatType":"PublishedMsgsHighest","ResultCount":"25" }
    elif objectType == 'Subscription' :
        payload_str = payload_str +    ',"Action":"'+ objectType +'"'
        if 'SubName' not in payload_str :
            payload_str = payload_str +',"SubName":"*"'
        if 'TopicString' not in payload_str :
            payload_str = payload_str +',"TopicString":"*"'
        if 'ClientID' not in payload_str :
            payload_str = payload_str +',"ClientID":"*"'
        if 'MessagingPolicy' not in payload_str :
            payload_str = payload_str +',"MessagingPolicy":"*"'
        if 'SubType' not in payload_str :
            payload_str = payload_str +',"SubType":"ALL"'
        if 'StatType' not in payload_str :
            payload_str = payload_str +',"StatType":"PublishedMsgsHighest"'
        if 'ResultCount' not in payload_str :
            payload_str = payload_str +',"ResultCount":"25"'
            

    # CMD: { "Action":"Topic","User":"root","Locale":"en_US","TopicString":"*","StatType":"PublishedMsgsHighest","ResultCount":"25" }
    elif objectType == 'Topic' :
        payload_str = payload_str +    ',"Action":"'+ objectType +'"'
        if 'TopicString' not in payload_str :
            payload_str = payload_str +',"TopicString":"*"'
        if 'StatType' not in payload_str :
            payload_str = payload_str +',"StatType":"PublishedMsgsHighest"'
        if 'ResultCount' not in payload_str :
            payload_str = payload_str +',"ResultCount":"25"'
            
    #CMD: { "Action":"Transaction","User":"root","Locale":"en_US","StatType":"LastStateChangeTime","TranState":"All","ResultCount":"25" }
    elif objectType == 'Transaction' :
        payload_str = payload_str +    ',"Action":"'+ objectType +'"'
        if 'TranState' not in payload_str :
            payload_str = payload_str +',"TranState":"ALL"'
        if 'StatType' not in payload_str :
            payload_str = payload_str +',"StatType":"LastStateChangeTime"'
        if 'ResultCount' not in payload_str :
            payload_str = payload_str +',"ResultCount":"25"'
    else :
        payload_str = payload_str +    ',"Action":"'+ objectType +'"'
        
        
    payload_str = payload_str  +' }'    
    trace( 'CMD: '+ payload_str )
    
    payload = json.loads(payload_str)

    # send POST request
    url = 'http://' + adminServer+uriPath

    try:
        response = requests.post(url=url,data=json.dumps(payload), headers=headers)
    except:
        print ('No response from server : "'+ adminServer +'".  CHECK the IP and PORT.' )
        sys.exit(1)

    # Process the response and print return information
    rc = parseResponse( 'Stat', adminServer, objectType, response )

    return( int(rc) )
        
#-----------------------------------------------------------------------------#
# Check and invoke main
if __name__ == "__main__":
   rc = main( sys.argv[1:] )

sys.exit( rc )
