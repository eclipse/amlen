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
def parseParamsOptions( type, params ) :
    trace( '...Entering: parseParamsOptions with: '+ type +' , '+ str(params) )
    param_str = ""
    trace( 'LEN(params) = '+ str(len(params)) )

    if type == 'File' :
        if len(params) == 3:
            param_str = ', "Arg1":"'+ params[0] +'", "Arg2":"'+ params[1] +'", "Password":"'+ params[2] +'","ScriptType":"file", "Command":"get","Action":"msshell"'
        else:
            param_str = 'ERROR:  Syntax error for command: '+ type
            
    trace( param_str )        
    return( param_str )



#------------------------------------------------------------------------------
# Main module to process arg and send POST
#------------------------------------------------------------------------------
def main(argv):
    adminServer = ''
    uriPath = '/ima/admin/config'

    # Parse arguments
	# Using EPILOG to get ConfigObject choices to print BUT ALWAYS Prints -- WHY!
	# The value of CHOICES - ONLY Shows on BAD option, NOT on -h or null
#    parser = argparse.ArgumentParser(description='Get configuration object', epilog=usage())
    parser = argparse.ArgumentParser(description='Put File (Download from Container' )
    parser.add_argument('-s', metavar='adminServerIP:port', required=True, type=str, help='adminServerIP:port')
    parser.add_argument('object', metavar='ConfigObject', type=str, help='Configuration object', choices=['Put'] )
    parser.add_argument('params', metavar='ParameterOptions', nargs=argparse.REMAINDER, help='Parameter Options are dependant on the ConfigObject choice.' )
    
    args = parser.parse_args()

    adminServer = args.s
    objectType = args.object
    
    param_str = ""
    if len(sys.argv) > 4 :
        param_str = parseParamsOptions( args.object, args.params )
        if 'ERROR' in param_str :
            print( param_str )
            sys.exit( 2 )

    # Set headers
    headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}

    # Set payload
    if objectType == 'File' :
        payload_str = '{"Locale":"en_US","User":"admin"'+ param_str +' }'
    else :
#        payload_str = '{"Locale":"en_US","User":"admin","Action":"put","Item":"' + objectType + '","Component":"Server"'+ param_str +' }'
        payload_str = '{"Locale":"en_US","User":"admin","Action":"put","Item":"' + objectType + '"'+ param_str +' }'
    trace( 'CMD: \n'+ payload_str )
    payload = json.loads(payload_str)

    # send POST request
    url = 'http://' + adminServer+uriPath

    try:
        response = requests.post(url=url,data=json.dumps(payload), headers=headers)
    except:
        print('No response from server : "'+ adminServer +'".  CHECK the IP and PORT.' )
        sys.exit(1)
    
    # Process the response and print return information
    rc = parseResponse( 'Put', adminServer, objectType, response )

    return( int(rc) )


# Check and invoke main
if __name__ == "__main__":
   rc = main(sys.argv[1:])
sys.exit( rc )

