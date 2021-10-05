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

# MessageSight command line interface to list object

# Import the Message Sight Python Library
import os
path = os.path.dirname(os.path.realpath(__file__))

execfile(path + '/messageSightPythonLib.py')

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------

# Main module to process arg and send POST
def main(argv):
    adminServer = ''
    uriPath = 'staging.internetofthings.ibmcloud.com/api/v0001/organizations/dxdwqs/devices'
    #  -u a-dxdwqs-6xzmkugb93 -p wSON-Grf16*kNSasC5
    #  uriPath = 'dzh0qk.internetofthings.ibmcloud.com/api/v0001/organizations/dzh0qk/devices'
    #  -u a-dzh0qk-jiemhesv3x -p  U!qCKsc(vJn(rEF(R2
    
    # Parse arguments
    # parser = argparse.ArgumentParser(description='List all Configurations of  ObjectType')
    # parser.add_argument('-s', metavar='adminServerIP:port', required=True, type=str, help='adminServerIP:port, ex: 198.23.126.136:9089')
    # parser.add_argument('object', metavar='ObjectType', type=str, help='Configuration object type,', choices=['CertificateProfile', 'LTPAProfile', 'OauthProfile', 'SecurityProfile', 'TrustedCertificate', 'ConnectioPolicy', 'Endpoint', 'MessageHub', 'MessagingPolicy', 'Queue', 'DestinationMappingRule', 'QueueManagerConnection', 'TopicMonitor', 'File' ])

    # args = parser.parse_args()

    # adminServer = args.s
    # objectType = args.object

    payload_str='{}'

    # Set headers
    headers = {"Content-type": "application/x-www-form-urlencoded"}

    # Set payload
    trace( 'CMD: '+ payload_str )
    payload = json.loads(payload_str)

    # send POST request
    url = 'https://' + adminServer+uriPath
    trace( 'URL:  \n'+ url )
    try:
#        response = requests.get(url=url,data=json.dumps(payload), headers=headers)
#        response = requests.get(url=url,data="",auth=('a-dxdwqs-6xzmkugb93' , 'wSON-Grf16*kNSasC5'), headers=headers, verify=True)
        response = requests.get(url=url,data="",auth=('a-dzh0qk-jiemhesv3x' , 'U!qCKsc(vJn(rEF(R2'), headers=headers, verify=True)
    except:
        print('No response from server : "'+ adminServer +'".  CHECK the IP and PORT.' )
        sys.exit(1)
    
    # Process the response and print return information
    rc = parseResponse( 'List', 'iotf', 'iotf', response )

    return( int(rc) )


# Check and invoke main
if __name__ == "__main__":
   rc = main(sys.argv[1:])
sys.exit( rc )
