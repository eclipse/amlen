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

var uriObject = 'config/Forwarder/';

// assuming Bridge Config Bridge.A1A2.cfg was loaded 
var ForwarderName='m2w';
var expectedForwarderProperties = '{ "Topic": [ "wiotp/+/+/+/+" ], "Enabled": true, "Source": "MqttServer", "Destination": "WIoTP", "TopicMap": "iot-2/${Topic1*}" }';
var expectedForwarder = '{"'+ ForwarderName +'": '+ expectedForwarderProperties +'}';
var expectForwarderObject = '{ "Forwarder": '+ expectedForwarder +'}';

var expectAllDefaultConfig = '{ "LicensedUsage": "Production", \
                                "Connection": { "WIoTP": { "MQTTServerList": [ "'+ FVT.A2_HOST +':1883" ], "ClientID": "A:x:myBrApp:SimpleA1A2-", "Version": "5.0" }, \
                                           "MqttServer": { "MQTTServerList": [ "'+ FVT.A1_HOST +':1883" ], "ClientID": "BR.SimpleA1A2-", "Version": "5.0" } }, \
                                "Forwarder": { "m2w": { "Topic": ["wiotp/+/+/+/+"], "Enabled": true, "Source": "MqttServer", "Destination": "WIoTP", "TopicMap": "iot-2/${Topic1*}" } }, \
                                "Endpoint": { "admin": { "Port": 9961, "Interface": "*", "Enabled": true, "Secure": false, "Protocol": "Admin", "UseClientCipher": false } }, \
                                 "User": { "admin": { "Password": "=AJFIt0p+ZYW1tPpDnh9EVOMsVpxxv9PuWdTfM+ULU3A=" } } }' ;

var expectAllUpdatedConfig = '{ "LicensedUsage": "Production", \
                                "Connection": { "WIoTP": { "MQTTServerList": [ "'+ FVT.A2_HOST +':1883" ], "ClientID": "A:x:myBrApp:SimpleA1A2-", "Version": "5.0" }, \
                                         "MqttServer": { "MQTTServerList": [ "'+ FVT.A1_HOST +':1883" ], "ClientID": "BR.SimpleA1A2-", "Version": "5.0" }, \
                                         "TempSource": { "MQTTServerList" : ["'+ FVT.A1_HOST +':16102"], "ClientID" : "BridgeRestAPI_CID",  "Version" : "5.0" } , \
                                         "TempDestination": { "MQTTServerList" : ["'+ FVT.A2_HOST +':16102"], "ClientID" : "A:x:BrMS2:SimpleA1A2-",  "Version" : "5.0" } }, \
                                "Forwarder": { "m2w": { "Topic": ["wiotp/+/+/+/+"], "Enabled": true, "Source": "MqttServer", "Destination": "WIoTP", "TopicMap": "iot-2/${Topic1*}" } }, \
                                "Endpoint": { "admin": { "Port": 9961, "Interface": "*", "Enabled": true, "Secure": false, "Protocol": "Admin", "UseClientCipher": false } }, \
                                "User": { "admin": { "Password": "=AJFIt0p+ZYW1tPpDnh9EVOMsVpxxv9PuWdTfM+ULU3A=" } } }' ;

var BridgeURI = FVT.P1_HOST +":9961"
var BridgeAuth = FVT.P1_REST_USER +','+ FVT.P1_REST_PW

var TestTargetProperties = '{ "Topic": [ "wiotp/+/+/+/+" ], "Enabled": true, "Source": "MqttServer", "Destination": "WIoTP", "TopicMap": "iot-2/${Topic1*}" }';
var TestTargetForwarder = '{ "TestTarget": '+ TestTargetProperties +'}';
var TestTargetForwarderObject = '{"Forwarder": '+ TestTargetForwarder +'}';

// TestTarget properties
var TestTargetForwarderSource = '{ "TestTarget": { "Source": "From This MQTT Server: '+ FVT.long16Name +'"}}';  
var TestTargetForwarderSourceObject = '{"Forwarder": '+ TestTargetForwarderSource +'}';
var TestTargetForwarderDestination  = '{ "TestTarget": { "Destination": "ToThisMQTTServer'+ FVT.long16Name +'"}}';   
var TestTargetForwarderDestinationObject = '{"Forwarder": '+ TestTargetForwarderDestination +'}';
var TestTargetForwarderEnabled_True = '{ "TestTarget": { "Enabled": true }}';  
var TestTargetForwarderEnabled_TrueObject = '{"Forwarder": '+ TestTargetForwarderEnabled_True +'}';
var TestTargetForwarderEnabled_False = '{ "TestTarget": { "Enabled": false }}';  
var TestTargetForwarderEnabled_FalseObject = '{"Forwarder": '+ TestTargetForwarderEnabled_False +'}';
var TestTargetForwarderInstances_MIN = '{ "TestTarget": { "Instances": 0 }}';  
var TestTargetForwarderInstances_MIN_apply = '{ "TestTarget": { "Instances": null }}';  
var TestTargetForwarderInstances_MINObject = '{"Forwarder": '+ TestTargetForwarderInstances_MIN +'}';
var TestTargetForwarderInstances_LOW = '{ "TestTarget": { "Instances": 1 }}';  
var TestTargetForwarderInstances_LOWObject = '{"Forwarder": '+ TestTargetForwarderInstances_LOW +'}';
var TestTargetForwarderInstances_MAX = '{ "TestTarget": { "Instances": 99 }}';  
var TestTargetForwarderInstances_MAXObject = '{"Forwarder": '+ TestTargetForwarderInstances_MAX +'}';
var TestTargetForwarderTopic_STRING = '{ "TestTarget": { "Topic":"iot-2/type/+/id/+/evt/+/fmt/+"}}';  
var TestTargetForwarderTopic_STRING_apply = '{ "TestTarget": { "Topic": [ "iot-2/type/+/id/+/evt/+/fmt/+" ] }}';  
var TestTargetForwarderTopic_STRINGObject = '{"Forwarder": '+ TestTargetForwarderTopic_STRING +'}';
var TestTargetForwarderTopic_ARRAY = '{ "TestTarget": { "Topic": ["iot-2/type/+/id/+/evt/+/fmt/1" , "iot-2/type/+/id/+/evt/+/fmt/2" , "iot-2/type/+/id/+/evt/+/fmt/3" , "iot-2/type/+/id/+/evt/+/fmt/4" , "iot-2/type/+/id/+/evt/+/fmt/5" , "iot-2/type/+/id/+/evt/+/fmt/6", \
                                "iot-2/type/+/id/+/evt/+/fmt/7" , "iot-2/type/+/id/+/evt/+/fmt/8" , "iot-2/type/+/id/+/evt/+/fmt/9" , "iot-2/type/+/id/+/evt/+/fmt/10" , "iot-2/type/+/id/+/evt/+/fmt/11" , "iot-2/type/+/id/+/evt/+/fmt/12" , "iot-2/type/+/id/+/evt/+/fmt/13", \
                                "iot-2/type/+/id/+/evt/+/fmt/14" , "iot-2/type/+/id/+/evt/+/fmt/15" , "iot-2/type/+/id/+/evt/+/fmt/16" ] }}';  
var TestTargetForwarderTopic_ARRAYObject = '{"Forwarder": '+ TestTargetForwarderTopic_ARRAY +'}';
var TestTargetForwarderTopic_ARRAY17 = '{ "TestTarget": { "Topic": ["iot-2/type/+/id/+/evt/+/fmt/1" , "iot-2/type/+/id/+/evt/+/fmt/2" , "iot-2/type/+/id/+/evt/+/fmt/3" , "iot-2/type/+/id/+/evt/+/fmt/4" , "iot-2/type/+/id/+/evt/+/fmt/5" , "iot-2/type/+/id/+/evt/+/fmt/6", \
                                  "iot-2/type/+/id/+/evt/+/fmt/7" , "iot-2/type/+/id/+/evt/+/fmt/8" , "iot-2/type/+/id/+/evt/+/fmt/9" , "iot-2/type/+/id/+/evt/+/fmt/10" , "iot-2/type/+/id/+/evt/+/fmt/11" , "iot-2/type/+/id/+/evt/+/fmt/12" , "iot-2/type/+/id/+/evt/+/fmt/13", \
                                  "iot-2/type/+/id/+/evt/+/fmt/14" , "iot-2/type/+/id/+/evt/+/fmt/15" , "iot-2/type/+/id/+/evt/+/fmt/16" , "iot-2/type/+/id/+/evt/+/fmt/17" ] }}';  
var TestTargetForwarderTopic_ARRAY17Object = '{"Forwarder": '+ TestTargetForwarderTopic_ARRAY17 +'}';
var TestTargetForwarderTopicMap = '{ "TestTarget": { "TopicMap":"iot-2/type/+/id/+/evt/+/fmt/+"}}';  
var TestTargetForwarderTopicMapObject = '{"Forwarder": '+ TestTargetForwarderTopicMap +'}';
var TestTargetForwarderTopicMap_ARRAY = '{ "TestTarget": { "TopicMap": [ "iot-2/type/+/id/+/evt/+/fmt/+", "iot-2/type/+/id/+/cmd/+/fmt/+" ] }}';  
var TestTargetForwarderTopicMap_ARRAYObject = '{"Forwarder": '+ TestTargetForwarderTopicMap_ARRAY +'}';
var TestTargetForwarderSourceQoS_0 = '{ "TestTarget": { "SourceQoS": 0 }}';  
var TestTargetForwarderSourceQoS_0Object = '{"Forwarder": '+ TestTargetForwarderSourceQoS_0 +'}';
var TestTargetForwarderSourceQoS_1 = '{ "TestTarget": { "SourceQoS": 1 }}';  
var TestTargetForwarderSourceQoS_1Object = '{"Forwarder": '+ TestTargetForwarderSourceQoS_1 +'}';
var TestTargetForwarderSourceQoS_2 = '{ "TestTarget": { "SourceQoS": 2 }}';  
var TestTargetForwarderSourceQoS_2Object = '{"Forwarder": '+ TestTargetForwarderSourceQoS_2 +'}';
var TestTargetForwarderSelector_1 = '{ "TestTarget": { "Selector": "QoS >=1 and Topic6 in ( \'Bridge\' , \'Catwalk\' ) " }}';  
var TestTargetForwarderSelector_1Object = '{"Forwarder": '+ TestTargetForwarderSelector_1 +'}';
var TestTargetForwarderSelector_2 = '{ "TestTarget": { "Selector": "QoS >=1 and Topic6 like \'Catwalk\'  " }}';  
var TestTargetForwarderSelector_2Object = '{"Forwarder": '+ TestTargetForwarderSelector_2 +'}';


//  ====================  Test Cases Start Here  ====================  //

describe('ConfigForwarder:', function() {
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
        it('should return status 200 on GET, DEFAULT of "Forwarder":{UpperCased}', function(done) {
            verifyConfig = JSON.parse(expectedForwarder);
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject, BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on GET, DEFAULT of "forwarder":{lowercased}', function(done) {
            verifyConfig = JSON.parse(expectedForwarder);
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config/forwarder", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on GET, "forwarder/*"', function(done) {
            verifyConfig = JSON.parse(expectedForwarder);
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config/forwarder/*", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on GET the Entire CONFIG', function(done) {
            verifyConfig = JSON.parse(expectAllDefaultConfig);
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

//  ====================   Get Specific Forwarder  ====================  //
    describe('Get/Update Forwarder', function() {

        it('should return status 200 on GET, "Forwarder/m2w"', function(done) {
            verifyConfig = JSON.parse(expectedForwarderProperties);
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+ForwarderName, BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when REPOST m2w Forwarder', function(done) {
            var payload = expectForwarderObject ;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return stat us 200 on GET, updated "Forwarder/m2w"', function(done) {
            verifyConfig = JSON.parse(expectedForwarder);
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+"config/Forwarder", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

    });
    
//  ====================   Create Forwarder  ====================  //
    describe('Create Forwarder', function() {

        it('should return status 200 when POST to ADD a Forwarder object TestTarget', function(done) {
            var payload = TestTargetForwarderObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET new TestTarget', function(done) {
            var payload = TestTargetProperties ;
            verifyConfig = JSON.parse( payload );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"TestTarget", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET All Forwarders', function(done) {
            var payload = TestTargetForwarder ;
            verifyConfig = JSON.parse( payload );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject, BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

//  ====================  Source  ====================  //
// TypeOf: String 
     describe('Source:', function() {     

        it('should return status 200 when POST new Source value ', function(done) {
            var payload = TestTargetForwarderSourceObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Forwarder with new Source', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderSource ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Errors
        it('should return status 400 when POST Source:boolean  ', function(done) {
            var payload = '{ "Forwarder": { "NoBoolean" :{ "Source": true }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Source Value: \"true\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
       });
        it('should return status 404 on GET, Source:boolean', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Forwarder/NoBoolean"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"NoBoolean", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

       it('should return status 400 when POST with Source:numeric', function(done) {
            var payload = '{ "Forwarder": { "TestTarget" :{ "Source": 1883 }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112", "Message": "The property value is not valid: Property: Source Value: \"1883\"." } ;
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET without change to Source:numeric', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderSource ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });        
        
// Default, there is no Default, must provide a value
     
        it('should return status 400 when POST Source:null, INVALID ', function(done) {
            var payload = '{ "Forwarder": { "TestTarget" :{ "Source": null }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Source Value: \"null\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Source:null, INVALID', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderSource ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });


    }); 

//  ====================  Destination  ====================  //
//  TypeOf String: 
    describe('Destination:', function() {

        it('should return status 200 when POST new Destination Name', function(done) {
            var payload = TestTargetForwarderDestinationObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Forwarder with new Destination Name', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderDestination ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Errors
        it('should return status 400 when POST Destination:NoBoolean  ', function(done) {
            var payload = '{ "Forwarder": { "NoBoolean" :{ "Destination": true }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Destination Value: \"true\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
       });
        it('should return status 404 on GET, Destination:NoBoolean', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Forwarder/NoBoolean"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"NoBoolean", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

       it('should return status 400 when POST with Destination:numeric', function(done) {
            var payload = '{ "Forwarder": { "TestTarget" :{ "Destination": 1883 }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112", "Message": "The property value is not valid: Property: Destination Value: \"1883\"." } ;
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET without change to Destination:numeric', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderDestination ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });        
        
// Default, there is no Default, must provide a value

     
        it('should return status 400 when POST Destination:null, INVALID ', function(done) {
            var payload = '{ "Forwarder": { "TestTarget" :{ "Destination": null }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Destination Value: \"null\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Destination:null, INVALID', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderDestination ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
    
    }); 

//  ====================  Enabled  ====================  //
//  TypeOf Boolean  Default: false
     describe('Enabled:', function() {

        it('should return status 200 when POST new Enabled:false ', function(done) {
            var payload = TestTargetForwarderEnabled_FalseObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Forwarder with new Enabled:false', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderEnabled_False ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST new Enabled:true ', function(done) {
            var payload = TestTargetForwarderEnabled_TrueObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Forwarder with new Enabled:true', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderEnabled_True ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Errors
        it('should return status 400 when POST Enabled:NoString  ', function(done) {
            var payload = '{ "Forwarder": { "NoString" :{ "Enabled": "true" }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Enabled Value: \"true\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
       });
        it('should return status 404 on GET, without change to Enabled:NoString', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Forwarder/NoString"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"NoString", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

       it('should return status 400 when POST with Enabled:numeric', function(done) {
            var payload = '{ "Forwarder": { "TestTarget" :{ "Enabled": 0 }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112", "Message": "The property value is not valid: Property: Enabled Value: \"0\"." } ;
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET without change to Enabled:numeric', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderEnabled_True ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });        
        
// Default
     
        it('should return status 400 when POST Enabled:null Default:false?', function(done) {
            var payload = '{ "Forwarder": { "TestTarget" :{ "Enabled": null }}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Enabled Value: \"null\"." };
//            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Enabled:null, Default:false?', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderEnabled_False ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
    
    }); 

//  ====================  Instances  ====================  //
//  Int 0-99
     describe('Instances:', function() {

        it('should return status 200 when POST new Instances:LOW ', function(done) {
            var payload = TestTargetForwarderInstances_LOWObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Forwarder with new Instances:LOW', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderInstances_LOW ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST new Instances:MIN ', function(done) {
            var payload = TestTargetForwarderInstances_MINObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Forwarder with new Instances:MIN', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderInstances_MIN_apply ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST new Instances:MAX ', function(done) {
            var payload = TestTargetForwarderInstances_MAXObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Forwarder with new Instances:MAX', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderInstances_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Errors
        it('should return status 400 when POST Instances:NoString  ', function(done) {
            var payload = '{ "Forwarder": { "NoString" :{ "Instances": "true" }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Instances Value: \"true\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
       });
        it('should return status 404 on GET, without change to Instances:NoString', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Forwarder/NoString"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"NoString", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

       it('should return status 400 when POST with Instances:boolean', function(done) {
            var payload = '{ "Forwarder": { "TestTarget" :{ "Instances": false }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112", "Message": "The property value is not valid: Property: Instances Value: \"false\"." } ;
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET without change to Instances:boolean', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderInstances_MAX ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });        
        
// Default
// Defect 217829 Default=99 really!
        it('should return status 200 when POST Instances:null ', function(done) {
            var payload = '{ "Forwarder": { "TestTarget" :{ "Instances": null }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, after Instances:null', function(done) {
            verifyConfig = JSON.parse( '{ "TestTarget": { "Instances": null }}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    }); 

//  ====================  Topic  ====================  //
//  String or Array of Strings [ max 16]
     describe('Topic:', function() {

        it('should return status 200 when POST Topic:ARRAY ', function(done) {
            var payload = TestTargetForwarderTopic_ARRAYObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Forwarder with Topic:ARRAY', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderTopic_ARRAY ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST Topic:string ', function(done) {
            var payload = TestTargetForwarderTopic_STRINGObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Forwarder with Topic:string', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderTopic_STRING_apply ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Errors

        it('should return status 400 when POST Topic:ARRAY17 ', function(done) {
            var payload = TestTargetForwarderTopic_ARRAY17Object;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Topic Value: \"array\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Forwarder without Topic:ARRAY17', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderTopic_STRING_apply ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when POST Topic:boolean  ', function(done) {
            var payload = '{ "Forwarder": { "NoBoolean" :{ "Topic": true }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Topic Value: \"true\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
       });
        it('should return status 404 on GET, without change to Topic:boolean', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Forwarder/NoBoolean"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"NoBoolean", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

       it('should return status 400 when POST with Topic:numeric', function(done) {
            var payload = '{ "Forwarder": { "TestTarget" :{ "Topic": 1 }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112", "Message": "The property value is not valid: Property: Topic Value: \"1\"." } ;
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET without change to Topic:numeric', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderTopic_STRING_apply ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });        
        
// Default does not exist, must provide a value
     
        it('should return status 400 when POST Topic:null Invalid', function(done) {
            var payload = '{ "Forwarder": { "TestTarget" :{ "Topic": null }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Topic Value: \"null\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
//            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
//            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Topic:null INVALID', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderTopic_STRING_apply ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    }); 

//  ====================  Selector  ====================  //
// TypeOf String
     describe('Selector:', function() {

        it('should return status 200 when POST Selector:string_1 ', function(done) {
            var payload = TestTargetForwarderSelector_1Object;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Forwarder with Selector:string_1', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderSelector_1 ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST overwrite Selector:string_2 ', function(done) {
            var payload = TestTargetForwarderSelector_2Object;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Forwarder with overwrite Selector:string_2', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderSelector_2 ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Errors

        // it('should return status 400 when POST Selector:ARRAYSelector ', function(done) {
            // var payload = TestTargetForwarderSelector_ARRAYSelectorObject;
            // verifyConfig = JSON.parse( payload );
            // verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Selector Value: \"array\"." };
            // FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        // });
        // it('should return status 200 on GET, Forwarder without Selector:ARRAYSelector', function(done) {
            // verifyConfig = JSON.parse( TestTargetForwarderTopic_STRING_apply ) ;
            // FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        // });

        it('should return status 400 when POST Selector:boolean  ', function(done) {
            var payload = '{ "Forwarder": { "NoBoolean" :{ "Selector": true }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: Selector Value: \"true\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
       });
        it('should return status 404 on GET, without change to Selector:boolean', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Forwarder/NoBoolean"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"NoBoolean", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

       it('should return status 400 when POST with Selector:numeric', function(done) {
            var payload = '{ "Forwarder": { "TestTarget" :{ "Selector": 1 }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112", "Message": "The property value is not valid: Property: Selector Value: \"1\"." } ;
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET without change to Selector:numeric', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderSelector_2 ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });        
        
// Default 
     
        it('should return status 200 when POST Selector:null ', function(done) {
            var payload = '{ "Forwarder": { "TestTarget" :{ "Selector": null }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Selector:null ', function(done) {
//            verifyConfig = JSON.parse( '{ "TestTarget" :{ "Selector": null }}' ) ;
            verifyConfig = JSON.parse( '{ "Selector": null }' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"TestTarget" , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
		
    }); 

//  ====================  SourceQoS  ====================  //
//  TypeOf: ENUM of INTs : 0, 1, 2
     describe('SourceQoS:', function() {
// Defect 217840
        it('should return status 200 when POST SourceQoS:0 ', function(done) {
            var payload = TestTargetForwarderSourceQoS_0Object;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Forwarder with SourceQoS:0', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderSourceQoS_0 ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST SourceQoS:2 ', function(done) {
            var payload = TestTargetForwarderSourceQoS_2Object;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Forwarder with SourceQoS:2', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderSourceQoS_2 ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST SourceQoS:1 ', function(done) {
            var payload = TestTargetForwarderSourceQoS_1Object;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Forwarder with SourceQoS:1', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderSourceQoS_1 ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Errors

        it('should return status 400 when POST SourceQoS:3  ', function(done) {
            var payload = '{ "Forwarder": { "NoQoS3" :{ "SourceQoS": 3 }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: SourceQoS Value: \"3\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
       });
        it('should return status 404 on GET, without change to SourceQoS:3', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Forwarder/NoQoS3"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"NoQoS3", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 when POST SourceQoS:boolean  ', function(done) {
            var payload = '{ "Forwarder": { "NoBoolean" :{ "SourceQoS": true }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: SourceQoS Value: \"true\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
       });
        it('should return status 404 on GET, without change to SourceQoS:boolean', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Forwarder/NoBoolean"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"NoBoolean", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

       it('should return status 400 when POST with SourceQoS:string', function(done) {
            var payload = '{ "Forwarder": { "TestTarget" :{ "SourceQoS": "1" }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112", "Message": "The property value is not valid: Property: SourceQoS Value: \"1\"." } ;
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET without change to SourceQoS:string', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderSourceQoS_1 ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });        
        
// Default - SourceQoS default is 1 or NULL does not reset
// Defect 217913 - see comment in defect
        it('should return status 200 when POST SourceQoS:null ', function(done) {
            var payload = '{ "Forwarder": { "TestTarget" :{ "SourceQoS": null }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, SourceQoS:null ', function(done) {
            verifyConfig = JSON.parse( '{ "TestTarget" :{ "SourceQoS": 1 }}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
     
    }); 


//  ====================  TopicMap  ====================  //
//  TypeOf String
     describe('TopicMap:', function() {

        it('should return status 200 when POST TopicMap:string ', function(done) {
            var payload = TestTargetForwarderTopicMapObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Forwarder with TopicMap:string', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderTopicMap ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

// Errors

        it('should return status 400 when POST TopicMap:ARRAY ', function(done) {
            var payload = TestTargetForwarderTopicMap_ARRAYObject;
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: TopicMap Value: \"array\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, Forwarder without TopicMap:ARRAY', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderTopicMap ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when POST TopicMap:boolean  ', function(done) {
            var payload = '{ "Forwarder": { "NoBoolean" :{ "TopicMap": true }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0112", "Message": "The property value is not valid: Property: TopicMap Value: \"true\"." };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
       });
        it('should return status 404 on GET, without change to TopicMap:boolean', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Forwarder/NoBoolean"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"NoBoolean", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

       it('should return status 400 when POST with TopicMap:numeric', function(done) {
            var payload = '{ "Forwarder": { "TestTarget" :{ "TopicMap": 1 }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112", "Message": "The property value is not valid: Property: TopicMap Value: \"1\"." } ;
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET without change to TopicMap:numeric', function(done) {
            verifyConfig = JSON.parse( TestTargetForwarderTopicMap ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });        
        
// Default none, not required and not used if KAFKA
// Defect 217912 - NULL will mean there is no transformation... in topic = out topic 
        it('should return status 200 when POST TopicMap:null ', function(done) {
            var payload = '{ "Forwarder": { "TestTarget" :{ "TopicMap": null }}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = { "Code":"CWLNA0000", "Message": "Success" };
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain + "config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET, TopicMap:null', function(done) {
            verifyConfig = JSON.parse( '{ "TestTarget" :{ "TopicMap": null }}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject , BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
     
    }); 

    
    
//  ====================  Delete test cases  ====================  //
    describe('Delete:', function() {

        it('should return status 404 on DELETE "NotAForwarder"', function(done) {
            var payload = '{"Forwarder":{"NotAForwarder":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":404, "Code": "CWLNA0404","Message": "The HTTP request is for an object which does not exist.: Forwarder/NotAForwarder"};
            FVT.makeDeleteRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"NotAForwarder", BridgeAuth, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after DELETE; "NotAForwarder" Not Found', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Forwarder/NotAForwarder"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"NotAForwarder", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 on DELETE "TestTarget"', function(done) {
            var payload = '{"Forwarder":{"TestTarget":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA0000","Message": "Success"};
            FVT.makeDeleteRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"TestTarget", BridgeAuth, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET all after DELETE "TestTarget" gone', function(done) {
            verifyConfig = {"TestTarget":null};
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject, BridgeAuth, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on GET after DELETE; "TestTarget" Not Found', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Forwarder/TestTarget"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"TestTarget", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 404 on second DELETE "TestTarget"', function(done) {
            var payload = '{"Forwarder":{"TestTarget":null}}';
            verifyConfig = {"Code": "CWLNA0404","Message": "The HTTP request is for an object which does not exist.: Forwarder/TestTarget"};
            verifyMessage = {"Code": "CWLNA0404","Message": "The HTTP request is for an object which does not exist.: Forwarder/TestTarget"};
            FVT.makeDeleteRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"TestTarget", BridgeAuth, FVT.verify404NotFound, verifyConfig, verifyMessage, done);
        });

        // it('should return status 200 when POST NULL "TempDestination" (Bridge HAS Post DELETE)', function(done) {
            // var payload = '{"Forwarder":{"TempDestination":null}}';
            // verifyConfig = JSON.parse( payload );
            // verifyMessage = {"Code": "CWLNA0000","Message": "Success"};
            // FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+"config", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        // });
        // it('should return status 404 on GET after POST DELETE, "TempDestination" is NOT found', function(done) {
            // verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Forwarder/TempDestination"}' ) ;
            // FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"TempDestination", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        // });

    });

//  ====================  Error test cases  ====================  //
     describe('Error General:', function() {

        it('should return status 400 when POST with Unknown Property ', function(done) {
            var payload = '{"Forwarder":{ "Error_Target": {"MQTTServerList": [ "'+ FVT.A1_HOST +':1883" ] }}}';
            verifyConfig = JSON.parse( payload );
// Defect 217600
            verifyMessage = {"status":400,"Code":"CWLNA0111","Message":"The property name is not valid: Property: MQTTServerList."};
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+ "config", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET, Forwarder with Unknown Property should not be added', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Forwarder/Error_Target"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject + "Error_Target", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

// Defect 217823 - undocumented but is going to be acceptable :-/ lazy, yet URL2 is still denied ) 
        it('should return status 200 when POST with URL incorrect 1 (undocumented)', function(done) {
            var payload = TestTargetProperties;
            verifyConfig = JSON.parse( payload );
//??            verifyMessage = {"status":404,"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Forwarder/NoPostURL1"};
            verifyMessage = { "Code":"CWLNA0000","Message":"Success"};
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+ "config/Forwarder/NoPostURL1", BridgeAuth, payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET, when POST with URL incorrect 1 (undocumented)', function(done) {
            verifyConfig = JSON.parse( TestTargetProperties );
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject + "NoPostURL1", BridgeAuth, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on DELETE "NoPostURL1"', function(done) {
            var payload = '{"Forwarder":{"NoPostURL1":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA0000","Message": "Success"};
            FVT.makeDeleteRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"NoPostURL1", BridgeAuth, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after DELETE; "NoPostURL1" Not Found', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Forwarder/NoPostURL1"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject+"NoPostURL1", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });


        it('should return status 404 when POST with URL incorrect 2', function(done) {
            var payload = '{ "NoPostURL2" : '+ TestTargetProperties +'}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":404,"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Forwarder/"};
            FVT.makePostRequestWithAuthVerify( BridgeURI, FVT.uriBridgeDomain+ "config/Forwarder/", BridgeAuth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET, when POST with URL incorrect 2', function(done) {
            verifyConfig = JSON.parse( '{"Code":"CWLNA0404","Message":"The HTTP request is for an object which does not exist.: Forwarder/NoPostURL2"}' ) ;
            FVT.makeGetRequestWithAuth( BridgeURI, FVT.uriBridgeDomain+uriObject + "NoPostURL2", BridgeAuth, FVT.verify404NotFound, verifyConfig, done);
        });

    }); 


});
