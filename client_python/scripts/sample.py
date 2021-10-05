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
import os, urllib, urlparse
# Py3.0 import urllib.parse replaces urlparse
path = os.path.dirname(os.path.realpath(__file__))

execfile(path + '/restapiPythonLib.py')

#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Main module to process input args and submit HTTP Request
#------------------------------------------------------------------------------
def main(argv):

    # Parse arguments
    parser = argparse.ArgumentParser(description='REST API Sample for Messaging Server' )
    parser.add_argument('-u', '--URL', metavar='RestAPI Domain URI', type=str, help='http://adminServerIP:port/... or if /ima, use Env. Var:  IMA_AdminEndpoint for host.')
    parser.add_argument('-x', '--request', metavar='HTTP Request', default='get', type=str, help='HTTP ACTION to execute', choices=['get', 'put', 'post', 'delete' ] )
    parser.add_argument('-o', '--output', metavar='OutputDisplayOptions', default='json', type=str, help='How to display the output', choices=['json', 'parameter', 'csv' ] )
    parser.add_argument('-f', '--file', metavar='FileName', required=False, type=str, help='Import a Set of Configuration Objects, ConfigObject may be specified, ParameterOptions should NOT be specified.')
    parser.add_argument('-v', '--verbose', default=0, action='count', help='Set the verbosity of the tracing output' )
    parser.add_argument('-r', '--repeat', metavar='RepeatEverySec', default=0, type=int, help='Repeat this command every [r] seconds' )
    parser.add_argument('-d', '--data', metavar='Data', default='{}', help='IF GET, then Parameter Filter data appended to URL after ?; For POST, then JSON for payload.' )
    parser.add_argument('-e', '--encode', action='store_true', help='encode the path of the URL' )
    
    args = parser.parse_args()
    
    global DEBUG, OUT_FORMAT, OUT_FILE, REPEATS
    OUT_FORMAT = 'json'
    DEBUG = args.verbose
    repeatTime = args.repeat
    REPEATS = 1
    
    if args.URL.startswith('/ima') :
        adminServer = os.environ[ "IMA_AdminEndpoint" ]
        if args.encode == True :
            trace(0, "encoding commences on "+ args.URL +" ..." )
            uriPath = 'http://' + adminServer + urllib.quote(args.URL)
        else :
            uriPath = 'http://' + adminServer + args.URL
        trace(1, "IMA_AdminEndpoint: "+ adminServer)
    else :
        
        try :
            sIndex = args.URL.index('//') + 2
            eIndex = args.URL.index('/ima')
            adminServer = args.URL[sIndex:eIndex]
            if args.encode == True:
                trace(0, "encoding commences.." )
                # Encode the path if it has 'Special Characters like ? in Objectname confuse urlparse'
                encodedPath = urllib.quote( args.URL[eIndex:] )
                trace(0, encodedPath )
#                urlParts = urlparse.urlsplit(args.URL)
                args.URL = args.URL[0:eIndex] + encodedPath
            trace(1, args.URL +" HOSTNAME start@: "+ str(sIndex) +" ends@: "+ str(eIndex) + " for IMA_AdminEndpoint of" + adminServer )
            
        except ValueError :
            adminServer = args.URL
            
        uriPath = args.URL

    trace(2,' -- args.data '+ str(args.data) )
        
    # Set headers
    headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}

    # Set uri and payload
    url = uriPath

    if args.request == 'get' :
        payload = args.data
        trace(1, 'Params: \n'+ args.data )
    else :
        trace(0, 'DATA: \n'+ args.data )
        payload = json.loads(args.data)
        trace(1, 'DATA: \n'+ args.data )
        
    trace(1, 'HTTP Request: "'+ args.request +'" from URL: \n'+ url +'\n')
    

    # send HTTP Request
    
    try:
        rc = 0
        while REPEATS > 0 :
            try:
                if repeatTime == 0 :
                    if args.request == 'get' :
                        response = requests.get(url=url,params=json.dumps(payload), headers=headers)
                    elif args.request == 'put' :
                        response = requests.put(url=url,data=json.dumps(payload), headers=headers)
                    elif args.request == 'post' :
                        response = requests.post(url=url,data=json.dumps(payload), headers=headers)
                    elif args.request == 'delete' :
                        response = requests.delete(url=url,data=json.dumps(payload), headers=headers)
                else : 
                    if args.request == 'get' :
                        response = requests.get(url=url,params=json.dumps(payload), headers=headers, stream=True)
                    elif args.request == 'put' :
                        response = requests.put(url=url,data=json.dumps(payload), headers=headers, stream=True)
                    elif args.request == 'post' :
                        response = requests.post(url=url,data=json.dumps(payload), headers=headers, stream=True)
                    elif args.request == 'delete' :
                        response = requests.delete(url=url,data=json.dumps(payload), headers=headers, stream=True)
            except:
                print('No response from server : "'+ adminServer +'".  CHECK the IP and PORT.' )
                sys.exit(1)
    
            # Process the response and print return information
            rc = parseResponse( args.request, adminServer, "args.object", "args.name", response )
        
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

