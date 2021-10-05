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

# MessageSight command line interface Python Library

# Import required libs
import json
import requests
import sys
import argparse
import pprint
import types
import time

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def trace(traceLevel, name):
    """
    Trace function to print out messages with timestamps, traceLevel is set by the -v CLI option
    """
    if DEBUG >= traceLevel :
        formatting_string = '[%Y %m-%d %H:%M-%S00]'
        timestamp = time.strftime(formatting_string)
        print( '\n%s %s ' % (timestamp,name) )


#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def parseResponseLine( action, line ):
    """
    """
#    trace(1, '...Entering:  parseResponseLine on: '+ str(line) )
    items = line.items()
    for item in reversed(items):
        # print( type( item[1] ) ) 
        if type( item[1] ) is types.FloatType  or  type( item[1] ) is types.IntType  or  type( item[1] ) is types.DictType :
            val = str( item[1] )
        else :
            val = item[1].decode('unicode_escape').encode('ascii','ignore')
            
        if action == 'List' :
            print(  '  '+ val )
        else :
            key = item[0].decode('unicode_escape').encode('ascii','ignore')
            print( '  '+ key +' = '+ val )
    


#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def parseResponse( action, adminServer, objectType, objectName, response ) :
    """
    Process the response object of any command
    """
    trace(1, '...Entering:  parseResponse on: '+ action +', '+ objectType +'. ' )
    rc = 0
    try:
        response.raise_for_status() 
    
    except:   #if response.status_code != 200 :
        # print response status code
        print( '\nAction:  "'+ action +' '+ objectType +'" sent to Message Sight at: '+ adminServer +' failed!')
        print( 'Returned ERROR:  '+ str(response.status_code) )
        if response.text :
            print('TEXT: \n'+ response.text )
        rc = response.status_code 
# # Both fail, should they?
# #        print( __context__ )        
# #        print( __cause__ )       
    else:

        # Get response data and print
        trace(1, 'RESPONSE.headers: \n'+ str(response.headers) )
        trace(1, 'RESPONSE.cookies: \n'+ str(response.cookies) )
        trace(1, 'RESPONSE.content: \n'+ str(response.content) )
#        trace(1, 'RESPONSE.text: \n'+ str(response.text) )
#        trace(1, dir( response ) )
#    trace(1,'\nApparentEncoding: ')
#    trace(1, response.apparent_encoding ) 
#    trace(1,'\nConnection: ')
#    trace(1, response.connection ) 
#    trace(1,'\nCONTENT: ')
#    trace(1, response.content ) 
#    trace(1,'\nCOOKIES: ')
#    trace(1, response.cookies ) 
#    trace(1,'Elapsed: \n')
#    trace(1, response.elapsed ) 
#    trace(1,'Encoding: \n')
#    trace(1, response.encoding ) 
#    trace(1,'History: \n')
#    trace(1, response.history ) 
#    trace(1,'links: \n')
#    trace(1, response.links ) 
#    trace(1,'\nOK: ')
#    trace(1, response.ok ) 
#    trace(1,'\nRAW: ')
#    trace(1, response.raw ) 
#    trace(1,'\nREASON: ')
#    trace(1, response.reason ) 
#    trace(1,'url: \n')
#    trace(1, response.url ) 
#    trace(1,'iterContent: \n')
#    trace(1, response.iter_content ) 
#    trace(1,'iterLines: \n')
#    trace(1, response.iter_lines )
#    trace(1,'DOC: \n')
#    trace(1, response.__doc__ )
#    trace(1,'CLOSE: \n')
#    trace(1, response.close.__doc__ )    
    
        if response.headers["content-type"] == 'application/json' :
            data = json.loads(response.text)
    
        elif response.headers["content-type"] == 'text/plain;charset=utf-8' :
            data = response.text.decode('unicode_escape').encode('ascii','ignore')

        else:
            msgstr = '\nERROR:  Unexpected response.headers["content-type"] of "'+ response.headers["content-type"] +'".\nAttempt to force processing.\n '
            print( msgstr )
            data = response.content.decode('unicode_escape').encode('ascii','ignore')

        # Check for error
    
        try:
            msgstr = data["ErrorString"].decode('unicode_escape').encode('ascii','ignore')
            rc = data["RC"].decode('unicode_escape').encode('ascii','ignore')
            
        except:
            # No errors - print list or create the file
            if objectType == 'file' :
            
                f = open(objectName,'w')
                f.write(response.content)
                f.close()


            else :            
                if OUT_FORMAT == 'json' :
                    print json.dumps(data, indent=2, sort_keys=True)
                elif OUT_FORMAT == 'list' :
                    print('Oops, Output Format of "list" is not implemented yet.  Sorry.')
                elif OUT_FORMAT == 'csv' :
                    print('Oops, Output Format of "csv" is not implemented yet.  Sorry.')
                else :   # assume you took default 'parameter'
            
            
                    if type( data ) is types.StringType :
                        print( '\n'+ data ) 
                
                    elif  type( data ) is types.ListType :
                        print( '\n')
                        for row in data:
                            parseResponseLine( action, row )
                            if action == 'Show' or action == 'Stat' :
                                print( '\n')

                    elif type( data ) is types.DictionaryType :
                        print( '\n')
                        if (action == 'Get' and  objectType == 'imaserver') :
                            try:
                                srvstat = data["ServerStateStr"]
                                print( srvstat.decode('unicode_escape').encode('ascii','ignore') )
                                print( "ServerUpTime = "+ data["ServerUPTimeStr"].decode('unicode_escape').encode('ascii','ignore') )

                            except:
                                print( "Server didn't return status" )
                                print( "Return result string:" )
                                print( data )
                        
                        else : 
                            parseResponseLine( action, data )
                
                    else : 
                        print( '\nERROR:  Unexpected return type of data: "'+ type(data) +'".')
                        print( '\n'+ data ) 
             
        else:
            if rc != "0":
                print('\nERROR:  '+ msgstr )
            else:
                print('\nMSG:  '+ msgstr )
    print( '\n' )        
    return( int(rc) )

