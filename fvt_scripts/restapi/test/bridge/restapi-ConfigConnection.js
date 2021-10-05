/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

var assert = require('assert')
var request = require('supertest')
var should = require('should')

var FVT = require('../restapi-CommonFunctions.js')
FVT.MS_RESTAPI_TYPE=false  // Not MessageSight RESTAPI, but a Bridge RESTAPI -- refer to Defect 217232 - Connection object parsing for validate FVT.getSuccessCallback

var verifyConfig = {};
var verifyMessage = {};  // {"status":#,"Code":"CWLNAxxx","Message":"blah blah"}

var uriObject = 'config/Connection/';

// assuming Bridge Config Bridge.A1A2.cfg was loaded 
var expectDefault = '{"WIoTP": { "MQTTServerList": [   "'+ FVT.A2_HOST +':1883" ], "ClientID": "A:x:myBrApp:SimpleA1A2-", "Version": "5.0"   },   "MqttServer": { "MQTTServerList": [   "'+ FVT.A1_HOST +':1883" ], "ClientID": "BR.SimpleA1A2-", "Version": "5.0"   } } '

var BridgeSourceName='MqttServer';
var expectSource = '{ "MQTTServerList": [   "'+ FVT.A1_HOST +':1883" ], "ClientID": "BR.SimpleA1A2-", "Version": "5.0" }';

var BridgeDestinationName='WIoTP';
var expectDestination = '{ "MQTTServerList": [   "'+ FVT.A2_HOST +':1883" ], "ClientID": "A:x:myBrApp:SimpleA1A2-", "Version": "5.0" }';

var TempSource           = '{ "MQTTServerList" : ["'+ FVT.A1_HOST +':16102"], "ClientID" : "BridgeRestAPI_CID",  "Version" : "5.0" }'; 
var TempSourceConnection = '{"TempSource": '+ TempSource +' }'; 
var TempSourceConnectionObject = '{"Connection": '+ TempSourceConnection +'}';

var TempDestination           = '{ "MQTTServerList" : ["'+ FVT.A2_HOST +':16102"], "ClientID" : "A:x:BrMS2:SimpleA1A2-",  "Version" : "5.0" }'; 
var TempDestinationConnection = '{"TempDestination": '+ TempDestination +' }'; 
var TempDestinationConnectionObject = '{"Connection": '+ TempDestinationConnection +'}';

var KafkaDestinationProperties = '{ "EventStreamsBrokerList" : ["'+ FVT.A2_HOST +':16102"], "ClientID" : "A:x:BrMS2:SimpleA1A2-",  "Version" : "5.0" }'; 
var KafkaDestinationConnection = '{"KafkaDestination": '+ KafkaDestinationProperties +' }'; 
var KafkaDestinationConnectionObject = '{"Connection": '+ KafkaDestinationConnection +'}';

var expectAllDefaultConfig = '{ "Connection": { "WIoTP": { "MQTTServerList": [ "'+ FVT.A2_HOST +':1883" ], "ClientID": "A:x:myBrApp:SimpleA1A2-", "Version": "5.0" }, \
                                         "MqttServer": { "MQTTServerList": [ "'+ FVT.A1_HOST +':1883" ], "ClientID": "BR.SimpleA1A2-", "Version": "5.0" } }, \
                                         "Forwarder": { "m2w": { "Topic": ["wiotp/+/+/+/+"], "Enabled": true, "Source": "MqttServer", "Destination": "WIoTP", "TopicMap": "iot-2/${Topic1*}" } }, \
                                         "Endpoint": { "admin": { "Port": 9961, "Interface": "*", "Enabled": true, "Secure": false, "Protocol": "Admin", "UseClientCipher": false } }, \
                                         "User": { "admin": { "Password": "=AJFIt0p+ZYW1tPpDnh9EVOMsVpxxv9PuWdTfM+ULU3A=" } } }' ;

var expectAllUpdatedConfig = '{ "Connection": { "WIoTP": { "MQTTServerList": [ "'+ FVT.A2_HOST +':1883" ], "ClientID": "A:x:myBrApp:SimpleA1A2-", "Version": "5.0" }, \
                                         "MqttServer": { "MQTTServerList": [ "'+ FVT.A1_HOST +':1883" ], "ClientID": "BR.SimpleA1A2-", "Version": "5.0" }, \
                                         "TempSource": { "MQTTServerList" : ["'+ FVT.A1_HOST +':16102"], "ClientID" : "BridgeRestAPI_CID",  "Version" : "5.0" } , \
                                         "TempDestination": { "MQTTServerList" : ["'+ FVT.A2_HOST +':16102"], "ClientID" : "A:x:BrMS2:SimpleA1A2-",  "Version" : "5.0" } }, \
                                         "Forwarder": { "m2w": { "Topic": ["wiotp/+/+/+/+"], "Enabled": true, "Source": "MqttServer", "Destination": "WIoTP", "TopicMap": "iot-2/${Topic1*}" } }, \
                                         "Endpoint": { "admin": { "Port": 9961, "Interface": "*", "Enabled": true, "Secure": false, "Protocol": "Admin", "UseClientCipher": false } }, \
                                         "User": { "admin": { "Password": "=AJFIt0p+ZYW1tPpDnh9EVOMsVpxxv9PuWdTfM+ULU3A=" } } }' ;

var BridgeURI = FVT.P1_HOST +":9961"
var BridgeAuth = FVT.P1_REST_USER +','+ FVT.P1_REST_PW
//var BridgeAuthUser = "'"+ FVT.P1_REST_USER + "'"
//var BridgeAuthPSWD = "'"+ FVT.P1_REST_PW + "'"
var P16_List = '[ "'+ FVT.A1_HOSTNAME_OS +':1881" , "'+ FVT.A1_HOST +':1882" , "'+ FVT.A1_HOST +':1883" , "'+ FVT.A1_HOST +':1884" , "'+ FVT.A1_HOST +':1885" , "'+ FVT.A1_HOST +':1886" , "'+ FVT.A1_HOST +':1887" , "'+ FVT.A1_HOST +':1888" , "'+ FVT.A1_HOST +':1889" , "'+ FVT.A1_HOST +':1890" , "'+ FVT.A1_HOST +':1891" , "'+ FVT.A1_HOST +':1892" , "'+ FVT.A1_HOST +':1893" , "'+ FVT.A1_HOST +':1894" , "'+ FVT.A1_HOST +':1895" , "'+ FVT.A1_HOST +':1896" ] ';
var P16_Target = '{ "MQTTServerList" : '+ P16_List +', "ClientID": "A:orgDestIoT2:P2Br:MultiFwdV3S-", "Version": "5.0", "Username": "A:orgDestIoT2:P2Br:MultiFwdV3S-", "Password": "!JjaKkTrAz1/+b19URCFF+ic="}';
var P16_TargetConnection = '{ "P16_Target": '+ P16_Target +'}';
var P16_TargetObject = '{"Connection": '+ P16_TargetConnection +'}';

var KafkaDest16Properties = '{ "EventStreamsBrokerList" : '+ P16_List +', "ClientID": "A:orgDestIoT2:P2Br:MultiFwdV3S-", "Version": "5.0", "Username": "A:orgDestIoT2:P2Br:MultiFwdV3S-", "Password": "!JjaKkTrAz1/+b19URCFF+ic="}';
var KafkaDest16Connection = '{ "KafkaDestination": '+ KafkaDest16Properties +'}';
var KafkaDest16ConnectionObject = '{"Connection": '+ KafkaDest16Connection +'}';

var P17_List = '[ "'+ FVT.A1_HOST +':1880" , "'+ FVT.A1_HOST +':1881" , "'+ FVT.A1_HOST +':1882" , "'+ FVT.A1_HOST +':1883" , "'+ FVT.A1_HOST +':1884" , "'+ FVT.A1_HOST +':1885" , "'+ FVT.A1_HOST +':1886" , "'+ FVT.A1_HOST +':1887" , "'+ FVT.A1_HOST +':1888" , "'+ FVT.A1_HOST +':1889" , "'+ FVT.A1_HOST +':1890" , "'+ FVT.A1_HOST +':1891" , "'+ FVT.A1_HOST +':1892" , "'+ FVT.A1_HOST +':1893" , "'+ FVT.A1_HOST +':1894" , "'+ FVT.A1_HOST +':1895" , "'+ FVT.A1_HOST +':1896" ]';
var P17_TargetConnection = '{"Connection":{ "P17_Target": {"MQTTServerList": '+ P17_List +', "ClientID": "A:orgDestIoT2:P2Br:MultiFwdV3S-", "Version": "5.0", "Username": "A:orgDestIoT2:P2Br:MultiFwdV3S-", "Password": "!JjaKkTrAz1/+b19URCFF+ic="}}}';
var P17_TargetObject = '{"Connection": '+ P17_TargetConnection +'}';

var KafkaDest17Properties = '{"EventStreamsBrokerList": '+ P17_List +', "ClientID": "A:orgDestIoT2:P2Br:MultiFwdV3S-", "Version": "5.0", "Username": "A:orgDestIoT2:P2Br:MultiFwdV3S-", "Password": "!JjaKkTrAz1/+b19URCFF+ic="}';
var KafkaDest17Connection = '{ "KafkaDest17": '+ KafkaDest17Properties +'}';
var KafkaDest17ConnectionObject = '{"Connection": '+ KafkaDest17Connection +'}';

// P16_Target properties
var P16_TargetCipherTrue = '{ "P16_Target": { "Ciphers":true}}';  // invalid value
var P16_TargetCipher1  = '{ "P16_Target": { "Ciphers":1}}';   // invalid value
var P16_TargetCipher1_1 = '{ "P16_Target": { "Ciphers":"TLSv1.1"}}';  // supported
var P16_TargetCipher1_2 = '{ "P16_Target": { "Ciphers":"TLSv1.2"}}';  // supported
var P16_TargetCipher1_3 = '{ "P16_Target": { "Ciphers":"TLSv1.3"}}';  // This is now valid!
// Bridge actually does NO Validation of Ciphers other than data type of STRING
// ---  ISN'T THIS Suppose to be Best, Medium, Fast, In MS endpoint/security Ciphers should allow only Fast, Best, Medium, and any string starting with a colon
// https://www.ibm.com/support/knowledgecenter/en/SSWMAJ_2.0.0/com.ibm.ism.doc/Administering/ad00743_.html

//  ====================  Test Cases Start Here  ====================  //

describe('ConfigConnection:', function() {
this.timeout( FVT.defaultTimeout );

    // Accept Bridge License
    describe('AcceptBridgeLicense:', function() {
        it('should return status 200 on /admin/accept/license', function(done) {
            var payload = null;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "accept/license/Production", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
	    });
        it('should return status 200 on GET, LicensedUsage', function(done) {
            verifyConfig = JSON.parse('{"LicensedUsage":"Production"}' );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"set", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

	});

    // Get test cases
    describe('Pristine Get:', function() {
        it('should return status 200 on GET, DEFAULT of "Connection":{UpperCased}', function(done) {
            verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject, BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on GET, DEFAULT of "connection":{lowercased}', function(done) {
            verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config/connection", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on GET, "connection/*"', function(done) {
            verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config/connection/*", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on GET the Entire CONFIG', function(done) {
            verifyConfig = JSON.parse(expectAllDefaultConfig);
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

//  ====================   Get Specific Connection  ====================  //
    describe('Get Connection', function() {

        it('should return status 200 on GET, "Connection/Source"', function(done) {
            verifyConfig = JSON.parse(expectSource);
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+BridgeSourceName, BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return stat us 200 on GET, "connection/Destination"', function(done) {
            verifyConfig = JSON.parse(expectDestination);
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config/connection/"+BridgeDestinationName, BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

//  ====================   Create Connection  ====================  //
    describe('Create Connection', function() {

        it('should return status 200 when POST to ADD a Connection object TempSource', function(done) {
            var payload = TempSourceConnectionObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET roundtrip TempSource', function(done) {
            var payload = TempSource ;
            verifyConfig = JSON.parse( payload );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"TempSource", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET All Connections', function(done) {
            var payload = TempSourceConnection ;
            verifyConfig = JSON.parse( payload );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject, BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST to ADD a Connection object TempDestination', function(done) {
            var payload = TempDestinationConnectionObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET roundtrip TempDestination', function(done) {
            var payload = TempDestination ;
            verifyConfig = JSON.parse( payload );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"TempDestination", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET All Updated Config', function(done) {
            var payload = expectAllUpdatedConfig ;
            verifyConfig = JSON.parse( payload );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST to ADD a Connection object KafkaDestination', function(done) {
            var payload = KafkaDestinationConnectionObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET roundtrip KafkaDestination', function(done) {
            var payload = KafkaDestinationProperties ;
            verifyConfig = JSON.parse( payload );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"KafkaDestination", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET All Updated Config', function(done) {
            var payload = KafkaDestinationConnectionObject ;
            verifyConfig = JSON.parse( payload );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });        

    });

//  ====================  MQTTServerList  ====================  //
// TypeOf:  String OR Array of Strings [MAX 16]
     describe('MQTTServerList:', function() {
        it('should return status 200 when POST with Array of 16 IPs and Names ', function(done) {
            var payload = P16_TargetObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with 16 MQTTTServerList elements', function(done) {
            verifyConfig = JSON.parse( P16_TargetConnection ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST and OVERWRITE Array of 16 IPs and Names ', function(done) {
            var payload = '{ "Connection": { "P16_Target": { "MQTTServerList": [ "'+ FVT.A1_HOST +':1883" ] }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection and OVERWRITE 16 MQTTTServerList elements', function(done) {
            verifyConfig = JSON.parse( '{ "P16_Target": { "MQTTServerList": [ "'+ FVT.A1_HOST +':1883" ]}}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Defect 217601 Crashes Admin Endpoint
       it('should return status 400 when POST with Array of 17 ', function(done) {
           var payload = P17_TargetConnection;
           verifyConfig = JSON.parse( payload );
              verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: MQTTServerList Value: \"array\"." };
           FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
       });
        it('should return status 404 on GET, Connection with 17 MQTTTServerList elements', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/P17_Target"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject + "P17_Target", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });
        
//Defect 217669  - Are we allowing STRING and Array of (16)Strings?
        it('should return status 200 when POST without Array  ', function(done) {
            var payload = '{ "Connection": { "NoArray" :{ "MQTTServerList": "'+ FVT.A1_HOST +':1883" }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = JSON.parse( '{ "Code":"CWLNA0112", "Message": "The property value is not valid: Property: MQTTServerList Value: \\\"'+ FVT.A1_HOST +':1883\\\"." }' );
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
       });
        it('should return status 200 on GET, Connection without MQTTServerList Array', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/NoArray"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject + "NoArray", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });
    }); 

//  ====================  EventStreamsBrokerList  ====================  //
// TypeOf:  String OR Array of Strings [MAX 16]
     describe('EventStreamsBrokerList:', function() {
        it('should return status 200 when POST with Array of 16 IPs and Names ', function(done) {
            var payload = KafkaDest16ConnectionObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with 16 EventStreamsBrokerList elements', function(done) {
            verifyConfig = JSON.parse( KafkaDest16Connection ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST and OVERWRITE Array of 16 IPs and Names ', function(done) {
            var payload = '{ "Connection": { "KafkaDestination": { "EventStreamsBrokerList": [ "'+ FVT.A2_HOST +':1883" ] }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection and OVERWRITE 16 EventStreamsBrokerList elements', function(done) {
            verifyConfig = JSON.parse( '{ "KafkaDestination": { "EventStreamsBrokerList": [ "'+ FVT.A2_HOST +':1883" ]}}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Errors

       it('should return status 400 when POST with Array of 17 ', function(done) {
           var payload = KafkaDest17ConnectionObject;
           verifyConfig = JSON.parse( payload );
              verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: EventStreamsBrokerList Value: \"array\"." };
           FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
       });
        it('should return status 404 on GET, Connection with 17 EventStreamsBrokerList elements', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/KafkaDest17"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject + "KafkaDest17", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });
        
//Defect 217669  
        it('should return status 400 when POST without Array  ', function(done) {
            var payload = '{ "Connection": { "NoArray" :{ "EventStreamsBrokerList": "'+ FVT.A2_HOST +':1883" }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = JSON.parse( '{ "Code":"CWLNA0112", "Message": "The property value is not valid: Property: EventStreamsBrokerList Value: \\\"'+ FVT.A2_HOST +':1883\\\"." }' );
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
       });
        it('should return status 200 on GET, Connection without EventStreamsBrokerList Array', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/NoArray"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject + "NoArray", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });
		
    }); 

//  ====================  MaxBatchTimeMS  ====================  //
//  MQTT V5 Spec says 0 is invalid, range is from 0x01 - 0xffff (65535), Bridge:  default=0 (use system default time), like NULL otherwise no value limits
     describe('MaxBatchTimeMS:', function() {

        var KafkaDestMaxPacketSize_MIN = '{ "KafkaDestination": { "MaxBatchTimeMS": 1 }}';
        var KafkaDestMaxPacketSizeObject1 = '{"Connection": '+ KafkaDestMaxPacketSize_MIN +'}';

        var KafkaDestMaxPacketSize_MAX = '{ "KafkaDestination": { "MaxBatchTimeMS": 65535 }}';
        var KafkaDestMaxPacketSizeObject2 = '{"Connection": '+ KafkaDestMaxPacketSize_MAX +'}';

        // NOT Valid 
        var KafkaDestMaxPacketSize_ZERO = '{ "KafkaDestination": { "MaxBatchTimeMS": 0 }}';
        var KafkaDestMaxPacketSize_ZERO_nulls = '{ "KafkaDestination": { "MaxBatchTimeMS": null }}';
        var KafkaDestMaxPacketSizeObjectBad1 = '{"Connection": '+ KafkaDestMaxPacketSize_ZERO +'}';
        var KafkaDestMaxPacketSize_Boolean = '{ "KafkaDestination": { "MaxBatchTimeMS": false }}';
        var KafkaDestMaxPacketSizeObjectBad2 = '{"Connection": '+ KafkaDestMaxPacketSize_Boolean +'}';
        var KafkaDestMaxPacketSize_String = '{ "KafkaDestination": { "MaxBatchTimeMS": "A Really Big Number" }}';
        var KafkaDestMaxPacketSizeObjectBad3 = '{"Connection": '+ KafkaDestMaxPacketSize_String +'}';
        var KafkaDestMaxPacketSize_BeyondMAX = '{ "KafkaDestination": { "MaxBatchTimeMS": 65536 }}';
        var KafkaDestMaxPacketSizeObjectBad4 = '{"Connection": '+ KafkaDestMaxPacketSize_BeyondMAX +'}';
        var KafkaDestMaxPacketSize_Negative = '{ "KafkaDestination": { "MaxBatchTimeMS": -69 }}';
        var KafkaDestMaxPacketSizeObjectBad5 = '{"Connection": '+ KafkaDestMaxPacketSize_Negative +'}';


        it('should return status 200 when POST with MaxBatchTimeMS:1 ', function(done) {
            var payload = KafkaDestMaxPacketSizeObject1;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with MaxBatchTimeMS:1', function(done) {
            verifyConfig = JSON.parse( KafkaDestMaxPacketSize_MIN ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Defect 217748 SHOULD ZERO BE ALLOWED?  MQTTv5 says INVALID - Bridge will remove the property if ZERO
        it('should return status 200 when POST with MaxBatchTimeMS:0 ', function(done) {
            var payload = KafkaDestMaxPacketSizeObjectBad1;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0200", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with MaxBatchTimeMS:0', function(done) {
            verifyConfig = JSON.parse( KafkaDestMaxPacketSize_ZERO_nulls  ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 200 when POST with MaxBatchTimeMS:MAX ', function(done) {
            var payload = KafkaDestMaxPacketSizeObject2;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with MaxBatchTimeMS:MAX', function(done) {
            verifyConfig = JSON.parse( KafkaDestMaxPacketSize_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Errors
        it('should return status 400 when POST with MaxBatchTimeMS:NEGATIVE ', function(done) {
            var payload = KafkaDestMaxPacketSizeObjectBad5;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: MaxBatchTimeMS Value: \"-69\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with MaxBatchTimeMS:NEGATIVE', function(done) {
            verifyConfig = JSON.parse( KafkaDestMaxPacketSize_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 when POST with MaxBatchTimeMS:Boolean ', function(done) {
            var payload = KafkaDestMaxPacketSizeObjectBad2;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: MaxBatchTimeMS Value: \"false\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with MaxBatchTimeMS:Boolean', function(done) {
            verifyConfig = JSON.parse( KafkaDestMaxPacketSize_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 when POST with MaxBatchTimeMS:String ', function(done) {
            var payload = KafkaDestMaxPacketSizeObjectBad3;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: MaxBatchTimeMS Value: \"A Really Big Number\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with MaxBatchTimeMS:String', function(done) {
            verifyConfig = JSON.parse( KafkaDestMaxPacketSize_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 200 when POST with MaxBatchTimeMS:BeyondMAX ', function(done) {
            var payload = KafkaDestMaxPacketSizeObjectBad4;
            verifyConfig = JSON.parse( payload );
//            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: MaxBatchTimeMS Value: \"65536\"." };
//            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with MaxBatchTimeMS:BeyondMAX', function(done) {
            verifyConfig = JSON.parse( KafkaDestMaxPacketSize_BeyondMAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        

// Default:
        it('should return status 200 when POST with MaxBatchTimeMS:null', function(done) {
            var payload = '{ "Connection": { "KafkaDestination": { "MaxBatchTimeMS":null}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with MaxBatchTimeMS:null', function(done) {
            verifyConfig = JSON.parse( KafkaDestMaxPacketSize_ZERO_nulls ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    }); 

//  ====================  Ciphers  ====================  //
//  TypeOf String: Valid Values: are TLSv1.1 or TLSv1.2 - defaults to use cipher selected based on version of TLS
// 01-14-2019  TLSv1.3 is valid now
// Bridge actually does NO Validation of Ciphers other than data type  ---  ISN'T THIS Suppose to be Best, Medium, Fast, In endpoint Ciphers should allow only Fast, Best, Medium, and any string starting with a colon
    describe('Ciphers:', function() {

        it('should return status 200 when POST with Cipher:TLSv1.1 ', function(done) {
            var payload = '{"Connection": '+ P16_TargetCipher1_1 +'}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET Cipher:TLSv1.1', function(done) {
            verifyConfig = JSON.parse( P16_TargetCipher1_1 ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST with Cipher:TLSv1.2 ', function(done) {
            var payload = '{"Connection": '+ P16_TargetCipher1_2 +'}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET Cipher:TLSv1.2', function(done) {
            verifyConfig = JSON.parse( P16_TargetCipher1_2 ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

       it('should return status 200 when POST with Cipher:TLSv1.3', function(done) {
            var payload = '{"Connection": '+ P16_TargetCipher1_3 +'}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
//  TLSv1.3 accepted -- NOW VALID
        it('should return status 200 on GET with Cipher:TLSv1.3', function(done) {
            verifyConfig = JSON.parse( P16_TargetCipher1_3 ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });


// Errors:
// Defect 217668
       it('should return status 400 when POST with Cipher:true', function(done) {
            var payload = '{"Connection": '+ P16_TargetCipherTrue +'}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112", "Message": "The property value is not valid: Property: Ciphers Value: \"true\"." } ;
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET without change to Cipher:true', function(done) {
            verifyConfig = JSON.parse( P16_TargetCipher1_3 ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Defect 217668
       it('should return status 400 when POST with Cipher:1', function(done) {
            var payload = '{"Connection": '+ P16_TargetCipher1 +'}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112", "Message": "The property value is not valid: Property: Ciphers Value: \"1\"." } ;
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET without change to Cipher:1', function(done) {
            verifyConfig = JSON.parse( P16_TargetCipher1_3 ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });


       // it('should return status 200 when POST with Cipher:TLSv1.4', function(done) {
            // var payload = '{"Connection": '+ P16_TargetCipher1_4 +'}';
            // verifyConfig = JSON.parse( payload );
            // verifyMessage = {"status":400,"Code":"CWLNA0112", "Message": "The property value is not valid: Property: Ciphers Value: \"TLSv1.4\"." } ;
            // FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        // });
//DEFECT 221781 - tlsv1.3 accepted -- NOW VALID, change to tlsv1.4
        // it('should return status 200 on GET after Cipher:TLSv1.4 fails', function(done) {
            // verifyConfig = JSON.parse( P16_TargetCipher1_3 ) ;
            // FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        // });

// Default:
        it('should return status 200 when POST (to remove) Cipher:null', function(done) {
            var payload = '{"Connection": { "P16_Target": {"Ciphers": null }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET t(to remove) Cipher:null', function(done) {
            verifyConfig = JSON.parse( '{ "Ciphers": null }' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"P16_Target" , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    }); 

//  ====================  ClientID  ====================  //
//  TypeOf String:  Bridge DOES NOT enforce any length or char checks, but will append the ForwarderName and Instance Number to the end of this name
     describe('ClientID:', function() {
        var CID36char = 'c36-appId.typeId_deviceType.deviceId';
        var CID36Connection = '{ "CID36_Target": {"MQTTServerList": [ "'+ FVT.A1_HOST +':1883" ], "ClientID": "A:orgDestIoT2:'+ CID36char +':'+ CID36char +'", "Version": "5.0", "Username": "A:orgDestIoT2:'+ CID36char +':'+ CID36char +'", "Password": "!JjaKkTrAz1/+b19URCFF+ic="}}';
        var CID36ConnectionObject = '{"Connection": '+ CID36Connection +'}';

        var CID_NLSCharConnection = '{ "CID_NLSChar_Target": {"MQTTServerList": [ "'+ FVT.A1_HOST +':1883" ], "ClientID": "g:orgDestIoT2:'+ FVT.long16Name +':'+ CID36char +'", "Version": "5.0", "Username": "g:orgDestIoT2:'+ FVT.long16Name +':'+ CID36char +'", "Password": "!JjaKkTrAz1/+b19URCFF+ic="}}';
        var CID_NLSCharConnectionObject = '{"Connection": '+ CID_NLSCharConnection +'}';

        it('should return status 200 when POST with CID sectors 36 chars ', function(done) {
            var payload = CID36ConnectionObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with 36 char CID', function(done) {
            verifyConfig = JSON.parse( CID36Connection ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Defect 217606 (Crash)
        it('should return status 200 when POST with CID with NLS Chars ', function(done) {
            var payload = CID_NLSCharConnectionObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with CID with NLS Chars', function(done) {
            verifyConfig = JSON.parse( CID_NLSCharConnection ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Default: really doesn't have one, but can create without it...
// Defect 217737
        it('should return status 200 when POST with ClientID:null', function(done) {
            var payload = '{ "Connection": { "P16_Target": { "ClientID":null}}}';
            verifyConfig = JSON.parse( payload );
//??            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
//            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: ClientID Value: \"null\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with ClientID:null', function(done) {
            verifyConfig = JSON.parse( CID_NLSCharConnection ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        
    }); 

//  ====================  MaxPacketSize  ====================  //
//  MQTT V5 Spec says 0 is invalid, range is from 0x01 - 0xffff (65535), Bridge:  0 removes, like NULL otherwise no value limits
     describe('MaxPacketSize:', function() {

        var P16_TargetMaxPacketSize_MIN = '{ "P16_Target": { "MaxPacketSize": 1 }}';
        var P16_TargetMaxPacketSizeObject1 = '{"Connection": '+ P16_TargetMaxPacketSize_MIN +'}';

        var P16_TargetMaxPacketSize_MAX = '{ "P16_Target": { "MaxPacketSize": 65535 }}';
        var P16_TargetMaxPacketSizeObject2 = '{"Connection": '+ P16_TargetMaxPacketSize_MAX +'}';

        // NOT Valid 
        var P16_TargetMaxPacketSize_ZERO = '{ "P16_Target": { "MaxPacketSize": 0 }}';
        var P16_TargetMaxPacketSize_ZERO_nulls = '{ "P16_Target": { "MaxPacketSize": null }}';
        var P16_TargetMaxPacketSizeObjectBad1 = '{"Connection": '+ P16_TargetMaxPacketSize_ZERO +'}';
        var P16_TargetMaxPacketSize_Boolean = '{ "P16_Target": { "MaxPacketSize": false }}';
        var P16_TargetMaxPacketSizeObjectBad2 = '{"Connection": '+ P16_TargetMaxPacketSize_Boolean +'}';
        var P16_TargetMaxPacketSize_String = '{ "P16_Target": { "MaxPacketSize": "A Really Big Number" }}';
        var P16_TargetMaxPacketSizeObjectBad3 = '{"Connection": '+ P16_TargetMaxPacketSize_String +'}';
        var P16_TargetMaxPacketSize_BeyondMAX = '{ "P16_Target": { "MaxPacketSize": 65536 }}';
        var P16_TargetMaxPacketSizeObjectBad4 = '{"Connection": '+ P16_TargetMaxPacketSize_BeyondMAX +'}';
        var P16_TargetMaxPacketSize_Negative = '{ "P16_Target": { "MaxPacketSize": -69 }}';
        var P16_TargetMaxPacketSizeObjectBad5 = '{"Connection": '+ P16_TargetMaxPacketSize_Negative +'}';

// Defect 217743 - NONE are SET by POST!
        it('should return status 200 when POST with MaxPacketSize:1 ', function(done) {
            var payload = P16_TargetMaxPacketSizeObject1;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with MaxPacketSize:1', function(done) {
            verifyConfig = JSON.parse( P16_TargetMaxPacketSize_MIN ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Defect 217748 SHOULD ZERO BE ALLOWED?  MQTTv5 says INVALID - Bridge will remove the property if ZERO
        it('should return status 200 when POST with MaxPacketSize:0 ', function(done) {
            var payload = P16_TargetMaxPacketSizeObjectBad1;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0200", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with MaxPacketSize:0', function(done) {
            verifyConfig = JSON.parse( P16_TargetMaxPacketSize_ZERO_nulls  ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 200 when POST with MaxPacketSize:MAX ', function(done) {
            var payload = P16_TargetMaxPacketSizeObject2;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with MaxPacketSize:MAX', function(done) {
            verifyConfig = JSON.parse( P16_TargetMaxPacketSize_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Errors
        it('should return status 400 when POST with MaxPacketSize:NEGATIVE ', function(done) {
            var payload = P16_TargetMaxPacketSizeObjectBad5;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: MaxPacketSize Value: \"-69\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with MaxPacketSize:NEGATIVE', function(done) {
            verifyConfig = JSON.parse( P16_TargetMaxPacketSize_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 when POST with MaxPacketSize:Boolean ', function(done) {
            var payload = P16_TargetMaxPacketSizeObjectBad2;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: MaxPacketSize Value: \"false\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with MaxPacketSize:Boolean', function(done) {
            verifyConfig = JSON.parse( P16_TargetMaxPacketSize_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 when POST with MaxPacketSize:String ', function(done) {
            var payload = P16_TargetMaxPacketSizeObjectBad3;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: MaxPacketSize Value: \"A Really Big Number\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with MaxPacketSize:String', function(done) {
            verifyConfig = JSON.parse( P16_TargetMaxPacketSize_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 200 when POST with MaxPacketSize:BeyondMAX ', function(done) {
            var payload = P16_TargetMaxPacketSizeObjectBad4;
            verifyConfig = JSON.parse( payload );
//            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: MaxPacketSize Value: \"65536\"." };
//            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with MaxPacketSize:BeyondMAX', function(done) {
            verifyConfig = JSON.parse( P16_TargetMaxPacketSize_BeyondMAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        

// Default:
//Defect 217737, 217749, 217824
        it('should return status 200 when POST with MaxPacketSize:null', function(done) {
            var payload = '{ "Connection": { "P16_Target": { "MaxPacketSize":null}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with MaxPacketSize:null', function(done) {
            verifyConfig = JSON.parse( '{ "P16_Target": { "MaxPacketSize":null}}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    }); 

//  ====================  ServerName  ====================  //
//  Default is the MQTTServerList connection's entry, unless it is an IP Address
     describe('ServerName:', function() {
        var P16_TargetServerName_HostNameOS = '{ "P16_Target": { "ServerName":"'+ FVT.A1_HOSTNAME_OS +'" }}';
        var P16_TargetServerNameObject1 = '{"Connection": '+ P16_TargetServerName_HostNameOS +'}';

        var P16_TargetServerName_ServerName = '{ "P16_Target": { "ServerName":"'+ FVT.A1_SERVERNAME +'" }}';
        var P16_TargetServerNameObject2 = '{"Connection": '+ P16_TargetServerName_ServerName +'}';
        // NOT Valid 
        var P16_TargetServerName_Boolean = '{ "P16_Target": { "ServerName": false }}';
        var P16_TargetServerNameObject3 = '{"Connection": '+ P16_TargetServerName_Boolean +'}';
        var P16_TargetServerName_Number = '{ "P16_Target": { "ServerName": 69 }}';
        var P16_TargetServerNameObject4 = '{"Connection": '+ P16_TargetServerName_Number +'}';

        it('should return status 200 when POST with ServerName:A1_HOSTNAME_OS', function(done) {
            var payload = P16_TargetServerNameObject1;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with ServerName:A1_HOSTNAME_OS', function(done) {
            verifyConfig = JSON.parse( P16_TargetServerName_HostNameOS ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST with ServerName:A1_SERVERNAME', function(done) {
            var payload = P16_TargetServerNameObject2;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with ServerName:A1_SERVERNAME', function(done) {
            verifyConfig = JSON.parse( P16_TargetServerName_ServerName ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Error:
// Defect 217668
        it('should return status 400 when POST with ServerName:boolean', function(done) {
            var payload = P16_TargetServerNameObject3;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: ServerName Value: \"false\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with ServerName:boolean', function(done) {
            verifyConfig = JSON.parse( P16_TargetServerName_ServerName ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Defect 217668
        it('should return status 400 when POST with ServerName:number', function(done) {
            var payload = P16_TargetServerNameObject4;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: ServerName Value: \"69\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with ServerName:number', function(done) {
            verifyConfig = JSON.parse( P16_TargetServerName_ServerName ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });


// Default:
        it('should return status 200 when POST with ServerName:null', function(done) {
            var payload = '{ "Connection": { "P16_Target": { "ServerName":null}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with ServerName:null', function(done) {
            verifyConfig = JSON.parse( '{ "P16_Target": { "ServerName":null}}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

    }); 

//  ====================  SessionExpiry  ====================  //
// MIN:0, MAX:2147483647  (NOTE MAX in MS is 4294967295), Default=0
     describe('SessionExpiry:', function() {
        var P16_TargetSessionExpiry_ZERO = '{ "P16_Target": { "SessionExpiry": 0 }}';
        var P16_TargetSessionExpiryObject1 = '{"Connection": '+ P16_TargetSessionExpiry_ZERO +'}';

        var P16_TargetSessionExpiry_MIN = '{ "P16_Target": { "SessionExpiry": 1 }}';
        var P16_TargetSessionExpiryObject2 = '{"Connection": '+ P16_TargetSessionExpiry_MIN +'}';

        var P16_TargetSessionExpiry_MAX = '{ "P16_Target": { "SessionExpiry": 2147483647 }}';
        var P16_TargetSessionExpiryObject3 = '{"Connection": '+ P16_TargetSessionExpiry_MAX +'}';

        // NOT Valid 
        var P16_TargetSessionExpiry_Boolean = '{ "P16_Target": { "SessionExpiry": false }}';
        var P16_TargetSessionExpiryObjectBad1 = '{"Connection": '+ P16_TargetSessionExpiry_Boolean +'}';
        var P16_TargetSessionExpiry_String = '{ "P16_Target": { "SessionExpiry": "Twenty Nine" }}';
        var P16_TargetSessionExpiryObjectBad2 = '{"Connection": '+ P16_TargetSessionExpiry_String +'}';
        var P16_TargetSessionExpiry_BeyondMAX = '{ "P16_Target": { "SessionExpiry": 2147483648 }}';
        var P16_TargetSessionExpiryObjectBad3 = '{"Connection": '+ P16_TargetSessionExpiry_BeyondMAX +'}';
// Defect 217735
        it('should return status 200 when POST with SessionExpiry:MIN', function(done) {
            var payload = P16_TargetSessionExpiryObject2;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with SessionExpiry:MIN', function(done) {
            verifyConfig = JSON.parse( P16_TargetSessionExpiry_MIN ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST with SessionExpiry:ZERO', function(done) {
            var payload = P16_TargetSessionExpiryObject1;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with SessionExpiry:ZERO', function(done) {
            verifyConfig = JSON.parse( P16_TargetSessionExpiry_ZERO ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });


        it('should return status 200 when POST with SessionExpiry:MAX', function(done) {
            var payload = P16_TargetSessionExpiryObject3;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with SessionExpiry:MAX', function(done) {
            verifyConfig = JSON.parse( P16_TargetSessionExpiry_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Error:
// Defect 217668
        it('should return status 400 when POST with SessionExpiry:boolean', function(done) {
            var payload = P16_TargetSessionExpiryObjectBad1;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: SessionExpiry Value: \"false\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with SessionExpiry:boolean', function(done) {
            verifyConfig = JSON.parse( P16_TargetSessionExpiry_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Defect 217668
        it('should return status 400 when POST with SessionExpiry:String', function(done) {
            var payload = P16_TargetSessionExpiryObjectBad2;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: SessionExpiry Value: \"Twenty Nine\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with SessionExpiry:string', function(done) {
            verifyConfig = JSON.parse( P16_TargetSessionExpiry_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when POST with SessionExpiry:BeyondMAX', function(done) {
            var payload = P16_TargetSessionExpiryObjectBad3;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: SessionExpiry Value: \"2147483648\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with SessionExpiry:BeyondMAX', function(done) {
            verifyConfig = JSON.parse( P16_TargetSessionExpiry_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });


// Default:
        it('should return status 200 when POST with SessionExpiry:null', function(done) {
            var payload = '{ "Connection": { "P16_Target": { "SessionExpiry":null}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with SessionExpiry:null', function(done) {
            verifyConfig = JSON.parse( P16_TargetSessionExpiry_ZERO ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

    }); 

//  ====================  TLS  ====================  //
//  Type: ENUM : None, TLSv1.1 or TLSv1.2   and now TLSv1.3
     describe('TLS:', function() {

        var P16_TargetTLS_MIN = '{ "P16_Target": { "TLS": "TLSv1.1" }}';
        var P16_TargetTLSObject1 = '{"Connection": '+ P16_TargetTLS_MIN +'}';

        var P16_TargetTLS_MEDIAN = '{ "P16_Target": { "TLS": "TLSv1.2" }}';
        var P16_TargetTLSObject2 = '{"Connection": '+ P16_TargetTLS_MEDIAN +'}';

        var P16_TargetTLS_MAX = '{ "P16_Target": { "TLS": "TLSv1.3" }}';
        var P16_TargetTLSObject3 = '{"Connection": '+ P16_TargetTLS_MAX +'}';

        var P16_TargetTLS_NONE = '{ "P16_Target": { "TLS": "None" }}';
        var P16_TargetTLSObject0 = '{"Connection": '+ P16_TargetTLS_NONE +'}';

        // NOT Valid 
        var P16_TargetTLS_ZERO = '{ "P16_Target": { "TLS": 0 }}';
        var P16_TargetTLSObjectBad1 = '{"Connection": '+ P16_TargetTLS_ZERO +'}';
        var P16_TargetTLS_Boolean = '{ "P16_Target": { "TLS": false }}';
        var P16_TargetTLSObjectBad2 = '{"Connection": '+ P16_TargetTLS_Boolean +'}';
        var P16_TargetTLS_String = '{ "P16_Target": { "TLS": "A Random String, not in ENUM" }}';
        var P16_TargetTLSObjectBad3 = '{"Connection": '+ P16_TargetTLS_String +'}';
        var P16_TargetTLS_BeyondMAX = '{ "P16_Target": { "TLS": "TLSv2.0" }}';
        var P16_TargetTLSObjectBad4 = '{"Connection": '+ P16_TargetTLS_BeyondMAX +'}';
        var P16_TargetTLS_NEGATIVE = '{ "P16_Target": { "TLS": -9 }}';
        var P16_TargetTLSObjectBad5 = '{"Connection": '+ P16_TargetTLS_NEGATIVE +'}';
        var P16_TargetTLS_COLON = '{ "P16_Target": { "TLS": ":uniqueProcessing" }}';
        var P16_TargetTLSObjectBad6 = '{"Connection": '+ P16_TargetTLS_COLON +'}';

        it('should return status 200 when POST with TLS:MIN ', function(done) {
            var payload = P16_TargetTLSObject1;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with TLS:MIN', function(done) {
            verifyConfig = JSON.parse( P16_TargetTLS_MIN ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST with TLS:NONE ', function(done) {
            var payload = P16_TargetTLSObject0;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with TLS:NONE', function(done) {
//            verifyConfig = JSON.parse( P16_TargetTLS_NONE ) ;
            verifyConfig = JSON.parse( '{ "P16_Target": { "TLS": null }}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST with TLS:MEDIAN ', function(done) {
            var payload = P16_TargetTLSObject2;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with TLS:MEDIAN', function(done) {
            verifyConfig = JSON.parse( P16_TargetTLS_MEDIAN ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST with TLS:MAX ', function(done) {
            var payload = P16_TargetTLSObject3;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with TLS:MAX', function(done) {
            verifyConfig = JSON.parse( P16_TargetTLS_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });


// Error
        it('should return status 400 when POST with TLS:COLON ', function(done) {
            var payload = P16_TargetTLSObjectBad6;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: TLS Value: \":uniqueProcessing\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with TLS:COLON', function(done) {
            verifyConfig = JSON.parse( P16_TargetTLS_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when POST with TLS:NEGATIVE ', function(done) {
            var payload = P16_TargetTLSObjectBad5;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: TLS Value: \"-9\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with TLS:NEGATIVE', function(done) {
            verifyConfig = JSON.parse( P16_TargetTLS_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 when POST with TLS:0 ', function(done) {
            var payload = P16_TargetTLSObjectBad1;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: TLS Value: \"0\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with TLS:0', function(done) {
            verifyConfig = JSON.parse( P16_TargetTLS_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 when POST with TLS:Boolean ', function(done) {
            var payload = P16_TargetTLSObjectBad2;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: TLS Value: \"false\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with TLS:Boolean', function(done) {
            verifyConfig = JSON.parse( P16_TargetTLS_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 when POST with TLS:String ', function(done) {
            var payload = P16_TargetTLSObjectBad3;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: TLS Value: \"A Random String, not in ENUM\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with TLS:String', function(done) {
            verifyConfig = JSON.parse( P16_TargetTLS_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 when POST with TLS:BeyondMAX ', function(done) {
            var payload = P16_TargetTLSObjectBad4;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: TLS Value: \"TLSv2.0\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with TLS:BeyondMAX', function(done) {
            verifyConfig = JSON.parse( P16_TargetTLS_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        

// Default:
//Defect 217737 217751
        it('should return status 200 when POST with TLS:null', function(done) {
            var payload = '{ "Connection": { "P16_Target": { "TLS":null}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with TLS:null', function(done) {
            verifyConfig = JSON.parse( '{ "P16_Target": { "TLS":null}}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    }); 

//  ====================  Version  ====================  //
//  TypeOf ENUM: 3.1, 3.1.1, 5.0
     describe('Version:', function() {

        var P16_TargetVersion_MIN = '{ "P16_Target": { "Version": "3.1" }}';
        var P16_TargetVersionObjectMIN = '{"Connection": '+ P16_TargetVersion_MIN +'}';

        var P16_TargetVersion_3_1_1 = '{ "P16_Target": { "Version": "3.1.1" }}';
        var P16_TargetVersionObject3_1_1 = '{"Connection": '+ P16_TargetVersion_3_1_1 +'}';

        var P16_TargetVersion_MAX = '{ "P16_Target": { "Version": "5.0" }}';
        var P16_TargetVersionObjectMAX = '{"Connection": '+ P16_TargetVersion_MAX +'}';

        // NOT Valid 
        var P16_TargetVersion_ZERO = '{ "P16_Target": { "Version": 0 }}';
        var P16_TargetVersionObjectBad1 = '{"Connection": '+ P16_TargetVersion_ZERO +'}';
        var P16_TargetVersion_Boolean = '{ "P16_Target": { "Version": false }}';
        var P16_TargetVersionObjectBad2 = '{"Connection": '+ P16_TargetVersion_Boolean +'}';
        var P16_TargetVersion_String = '{ "P16_Target": { "Version": "A Really Big Number" }}';
        var P16_TargetVersionObjectBad3 = '{"Connection": '+ P16_TargetVersion_String +'}';
        var P16_TargetVersion_BeyondMAX = '{ "P16_Target": { "Version": 65536 }}';
        var P16_TargetVersionObjectBad4 = '{"Connection": '+ P16_TargetVersion_BeyondMAX +'}';
        var P16_TargetVersion_NEGATIVE = '{ "P16_Target": { "Version": -69 }}';
        var P16_TargetVersionObjectBad5 = '{"Connection": '+ P16_TargetVersion_NEGATIVE +'}';

        it('should return status 200 when POST with Version:3.1.1 ', function(done) {
            var payload = P16_TargetVersionObject3_1_1;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with Version:3.1.1', function(done) {
            verifyConfig = JSON.parse( P16_TargetVersion_3_1_1 ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST with Version:MAX ', function(done) {
            var payload = P16_TargetVersionObjectMAX;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with Version:MAX', function(done) {
            verifyConfig = JSON.parse( P16_TargetVersion_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST with Version:MIN ', function(done) {
            var payload = P16_TargetVersionObjectMIN;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with Version:MIN', function(done) {
            verifyConfig = JSON.parse( P16_TargetVersion_MIN ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Error
        it('should return status 400 when POST with Version:0 ', function(done) {
            var payload = P16_TargetVersionObjectBad1;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Version Value: \"0\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with Version:0', function(done) {
            verifyConfig = JSON.parse( P16_TargetVersion_MIN ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 when POST with Version:Boolean ', function(done) {
            var payload = P16_TargetVersionObjectBad2;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Version Value: \"false\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with Version:Boolean', function(done) {
            verifyConfig = JSON.parse( P16_TargetVersion_MIN ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 when POST with Version:String ', function(done) {
            var payload = P16_TargetVersionObjectBad3;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Version Value: \"A Really Big Number\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with Version:String', function(done) {
            verifyConfig = JSON.parse( P16_TargetVersion_MIN ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 when POST with Version:BeyondMAX ', function(done) {
            var payload = P16_TargetVersionObjectBad4;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Version Value: \"65536\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with Version:BeyondMAX', function(done) {
            verifyConfig = JSON.parse( P16_TargetVersion_MIN ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        

// Default:
//Defect 217737
        it('should return status 200 when POST with Version:null', function(done) {
            var payload = '{ "Connection": { "P16_Target": { "Version":null}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with Version:null', function(done) {
            verifyConfig = JSON.parse( P16_TargetVersion_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    }); 

//  ====================  Username  ====================  //
//  TypeOf String: no limits
     describe('Username:', function() {

        var P16_TargetUsername_NLS = '{ "P16_Target": { "Username": "'+ FVT.long16Name +'" }}';
        var P16_TargetUsernameObject1 = '{"Connection": '+ P16_TargetUsername_NLS +'}';

        // NOT Valid 
        var P16_TargetUsername_ZERO = '{ "P16_Target": { "Username": 0 }}';
        var P16_TargetUsernameObjectBad1 = '{"Connection": '+ P16_TargetUsername_ZERO +'}';
        var P16_TargetUsername_Boolean = '{ "P16_Target": { "Username": false }}';
        var P16_TargetUsernameObjectBad2 = '{"Connection": '+ P16_TargetUsername_Boolean +'}';

        it('should return status 200 when POST with Username:NLS ', function(done) {
            var payload = P16_TargetUsernameObject1;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with Username:NLS', function(done) {
            verifyConfig = JSON.parse( P16_TargetUsername_NLS ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Error
        it('should return status 400 when POST with Username:NUMERIC ', function(done) {
            var payload = P16_TargetUsernameObjectBad1;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Username Value: \"0\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with Username:NUMERIC', function(done) {
            verifyConfig = JSON.parse( P16_TargetUsername_NLS ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 when POST with Username:Boolean ', function(done) {
            var payload = P16_TargetUsernameObjectBad2;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Username Value: \"false\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with Username:Boolean', function(done) {
            verifyConfig = JSON.parse( P16_TargetUsername_NLS ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        

// Default:
//Defect 217737
        it('should return status 200 when POST with Username:null', function(done) {
            var payload = '{ "Connection": { "P16_Target": { "Username":null}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with Username:null', function(done) {
            verifyConfig = JSON.parse( '{ "P16_Target": { "Username":null}}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    }); 

//  ====================  Password  ====================  //
//  TypeOf String:  not limits, may be obfuscated or not
     describe('Password:', function() {

        var P16_TargetPassword_ClearText = '{ "P16_Target": { "Password": "password" }}';
        var P16_TargetPassword_ClearText_obs = '{ "P16_Target": { "Password": "XXXXXX" }}';
        var P16_TargetPasswordObject1 = '{"Connection": '+ P16_TargetPassword_ClearText +'}';

        var P16_TargetPassword_Obfuscate = '{ "P16_Target": { "Username":"user", "Password": "=0hRDkZDpoF+e0sLJrgMpv7c2xeyApDR/6DWWzD8JfWI=" }}';
        var P16_TargetPassword_Obfuscate_obs = '{ "P16_Target": { "Password": "XXXXXX" }}';
        var P16_TargetPasswordObject2 = '{"Connection": '+ P16_TargetPassword_Obfuscate +'}';

        // NOT Valid 
        var P16_TargetPassword_ZERO = '{ "P16_Target": { "Password": 0 }}';
        var P16_TargetPasswordObjectBad1 = '{"Connection": '+ P16_TargetPassword_ZERO +'}';
        var P16_TargetPassword_Boolean = '{ "P16_Target": { "Password": false }}';
        var P16_TargetPasswordObjectBad2 = '{"Connection": '+ P16_TargetPassword_Boolean +'}';

        it('should return status 200 when POST with Password:clearText ', function(done) {
            var payload = P16_TargetPasswordObject1;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with Password:clearText', function(done) {
            verifyConfig = JSON.parse( P16_TargetPassword_ClearText_obs ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST with Password:Obfuscate ', function(done) {
            var payload = P16_TargetPasswordObject2;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with Password:Obfuscate', function(done) {
            verifyConfig = JSON.parse( P16_TargetPassword_Obfuscate_obs ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Error
        it('should return status 400 when POST with Password:Numeric ', function(done) {
            var payload = P16_TargetPasswordObjectBad1;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Password Value: \"0\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with Password:Numeric', function(done) {
//            verifyConfig = JSON.parse( P16_TargetPassword_ZERO ) ;
            verifyConfig = JSON.parse( P16_TargetPassword_Obfuscate_obs ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 when POST with Password:Boolean ', function(done) {
            var payload = P16_TargetPasswordObjectBad2;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Password Value: \"false\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Connection with Password:Boolean', function(done) {
//            verifyConfig = JSON.parse( P16_TargetPassword_Boolean ) ;
            verifyConfig = JSON.parse( P16_TargetPassword_Obfuscate_obs ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        

// Default:
//Defect 217737
        it('should return status 200 when POST with Password:null', function(done) {
            var payload = '{ "Connection": { "P16_Target": { "Password":null}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, with Password:null', function(done) {
            verifyConfig = JSON.parse( '{ "P16_Target": { "Password":null}}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    }); 

//  ====================  Delete test cases  ====================  //
    describe('Delete:', function() {

        it('should return status 200 on DELETE "TempSource"', function(done) {
            var payload = '{"Connection":{"TempSource":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA0000","Message": "Success"};
            FVT.makeDeleteRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"TempSource", BridgeAuth, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET all after DELETE "TempSource" gone', function(done) {
            verifyConfig = {"TempSource":null};
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject, BridgeAuth, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on GET after DELETE; "TempSource" Not Found', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/TempSource"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"TempSource", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 404 on second DELETE "TempSource"', function(done) {
            var payload = '{"Connection":{"TempSource":null}}';
            verifyConfig = {"Code": "CWLNA0404","Message": "The HTTP request is for an object which does not exist.: Connection/TempSource"};
            verifyMessage = {"Code": "CWLNA0404","Message": "The HTTP request is for an object which does not exist.: Connection/TempSource"};
            FVT.makeDeleteRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"TempSource", BridgeAuth, FVT.verify404NotFound, verifyConfig, verifyMessage, done);
        });

        it('should return status 200 when POST NULL "TempDestination" (Bridge HAS Post DELETE)', function(done) {
            var payload = '{"Connection":{"TempDestination":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA0000","Message": "Success"};
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+"config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST DELETE, "TempDestination" is NOT found', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/TempDestination"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"TempDestination", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });
        
    });

//  ====================  Error test cases  ====================  //
     describe('Error General:', function() {

        it('should return status 400 when POST with Unknown Property ', function(done) {
            var payload = '{"Connection":{ "Unknown_Target": {"MQTTServerList": [ "'+ FVT.A1_HOST +':1883" ], "Unknown":"Property",  "ClientID": "A:orgDestIoT2:P2Br:MultiFwdV3S-", "Version": "5.0", "Username": "A:orgDestIoT2:P2Br:MultiFwdV3S-", "Password": "!JjaKkTrAz1/+b19URCFF+ic="}}}';
            verifyConfig = JSON.parse( payload );
// Defect 217600
            verifyMessage = {"status":400,"Code":"CWLNA0111","Message":"The property name is not valid: Property: Unknown."};
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+ "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET, Connection with Unknown Property should not be added', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/Unknown_Target"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject + "Unknown_Target", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });
        
// MisMatch Properties of MQTT and Event Streams
// Defect 218601
        it('should return status 400 when POST with MQTT and EventStreams List Array Property ', function(done) {
            var payload = '{"Connection":{ "Event_Target": {"MQTTServerList": [ "'+ FVT.A1_HOST +':1883" ], "EventStreamsBrokerList": [ "'+ FVT.A1_HOST +':1883" ] }}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0111","Message":"The property name is not valid: Property: MQTTServerList."};
            verifyMessage = {"status":400,"Code":"CWLNA0111","Message":"The property name is not valid: Property: EventStreamsBrokerList."};
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+ "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET, Connection with MQTT and EventStreams List Array Property should not be added', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/Event_Target"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject + "Event_Target", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

// EventStreams does not use ClientID SessionExpiry or Version
// Defect 218602 - URX: doc says just NOT USED, not that NOT Allowed....  ;-(
//        it('should return status 400 when POST with EventStreams List with Version Property ', function(done) {
        it('should return status 200 when POST with EventStreams List with Version Property (IGNORED!) ', function(done) {
            var payload = '{"Connection":{ "Event_Target": { "EventStreamsBrokerList": [ "'+ FVT.A1_HOST +':1883" ],  "Version": "5.0" }}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0111","Message":"The property name is not valid: Property: Version."};
//            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+ "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            verifyMessage = {"Code":"CWLNA0000","Message":"Success"};
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+ "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        // it('should return status 404 on GET, Connection with EventStreams List with Version should not be added', function(done) {
            // verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/Event_Target"}' ) ;
            // FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject + "Event_Target", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        // });
        it('should return status 200 on GET, with Connection with EventStreams List with Version (IGNORED!) ', function(done) {
            verifyConfig = JSON.parse( '{ "EventStreamsBrokerList": [ "'+ FVT.A1_HOST +':1883" ],  "Version": "5.0" }' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"Event_Target" , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Defect 218602 - URX: doc says just NOT USED, not that NOT Allowed....  ;-(
//        it('should return status 400 when POST with EventStreams List with ClientId Property ', function(done) {
        it('should return status 200 when POST with EventStreams List with ClientId Property (IGNORED!)', function(done) {
            var payload = '{"Connection":{ "Event_Target": { "EventStreamsBrokerList": [ "'+ FVT.A1_HOST +':1883" ],  "SessionExpiry": 100}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0111","Message":"The property name is not valid: Property: ClientID."};
//            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+ "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            verifyMessage = {"Code":"CWLNA0000","Message":"Success"};
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+ "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        // it('should return status 404 on GET, Connection with EventStreams List with ClientId Property should not be added', function(done) {
            // verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/Event_Target"}' ) ;
            // FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject + "Event_Target", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        // });
        it('should return status 200 on GET, with Connection with EventStreams List with ClientId (IGNORED!) ', function(done) {
            verifyConfig = JSON.parse( '{ "EventStreamsBrokerList": [ "'+ FVT.A1_HOST +':1883" ],  "SessionExpiry": 100 }' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"Event_Target" , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Defect 218602 - URX: doc says just NOT USED, not that NOT Allowed....  ;-(
//        it('should return status 400 when POST with EventStreams List with SessionExpiry Property ', function(done) {
        it('should return status 200 when POST with EventStreams List with SessionExpiry Property (IGNORED!)', function(done) {
            var payload = '{"Connection":{ "Event_Target": { "EventStreamsBrokerList": [ "'+ FVT.A1_HOST +':1883" ],  "SessionExpiry": 100 }}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0111","Message":"The property name is not valid: Property: SessionExpiry."};
//            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+ "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            verifyMessage = {"Code":"CWLNA0000","Message":"Success"};
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+ "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        // it('should return status 404 on GET, Connection with EventStreams List with SessionExpiry Property should not be added', function(done) {
            // verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/Event_Target"}' ) ;
            // FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject + "Event_Target", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        // });
        it('should return status 200 on GET, with Connection with EventStreams List with SessionExpiry (IGNORED!) ', function(done) {
            verifyConfig = JSON.parse( '{ "EventStreamsBrokerList": [ "'+ FVT.A1_HOST +':1883" ],  "SessionExpiry": 100 }' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"Event_Target" , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

		
// MQTTServerList does not use MaxBatchTimeMS
// Defect 218603 - URX: doc says just NOT USED, not that NOT Allowed....  ;-(
//        it('should return status 400 when POST with MQTTServerList with MaxBatchTimeMS Property ', function(done) {
        it('should return status 200 when POST with MQTTServerList with MaxBatchTimeMS Property (IGNORED!)', function(done) {
            var payload = '{"Connection":{ "P3_Target": {"MQTTServerList": [ "'+ FVT.A1_HOST +':1883" ], "MaxBatchTimeMS": 666 }}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0111","Message":"The property name is not valid: Property: MaxBatchTimeMS."};
//            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+ "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            verifyMessage = {"Code":"CWLNA0000","Message":"Success"};
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+ "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        // it('should return status 404 on GET, Connection with MQTTServerList with MaxBatchTimeMS Property should not be added', function(done) {
            // verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/P3_Target"}' ) ;
            // FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject + "P3_Target", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        // });
        it('should return status 200 on GET, with Connection with EventStreams List with MaxBatchTimeMS (IGNORED!) ', function(done) {
            verifyConfig = JSON.parse( '{ "MQTTServerList": [ "'+ FVT.A1_HOST +':1883" ], "MaxBatchTimeMS": 666 }' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"P3_Target" , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    }); 

//  ====================  CleanUp lingering objects  ====================  //
    describe('DeleteCleanUP:', function() {

        it('should return status 200 on DELETE "KafkaDestination"', function(done) {
            var payload = '{"Connection":{"KafkaDestination":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA0000","Message": "Success"};
            FVT.makeDeleteRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"KafkaDestination", BridgeAuth, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after DELETE; "KafkaDestination" Not Found', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/KafkaDestination"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"KafkaDestination", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 when POST NULL "P16_Target" (Bridge HAS Post DELETE)', function(done) {
            var payload = '{"Connection":{"P16_Target":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA0000","Message": "Success"};
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+"config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST DELETE, "P16_Target" is NOT found', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/P16_Target"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"P16_Target", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 on DELETE "CID36_Target"', function(done) {
            var payload = '{"Connection":{"CID36_Target":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA0000","Message": "Success"};
            FVT.makeDeleteRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"CID36_Target", BridgeAuth, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after DELETE; "CID36_Target" Not Found', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/CID36_Target"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"CID36_Target", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 on DELETE "CID_NLSChar_Target"', function(done) {
            var payload = '{"Connection":{"CID_NLSChar_Target":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA0000","Message": "Success"};
            FVT.makeDeleteRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"CID_NLSChar_Target", BridgeAuth, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after DELETE; "CID_NLSChar_Target" Not Found', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/CID_NLSChar_Target"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"CID_NLSChar_Target", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });
// defect workarounds may need to be removed later
        it('should return status 200 when POST NULL "P3_Target" (defect workaround)', function(done) {
            var payload = '{"Connection":{"P3_Target":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA0000","Message": "Success"};
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+"config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST DELETE, "P3_Target" is NOT found', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/P3_Target"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"P3_Target", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 when POST NULL "Event_Target" (defect workaround)', function(done) {
            var payload = '{"Connection":{"Event_Target":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA0000","Message": "Success"};
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+"config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST DELETE, "Event_Target" is NOT found', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Connection/Event_Target"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"Event_Target", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });
        
    });


});
