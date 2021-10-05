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

import os, sys, time, json

path = os.path.dirname(os.path.realpath(__file__))

execfile(path + '/restapiPythonLib.py')
#------------------------------------------------------------------------------
def parseEnvConfig(filename):
    """
    Parse automation configuration file Env Var to use for variable replacement
    Returns options[] array objects. Ex) {'ear.name': 'testTools_JCAtests'}
    """
    trace(1,'...Entering  parseEnvConfig  on file:' + filename)
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
            export, option = option.split( ' ' , 1)

#            print 'option: %s, value: %s' % (option,value)
            options[option] = value
    return options


#------------------------------------------------------------------------------
def parseInput(filename, group):
    """
    Parse automation CLI file and convert to JSON
    """
    trace(1,'...Entering  parseInput  on file:' + filename + ' looking for: '+ group)
    FOUND_GROUP = 0
    ERROR_COUNT = 0 
    CMD_COUNT = 0

    json_data = json.loads( open(filename).read() )
    #print( '\n'+ str(json_data[group][0]['request'][0] ))
    #print( '\n'+ str(json_data[group][0]['request'][1] ))
    
    
    for item in json_data:
        trace(3, '\nitem is '+ str(type( item )) + ' with value of: '+ str(item).decode('unicode_escape').encode('ascii','ignore') )
        # Look for the COMMAND GROUP passed as 'group'
        if item == group :
            FOUND_GROUP = 1
            for request in json_data[item] :
                trace(3, '\nrequest is '+ str(type( request )) + ' with value of:'+ str(request).decode('unicode_escape').encode('ascii','ignore') )
                # REQUEST is a dict_type with value of any array of objects
                
                for key, value in request.iteritems() :
                    trace(3, '\nkey is '+ str(type( key )) + ' with value of:'+ str(key).decode('unicode_escape').encode('ascii','ignore') )
                    trace(3, '\nvalue is '+ str(type( value )) + ' with value of:'+ str(value) )
                    
                    for command in value :
                        trace(3, '\nA command is: '+ str( command ) )
                        
                        for clause1, clause2 in command.iteritems() :
                            trace(3, '\nClause1 :'+ str(clause1)  )
                            trace(3, '\nClause2 :'+ str(clause2)  )

                            if clause1 == 'response' :
                               validate_this = clause2
                            else :
                                action = clause1
                                execute_this = clause2
                                
                        trace(1, '\nExecute Action: '+ str(action) + " on " + str(execute_this) + '\nand Validate This: '+ str(validate_this) )
                        for objectType, clause3 in execute_this.iteritems() :
                            trace(2, '\nThe Object is: '+ objectType +' and Remainder is: '+ str(clause3) )
                            uriObject = '/'+ objectType
                            
                            for objectName, params in clause3.iteritems() :
                                trace(2, '  The ObjectName is: "'+ str(objectName) +'" with Domain URI: '+ params['domain'] +' and PAYLOAD: '+ str(params['data']) )
                                uriName = '/'+ str(objectName)
                                uriPath = params['domain']
                                payload = params['data']
                                
                            expectRC = validate_this['rc']
                            expectData = validate_this['text']
                            
                            # send HTTP request
                            headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}
                            url = 'http://' + adminServer + uriPath + uriObject + uriName

                            CMD_COUNT += 1
                            print('\nCOMMAND #'+ str(CMD_COUNT) +' is HTTP '+ action )
                            trace(1, 'URL: \n'+ url)
                            if action == 'get' :
                                trace(0, 'PARAMS: \n'+ json.dumps(payload))
                            else :
                                trace(0, 'DATA: \n'+ json.dumps(payload))
                            try:
                                if action == 'get' :
                                    response = requests.get(url=url,params=json.dumps(payload), headers=headers)
                                elif action == 'post' :
                                    response = requests.post(url=url,data=json.dumps(payload), headers=headers)
                                elif action == 'delete' :
                                    response = requests.delete(url=url,data=json.dumps(payload), headers=headers)
                                elif action == 'put' :
                                    response = requests.put(url=url,data=json.dumps(payload), headers=headers)
                                else :
                                    print( '\nERROR:  ACTION: "'+ action +'" is Not Found.  Valid actions are:  get, post, delete, and put' )



                            except:
                                print('No response from server : "'+ adminServer +'".  CHECK the IP and PORT.' )
                                sys.exit(1)
    
                            # Process the response and print return information
                            
                                
                            rc = parseResponse( action, adminServer, objectType, objectName , response )

                            if response.status_code == expectRC :
                                print( 'GOT EXPECTED RC' )
                            else :
                                ERROR_COUNT+=1
                                print( '\n ERROR #'+ str(ERROR_COUNT) +':  DID NOT GET EXPECTED RC, '+ str(expectRC) +', instead ACTUALLY GOT: '+ str(response.status_code) +'\n' )
                                
                            if json.loads(response.text) == expectData :
                                print( 'GOT EXPECTED Data\n' )
                            else :
                                ERROR_COUNT+=1
                                print( '\n ERROR #'+ str(ERROR_COUNT) +':  DID NOT GET EXPECTED data, \n'+ json.dumps(expectData) +'\ninstead ACTUALLY GOT: \n'+ str(response.text)  +'\n')
        else :
            trace(0, "Found 'group': " + str(item) +".  Still looking for 'group':" + group )
            
    if FOUND_GROUP == 0 :
        ERROR_COUNT += 1
        print( '\n ERROR #'+ str(ERROR_COUNT) +':  DID NOT GROUP: "'+ group  +'". NOTHING WAS RUN!\n')
    return ERROR_COUNT

#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# MAIN
#------------------------------------------------------------------------------
# Parse arguments
# Using EPILOG to get ConfigObject choices to print BUT ALWAYS Prints, not just on HELP -- WHY!
# The value of CHOICES - ONLY Shows on BAD option, NOT on -h or null
#    parser = argparse.ArgumentParser(description='Get configuration object', epilog=usage())
parser = argparse.ArgumentParser(description='Execute a Set of REST API CLI Commands by Group' )
parser.add_argument('-s', '--server', metavar='adminServerIP:port', default="IMA_AdminEndpoint", type=str, help='adminServerIP:port or set Env. Var:  IMA_AdminEndpoint')
parser.add_argument('-v', '--verbose', default=0, action='count', help='Set the verbosity of the tracing output' )
parser.add_argument('file', metavar='File', type=str, help='JSON Command File')
parser.add_argument('group', metavar='Group', type=str, help='Command Group to execute')
    
args = parser.parse_args()
    
global DEBUG, OUT_FORMAT
    
if args.server == "IMA_AdminEndpoint" :
    adminServer = os.environ[ args.server ]
else : 
    adminServer = args.server

OUT_FORMAT = 'json'
DEBUG=args.verbose


print('\n'+ sys.argv[0] +' will execute the "'+ args.group +'" commands in file: '+ args.file +' - running DebugMode '+ str(DEBUG) + '\n')

## Parse the Environment configuration file
envfile = '/niagara/test/testEnv.sh'
if os.path.exists( envfile ):
   trace(2,envfile)
   options = parseEnvConfig( envfile )
else:
   envfile = '/niagara/test/scripts/ISMsetup.sh'
   trace(2,envfile)
   options = parseEnvConfig( envfile )

#print options

trace(1,'A1_HOST has the value of: %s' % (options['A1_HOST']) )


rc = parseInput( args.file, args.group )

if rc != 0 :
   print( '\nFAILED:  '+ str(rc) +' ERRORS occurred!')
else :
   print( '\nPASSED:  no errors found!')
   
   

sys.exit( rc )
