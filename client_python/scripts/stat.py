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

execfile(path + '/restapiPythonLib.py')

#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
def parseObjArgs( obj_args ):
    trace(1, '...Entering: parseObjArgs on args: '+ str(obj_args) )
#    json_args = json.dumps( obj_args, separators=(', ', '=') )
#    print( json_args )
    json_str = ""
    for item in obj_args:
        if '=' in item:
           name, value = item.split( '=', 1 )
           trace(2, 'Name: '+ name +' , Value: '+ value )
           json_str = json_str +', "'+ name +'":"'+ value +'"'
        else:
           json_str = json_str +', "Name":"'+ item +'"'
    trace(1, 'JSON_STR: '+ json_str )
    return ( json_str )    

    
#------------------------------------------------------------------------------
# Main module to process arg and send POST
#------------------------------------------------------------------------------
def main(argv):
    adminServer = ''
    uriPath = '/ima/v1/monitor'

    # Parse arguments
    parser = argparse.ArgumentParser(description='Show the Configuration of an Object')
    parser.add_argument('-s', '--server', metavar='adminServerIP:port', default="IMA_AdminEndpoint", type=str, help='adminServerIP:port or set Env. Var:  IMA_AdminEndpoint')
    parser.add_argument('-o', '--output', metavar='OutputDisplayOptions', default='json', type=str, help='How to display the output', choices=['list', 'json', 'pretty', 'csv' ] )
    parser.add_argument('-f', '--file', metavar='FileName', required=False, type=str, help='Import a Set of Configuration Objects, ConfigObject may be specified, ParameterOptions should NOT be specified.')
    parser.add_argument('-v', '--verbose', default=0, action='count', help='Set the verbosity of the tracing output' )
    parser.add_argument('-r', '--repeat', metavar='RepeatEverySec', default=0, type=int, help='Repeat this command every [r] seconds' )
    parser.add_argument('object', metavar='ObjectType', type=str, help='Configuration object type', choices=['LogAutoTransfer', 'engine', 'Endpoint', 'Queue', 'DestinationMappingRule', 'MQTTClient', 'Subscription', 'Connection', 'Memory', 'Server', 'Store', 'Topic', 'Transaction', 'MessagingChannel' ])
    parser.add_argument('name', metavar='Name', type=str, nargs='?', help='Configuration object\'s name')
    parser.add_argument('params', metavar='ParameterOptions', nargs=argparse.REMAINDER , help='Context specific Optional Parameters based on the ConfigObject and Name chosen.' )

    args = parser.parse_args()

    global DEBUG, OUT_FORMAT, OUT_FILE, REPEATS
    
    if args.server == "IMA_AdminEndpoint" :
        adminServer = os.environ[ args.server ]
    else : 
        adminServer = args.server

    OUT_FORMAT = args.output
    if args.file is not None :
        OUT_FILE = args.file
    DEBUG=args.verbose
    repeatTime = args.repeat
    REPEATS = 1

    objectType = ''
    trace(2,' -- args.object '+ str(args.object) )
    if args.object :
        objectType = args.object
    
    objectNameJSON = ''
    objectName = ''
    trace(2,' -- args.name '+ str(args.name) )
    if args.name :
        objectNameJSON = ',"Name":"' + args.name + '"'
        objectName = args.name
    
    # Parse the rest of the Args and build the JSON string
    json_args = ''
    trace(2,' -- args.params '+ str(args.params) )
    if args.params :
        json_args = parseObjArgs( args.params )
        if 'ERROR' in json_args :
            print( json_args )
            sys.exit( 2 )
            
    # Set headers
    headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}

    # Set payload
    payload_str = '{ "Locale":"en_US","User":"admin"'+ objectNameJSON + json_args 
    
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
            payload_str = payload_str +',"SubName":"' + objectName + '"'
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
    trace(1, 'CMD: '+ payload_str )
    
    payload = json.loads(payload_str)

    # send POST request
    url = 'http://' + adminServer+uriPath
    trace(1, 'URL: \n'+ url)

    try:
        rc = 0
        while REPEATS > 0 :
            try:
                response = requests.post(url=url,data=json.dumps(payload), headers=headers)
            except:
                print('No response from server : "'+ adminServer +'".  CHECK the IP and PORT.' )
                sys.exit(1)
    
            # Process the response and print return information
            rc = parseResponse( 'Stat', adminServer, args.object, args.name, response )
        
            if repeatTime != 0 :
                time.sleep( repeatTime )
            else :
                REPEATS = 0      
            
    except KeyboardInterrupt:
        print("_DONE_")


    return( int(rc) )
        
#-----------------------------------------------------------------------------#
# Check and invoke main
if __name__ == "__main__":
   rc = main( sys.argv[1:] )

sys.exit( rc )
