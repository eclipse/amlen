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

# MessageSight command line interface to query/set RUNMODE

# Import the Message Sight Python Library
import os
path = os.path.dirname(os.path.realpath(__file__))

execfile(path + '/messageSightPythonLib.py')

#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
def parseModeArgs( obj_args ):
#    print( 'Entering: '+ func_name + __name__ + ' parseModeArgs ' +'  on args: '+ str(obj_args) )
    trace( '...Entering:  parseModeArgs  on args: '+ str(obj_args) )
#    Really do not expect multiple arguments, but will pass on what is received if it is a known mode.
    imamodes = [ 'production=0', 'maintenance=1', 'clean_store=2' ]
    json_str = ""
    for item in obj_args:
        if '=' in item :
            name, value = item.split( '=', 1 )
            json_str = json_str +', "'+ name +'":"'+ value +'"'
            
            print( 'Name: '+ name +' , Value: '+ value )
        else :
            modecode = [s for s in imamodes if item in s]
            if len(modecode) > 0 :
                trace( 'MODE: '+ str(modecode) )
                name, value = modecode[0].split('=')
                json_str = json_str +',"Mode":"'+ value +'"'
            else:
                print( '\nWARNING!  Unrecognized option: "'+ item +'", it will be passed as is.' )
                json_str = json_str +',"Mode":"'+ item +'"'
                
        
    trace( 'JSON_STR: '+ json_str )
    return ( json_str )    
    
#------------------------------------------------------------------------------
# Main module to process arg and send POST
#------------------------------------------------------------------------------
def main(argv):
    adminServer = ''
    uriPath = '/ima/admin/config'

    # Parse arguments
    parser = argparse.ArgumentParser(description='Query or Set MessageSight RUNMODE')
    parser.add_argument('-s', metavar='adminServerIP:port', required=True, type=str, help='adminServerIP:port, ex: 198.23.126.136:9089')
    parser.add_argument('mode', metavar='Mode', type=str, nargs=argparse.REMAINDER, help='Configuration object type', choices=[ 'production', 'maintenance', 'clean_store'] )

    args = parser.parse_args()

    adminServer = args.s
    trace( 'LEN(args )='+ str(len(sys.argv)) )
    objectType = ""
    if len( sys.argv ) >= 4 :
        objectType = parseModeArgs( args.mode )

    # Set headers
    headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}

    # Set payload
    # CMD: { "Action":"setmode","User":"root"  [,"Mode":"[0|1|2]" ]}  [Production|Maintenance|Clean_store]
    payload_str = '{"Locale":"en_US","User":"admin","Action":"setmode"'+ str(objectType) + '}'
    trace( 'CMD: '+ payload_str )
    payload = json.loads(payload_str)

    # send POST request
    url = 'http://' + adminServer+uriPath

    try:
        response = requests.post(url=url,data=json.dumps(payload), headers=headers)
    except:
        print('No response from server : "'+ adminServer +'".  CHECK the IP and PORT.' )
        sys.exit(1)

    # Process the response and print return information
    rc = parseResponse( 'Runmode', adminServer, objectType, response )

    return( int(rc) )

    
# Check and invoke main
if __name__ == "__main__":
   main(sys.argv[1:])

