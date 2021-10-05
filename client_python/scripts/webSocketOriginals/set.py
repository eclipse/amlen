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
def getDefaultValue( name ):
    if name in ['LogLevel', 'ConnectionLog', 'AdminLog', 'SecurityLog' ]:
        value = 'NORMAL'
    elif name in [ 'TraceLevel', 'TraceFilter', 'TraceSelected', 'TraceConnection', 'TraceOptions' ]:
        value = ""
    elif name in [ 'TraceMax' ]:
        value = '200M'
    elif name in [ 'TraceMessageData' ]:
        value = 0
    else:
        print( 'There is NO DEFAULT Value for "'+ name +'".  Allowing it to be NULL, which will UNSET!' )
        value = ""
    return value


#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
def validateNameValue( name, value ):
    print( 'Entering validateNameValue: '+ name +' , '+ value )
    r_msg = ""
    if name in ['LogLevel', 'ConnectionLog', 'AdminLog', 'SecurityLog' ]:
        if value not in [ 'MIN', 'NORMAL', 'MAX' ]:
            r_msg = 'Value of '+ name +' must be MIN, NORMAL, or MAX'
    elif name in [ 'TraceMessageData' ]:
        if ( value < 0 & value >= 65535 ):
            r_msg = 'Value of '+ name +' must be within range [0..65535]'
            
    print( 'Exiting validateNameValue: "'+ r_msg +'"' )
    return r_msg
    

#------------------------------------------------------------------------------
# Main module to process arg and send POST
#------------------------------------------------------------------------------
def main(argv):
    adminServer = ''
    uriPath = '/ima/admin/config'

    # Parse arguments
    # Using EPILOG to get ConfigObject choices to print BUT ALWAYS Prints!
    # The value of CHOICES - ONLY Shows on BAD option, NOT on -h or null
#    parser = argparse.ArgumentParser(description='Set configuration object', epilog=usage())
    parser = argparse.ArgumentParser(description='Set configuration object' )
    parser.add_argument('-s', metavar='adminServerIP:port', required=True, type=str, help='adminServerIP:port')
#    parser.add_argument('object', metavar='ConfigObject=Value', type=str, help='ConfigObject=Value; where ConfigObject in [LogLevel, ConnectionLog, AdminLog, SecurityLog, TraceLevel, TraceFilter, TraceSelected, TraceMax, TraceConnection, TraceOptions, TraceMessageData]' )
    parser.add_argument('object', metavar='ConfigObject', type=str, help='Configuration object', choices=['AdminLog', 'ConnectionLog', 'LogLevel', 'MQConnectivityLog', 'SecurityLog', 'LogConfig', 'WebUIHost', 'WebUIPort', 'FIPS', 'EnableDiskPersistence', 'LicensedUsage', 'TraceFilter', 'TraceLevel', 'TraceSelected', 'TraceMax', 'TraceConnection', 'TraceOptions', 'TraceMessageData', 'SSHHost'] )
    parser.add_argument('value',metavar='ConfigValue', type=str, nargs=argparse.REMAINDER, help='Value of ConfigObject' )
    args = parser.parse_args()

    adminServer = args.s

    OPTION_CHAR = '='
    if OPTION_CHAR in args.object:
        objectType, objectValue = args.object.split(OPTION_CHAR, 1)
    else:
        objectType=args.object
        if len(sys.argv) > 4 :
            objectValue = args.value[0]
        else:
            objectValue = getDefaultValue( args.object )
    trace( 'object(Type, Value): ('+ objectType +' , ' + objectValue +' )' )
    
    msg = validateNameValue( objectType, objectValue )
    if len(msg) > 0:
        print( 'ERROR Validating input ' )
    else:
        print( "GO FOR IT!" )

    # Set headers
    headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}

    # Set payload
#    payload_str = '{"Locale":"en_US","User":"admin","Action":"set","Item":"' + str(objectType) +'","Value":"' + str(objectValue) + '","Component":"Server"}'
    payload_str = '{"Locale":"en_US","User":"admin","Action":"set","Item":"' + str(objectType) +'","Value":"' + str(objectValue) + '"}'
    trace( 'CMD:  '+ payload_str )
    payload = json.loads(payload_str)
    
    # send POST request
    url = 'http://' + adminServer+uriPath

    try:
        response = requests.post(url=url,data=json.dumps(payload), headers=headers)
    except:
        print('No response from server : "'+ adminServer +'".  CHECK the IP and PORT.' )
        sys.exit(1)

    # Process the response and print return information
    rc = parseResponse( 'List', adminServer, objectType, response )

    return( int(rc) )

# Check and invoke main
if __name__ == "__main__":
   rc = main(sys.argv[1:])
sys.exit( rc )

