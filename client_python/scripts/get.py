#!/usr/bin/python
# Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

# MessageSight REST API interface to show configuration of an object

# Import the Rest API Python Library
import os, urllib
path = os.path.dirname(os.path.realpath(__file__))

execfile(path + '/restapiPythonLib.py')

#------------------------------------------------------------------------------
def parseParamsOptions( type, params ) :
    """
    Parse the INPUT when it is NOT JSON, currently Accepts "-i parameter"
    Convert the key=value pairs into JSON:  {"key":"value"}
    """
    
    trace(1, '...Entering: parseParamsOptions with: '+ type +' , '+ str(params) )
    trace(2, 'LEN(params) = '+ str(len(params)) )

    param_str = ""
    DELIMITER = '='

    if type == 'File' :
        if len(params) == 3:
#            param_str = ', "Arg1":"'+ params[0] +'", "Arg2":"'+ params[1] +'", "Password":"'+ params[2] +'","ScriptType":"file", "Command":"get","Action":"msshell"'
            param_str = '"Arg1":"'+ params[0] +'", "Arg2":"'+ params[1] +'", "Password":"'+ params[2] +'","ScriptType":"file", "Command":"get","Action":"msshell"'
        else:
            param_str = 'ERROR:  Syntax error for command: '+ type
    else :
        for item in params:
            name, value = item.split( DELIMITER, 1 )
            trace(2, 'Name: '+ name +' , Value: '+ value )
            # Handle First param.vs.subsequent
            if param_str :
                if value.isdigit() :
                    param_str = param_str +', "'+ name +'":'+ value 
                else:
                    param_str = param_str +', "'+ name +'":"'+ value +'"'
            else :
                if value.isdigit() :
                    param_str = '{"'+ name +'":'+ value
                else:
                    param_str = '{"'+ name +'":"'+ value +'"'
                    
        param_str = param_str +"}"
        
    trace(1, 'param_str: '+ param_str )
    return( param_str )



#------------------------------------------------------------------------------
# Main module to process arg and send POST
#------------------------------------------------------------------------------
def main(argv):

    # Parse arguments
	# Using EPILOG to get ConfigObject choices to print BUT ALWAYS Prints, not just on HELP -- WHY!
	# The value of CHOICES - ONLY Shows on BAD option, NOT on -h or null
#    parser = argparse.ArgumentParser(description='Get configuration object', epilog=usage())
    parser = argparse.ArgumentParser(description='Get a Configuration object' )
    parser.add_argument('-s', '--server', metavar='adminServerIP:port', default="IMA_AdminEndpoint", type=str, help='adminServerIP:port or set Env. Var:  IMA_AdminEndpoint')
    parser.add_argument('-i', '--input', metavar='InputFormatOptions', default='json', type=str, help='How to provide the ParameterOptions input', choices=['json', 'parameter' ] )
    parser.add_argument('-o', '--output', metavar='OutputDisplayOptions', default='json', type=str, help='How to display the output', choices=['json', 'parameter', 'csv' ] )
    parser.add_argument('-f', '--file', metavar='FileName', required=False, type=str, help='Import a Set of Configuration Objects, ConfigObject may be specified, ParameterOptions should NOT be specified.')
    parser.add_argument('-v', '--verbose', default=0, action='count', help='Set the verbosity of the tracing output' )
    parser.add_argument('-r', '--repeat', metavar='RepeatEverySec', default=0, type=int, help='Repeat this command every [r] seconds' )
#  Review: asked to remove validation of ObjectTypes
#   parser.add_argument('object', metavar='ObjectType', type=str, nargs="?", default='', help='Configuration object\'s type requested', choices=['', 'AdminLog', 'ConnectionLog', 'LogLevel', 'MQConnectivityLog', 'SecurityLog', 'LogConfig', 'WebUIHost', 'WebUIPort', 'File', 'FIPS', 'EnableDiskPersistence', 'LicensedUsage', 'TraceFilter', 'TraceLevel', 'TraceSelected', 'TraceMax', 'TraceConnection', 'TraceOptions', 'TraceMessageData', 'SSHHost', 'LogAutoTransfer', 'RemoteLogServer',  'CertificateProfile', 'LTPAProfile', 'OAuthProfile', 'SecurityProfile', 'TrustedCertificate', 'LDAP', 'ConnectionPolicy', 'Endpoint', 'MessageHub', 'MessagingPolicy', 'Queue', 'DestinationMappingRule', 'QueueManagerConnection', 'HighAvailability', 'Logging-settings', 'TopicMonitor', 'ClusterMembership', 'imaserver', 'webui' ] )
    parser.add_argument('object', metavar='ObjectType', type=str, nargs="?", default='', help='Configuration object\'s type requested' )
    parser.add_argument('name', metavar='Name', type=str, nargs='?', help='Configuration object\'s name')
    parser.add_argument('params', metavar='ParameterOptions', nargs=argparse.REMAINDER, help='Context specific Optional Parameters based on the ObjectType and Name chosen, input must be valid JSON by default.' )
    
    args = parser.parse_args()
    
    global DEBUG, OUT_FORMAT, OUT_FILE, REPEATS
    
    if args.server == "IMA_AdminEndpoint" :
        adminServer = os.environ[ args.server ]
    else : 
        adminServer = args.server

    IN_FORMAT = args.input
    OUT_FORMAT = args.output
    if args.file is not None :
        OUT_FILE = args.file
    DEBUG=args.verbose
    repeatTime = args.repeat
    REPEATS = 1

    uriObject = ''
    trace(2,' -- args.object '+ str(args.object) )
    if args.object :
        uriObject = '/' + args.object
    
    uriName = ''
    trace(2,' -- args.name '+ str(args.name) )
    if args.name :
        # Handle the unnamed Composite Objects, mistook 1st parameter as the 'name' or if NO ObjectType was given
        if args.object == 'ClusterMembership' or args.object == 'HighAvailability' or args.object == 'LDAP' or args.object == '' :
            args.params.insert(0, args.name )
        else :
#            uriName = '/' + args.name
            uriName = '/' + urllib.quote(args.name)
    
    param_str = '{}'
    trace(2,' -- args.params '+ str(args.params) )
    if args.params :
        if args.input == 'parameter' :
           param_str = parseParamsOptions( args.object, args.params )
        elif args.input == 'json' :
           param_str = str(args.params[0])
           
        if 'ERROR' in param_str :
            print( param_str )
            sys.exit( 2 )
            
    # Set headers
    headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}

    # Set uri and payload
    if args.object == 'imaserver' or args.object == 'mqconnectivity' or args.object == 'Status' :      
        uriPath = '/ima/v1/service'
    elif args.object == 'file' :
        uriPath = '/ima/v1/file'
    else :
        uriPath = '/ima/v1/configuration'

    payload_str = param_str 

    trace(1, 'PARAMS: \n'+ payload_str )
    payload = json.loads(payload_str)

    # send HTTP GET request
    url = 'http://' + adminServer + uriPath + uriObject + uriName
    trace(1, 'URL: \n'+ url)
    
    try:
        rc = 0
        while REPEATS > 0 :
            try:
                if repeatTime == 0 :
                    response = requests.get(url=url,params=json.dumps(payload), headers=headers)
                else : 
                    response = requests.get(url=url,params=json.dumps(payload), headers=headers, stream=True)
            except:
                print('No response from server : "'+ adminServer +'".  CHECK the IP and PORT.' )
                sys.exit(1)
    
            # Process the response and print return information
            rc = parseResponse( 'Get', adminServer, args.object, args.name, response )
        
            if repeatTime != 0 :
                time.sleep( repeatTime )
            else :
                REPEATS = 0      
            
    except KeyboardInterrupt:
        trace(1,"_DONE_")
        response.close
        
    main_rc = 0
    if int( rc ) != 0 :
        main_rc = 1
        
    return( main_rc )

#-----------------------------------------------------------------------------#
# Check and invoke main
if __name__ == "__main__":
   rc = main(sys.argv[1:])

sys.exit( rc )

