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

# MessageSight command line interface Python Library

# Import required libs
import json
import requests
import sys
import argparse
import pprint
import types
import time

DEBUG=0

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#def trace(name,pos):
def trace(name):
    """
    Trace function to print out messages with timestamps
    """
    global DEBUG
    if (DEBUG):
        formatting_string = '[%Y %m-%d %H:%M-%S00]'
        timestamp = time.strftime(formatting_string)
        print( '\n%s %s ' % (timestamp,name) )


#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def parseResponseLine( action, line ):
#    trace( '...Entering:  parseResponseLine on: '+ str(line) )
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
# Process the response object of any command
#------------------------------------------------------------------------------
def parseResponse( action, adminServer, objectType, response ) :
    trace( '...Entering:  parseResponse on: '+ action +', '+ objectType +'. ' )
    rc = 0
    try:
        response.raise_for_status() 
    
    except:   #if response.status_code != 200 :
        # print response status code
        print( '\nAction:  "'+ action +' '+ objectType +'" sent to Message Sight at: '+ adminServer +' failed!')
        print( 'Returned ERROR:  '+ str(response.status_code) )
        rc = response.status_code 
# # Both fail, should they?
# #        print( __context__ )        
# #        print( __cause__ )       
    else:

        # Get response data and print
        trace( 'RESPONSE.headers: \n'+ str(response.headers) )
        trace( 'RESPONSE.cookies: \n'+ str(response.cookies) )
        trace( 'RESPONSE.content: \n'+ str(response.content) )
#        trace( 'RESPONSE.text: \n'+ str(response.text) )
#        trace( dir( response ) )
#    trace('\nApparentEncoding: ')
#    trace( response.apparent_encoding ) 
#    trace('\nConnection: ')
#    trace( response.connection ) 
#    trace('\nCONTENT: ')
#    trace( response.content ) 
#    trace('\nCOOKIES: ')
#    trace( response.cookies ) 
#    trace('Elapsed: \n')
#    trace( response.elapsed ) 
#    trace('Encoding: \n')
#    trace( response.encoding ) 
#    trace('History: \n')
#    trace( response.history ) 
#    trace('links: \n')
#    trace( response.links ) 
#    trace('\nOK: ')
#    trace( response.ok ) 
#    trace('\nRAW: ')
#    trace( response.raw ) 
#    trace('\nREASON: ')
#    trace( response.reason ) 
#    trace('url: \n')
#    trace( response.url ) 
#    trace('iterContent: \n')
#    trace( response.iter_content ) 
#    trace('iterLines: \n')
#    trace( response.iter_lines )
#    trace('DOC: \n')
#    trace( response.__doc__ )
#    trace('CLOSE: \n')
#    trace( response.close.__doc__ )    
    
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
            # No errors - print list
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
                if (action == 'Status' and  objectType == 'imaserver') or (action == 'Imaserver' and objectType == 'Status' ):
                    try:
                        srvstat = data["ServerStateStr"]
                        print( srvstat.decode('unicode_escape').encode('ascii','ignore') )
                        print( "ServerUpTime = ", data["ServerUPTimeStr"].decode('unicode_escape').encode('ascii','ignore') )

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

