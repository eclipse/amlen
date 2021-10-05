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

var uriObject = 'accept/license/';

// assuming Bridge Config Bridge.A1A2.cfg was loaded 
var ForwarderName='m2w';
var expectedForwarderProperties = '{ "Topic": [ "wiotp/+/+/+/+" ], "Enabled": true, "Source": "MqttServer", "Destination": "WIoTP", "TopicMap": "iot-2/${Topic1*}" }';
var expectedForwarder = '{"'+ ForwarderName +'": '+ expectedForwarderProperties +'}';
var expectForwarderObject = '{ "Forwarder": '+ expectedForwarder +'}';

var licenseNone = ' "LicensedUsage": "None" ';
var licenseProduction = ' "LicensedUsage": "Production" ';
var licenseNon_Production = ' "LicensedUsage": "Non-Production" ';
var licenseDevelopers = ' "LicensedUsage": "Developers" ';

var expectNoneConfig = '{ '+ licenseNone +', \
                                "Connection": { "WIoTP": { "MQTTServerList": [ "'+ FVT.A2_HOST +':1883" ], "ClientID": "A:x:myBrApp:SimpleA1A2-", "Version": "5.0" }, \
                                           "MqttServer": { "MQTTServerList": [ "'+ FVT.A1_HOST +':1883" ], "ClientID": "BR.SimpleA1A2-", "Version": "5.0" } }, \
                                "Forwarder": { "m2w": { "Topic": ["wiotp/+/+/+/+"], "Enabled": true, "Source": "MqttServer", "Destination": "WIoTP", "TopicMap": "iot-2/${Topic1*}" } }, \
                                "Endpoint": { "admin": { "Port": 9961, "Interface": "*", "Enabled": true, "Secure": false, "Protocol": "Admin", "UseClientCipher": false } }, \
                                 "User": { "admin": { "Password": "=AJFIt0p+ZYW1tPpDnh9EVOMsVpxxv9PuWdTfM+ULU3A=" } } }' ;
var expectNoneSingletons = '{ "TraceLevel": "5,mqtt=9,http=6,kafka=6", "TraceMessageData": 1024, "TraceFile": "/var/imabridge/diag/logs/imabridge.trace", "TraceMax": "40M", "TraceFlush": 1000, "LogLevel": "Max", \
                                    "LogDestinationType": "file", "LogDestination": "/var/imabridge/diag/logs/imabridge.log", "TrustStore": "/var/imabridge/truststore", "KeyStore": "/var/imabridge/keystore", \
									"HttpDir": "/opt/ibm/imabridge/http", "DynamicConfig": "/var/imabridge/bridge.cfg", "ResourceDir": "/opt/ibm/imabridge/resource", "FileLimit": 20000, "IOThreads": 3, \
									"ServerName": "Bridge", '+ licenseNone +' }';


var expectProductionConfig = '{ '+ licenseProduction +', \
                                "Connection": { "WIoTP": { "MQTTServerList": [ "'+ FVT.A2_HOST +':1883" ], "ClientID": "A:x:myBrApp:SimpleA1A2-", "Version": "5.0" }, \
                                           "MqttServer": { "MQTTServerList": [ "'+ FVT.A1_HOST +':1883" ], "ClientID": "BR.SimpleA1A2-", "Version": "5.0" } }, \
                                "Forwarder": { "m2w": { "Topic": ["wiotp/+/+/+/+"], "Enabled": true, "Source": "MqttServer", "Destination": "WIoTP", "TopicMap": "iot-2/${Topic1*}" } }, \
                                "Endpoint": { "admin": { "Port": 9961, "Interface": "*", "Enabled": true, "Secure": false, "Protocol": "Admin", "UseClientCipher": false } }, \
                                 "User": { "admin": { "Password": "=AJFIt0p+ZYW1tPpDnh9EVOMsVpxxv9PuWdTfM+ULU3A=" } } }' ;
var expectProductionSingletons = '{ "TraceLevel": "5,mqtt=9,http=6,kafka=6", "TraceMessageData": 1024, "TraceFile": "/var/imabridge/diag/logs/imabridge.trace", "TraceMax": "40M", "TraceFlush": 1000, "LogLevel": "Max", \
                                    "LogDestinationType": "file", "LogDestination": "/var/imabridge/diag/logs/imabridge.log", "TrustStore": "/var/imabridge/truststore", "KeyStore": "/var/imabridge/keystore", \
									"HttpDir": "/opt/ibm/imabridge/http", "DynamicConfig": "/var/imabridge/bridge.cfg", "ResourceDir": "/opt/ibm/imabridge/resource", "FileLimit": 20000, "IOThreads": 3, \
									"ServerName": "Bridge", '+ licenseProduction +' }';


var expectNon_ProductionConfig = '{ '+ licenseNon_Production +', \
                                "Connection": { "WIoTP": { "MQTTServerList": [ "'+ FVT.A2_HOST +':1883" ], "ClientID": "A:x:myBrApp:SimpleA1A2-", "Version": "5.0" }, \
                                           "MqttServer": { "MQTTServerList": [ "'+ FVT.A1_HOST +':1883" ], "ClientID": "BR.SimpleA1A2-", "Version": "5.0" } }, \
                                "Forwarder": { "m2w": { "Topic": ["wiotp/+/+/+/+"], "Enabled": true, "Source": "MqttServer", "Destination": "WIoTP", "TopicMap": "iot-2/${Topic1*}" } }, \
                                "Endpoint": { "admin": { "Port": 9961, "Interface": "*", "Enabled": true, "Secure": false, "Protocol": "Admin", "UseClientCipher": false } }, \
                                 "User": { "admin": { "Password": "=AJFIt0p+ZYW1tPpDnh9EVOMsVpxxv9PuWdTfM+ULU3A=" } } }' ;
var expectNon_ProductionSingletons = '{ "TraceLevel": "5,mqtt=9,http=6,kafka=6", "TraceMessageData": 1024, "TraceFile": "/var/imabridge/diag/logs/imabridge.trace", "TraceMax": "40M", "TraceFlush": 1000, "LogLevel": "Max", \
                                    "LogDestinationType": "file", "LogDestination": "/var/imabridge/diag/logs/imabridge.log", "TrustStore": "/var/imabridge/truststore", "KeyStore": "/var/imabridge/keystore", \
									"HttpDir": "/opt/ibm/imabridge/http", "DynamicConfig": "/var/imabridge/bridge.cfg", "ResourceDir": "/opt/ibm/imabridge/resource", "FileLimit": 20000, "IOThreads": 3, \
									"ServerName": "Bridge", '+ licenseNon_Production +' }';


var expectDevelopersConfig = '{ '+ licenseDevelopers +', \
                                "Connection": { "WIoTP": { "MQTTServerList": [ "'+ FVT.A2_HOST +':1883" ], "ClientID": "A:x:myBrApp:SimpleA1A2-", "Version": "5.0" }, \
                                           "MqttServer": { "MQTTServerList": [ "'+ FVT.A1_HOST +':1883" ], "ClientID": "BR.SimpleA1A2-", "Version": "5.0" } }, \
                                "Forwarder": { "m2w": { "Topic": ["wiotp/+/+/+/+"], "Enabled": true, "Source": "MqttServer", "Destination": "WIoTP", "TopicMap": "iot-2/${Topic1*}" } }, \
                                "Endpoint": { "admin": { "Port": 9961, "Interface": "*", "Enabled": true, "Secure": false, "Protocol": "Admin", "UseClientCipher": false } }, \
                                 "User": { "admin": { "Password": "=AJFIt0p+ZYW1tPpDnh9EVOMsVpxxv9PuWdTfM+ULU3A=" } } }' ;
var expectDevelopersSingletons = '{ "TraceLevel": "5,mqtt=9,http=6,kafka=6", "TraceMessageData": 1024, "TraceFile": "/var/imabridge/diag/logs/imabridge.trace", "TraceMax": "40M", "TraceFlush": 1000, "LogLevel": "Max", \
                                    "LogDestinationType": "file", "LogDestination": "/var/imabridge/diag/logs/imabridge.log", "TrustStore": "/var/imabridge/truststore", "KeyStore": "/var/imabridge/keystore", \
									"HttpDir": "/opt/ibm/imabridge/http", "DynamicConfig": "/var/imabridge/bridge.cfg", "ResourceDir": "/opt/ibm/imabridge/resource", "FileLimit": 20000, "IOThreads": 3, \
									"ServerName": "Bridge", '+ licenseDevelopers +' }';

var BridgeURI = FVT.P1_HOST +":9961"
var BridgeAuth = FVT.P1_REST_USER +','+ FVT.P1_REST_PW



//  ====================  Test Cases Start Here  ====================  //

describe('AcceptLicense:', function() {
this.timeout( FVT.defaultTimeout );

    // Accept Bridge License
    describe('AcceptLicense:None', function() {
        it('should return status 200 to /accept/license/None', function(done) {
            var payload = null;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + uriObject +"None", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
	    });
        it('should return status 200 on GET, LicensedUsage:None Singletons', function(done) {
            verifyConfig = JSON.parse( expectNoneSingletons );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"set", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, LicensedUsage:None config', function(done) {
            verifyConfig = JSON.parse( expectNoneConfig );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
	
    describe('AcceptLicense:Non-Production', function() {
        it('should return status 200 to /accept/license/Non-Production', function(done) {
            var payload = null;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + uriObject +"Non-Production", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
	    });
        it('should return status 200 on GET, LicensedUsage:Non-Production Singletons', function(done) {
            verifyConfig = JSON.parse( expectNon_ProductionSingletons );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"set", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, LicensedUsage:Non-Production config', function(done) {
            verifyConfig = JSON.parse( expectNon_ProductionConfig );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
	
    describe('AcceptLicense:Developers', function() {
        it('should return status 200 to /accept/license/Developers', function(done) {
            var payload = null;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + uriObject +"Developers", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
	    });
        it('should return status 200 on GET, LicensedUsage:Developers singletons', function(done) {
            verifyConfig = JSON.parse( expectDevelopersSingletons );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"set", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, LicensedUsage:Developers config', function(done) {
            verifyConfig = JSON.parse( expectDevelopersConfig );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
	
    describe('AcceptLicense:Production', function() {
        it('should return status 200 to /accept/license/Production', function(done) {
            var payload = null;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + uriObject +"Production", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
	    });
        it('should return status 200 on GET, LicensedUsage:Production singletons', function(done) {
            verifyConfig = JSON.parse( expectProductionSingletons );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"set", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, LicensedUsage:Production config', function(done) {
            verifyConfig = JSON.parse( expectProductionConfig );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

	});
	
// Error

    describe('AcceptBridgeLicense:Errors', function() {
	
        it('should return status 400 to /accept/license/Trial', function(done) {
            var payload = null;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0400", "Message": "The property value is not valid: Property: LicensedUsage Value: \"Trial\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + uriObject +"Trial", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
	    });
        it('should return status 200 on GET, singletons after Trial fails', function(done) {
            verifyConfig = JSON.parse( expectProductionSingletons );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"set", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, config after Trial fails', function(done) {
            verifyConfig = JSON.parse( expectProductionConfig );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 404 to /config/LicensedUsage/Developers', function(done) {
            var payload = null;
            verifyConfig = JSON.parse( payload );
//            verifyMessage = { "status":404, "Code":"CWLNA0404", "Message": "The HTTP request is for an object which does not exist.: LicensedUsage/Developers" };
            verifyMessage = { "status":400, "Code":"CWLNA0400", "Message": "The HTTP request is not valid." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config/LicensedUsage/Developers", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
	    });
        it('should return status 200 on GET, LicensedUsage', function(done) {
            verifyConfig = JSON.parse( expectProductionSingletons );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"set", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, LicensedUsage', function(done) {
            verifyConfig = JSON.parse( expectProductionConfig );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
	
        it('should return status 404 to /config/license/Developers', function(done) {
            var payload = null;
            verifyConfig = JSON.parse( payload );
//            verifyMessage = { "status":404, "Code":"CWLNA0404", "Message": "The HTTP request is for an object which does not exist.: license/Developers" };
            verifyMessage = { "status":400, "Code":"CWLNA0400", "Message": "The HTTP request is not valid." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config/license/Developers", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
	    });
        it('should return status 200 on GET, LicensedUsage', function(done) {
            verifyConfig = JSON.parse( expectProductionSingletons );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"set", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, LicensedUsage', function(done) {
            verifyConfig = JSON.parse( expectProductionConfig );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
		
// Defect 217914
        it('should !NOW! return status 200 to /config with payload LicensedUsage/Developers', function(done) {
            var payload = '{ "LicensedUsage":"Developers" }';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = { "status":400, "Code":"CWLNA0400", "Message": "Failure" };
//            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
            verifyMessage = {"Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
	    });
        it('should return status 200 on GET, LicensedUsage', function(done) {
            verifyConfig = JSON.parse( expectDevelopersSingletons );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"set", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, LicensedUsage', function(done) {
            verifyConfig = JSON.parse( expectDevelopersConfig );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

	});


});
