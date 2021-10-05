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
var verifyConfig = {};
var verifyMessage = {};  // {"status":#,"Code":"CWLxxx","Message":"blah blah"}

var uriObject = 'ConnectionPolicy/';

var expectDefault = '{"Version": "' + FVT.version + '","ConnectionPolicy":{"DemoConnectionPolicy":{"ClientID": "*","Description": "Demo connection policy"}}}'

var expectAllConnectionPolicies = '{ "Version": "' + FVT.version + '", "ConnectionPolicy": { \
 "DemoConnectionPolicy": {  "Description": "Demo connection policy",  "ClientID": "*"  }, \
 "TestConnPol": {  "ClientID": "*",  "ClientAddress": "",  "Protocol": "",  "GroupID": "",  "Description": "The NEW TEST Connection Policy.",  "UserID": "",  "AllowDurable": true,  "CommonNames": "",  "AllowPersistentMessages": true, "ExpectedMessageRate": "Default" }, \
 "'+ FVT.long256Name +'": {  "ClientID": "",  "ClientAddress": "",  "Protocol": "MQTT,JMS",  "GroupID": "",  "Description": "",  "UserID": "",  "AllowDurable": true,  "CommonNames": "",  "AllowPersistentMessages": true, "ExpectedMessageRate": "Default", "MaxSessionExpiryInterval": 0 }}}' ;

 
// --------------------------------------------------------
//  validNameTest( instanceName )
// --------------------------------------------------------
function validNameTest( instanceName ){
    describe( 'Make ConnectionPolicy for: '+instanceName, function () {

        it('should return status 200 on POST "ConnectionPolicy for '+ instanceName +'"', function(done) {
console.log( "valid instanceName:" + instanceName + '  MakeGet will encoded name: '+ encodeURI(instanceName) );
            var payload = '{"ConnectionPolicy":{"'+ instanceName +'":{"UserID":"*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on GET "ConnectionPolicy for '+ instanceName +'"', function(done) {
//            verifyConfig = JSON.parse( '{"status":200,"ConnectionPolicy":{"'+ instanceName +'":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+instanceName, FVT.getSuccessCallback, verifyConfig, done);
        });

       it('should return status 200 when deleting '+ instanceName, function(done) {
            var payload = '{"ConnectionPolicy":{"'+ instanceName +'":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+instanceName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "'+ instanceName +'" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "'+ instanceName +'" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"ConnectionPolicy":{"'+ instanceName +'":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: '+ instanceName +'","ConnectionPolicy":{"'+ instanceName +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+instanceName, FVT.verify404NotFound, verifyConfig, done);
        });
  
    });
};    


// --------------------------------------------------------
//  validNameTestNoEncode( postInstanceName, getInstanceName )
//  uses FVT.makeGetRequestNoEncode
// --------------------------------------------------------
function validNameTestNoEncode( postInstanceName, getInstanceName ){
    describe( 'Make ConnectionPolicy for: '+postInstanceName, function () {

        it('should return status 200 on POST "ConnectionPolicy for '+ postInstanceName +'"', function(done) {
console.log( "valid instanceName:" + postInstanceName + '  MakeGetNoEncode name: '+ getInstanceName  );
            var payload = '{"ConnectionPolicy":{"'+ postInstanceName +'":{"UserID":"*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on GET "ConnectionPolicy for '+ getInstanceName +'"', function(done) {
            FVT.makeGetRequestNoEncode( FVT.uriConfigDomain+uriObject+getInstanceName, FVT.getSuccessCallback, verifyConfig, done);
        });

       it('should return status 200 when deleting '+ getInstanceName, function(done) {
            var payload = '{"ConnectionPolicy":{"'+ postInstanceName +'":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequestNoEncode( FVT.uriConfigDomain+uriObject+getInstanceName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "'+ getInstanceName +'" gone', function(done) {
            FVT.makeGetRequestNoEncode( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "'+ getInstanceName +'" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"ConnectionPolicy":{"'+ postInstanceName +'":{}}}' );
//            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: '+ getInstanceName +'","ConnectionPolicy":{"'+ postInstanceName +'":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: '+ postInstanceName +'","ConnectionPolicy":{"'+ postInstanceName +'":{}}}' );
            FVT.makeGetRequestNoEncode( FVT.uriConfigDomain+uriObject+getInstanceName, FVT.verify404NotFound, verifyConfig, done);
        });
  
    });
};    

// --------------------------------------------------------
//  invalidNameTest( instanceName, instanceStatus )
//  where :  instanceStatus( status, code, message <instanceNameMsg> )
// --------------------------------------------------------
function invalidNameTest( instanceName , instanceStatus){
    describe( 'ConnectionPolicy fail on BadName: '+instanceName +":"+ instanceStatus.instanceNameMsg, function () {
    this.timeout( 3000 );

        it('should return status 400 on POST "ConnectionPolicy" for '+ instanceName, function(done) {
            var payload = '{"ConnectionPolicy":{"'+ instanceName +'":{"UserID":"*"}}}';
console.log( "invalid instanceName:" + instanceName + '  MakeGet will encoded name: '+ encodeURI(instanceName) );
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid." };
            verifyMessage = JSON.parse( '{"status":'+ instanceStatus.status +',"Code":"'+ instanceStatus.Code +'","Message":"'+ instanceStatus.Message +'" }' );            
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET '+ instanceName +' was not created', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"ConnectionPolicy":{"'+ instanceName +'":{}}}' ) ;
            var instanceNameMsg = instanceName;
            if ( instanceStatus.instanceNameMsg !== undefined ) {
                instanceNameMsg = instanceStatus.instanceNameMsg ;
            }
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: '+ instanceNameMsg +'","ConnectionPolicy":{"'+ instanceName +'":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+instanceName, FVT.verify404NotFound, verifyConfig, done);
        });

    });
};    


// --------------------------------------------------------
//  invalidNameTestNoEncode ( postInstanceName, getInstanceName, instanceStatus )
//  uses FVT.makeGetRequestNoEncode
// --------------------------------------------------------
function invalidNameTestNoEncode( postInstanceName, getInstanceName , instanceStatus){
    describe( 'ConnectionPolicy fail on BadName: '+postInstanceName, function () {
    this.timeout( 3000 );

        it('should return status 400 on POST "ConnectionPolicy" for '+ postInstanceName, function(done) {
console.log( "invalid instanceName:" + postInstanceName + '  MakeGetNoEncode name: '+ getInstanceName );
            var payload = '{"ConnectionPolicy":{"'+ postInstanceName +'":{"UserID":"*"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid." };
            verifyMessage = JSON.parse( '{"status":'+ instanceStatus.status +',"Code":"'+ instanceStatus.Code +'","Message":"'+ instanceStatus.Message +'" }' );            
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET '+ getInstanceName +' was not created', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"ConnectionPolicy":{"'+ postInstanceName +'":{}}}' ) ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: '+ postInstanceName +'","ConnectionPolicy":{"'+ postInstanceName +'":{}}}' ) ;
            FVT.makeGetRequestNoEncode( FVT.uriConfigDomain+uriObject+getInstanceName, FVT.verify404NotFound, verifyConfig, done);
        });

    });
};    



//  ====================  Test Cases Start Here  ====================  //

describe('ConnectionPolicy:', function() {
this.timeout( FVT.defaultTimeout );

    // Get test cases
    describe('Pristine Get:', function() {
        it('should return status 200 on get, DEFAULT of "ConnectionPolicy":"DemoConnPol":{..}', function(done) {
            verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
    });

//  ====================   Create - Add - Update test cases  ====================  //
    describe('Add Update Field: Description', function() {

        it('should return status 200 on post "No description"', function(done) {
            var payload = '{"ConnectionPolicy":{"TestConnPol":{"ClientID": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "No description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Add Description"', function(done) {
            var payload = '{"ConnectionPolicy":{"TestConnPol":{"Description": "The TEST Connection Policy."}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Add Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Update Description"', function(done) {
            var payload = '{"ConnectionPolicy":{"TestConnPol":{"Description": "The NEW TEST Connection Policy."}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Update Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        // it('should return status 200 on post "Crazy!#$%&\'()*+-./:;<>?@Name"', function(done) {
            // var payload = '{"ConnectionPolicy":{"Crazy!#$%&\'()*+-./:;<>?@Name":{"ClientID": "6.9.6.9"}}}';
            // verifyConfig = JSON.parse(payload);
            // FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        // });
        // it('should return status 200 on get, "Crazy!#$%&\'()*+-./:;<>?@Name"', function(done) {
            // verifyConfig = {"status":200, "ConnectionPolicy":{"Crazy!#$%&\'()*+-./:;<>?@Name":{"ClientID": "6.9.6.9"}}} ;
            // FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"Crazy!\#$%\&'()*+-./:;<>\?@Name", FVT.getSuccessCallback, verifyConfig, done);
        // });

    });
//  ====================   Valid Range test cases  ====================  //
    describe('Name Valid Range', function() {

        it('should return status 200 on post "Max Length Name"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientID": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Name"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length ClientID"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientID": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length ClientID"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length ClientAddress":listOf'+ FVT.MAX_LISTMEMBERS, function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "' + FVT.CLIENT_ADDRESS_LIST +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length ClientAddress":listOf'+ FVT.MAX_LISTMEMBERS, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length ClientAddress":compressedListOf'+ FVT.MAX_LISTMEMBERS, function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "' + FVT.CLIENTADDRESSLIST +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length ClientAddress":compressedListOf'+ FVT.MAX_LISTMEMBERS, function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length UserID"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"UserID": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length UserID"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length GroupID"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"GroupID": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length GroupID"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length CommonNames"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"CommonNames": "' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length CommonNames"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length Protocol"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"Protocol": "MQTT,JMS"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Protocol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Max Length Description"', function(done) {
            var payload = '{"ConnectionPolicy":{"'+ FVT.long256Name +'":{"Description":"' + FVT.long1024String + '"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Max Length Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });
//  ====================   Reset to Default test cases  ====================  //
    describe('Reset:', function() {
//97501 (and many more to follow)
        it('should return status 200 on post "Reset ClientID"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientID": null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset ClientID"', function(done) {
            verifyConfig = JSON.parse( '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientID":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Reset ClientAddress"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset ClientAddress"', function(done) {
            verifyConfig = JSON.parse( '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientAddress":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Reset UserID"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"UserID": null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset UserID"', function(done) {
            verifyConfig = JSON.parse( '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"UserID":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Reset GroupID"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"GroupID": null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset GroupID"', function(done) {
            verifyConfig = JSON.parse( '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"GroupID":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Reset CommonNames"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"CommonNames": null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset CommonNames"', function(done) {
            verifyConfig = JSON.parse( '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"CommonNames":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Reset Protocol"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"Protocol": null,"ClientID":"*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset Protocol"', function(done) {
            verifyConfig = JSON.parse( '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"Protocol":"","ClientID":"*"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Reset Description"', function(done) {
            var payload = '{"ConnectionPolicy":{"'+ FVT.long256Name +'":{"Description":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Reset Description"', function(done) {
            verifyConfig = JSON.parse( '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"Description":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });
//  ====================   Must have One Of test cases  ====================  //
    describe('MustHave:', function() {
// 113148
        it('should return status 200 on post "Only ClientAddress":ipv4', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer +'","ClientID":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only ClientAddress":ipv4', function(done) {
            verifyConfig = JSON.parse( '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientID":"","ClientAddress":"'+ FVT.msgServer +'","UserID":"","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only ClientAddress":ipv6', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer_IPv6 +'","ClientID":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only ClientAddress":ipv6', function(done) {
            verifyConfig = JSON.parse( '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientID":"","ClientAddress":"'+ FVT.msgServer_IPv6 +'","UserID":"","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only ClientAddress":ipv6,ipv4', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientAddress": "'+ FVT.msgServer_IPv6 +','+ FVT.msgServer +'","ClientID":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only ClientAddress":ipv6,ipv4', function(done) {
            verifyConfig = JSON.parse( '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientID":"","ClientAddress":"'+ FVT.msgServer_IPv6 +','+ FVT.msgServer +'","UserID":"","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only UID"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"UserID": "'+ FVT.long1024String +'","ClientAddress":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only UID"', function(done) {
            verifyConfig = JSON.parse( '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientID":"","ClientAddress":"","UserID":"'+ FVT.long1024String +'","GroupID":"","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only GID"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"GroupID": "'+ FVT.long1024String +'","UserID":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only GID"', function(done) {
            verifyConfig = JSON.parse( '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientID":"","ClientAddress":"","UserID":"","GroupID":"'+ FVT.long1024String +'","CommonNames":"","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only CommonNames"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"CommonNames": "'+ FVT.long1024String +'","GroupID":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only CommonNames"', function(done) {
            verifyConfig = JSON.parse( '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientID":"","ClientAddress":"","UserID":"","GroupID":"","CommonNames":"'+ FVT.long1024String +'","Protocol":""}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Only Protocol"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"Protocol": "MQTT,JMS","CommonNames":null}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Only Protocol"', function(done) {
            verifyConfig = JSON.parse( '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ClientID":"","ClientAddress":"","UserID":"","GroupID":"","CommonNames":"","Protocol":"MQTT,JMS"}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });
//  ====================   AllowDurable test cases  ====================  //
    describe('AllowDurable:', function() {
//97161 (and many more to follow)
        it('should return status 200 on Post "Allow Durable false"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"AllowDurable":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Allow Durable false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Allow Durable true"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"AllowDurable":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Allow Durable true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Allow Durable false"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"AllowDurable":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Allow Durable false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Allow Durable true(Default)"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"AllowDurable":null}}}';
            verifyConfig = JSON.parse( '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"AllowDurable":true}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Allow Durable true(Default)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });
//  ====================   AllowPersistent test cases  ====================  //
    describe('AllowPersistent:', function() {

        it('should return status 200 on post "Allow Persistent Messages false"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"AllowPersistentMessages":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Allow Persistent Messages false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Allow Persistent Messages true"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"AllowPersistentMessages":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Allow Persistent Messages true"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Allow Persistent Messages false"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"AllowPersistentMessages":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Allow Persistent Messages false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Allow Persistent Messages true(Default)"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"AllowPersistentMessages":null}}}';
            verifyConfig = JSON.parse( '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"AllowPersistentMessages":true}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Allow Persistent Messages true(Default)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });
    
//  ====================   ExpectedMessageRate test cases  ====================  //
    describe('ExpectedMessageRate:', function() {

        it('should return status 200 on post "ExpectedMessageRate Low"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ExpectedMessageRate":"Low"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ExpectedMessageRate Low"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "ExpectedMessageRate High"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ExpectedMessageRate":"High"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ExpectedMessageRate High"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 200 on post "ExpectedMessageRate Max"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ExpectedMessageRate":"Max"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ExpectedMessageRate Max"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });


        it('should return status 200 on post "ExpectedMessageRate (Default)"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ExpectedMessageRate":null}}}';
            verifyConfig = JSON.parse( '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"ExpectedMessageRate":"Default"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ExpectedMessageRate (Default)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

//  ====================   MaxSessionExpiryInterval test cases  ====================  //
    describe('MaxSessionExpiryInterval:', function() {
    	
    	it('should return staus 200 on post "MaxSessionExpiryInterval = 100"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"MaxSessionExpiryInterval": 100}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
    
        it('should return status 200 on get, "MaxSessionExpiryInterval = 100"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    	it('should return staus 200 on post "MaxSessionExpiryInterval = Default Value (0)"', function(done) {
            var payload = '{"ConnectionPolicy":{"' + FVT.long256Name + '":{"MaxSessionExpiryInterval": 0}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
    
        it('should return status 200 on get, "MaxSessionExpiryInterval = Default Value (0)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
    });

    describe('Config Persists:', function() {
    
        it('should return status 200 POST restart Server', function(done) {
            var payload = '{"Service":"Server"}';
            var verifyPayload = JSON.parse( payload ) ;
            verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyPayload, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart with payload', function(done) {
            this.timeout( FVT.REBOOT + 5000 ) ; 
            FVT.sleep( FVT.REBOOT ) ;
            var verifyStatus = JSON.parse( FVT.expectDefaultStatus ) ;
//            FVT.verifyServerRestart( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
            FVT.makeGetRequest( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
        });
        it('should return status 200 on get, "ConnectionPolicies" persists', function(done) {
            verifyConfig = JSON.parse( expectAllConnectionPolicies ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });


//  ====================  Delete test cases  ====================  //
    describe('Delete:', function() {

        it('should return status 200 when deleting "Max Length Name"', function(done) {
            var payload = '{"ConnectionPolicy":{"'+ FVT.long256Name +'":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "Max Length Name" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "Max Length Name" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"ConnectionPolicy":{"'+ FVT.long256Name +'":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: '+ FVT.long256Name +'","ConnectionPolicy":{"'+ FVT.long256Name +'":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

        // it('should return status 200 when deleting "Crazy!#$%&\'()*+-./:;<>?@Name"', function(done) {
            // var payload = '{"ConnectionPolicy":{"Crazy!#$%&\'()*+-./:;<>?@Name":null}}';
            // verifyConfig = JSON.parse( payload );
            // verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            // FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+"Crazy!#$%&\'()*+-./:;<>?@Name", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        // });
        // it('should return status 200 on get all after delete, "Crazy!#$%&\'()*+-./:;<>?@Name" gone', function(done) {
            // FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        // });
        // it('should return status 404 on get after delete, "Crazy!#$%&\'()*+-./:;<>?@Name" not found', function(done) {
            // verifyConfig = JSON.parse( '{"status":404,"ConnectionPolicy":{"Crazy!#$%&\'()*+-./:;<>?@Name":{}}}' );
            // FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"Crazy!#$%&\'()*+-./:;<>?@Name", FVT.verify404NotFound, verifyConfig, done);
        // });
//96657 NO POST DELETE 9/2015
        it('should return status 400 when POST NULL "TestConnPol" ( No Post DELETE)', function(done) {
            var payload = '{"ConnectionPolicy":{"TestConnPol":null}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Status":200,"Code": "CWLNA0137","Message": "The requested configuration change has completed successfully."};
//            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The RESTful request: \"TestConnPol\":null is not valid."};
            verifyMessage = {"status":400,"Code": "CWLNA0137","Message": "The REST API call: \"TestConnPol\":null is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get after POST DELETE, "TestConnPol" is STILL found', function(done) {
            verifyConfig = {"ConnectionPolicy":{"TestConnPol":{"ClientID": "*","CommonNames": "", "UserID": "", "Protocol": "", "AllowPersistentMessages": true, "Description": "The NEW TEST Connection Policy.", "ClientAddress": "", "GroupID": "", "AllowDurable": true}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestConnPol', FVT.getSuccessCallback, verifyConfig, done);
        });
//        it('should return status 200 on get after POST DELETE, "TestConnPol"  gone', function(done) {
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
//        });
//        it('should return status 404 on get after POST DELETE, "TestConnPol" is found', function(done) {
//            verifyConfig = {"status":404,"ConnectionPolicy":{"TestConnPol":{}}} ;
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestConnPol', FVT.verify404NotFound, verifyConfig, done);
//        });
//  So until POST DELETE works, have to regular delete
        it('should return status 200 when deleting TestConnPol', function(done) {
            var payload = '{"ConnectionPolicy":{"TestConnPol":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'TestConnPol', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, TestConnPol gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, TestConnPol not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"ConnectionPolicy":{"TestConnPol":{}}}' );
//            verifyConfig = JSON.parse( '{"status":404,"ConnectionPolicy":{"TestConnPol":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: TestConnPol","ConnectionPolicy":{"TestConnPol":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestConnPol', FVT.verify404NotFound, verifyConfig, done);
        });
        
    });

//  ====================  Error test cases  ====================  //
    describe('Error General:', function() {

        it('should return status 400 when trying to Delete All Connection Policies, this is just bad form', function(done) {
            var payload = '{"ConnectionPolicy":null}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: {\"ConnectionPolicy\":null} is not valid." };
//            verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: \"ConnectionPolicy\":null is not valid." };
            verifyMessage = {"Code":"CWLNA0137","Message":"The REST API call: \"ConnectionPolicy\":null is not valid." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, Delete All Connection Policies, a Policy should be in use', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    });


//  ====================  Error test cases  ====================  //
    describe('Description:', function() {
//97262
        it('should return status 400 when trying to POST DescTOOLong', function(done) {
            var payload = '{"ConnectionPolicy":{"DescriptionTooLong":{"Description":"'+ FVT.long1025String +'","ClientID": "*" }}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Description Value: \\\"'+ FVT.long1025String +'\\\"." }' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: ConnectionPolicy Property: Description Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, DescTooLong was not created', function(done) {
//            verifyConfig = {"status":404,"ConnectionPolicy":{"DescriptionTooLong":{}}} ;
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: DescriptionTooLong","ConnectionPolicy":{"DescriptionTooLong":{}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'DescriptionTooLong', FVT.verify404NotFound, verifyConfig, done);
        });
        
    });
	
    describe('ClientAddress:', function() {

        it('should return status 400 when trying to POST ClientAddressListTooLong', function(done) {
            var payload = '{"ConnectionPolicy":{"ClientAddressListTooLong":{"ClientAddress":"'+ FVT.CLIENTADDRESSLIST + ', ' + FVT.IP_ADDRESS_STEM + (FVT.MAX_LISTMEMBERS + 1) +'","ClientID": "*" }}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: ConnectionPolicy Property: ClientAddress Value: '+ FVT.CLIENTADDRESSLIST + ', ' + FVT.IP_ADDRESS_STEM + (FVT.MAX_LISTMEMBERS + 1) +'." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0371","Message":"The number of client addresses exceeds the maximum number allowed: 100." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, ClientAddressListTooLong was not created', function(done) {
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: ClientAddressListTooLong","ConnectionPolicy":{"ClientAddressListTooLong":{}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'ClientAddressListTooLong', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 when trying to POST ClientAddressListTooLong', function(done) {
            var payload = '{"ConnectionPolicy":{"ClientAddressRangeDescending":{"ClientAddress":"10.10.10.10-10.10.10.10","ClientID": "*" }}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: \"10.10.10.10\"."} ;
            verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: \"10.10.10.10-10.10.10.10\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, ClientAddressListTooLong was not created', function(done) {
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: ClientAddressRangeDescending","ConnectionPolicy":{"ClientAddressRangeDescending":{}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'ClientAddressRangeDescending', FVT.verify404NotFound, verifyConfig, done);
        });
        
    });


//  ====================  Error test cases  ====================  //
    describe('Error Name:', function() {
// 97601
        it('should return status 400 when trying to POST NameTOOLong', function(done) {
            var payload = '{"ConnectionPolicy":{"'+ FVT.long257Name +'":{"ClientID": "*"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Name Value: '+ FVT.long257Name +'." }' );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: Name Name: '+ FVT.long257Name +'." }' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: ConnectionPolicy Property: Name Value: '+ FVT.long257Name +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, NameTooLong was not created', function(done) {
//            verifyConfig = {"status":404,"ConnectionPolicy":{"'+ FVT.long257Name +'":{}}} ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: '+ FVT.long257Name +'","ConnectionPolicy":{"'+ FVT.long257Name +'":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long257Name, FVT.verify404NotFound, verifyConfig, done);
        });


// 108767    
        it('should return status 400 when trying to POST NO"quotes"AtAll', function(done) {
//            var payload = '{"ConnectionPolicy":{"NO\"quotes\"AtAll":{"ClientID": "*"}}}';
            var payload = '{"ConnectionPolicy":{"NO\\\u0022quotes\\\u0022AtAll":{"ClientID": "*"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, GET NO"quotes"AtAll was not created', function(done) {
//            verifyConfig = {"status":404,"ConnectionPolicy":{"NO\"quotes\"AtAll":{}}} ;
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: NO\"quotes\"AtAll","ConnectionPolicy":{"NO\"quotes\"AtAll":{}}} ;
            FVT.makeGetRequestNoEncode( FVT.uriConfigDomain+uriObject+'NO%22quotes%22AtAll', FVT.verify404NotFound, verifyConfig, done);
        });



//Name Rules HELP:
// The name must not have leading or trailing spaces and cannot contain control characters, commas, double quotation marks, backslashes or equal signs. 
// The first character must not be a number, quotation mark, or any of the following special characters: ! # $ % & ( ) * + , - . / : ; < = > ? @
        var usualInvalidMessage = {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid." };
        invalidNameTest( ' NotFirstChar-20-Space', usualInvalidMessage );
        invalidNameTest( '\u0020NotFirstChar-20-Space', usualInvalidMessage );
        invalidNameTest( 'NotLastChar-20-Space ', {"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: NotLastChar-20-Space ." } );
        invalidNameTest( 'NotLastChar-20-Space\u0020', {"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: NotLastChar-20-Space ." } );
        invalidNameTest( '!NotFirstChar-21-Exclaim', usualInvalidMessage );
        invalidNameTest( '\u0021NotFirstChar-21-Exclaim', usualInvalidMessage );
//  UNABLE to Parse these characters no matter how you try to escape :  \u0022 DoubleQuote, \u0023 Pound, \u0027 Apostrophe
//  Using %NN UTF-8 hex syntax in PAYLOAD does not work either...
// for example: %22NotFirst in PAYLOAD is a %, 2, 2, N..., so this fails cause of % [x'25] is invalid, not that %22 is invalid
//        invalidNameTest( '"NotFirstChar-22-DoubleQuote', usualInvalidMessage );    //# can not send '"' without Parsing error on handling of Quote mark interpretation
//        invalidNameTest( '%22NotFirstChar-22-pDoubleQuote', usualInvalidMessage );  //#  %22Not in PAYLOAD is a %, 2, 2, N..., so this fails cause of % x'25 is invalid 
//        invalidNameTest( '\u0022NotFirstChar-22-uDoubleQuote', usualInvalidMessage );  //# \\\u0022 fails - PARSE fails no matter how you try to send
        invalidNameTestNoEncode( '\\\u0022NotFirstChar-22-suDoubleQuote', '\\\u0022NotFirstChar-22-suDoubleQuote', usualInvalidMessage );  //# \\\u0022 fails - PARSE fails no matter how you try to send
//        invalidNameTest( '#NotFirstChar-23-Pound', usualInvalidMessage );    //# problem with GET and # being a terminator of URI and GET ALL done
        invalidNameTest( '%23NotFirstChar-23-pPound', usualInvalidMessage ); 
//        invalidNameTest( '\u0023NotFirstChar-23-uPound', usualInvalidMessage ); //# problem with GET,DELETE and # being a terminator of URI and GET ALL done
        
        invalidNameTest( '$NotFirstChar-24-Dollar', usualInvalidMessage );
        invalidNameTest( '\u0024NotFirstChar-24-Dollar', usualInvalidMessage );
        invalidNameTest( '&NotFirstChar-26-Amperstand', usualInvalidMessage );
        invalidNameTest( '\u0026NotFirstChar-26-Amperstand', usualInvalidMessage );

//        invalidNameTest( "\\'NotFirstChar-27-Apostrophe", usualInvalidMessage );  //# problem with Parsing "'", not send no matter how escaped
        invalidNameTest( "\u0027NotFirstChar-27-Apostrophe", usualInvalidMessage ); 
        
        invalidNameTest( '(NotFirstChar-28-OpenParen', usualInvalidMessage );
        invalidNameTest( ')NotFirstChar-29-CloseParen', usualInvalidMessage );
        invalidNameTest( '*NotFirstChar-2a-Asterick', usualInvalidMessage );
        invalidNameTest( '+NotFirstChar-2b-Plus', usualInvalidMessage );
        invalidNameTest( ',NotFirstChar-2c-Comma', usualInvalidMessage );
        invalidNameTest( '-NotFirstChar-2d-Minus', usualInvalidMessage );
        invalidNameTest( '.NotFirstChar-2e-Period', usualInvalidMessage );
        invalidNameTest( '/NotFirstChar-2f-FwdSlash',  {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: /NotFirstChar-2f-FwdSlash.", "instanceNameMsg":"NotFirstChar-2f-FwdSlash" } );
        invalidNameTest( '\u002fNotFirstChar-u2f-FwdSlash',  {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: /NotFirstChar-u2f-FwdSlash.", "instanceNameMsg":"NotFirstChar-u2f-FwdSlash" } );
        invalidNameTest( '0NotFirstChar-30-Zero', usualInvalidMessage );
        invalidNameTest( '1NotFirstChar-31-One', usualInvalidMessage );
        invalidNameTest( '5NotFirstChar-35-Five', usualInvalidMessage );
        invalidNameTest( '9NotFirstChar-39-Nine', usualInvalidMessage );
        invalidNameTest( ':NotFirstChar-3a-Colon', usualInvalidMessage );
        invalidNameTest( ';NotFirstChar-3b-SemiColon', usualInvalidMessage );
        invalidNameTest( '<NotFirstChar-3c-LessThan', usualInvalidMessage );
        invalidNameTest( '=NotFirstChar-3d-EqualsSign', usualInvalidMessage );
        invalidNameTest( '>NotFirstChar-3e-GreaterThan', usualInvalidMessage );

//        invalidNameTest( '?NotFirstChar-3f-QuestionMark', usualInvalidMessage );  //# problem with GET and ? being an ignored QueryParameter of URI so GET ALL was run
        invalidNameTest( '%3fNotFirstChar-3f-pQuestionMark', usualInvalidMessage );  
//        invalidNameTest( '?NotFirstChar-3f-uQuestionMark', usualInvalidMessage );   //# problem with GET and ? being an ignored QueryParameter of URI so GET ALL was run

        invalidNameTest( '@NotFirstChar-40-AtSign', usualInvalidMessage );
        invalidNameTest( '\\\\NotFirstChar-5c-BackSlash',  {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid.", "instanceNameMsg":"\\\\\\\\NotFirstChar-5c-BackSlash" } );
        invalidNameTest( '\\\\\\\\NotFirstChar-5c-BackSlash8', {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid.", "instanceNameMsg":"\\\\\\\\\\\\\\\\NotFirstChar-5c-BackSlash8" } );
        
// =========  Special Chars in the MIDDLE  ================== //
// Never Valid:  Comma, Doucle Quotes, Backslash and Equals Sign 
// ========================================================== //
        validNameTest( 'OK- -20-Space' );        
        validNameTest( 'OK-!-21-Exclaim');        
//        invalidNameTest( 'NotOK-\"-22-DoubleQuote', usualInvalidMessage);    //# Problem parsing '"' no matter how escaped
        invalidNameTest( 'NotOK-\\"-22-DoubleQuote', {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid.", "instanceNameMsg":"NotOK-\\\\\\\"-22-DoubleQuote" } );    //# Problem parsing '"' no matter how escaped
        invalidNameTest( 'NotOK-\\\u0022-22-uDoubleQuote', {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid.", "instanceNameMsg":"NotOK-\\\\\\\"-22-uDoubleQuote" } );        
        
//  Difficult to Parse these characters no matter how you try to encode and/or escape :  \u0023 Pound, \u003f QuestionMark
//  NOT successful in a browser:       /ima/v1/configuration/ConnectionPolicy/OK-\u0023-23-uPound, /ima/v1/configuration/ConnectionPolicy/OK-#-23-uPound
//  This is successful in a browser:   /ima/v1/configuration/ConnectionPolicy/OK-%23-23-uPound
//  Have to special handle to switch between the (direct) character and the UTF-8 encoding
//        validNameTest( 'OK-#-23-Pound');     //# GET,DELETE URI must have # be %23 or else # is a terminator of the URI, HASHBANG for Ajax crawling in SearchEngines
//        validNameTest( 'OK-%23-23-Pound');   // # sadly, %23 will be encoded by MakeGet to %2523 in validNameTest encodeURI()
//        validNameTest( 'OK-\u0023-23-Pound');   // # I need a test that does not use encodeURI()
//jcd        validNameTestNoEncode( 'OK-#-23-Pound',    'OK-#-23-Pound');      // # POST ok, GET/DELETE fail 404, Browser as %23 is successful
//jcd        validNameTestNoEncode( 'OK-#-23-sPound',   'OK-\#-23-sPound');    // # POST ok, GET/DELETE fail 404, Browser as %23 is successful
//jcd        validNameTestNoEncode( 'OK-#-23-ssPound',  'OK-\\#-23-ssPound');  // # POST ok, GET/DELETE fail 404, Browser as %23 is successful
        validNameTestNoEncode( 'OK-#-23-pPound',       'OK-%23-23-pPound'); 
        validNameTestNoEncode( 'OK-\u0023-23-upPound', 'OK-%23-23-upPound');   
        validNameTestNoEncode( 'OK-#-23-spPound',      'OK-\%23-23-spPound');
        validNameTestNoEncode( 'OK-\u0023-23-uspPound','OK-\%23-23-uspPound');  
//jcd        validNameTestNoEncode( 'OK-#-23-sspPound', 'OK-\\%23-23-sspPound');     // # POST ok, GET/DELETE fail 404, Browser as %23 is successful
//jcd        validNameTestNoEncode( 'OK-#-23-uPound',   'OK-\u0023-23-uPound');      // # POST ok, GET/DELETE fail 404, Browser as %23 is successful
//jcd        validNameTestNoEncode( 'OK-#-23-ssuPound', 'OK-\\\u0023-23-ssuPound');  // # POST ok, GET/DELETE fail 404, Browser as %23 is successful

        validNameTest( 'OK-$-24-Dollar');        
        validNameTest( 'OK-%-25-Percent');        
        validNameTest( 'OK-&-26-Ampersand');        
        validNameTest( "OK-'-27-Apostrophe");        
        validNameTest( 'OK-(-28-OpenParen');        
        validNameTest( 'OK-)-29-CloseParen');        
        validNameTest( 'OK-*-2a-Asterisk');        
        validNameTest( 'OK-+-2b-Plus');        
        invalidNameTest( 'NotOK-,-2c-Comma', usualInvalidMessage);        
        invalidNameTest( 'NotOK-\u002c-2c-uComma', usualInvalidMessage);        
        validNameTest( 'OK---2d-Minus');        
        validNameTest( 'OK-.-2e-Period');        
//        validNameTest( 'OK-/-2f-FwdSlash');         //  CAN NOT DELETE this
        validNameTest( 'OK-2-32-Two' );
        validNameTest( 'OK-:-3a-Colon');        
        validNameTest( 'OK-;-3b-SemiColon');        
        validNameTest( 'OK-<-3c-LessThan');        
        invalidNameTest( 'NotOK-=-3d-EqualSign', usualInvalidMessage);        
        invalidNameTest( 'NotOK-\u003d-3d-uEqualSign', usualInvalidMessage);        
        validNameTest( 'OK->-3e-GreaterThan');        
//      validNameTest( 'OK-?-3f-QuestionMark');   //# problem with GET,DELETE and ? being an ignored QueryParameter of URI so GET ALL was run
// 109189
//jcd        validNameTestNoEncode( 'OK-?-3f-QuestionMark',   'OK-?-3f-QuestionMark');      // # POST ok, GET/DELETE fail 404, Browser as %23 is successful
//jcd        validNameTestNoEncode( 'OK-?-3f-sQuestionMark',  'OK-\?-3f-sQuestionMark');    // # POST ok, GET/DELETE fail 404, Browser as %23 is successful
//jcd        validNameTestNoEncode( 'OK-?-3f-ssQuestionMark', 'OK-\\?-3f-ssQuestionMark');  // # POST ok, GET/DELETE fail 404, Browser as %23 is successful
        validNameTestNoEncode( 'OK-?-3f-pQuestionMark',        'OK-%3f-3f-pQuestionMark'); 
        validNameTestNoEncode( 'OK-\u003f-3f-upQuestionMark',  'OK-%3f-3f-upQuestionMark'); 
        validNameTestNoEncode( 'OK-?-3f-spQuestionMark',       'OK-\%3f-3f-spQuestionMark'); 
        validNameTestNoEncode( 'OK-\u003f-3f-uspQuestionMark', 'OK-\%3f-3f-uspQuestionMark'); 
//jcd        validNameTestNoEncode( 'OK-?-3f-sspQuestionMark', 'OK-\\%3f-3f-sspQuestionMark');   // # POST ok, GET/DELETE fail 404, Browser as %23 is successful
//jcd        validNameTestNoEncode( 'OK-?-3f-uQuestionMark',   'OK-\u003f-3f-uQuestionMark');    // # POST ok, GET/DELETE fail 404, Browser as %23 is successful
//jcd        validNameTestNoEncode( 'OK-?-3f-suQuestionMark',  'OK-\\\u003f-3f-suQuestionMark'); // # POST ok, GET/DELETE fail 404, Browser as %23 is successful
        validNameTest( 'OK-@-40-AtSign');        
// 108767
//        invalidNameTest( 'NotOK-\\-5c-BackSlash', usualInvalidMessage);    //# problem parsing '/'  - next is the only way to escape 4 slashes
        invalidNameTest( 'NotOK-\\\\-5c-BackSlash',  {"status":400,"Code":"CWLNA0122","Message":"The Unicode value is not valid.", "instanceNameMsg":"NotOK-\\\\\\\\-5c-BackSlash" } );        
    });

//  ====================  Error test cases  ====================  //
    describe('Error Protocol:', function() {
    
        it('should return status 400 when trying to POST Invalid Protocol SCADA', function(done) {
            var payload = '{"ConnectionPolicy":{"BadProtocol":{"Protocol":"MQTT,JMS,SCADA","ClientID": "*"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Protocol Value: \"MQTT,JMS,SCADA\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, BadProtocol was not created', function(done) {
//            verifyConfig = {"status":404,"ConnectionPolicy":{"BadProtocol":{}}} ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: BadProtocol","ConnectionPolicy":{"BadProtocol":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'BadProtocol', FVT.verify404NotFound, verifyConfig, done);
        });

    });

    //  ====================  Error test cases  ====================  //
    describe('Error CommonNames:', function() {
//97712
        it('should return status 400 on post "CommonName(s) is misspelled"', function(done) {
            var payload = '{"ConnectionPolicy":{"BadParamCommonName":{"CommonName": "Missing_S_inNames"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":404,"Code":"CWLNA0115","Message":"An argument is not valid: Name: CommonName."};
//            verifyMessage = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: CommonName."};
//            verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: ConnectionPolicy Name: BadParamCommonName Property: CommonName"};
            verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: ConnectionPolicy Name: BadParamCommonName Property: CommonName"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, "BadParamCommonName" did not create', function(done) {
//            verifyConfig = {"status":404,"ConnectionPolicy":{"BadParamCommonName":{}}} ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: BadParamCommonName","ConnectionPolicy":{"BadParamCommonName":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'BadParamCommonName', FVT.verify404NotFound, verifyConfig, done);
        });

        
    });

//  ====================  Error test cases  ====================  //
    describe('Error AllowDurable:', function() {

    it('should return status 400 on Post "Allow Durable "FALSE" string', function(done) {
            var payload = '{"ConnectionPolicy":{"BooleanNotASTRING":{"AllowDurable":"FALSE","ClientID":"*"}}}';
            verifyConfig = JSON.parse(payload);
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property option is invalid. Property: ConnectionPolicy Option: AllowDurable Value: InvalidType."};
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: ConnectionPolicy Name: BooleanNotASTRING Property: AllowDurable Type: JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, "BooleanNotASTRING" did not create', function(done) {
//            verifyConfig = {"status":404,"ConnectionPolicy":{"BooleanNotASTRING":{}}} ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: BooleanNotASTRING","ConnectionPolicy":{"BooleanNotASTRING":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'BooleanNotASTRING', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on Post "AllowDurable":0 ', function(done) {
            var payload = '{"ConnectionPolicy":{"BooleanNotANumber":{"AllowDurable":0,"ClientID":"*"}}}';
            verifyConfig = JSON.parse(payload);
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property option is invalid. Property: ConnectionPolicy Option: AllowDurable Value: InvalidType."};
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: ConnectionPolicy Name: BooleanNotANumber Property: AllowDurable Type: JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, "BooleanNotANumber" did not create', function(done) {
//            verifyConfig = {"status":404,"ConnectionPolicy":{"BooleanNotANumber":{}}} ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: BooleanNotANumber","ConnectionPolicy":{"BooleanNotANumber":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'BooleanNotANumber', FVT.verify404NotFound, verifyConfig, done);
        });
        
    });

//  ====================  Error test cases  ====================  //
    describe('Error AllowPersistentMessages:', function() {

        it('should return status 400 on Post "AllowPersistentMessages "true" string', function(done) {
            var payload = '{"ConnectionPolicy":{"BooleanNotastring":{"AllowPersistentMessages":"true","ClientID":"*"}}}';
            verifyConfig = JSON.parse(payload);
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property option is invalid. Property: ConnectionPolicy Option: AllowPersistentMessages Value: InvalidType."};
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: ConnectionPolicy Name: BooleanNotastring Property: AllowPersistentMessages Type: JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, "BooleanNotastring" did not create', function(done) {
//            verifyConfig = {"status":404,"ConnectionPolicy":{"BooleanNotastring":{}}} ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: BooleanNotastring","ConnectionPolicy":{"BooleanNotastring":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'BooleanNotastring', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on Post "AllowPersistentMessages":0 ', function(done) {
            var payload = '{"ConnectionPolicy":{"BooleanNotANumber":{"AllowPersistentMessages":0,"ClientID":"*"}}}';
            verifyConfig = JSON.parse(payload);
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property option is invalid. Property: ConnectionPolicy Option: AllowPersistentMessages Value: InvalidType."};
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: ConnectionPolicy Name: BooleanNotANumber Property: AllowPersistentMessages Type: JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, "BooleanNotANumber" did not create', function(done) {
//            verifyConfig = {"status":404,"ConnectionPolicy":{"BooleanNotANumber":{}}} ;
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: BooleanNotANumber","ConnectionPolicy":{"BooleanNotANumber":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'BooleanNotANumber', FVT.verify404NotFound, verifyConfig, done);
        });

    });
    
//  ====================  Error test cases  ====================  //
    describe('Error ExpectedMessageRate:', function() {

        it('should return status 400 on Post "ExpectedMessageRate"  invalid option string ', function(done) {
            var payload = '{"ConnectionPolicy":{"InvalidOptionString":{"ExpectedMessageRate":"Medium","ClientID":"*"}}}';
            verifyConfig = JSON.parse(payload);
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: ExpectedMessageRate Value: \"Medium\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, "InvalidOptionString" did not create', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: InvalidOptionString","ConnectionPolicy":{"InvalidOptionString":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'InvalidOptionString', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on Post "ExpectedMessageRate":0 ', function(done) {
            var payload = '{"ConnectionPolicy":{"EnumStringNotANumber":{"ExpectedMessageRate":0,"ClientID":"*"}}}';
            verifyConfig = JSON.parse(payload);
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: ConnectionPolicy Name: EnumStringNotANumber Property: ExpectedMessageRate Type: JSON_INTEGER"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, "EnumStringNotANumber" did not create', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: EnumStringNotANumber","ConnectionPolicy":{"EnumStringNotANumber":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'EnumStringNotANumber', FVT.verify404NotFound, verifyConfig, done);
        });

    });
    
    describe('Error MaxSessionExpiryInterval:', function() {

        it('should return status 400 on Post "MaxSessionExpiryInterval"  invalid number ', function(done) {
            var payload = '{"ConnectionPolicy":{"InvalidNumber":{"MaxSessionExpiryInterval": -1,"ClientID":"*"}}}';
            verifyConfig = JSON.parse(payload);
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MaxSessionExpiryInterval Value: \"-1\"."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, "InvalidNumber" did not create', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: InvalidNumber","ConnectionPolicy":{"InvalidNumber":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'InvalidNumber', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on Post "MaxSessionExpiryInterval":"0" ', function(done) {
            var payload = '{"ConnectionPolicy":{"StringNotANumber":{"MaxSessionExpiryInterval":"0","ClientID":"*"}}}';
            verifyConfig = JSON.parse(payload);
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: ConnectionPolicy Name: StringNotANumber Property: MaxSessionExpiryInterval Type: JSON_STRING"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, "StringNotANumber" did not create', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConnectionPolicy Name: StringNotANumber","ConnectionPolicy":{"StringNotANumber":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'StringNotANumber', FVT.verify404NotFound, verifyConfig, done);
        });

    });
});
