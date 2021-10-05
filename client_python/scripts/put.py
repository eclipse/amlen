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

# REST API to PUT a FILE to Messaging Server

# Import the Rest API Python Library
import os
path = os.path.dirname(os.path.realpath(__file__))

execfile(path + '/restapiPythonLib.py')

#------------------------------------------------------------------------------
# Main module to process arg and send POST
#------------------------------------------------------------------------------
def main(argv):

    parser = argparse.ArgumentParser(description='Put File (upload to Docker Container' )
    parser.add_argument('-s', '--server', metavar='adminServerIP:port', default="IMA_AdminEndpoint", type=str, help='adminServerIP:port or set Env. Var:  IMA_AdminEndpoint')
    parser.add_argument('-v', '--verbose', default=0, action='count', help='Set the verbosity of the tracing output' )
    parser.add_argument('fileFrom', metavar='SourceTarget FileName', type=str, default='', help='Filename of the File to be uploaded to Messaging Server' )
    parser.add_argument('fileTo', metavar='Target FileName', type=str, nargs="?", default='', help='Filename of the File to be uploaded to Messaging Server' )
    
    args = parser.parse_args()

    global DEBUG, OUT_FORMAT
    
    if args.server == "IMA_AdminEndpoint" :
        adminServer = os.environ[ args.server ]
    else : 
        adminServer = args.server

    DEBUG = args.verbose
    OUT_FORMAT = 'json'

    uriObject = ''
    trace(2,' -- args.fileTo '+ str(args.fileTo) )
    if args.fileTo :
        uriObject = '/' + args.fileTo
    
    # Set headers
    headers = {"Content-type": "text/plain", "Accept": "text/plain"}

    # Set uri and payload
    uriPath = '/ima/v1/file'

    f = open(args.fileFrom,'r')
    filecontents = f.read()
    f.close()
    trace(2, 'PAYLOAD: '+filecontents )

    # send POST request
    url = 'http://' + adminServer + uriPath + uriObject
    trace(1, 'URL: \n'+ url)

    try:
        response = requests.put(url=url,data=filecontents, headers=headers)
    except:
        print('No response from server : "'+ adminServer +'".  CHECK the IP and PORT.' )
        sys.exit(1)
    
    # Process the response and print return information
    rc = parseResponse( 'Put', adminServer, 'file', args.fileTo, response )

    main_rc = 0
    if int( rc ) != 0 :
        main_rc = 1
        
    return( main_rc )

#-----------------------------------------------------------------------------#
# Check and invoke main
if __name__ == "__main__":
   rc = main(sys.argv[1:])

sys.exit( rc )

