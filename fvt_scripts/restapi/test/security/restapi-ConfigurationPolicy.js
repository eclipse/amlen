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
var fs = require('fs')

var FVT = require('../restapi-CommonFunctions.js')

var uriObject = 'ConfigurationPolicy/' ;
var expectDefault = '{"ConfigurationPolicy":{"AdminDefaultConfigPolicy":{"ActionList": "Configure,View,Monitor,Manage","ClientAddress": "*","Description": "Default configuration policy for AdminEndpoint"}},"Version": "'+ FVT.version +'"}' ;

var expectAllConfigurationPolicies = '{ "Version": "v1", "ConfigurationPolicy": { \
 "AdminDefaultConfigPolicy": {  "Description": "Default configuration policy for AdminEndpoint",  "ClientAddress": "*",  "ActionList": "Configure,View,Monitor,Manage"  },\
 "TestConfigPol": {  "ActionList": "Configure,View,Monitor,Manage",  "ClientAddress": "*",  "UserID": "",  "Description": "Few less chars...",  "GroupID": "",  "CommonNames": ""  }, \
 "ConfigureConfigPol": {  "ActionList": "Configure",  "UserID": "*",  "ClientAddress": "",  "Description": "",  "GroupID": "",  "CommonNames": ""  }, \
 "ViewConfigPol": {  "ActionList": "View",  "GroupID": "*",  "ClientAddress": "",  "UserID": "",  "Description": "",  "CommonNames": ""  }, \
 "MonitorConfigPol": {  "ActionList": "Monitor",  "CommonNames": "*",  "ClientAddress": "",  "UserID": "",  "Description": "",  "GroupID": ""  }, \
 "ManageConfigPol": {  "ActionList": "Manage",  "ClientAddress": "*",  "UserID": "c01*",  "Description": "",  "GroupID": "Cars",  "CommonNames": "c*"  }, \
 "'+ FVT.long256Name +'": {  "ActionList": "Configure,View,Monitor,Manage",  "ClientAddress": "127.0.0.1",  "UserID": "",  "Description": "",  "GroupID": "",  "CommonNames": ""  }, \
 "CoViMoMaConfigPol": {  "ActionList": "Configure,View,Manage,Monitor",  "ClientAddress": "",  "UserID": "*",  "Description": "",  "GroupID": "",  "CommonNames": "" }}}' ;


var verifyConfig = {};
var verifyMessage = {};

//  ====================  Test Cases Start Here  ====================  //
//      "ListObjects":"Name,Description,ClientAddress,UserID,GroupID,CommonNames,ActionList",

describe('ConfigurationPolicy:', function() {
this.timeout( FVT.defaultTimeout );

    //  ====================   Get test cases  ====================  //
    describe('Get: Pristine State:', function() {
        it('should return status 200 on get, DEFAULT of "ConfigPolicy":{"AdminPolicy":{}}}', function(done) {
            verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
    });

    //  ====================   Create - Add - Update test cases  ====================  //

    describe('Name[](256)R and ActionList[Configure,View,Monitor,Manage]R:', function() {

        it('should return status 200 on post "TestConfigPol"', function(done) {
            var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ActionList": "Configure,View,Monitor,Manage","ClientAddress": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestConfigPol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "ConfigureConfigPol"', function(done) {
            var payload = '{"ConfigurationPolicy":{"ConfigureConfigPol":{"ActionList": "Configure","UserID": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ConfigureConfigPol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "ViewConfigPol"', function(done) {
            var payload = '{"ConfigurationPolicy":{"ViewConfigPol":{"ActionList": "View","GroupID": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ViewConfigPol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "MonitorConfigPol"', function(done) {
            var payload = '{"ConfigurationPolicy":{"MonitorConfigPol":{"ActionList": "Monitor","CommonNames": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MonitorConfigPol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "ManageConfigPol"', function(done) {
            var payload = '{"ConfigurationPolicy":{"ManageConfigPol":{"ActionList": "Manage","ClientAddress": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ManageConfigPol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "MaxLength Name"', function(done) {
            var payload = '{"ConfigurationPolicy":{"'+ FVT.long256Name +'":{"ActionList": "Configure,View,Monitor,Manage","ClientAddress": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MaxLength Name"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    describe('ActionList Combos:', function() {
//          "About":"Comma-delimited list of allowed actions that the user can initiate.",

        it('should return status 200 on post "CoViMoMaConfigPol"', function(done) {
            var payload = '{"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList": "Configure","ClientAddress": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "CoViMoMaConfigPol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "CoViMoMaConfigPol" + View', function(done) {
            var payload = '{"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList": "Configure,View","ClientAddress": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "CoViMoMaConfigPol" + View', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "CoViMoMaConfigPol" - View', function(done) {
            var payload = '{"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList": "Configure","ClientAddress": "","UserID":"*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "CoViMoMaConfigPol" - View', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "CoViMoMaConfigPol" + View,Manage', function(done) {
            var payload = '{"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList": "Configure,View,Manage","UserID": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "CoViMoMaConfigPol" + View,Manage', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "CoViMoMaConfigPol" + Monitor', function(done) {
            var payload = '{"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList": "Configure,View,Manage,Monitor","UserID": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "CoViMoMaConfigPol" + Monitor', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "CoViMoMaConfigPol" - View,Monitor', function(done) {
            var payload = '{"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList": "Configure,Manage","UserID": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "CoViMoMaConfigPol" - View,Monitor', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "CoViMoMaConfigPol" - Manage + Montior', function(done) {
            var payload = '{"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList": "Configure,Monitor","UserID": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "CoViMoMaConfigPol" - Manage + Montior', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });


        it('should return status 200 on post "CoViMoMaConfigPol - CoMoMa + Vi"', function(done) {
            var payload = '{"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList": "View","UserID": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "CoViMoMaConfigPol" - CoMoMa + Vi', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "CoViMoMaConfigPol" + Montior', function(done) {
            var payload = '{"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList": "View,Monitor","UserID": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "CoViMoMaConfigPol" + Montior', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
        
        
        
        
        it('should return status 200 on post "CoViMoMaConfigPol" has all finally', function(done) {
            var payload = '{"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList": "Configure,View,Manage,Monitor","UserID": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "CoViMoMaConfigPol" has all finally', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });


    describe('Description[""](1024):', function() {

        it('should return status 200 on post to add Desc to "TestConfigPol"', function(done) {
            var payload = '{"ConfigurationPolicy":{"'+ FVT.long256Name +'":{"Description":"'+ FVT.long1024String +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestConfigPol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post add and update "MaxLength Name ConfigPol"', function(done) {
            var payload = '{"ConfigurationPolicy":{"'+ FVT.long256Name +'":{"Description":"Tell me something Good!","ClientAddress":"127.0.0.1"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "256 Char Name ConfigPol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post update "TestConfigPol"', function(done) {
            var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"Description":"Few less chars..."}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Update Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        

        it('should return status 200 on post to remove Description "MaxLength Name ConfigPol"', function(done) {
            var payload = '{"ConfigurationPolicy":{"'+ FVT.long256Name +'":{"Description":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Update Description"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        

    });


    describe('ClientAddress[""](100MaxList)MinOne:', function() {
//          "Restrictions":"IPv6 address should be enclosed in []",
//          "About":"* OR Comma-delimited list of IPv4 or IPv6 addresses or range, for example 10.10.10.10,9.41.0.0,10.10.1.1-10.10.1.100"

        it('should return status 200 on POST ClientAddress:"",UserID:"*"', function(done) {
            var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"", "UserID":"*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on Get ClientAddress:"",UserID:"*"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST ClientAddress:"IPv4"', function(done) {
            var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"' + FVT.M2_IPv4_1 + '", "UserID":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on Get ClientAddress:"IPv4"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 105272
        it('should return status 200 on POST (DEFAULT)ClientAddress:null,UserID:"*"', function(done) {
            var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":null, "UserID":"*"}}}';
            verifyConfig = JSON.parse(  '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"", "UserID":"*"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on Get (DEFAULT)ClientAddress:null,UserID:"*"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST ClientAddress:"IPv4 list"', function(done) {
            var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"' + FVT.M1_IPv4_1 +','+ FVT.M2_IPv4_1 + '","UserID":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on Get ClientAddress:"IPv4 list"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 113164
        it('should return status 200 on POST ClientAddress:"IPv4 MaxList"', function(done) {
            var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"'+ FVT.CLIENTADDRESSLIST + '","UserID":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on Get ClientAddress:"IPv4 MaxList"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 113164 (valid?)
        it('should return status 200 on POST ClientAddress:"IPv4 Max list"', function(done) {
            var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"'+ FVT.CLIENT_ADDRESS_LIST + '","UserID":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on Get ClientAddress:"IPv4 Max list"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST ClientAddress:"IPv4 ascending range"', function(done) {
            var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"10.10.10.10-10.10.10.10"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on Get ClientAddress:"IPv4 ascending range"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 105276 - Invalid
//        it('should return status 200 on POST ClientAddress:"IPv4 wild range"', function(done) {
//            var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"9.3.177.*-9.3.179.*"}}}';
//            verifyConfig = JSON.parse(payload);
//            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
//        });
//        it('should return status 200 on Get ClientAddress:"IPv4 wild range"', function(done) {
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
//        });

        if( FVT.A1_HOST_OS.substring(0, 3) != "EC2") {
            it('should return status 200 on POST ClientAddress:"[IPv6]"', function(done) {
                var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"[' + FVT.M1_IPv6_1 + ']"}}}';
                verifyConfig = JSON.parse(payload);
                FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
            });
            it('should return status 200 on Get ClientAddress:"[IPv6]"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 200 on POST ClientAddress:"[IPv6] list"', function(done) {
                var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"[' + FVT.M1_IPv6_1 +','+ FVT.M2_IPv6_1 + ']"}}}';
                verifyConfig = JSON.parse(payload);
                FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
            });
            it('should return status 200 on Get ClientAddress:"[IPv6] list"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
        }
// 105275 comment
        it('should return status 200 on POST ClientAddress:"[IPv6] range"', function(done) {
            var payload;
            console.log ("==== IP: "+ FVT.M1_IPv6_1 +" LASTIndex: "+ FVT.M1_IPv6_1.lastIndexOf(':') +" substr: "+ FVT.M1_IPv6_1.substr( FVT.M1_IPv6_1.lastIndexOf(':')+1 ) );
            console.log ("==== IP: "+ FVT.M2_IPv6_1 +" LASTIndex: "+ FVT.M2_IPv6_1.lastIndexOf(':') +" substr: "+ FVT.M2_IPv6_1.substr( FVT.M2_IPv6_1.lastIndexOf(':')+1 ) );
            if (  Number( '0x'+FVT.M1_IPv6_1.substr( FVT.M1_IPv6_1.lastIndexOf(':')+1 ) )  >  '0x'+Number( FVT.M2_IPv6_1.substr( FVT.M2_IPv6_1.lastIndexOf(':')+1  )  )) {
                payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"[' + FVT.M2_IPv6_1 +']-['+ FVT.M1_IPv6_1 + ']"}}}';
            } else {
                payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"[' + FVT.M1_IPv6_1 +']-['+ FVT.M2_IPv6_1 + ']"}}}';
            }
            // Tired of messing with this....
            payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"[2607:f0d0:1202:10e:47b:eaff:fe8f:aaa]-[2607:f0d0:1202:10e:47b:eaff:fe8f:bbb]"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on Get ClientAddress:"[IPv6] range"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 105276 not valid
//        it('should return status 200 on POST ClientAddress:"[IPv6] wild range"', function(done) {
//            var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"[fe80::4ac:9ff:fe81:*]-[fe80::417:d6ff:fe19:*]"}}}';
//            verifyConfig = JSON.parse(payload);
//            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
//        });
//        it('should return status 200 on Get ClientAddress:"[IPv6] wild range"', function(done) {
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
//        });

        it('should return status 200 on POST ClientAddress:"mixed bag"', function(done) {
            var payload;
            console.log ("==== IP: "+ FVT.M1_IPv6_1 +" LASTIndex: "+ FVT.M1_IPv6_1.lastIndexOf(':') +" substr: "+ FVT.M1_IPv6_1.substr( FVT.M1_IPv6_1.lastIndexOf(':')+1 ) );
            console.log ("==== IP: "+ FVT.M2_IPv6_1 +" LASTIndex: "+ FVT.M2_IPv6_1.lastIndexOf(':') +" substr: "+ FVT.M2_IPv6_1.substr( FVT.M2_IPv6_1.lastIndexOf(':')+1 ) );
            if (  Number( '0x'+FVT.M1_IPv6_1.substr( FVT.M1_IPv6_1.lastIndexOf(':')+1 ) )  >  Number( '0x'+FVT.M2_IPv6_1.substr( FVT.M2_IPv6_1.lastIndexOf(':')+1  )  )) {
                payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"' + FVT.M1_IPv4_1 +','+ FVT.M2_IPv4_1 + ',[' + FVT.M2_IPv6_1 +']-['+ FVT.M1_IPv6_1 + ']"}}}';
            } else {
                payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"' + FVT.M1_IPv4_1 +','+ FVT.M2_IPv4_1 + ',[' + FVT.M1_IPv6_1 +']-['+ FVT.M2_IPv6_1 + ']"}}}';
            }
            // Tired of messing with this....
            payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"' + FVT.M1_IPv4_1 +','+ FVT.M2_IPv4_1 + ',[2607:f0d0:1202:10e:47b:eaff:fe8f:aaa]-[2607:f0d0:1202:10e:47b:eaff:fe8f:bbb]"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on Get ClientAddress:"mixed bag"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST ClientAddress:"*" again', function(done) {
            var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on Get ClientAddress:"*" again', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });


    describe('UserID[""](1024)MinOne:', function() {
//          "About":"A valid user name. Asterisk matches 0 or more character rule for matching is allowed."

        it('should return status 200 on post "ConfigureConfigPol" UserID:"", GroupID":"*"', function(done) {
            var payload = '{"ConfigurationPolicy":{"ConfigureConfigPol":{"ActionList":"Configure", "UserID":"", "GroupID":"*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ConfigureConfigPol" UserID:"", GroupID":"*"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "ConfigureConfigPol" UserID:"root", GroupID":""', function(done) {
            var payload = '{"ConfigurationPolicy":{"ConfigureConfigPol":{"ActionList": "Configure","UserID":"root","GroupID":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ConfigureConfigPol" UserID:"root", GroupID":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 105272
        it('should return status 200 on post "ConfigureConfigPol" (DEFAULT)UserID:null, GroupID":"*"', function(done) {
            var payload = '{"ConfigurationPolicy":{"ConfigureConfigPol":{"ActionList": "Configure","UserID":null,"GroupID":"*"}}}';
            verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"ConfigureConfigPol":{"ActionList": "Configure","UserID":"","GroupID":"*"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ConfigureConfigPol" (DEFAULT)UserID:null, GroupID":"*"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "ConfigureConfigPol" UserID:"*", GroupID":""', function(done) {
            var payload = '{"ConfigurationPolicy":{"ConfigureConfigPol":{"ActionList": "Configure","UserID":"*","GroupID":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ConfigureConfigPol" UserID:"*", GroupID":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });


    describe('GroupID[""](1024)MinOne:', function() {
//          "About":"A valid group name. Asterisk matches 0 or more character rule for matching is allowed."

        it('should return status 200 on post "ViewConfigPol" "GroupID":"","CommonNames": "*"', function(done) {
            var payload = '{"ConfigurationPolicy":{"ViewConfigPol":{"ActionList": "View","GroupID":"","CommonNames": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ViewConfigPol"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "ViewConfigPol" "GroupID":"admin","CommonNames": ""', function(done) {
            var payload = '{"ConfigurationPolicy":{"ViewConfigPol":{"ActionList": "View","GroupID":"admin","CommonNames": ""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ViewConfigPol" "GroupID":"admin","CommonNames": ""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 105272
        it('should return status 200 on post "ViewConfigPol" (Default)"GroupID":null,"CommonNames": "*"', function(done) {
            var payload = '{"ConfigurationPolicy":{"ViewConfigPol":{"ActionList": "View","GroupID":null,"CommonNames": "*"}}}';
            verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"ViewConfigPol":{"ActionList": "View","GroupID":"","CommonNames": "*"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ViewConfigPol" (Default)"GroupID":null,"CommonNames": "*"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "ViewConfigPol" "GroupID":"*","CommonNames": ""', function(done) {
            var payload = '{"ConfigurationPolicy":{"ViewConfigPol":{"ActionList": "View","GroupID":"*","CommonNames": ""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ViewConfigPol" "GroupID":"*","CommonNames": ""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });


    describe('CommonNames[""](1024)MinOne:', function() {
//           "About":"Common names specified in the certificate. Asterisk matches 0 or more character rule for matching is allowed."

        it('should return status 200 on post "MonitorConfigPol" "CommonNames": "", "ClientAddress":"*"', function(done) {
            var payload = '{"ConfigurationPolicy":{"MonitorConfigPol":{"ActionList": "Monitor","CommonNames": "", "ClientAddress":"*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MonitorConfigPol" "CommonNames": "", "ClientAddress":"*"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "MonitorConfigPol" "CommonNames": "c00*", "ClientAddress":""', function(done) {
            var payload = '{"ConfigurationPolicy":{"MonitorConfigPol":{"ActionList": "Monitor","CommonNames": "c00*", "ClientAddress":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MonitorConfigPol" "CommonNames": "c00*", "ClientAddress":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 105272
        it('should return status 200 on post "MonitorConfigPol" (DEFAULT)"CommonNames":null, "ClientAddress":"*"', function(done) {
            var payload = '{"ConfigurationPolicy":{"MonitorConfigPol":{"ActionList": "Monitor","CommonNames":null, "ClientAddress":"*"}}}';
            verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"MonitorConfigPol":{"ActionList": "Monitor","CommonNames": "", "ClientAddress":"*"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MonitorConfigPol" (DEFAULT)"CommonNames":null, "ClientAddress":"*"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "MonitorConfigPol" "CommonNames": "*", "ClientAddress":""', function(done) {
            var payload = '{"ConfigurationPolicy":{"MonitorConfigPol":{"ActionList": "Monitor","CommonNames": "*", "ClientAddress":""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MonitorConfigPol" "CommonNames": "*", "ClientAddress":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });

    describe('AllMinOne:', function() {

        it('should return status 200 on post "ManageConfigPol" all MinOnes', function(done) {
            var payload = '{"ConfigurationPolicy":{"ManageConfigPol":{"ActionList": "Manage","ClientAddress": "*","UserID":"c01*", "GroupID":"Cars", "CommonNames":"c*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ManageConfigPol" all MinOnes', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on post "ManageConfigPol" return to original', function(done) {
            var payload = '{"ConfigurationPolicy":{"ManageConfigPol":{"ActionList": "Manage","ClientAddress": "*"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "ManageConfigPol" return to original', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });

    // Config persists Check
    describe('Restart Persists:', function() {
    
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
        it('should return status 200 on get, "ConfigurationPolicies" persists', function(done) {
            verifyConfig = JSON.parse( expectAllConfigurationPolicies ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

            
//  ====================  Error test cases  ====================  //

    describe('Error:', function() {
// 105248
        describe('General:', function() {

            it('should return status 400 when trying to Delete All ConfigurationPolicys with POST', function(done) {
                var payload = '{"ConfigurationPolicy":null}';
                verifyConfig = JSON.parse( payload );
//                verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: \{\"ConfigurationPolicy\": null\} is not valid."} ;
//                verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: \"ConfigurationPolicy\":null is not valid."} ;
                verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: \"ConfigurationPolicy\":null is not valid."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on get, Delete All ConfigurationPolicys', function(done) {
                verifyConfig = JSON.parse( expectDefault );
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
            
        });
    
        describe('Name[](256)R', function() {
//          "Restrictions":"No leading or trailing space. No control characters or comma.",

            it('should return status 400 POST  NoLeadingSpaceNames', function(done) {
//                var payload = '{"ConfigurationPolicy":{"\u20NoLeadingSpaceNames":{"ActionList":"View","ClientAddress":"*"}}}';  JavaScript KICK OUT as Invalid
                var payload = '{"ConfigurationPolicy":{" NoLeadingSpaceNames":{"ActionList":"View","ClientAddress":"*"}}}';
                verifyConfig = JSON.parse( payload );
                verifyMessage = {"Code":"CWLNA0122","Message":"The Unicode value is not valid."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 404 GET  NoLeadingSpaceNames', function(done) {
//                verifyConfig = JSON.parse( '{"status":404,"ConfigurationPolicy":{" NoLeadingSpaceNames":null}}' );
                verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConfigurationPolicy Name:  NoLeadingSpaceNames","ConfigurationPolicy":{" NoLeadingSpaceNames":null}}' );
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+' NoLeadingSpaceNames', FVT.verify404NotFound, verifyConfig, done)
            });
// 105273
            it('should return status 400 POST NoTrailingSpaceNames', function(done) {
                var payload = '{"ConfigurationPolicy":{"NoTrailingSpaceNames ":{"ActionList":"View","ClientAddress":"*"}}}';
                verifyConfig = JSON.parse( payload );
                verifyMessage = {"Code":"CWLNA0115","Message":"An argument is not valid: Name: NoTrailingSpaceNames ."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 404 GET NoTrailingSpaceNames', function(done) {
//                verifyConfig = JSON.parse( '{"status":404,"ConfigurationPolicy":{"NoTrailingSpaceNames ":null}}' );
                verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConfigurationPolicy Name: NoTrailingSpaceNames ","ConfigurationPolicy":{"NoTrailingSpaceNames ":null}}' );
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'NoTrailingSpaceNames ', FVT.verify404NotFound, verifyConfig, done)
            });

            it('should return status 400 POST No,CommaNames', function(done) {
                var payload = '{"ConfigurationPolicy":{"No,CommaNames":{"ActionList":"View","ClientAddress":"*"}}}';
                verifyConfig = JSON.parse( payload );
                verifyMessage = {"Code":"CWLNA0122","Message":"The Unicode value is not valid."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 404 GET No,CommaNames', function(done) {
//                verifyConfig = JSON.parse( '{"status":404,"ConfigurationPolicy":{"No,CommaNames":null}}' );
                verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConfigurationPolicy Name: No,CommaNames","ConfigurationPolicy":{"No,CommaNames":null}}' );
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'No,CommaNames', FVT.verify404NotFound, verifyConfig, done)
            });

        });


        describe('ActionList[Configure,View,Monitor,Manage]R:', function() {
//          "About":"Comma-delimited list of allowed actions that the user can initiate.",


            it('should return status 400 on post "CoViMoMaConfigPol" on ActionList spacing', function(done) {
                var payload = '{"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList": "Configure, Manage","UserID": "*"}}}';
                verifyConfig = {"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList": "Configure,View,Manage,Monitor","UserID": "*"}}} ;
                verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Configure, Manage\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on get, "CoViMoMaConfigPol" on ActionList spacing', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on post "CoViMoMaConfigPol" on BadActionList', function(done) {
                var payload = '{"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList": "Configure,Mangle","UserID": "*"}}}';
                verifyConfig = {"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList": "Configure,View,Manage,Monitor","UserID": "*"}}} ;
                verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"Configure,Mangle\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on get, "CoViMoMaConfigPol" on BadActionList', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on post "CoViMoMaConfigPol" on NoActionList', function(done) {
                var payload = '{"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList": "","UserID": "*"}}}';
                verifyConfig = {"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList": "Configure,View,Manage,Monitor","UserID": "*"}}} ;
//                verifyMessage = {"status":400, "Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: ActionList Value: null."} ;
                verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"\"\"\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on get, "CoViMoMaConfigPol" on NoActionList', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// 105274
            it('should return status 400 on post "CoViMoMaConfigPol" on NullActionList', function(done) {
                var payload = '{"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList":null,"UserID": "*"}}}';
                verifyConfig = {"ConfigurationPolicy":{"CoViMoMaConfigPol":{"ActionList": "Configure,View,Manage,Monitor","UserID": "*"}}} ;
//                verifyMessage = {"status":400, "Code":"CWLNA0132","Message":"The property value is not valid. Object: ConfigurationPolicy Name: ActionList Property: InvalidType Value: JSON_NULL."} ;
                verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ActionList Value: \"null\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on get, "CoViMoMaConfigPol" on NullActionList', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

        });


        describe('Description[""](1024):', function() {
//          "About":"A description of the Policy."

            it('should return status 400 POST Description tooLong', function(done) {
                var payload = '{"ConfigurationPolicy":{"'+ FVT.long256Name +'":{"Description":"'+ FVT.long1025String +'"}}}';
                verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"'+ FVT.long256Name +'":{"Description":""}}}' );
//                verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0137","Message":"The RESTful request:  is not valid."}' );
//                verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: Description Value: \\\"'+ FVT.long1025String +'\\\"."}' );
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: ConfigurationPolicy Property: Description Value: '+ FVT.long1025String +'." }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 Get after POST Description too long', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

        
        });


        describe('ClientAddress[""](100MaxList)MinOne:', function() {
    //          "Restrictions":"IPv6 address should be enclosed in []",
    //          "About":"* OR Comma-delimited list of IPv4 or IPv6 addresses or range, for example 10.10.10.10,9.41.0.0,10.10.1.1-10.10.1.100"

            it('should return status 400 on POST ClientAddress:"IPv4 exceed MaxList"', function(done) {
                var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"'+ FVT.CLIENTADDRESSLIST + ',' + FVT.IP_ADDRESS_STEM + (FVT.MAX_LISTMEMBERS + 1) +'","UserID":""}}}';
                verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"*"}}}' );
//                verifyMessage = {"status":400, "Code":"CWLNA0371","Message":"The number of client addresses exceeds the maximum number allowed: 20." } ;
                verifyMessage = {"status":400, "Code":"CWLNA0371","Message":"The number of client addresses exceeds the maximum number allowed: 100." } ;
//nope                verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: ConnectionPolicy Property: ClientAddress Value: '+ FVT.CLIENTADDRESSLIST + ',' + FVT.IP_ADDRESS_STEM + (FVT.MAX_LISTMEMBERS + 1) +'." }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on Get ClientAddress:"IPv4 exceed MaxList"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on POST ClientAddress:"IPv4 exceed MaxList"', function(done) {
                var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"'+ FVT.CLIENT_ADDRESS_LIST + ', ' + FVT.IP_ADDRESS_STEM + (FVT.MAX_LISTMEMBERS + 1) +'","UserID":""}}}';
                verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"*"}}}' );
//                verifyMessage = {"status":400, "Code":"CWLNA0371","Message":"The number of client addresses exceeds the maximum number allowed: 20." } ;
                verifyMessage = {"status":400, "Code":"CWLNA0371","Message":"The number of client addresses exceeds the maximum number allowed: 100." } ;
//nope                verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: ConnectionPolicy Property: ClientAddress Value: '+ FVT.CLIENT_ADDRESS_LIST + ', ' + FVT.IP_ADDRESS_STEM + (FVT.MAX_LISTMEMBERS + 1) +'." }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on Get ClientAddress:"IPv4 exceed MaxList"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// 105275
            it('should return status 400 on POST ClientAddress:"[IPv4]"', function(done) {
                var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"[' + FVT.M1_IPv4_1 + ']"}}}';
                verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"*"}}}' );
//                verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: [address]."} ;
//                verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: \"[' + FVT.M1_IPv4_1 + ']\"."}' ) ;
                verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: \\\"[' + FVT.M1_IPv4_1 + ']\\\"."}' ) ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on Get ClientAddress:"[IPv4]"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// 105275, 113157 (actually valid)
            it('should return status 200 on POST ClientAddress:"IPv6 no[]"', function(done) {
                var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"' + FVT.M1_IPv6_1 + '"}}}';
                verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"' + FVT.M1_IPv6_1 + '"}}}' );
                FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
            });
            it('should return status 200 on Get ClientAddress:"IPv6 no[]"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// 105275
            it('should return status 400 on POST ClientAddress:"localhost"', function(done) {
                var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"localhost"}}}';
                verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"' + FVT.M1_IPv6_1 + '"}}}' );
//                verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: [address]."} ;
                verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: \"localhost\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on Get ClientAddress:"localhost"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// 105281
            it('should return status 400 on POST ClientAddress:"IPv4 descending range"', function(done) {
                var payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"10.10.10.10-10.10.10.10"}}}';
                verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"' + FVT.M1_IPv6_1 + '"}}}' );
//                verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: [descending address]."} ;
//                verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: \"10.10.10.10\"."} ;
                verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: \"10.10.10.10-10.10.10.10\"."} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on Get ClientAddress:"IPv4 descending range"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

            it('should return status 400 on POST ClientAddress:"IPv6 descending range"', function(done) {
                var payload;
// Give up trying to make this programtically real IPv6 IPs                
//                if (  Number( FVT.M1_IPv6_1.substr( FVT.M1_IPv6_1.lastIndexOf(':')+1 ) )  <  Number( FVT.M2_IPv6_1.substr( FVT.M2_IPv6_1.lastIndexOf(':')+1  )  )) {
//                    payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"[' + FVT.M2_IPv6_1 +']-['+ FVT.M1_IPv6_1 + ']"}}}';
////                    verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: [descending address]."} ;
//                    verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: \\\"[' + FVT.M2_IPv6_1 +']\\\"."}' ) ;
//                } else {
                    payload = '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"[2607:f0d0:1202:10e:47c:99ff:fe5c:e860]-[2607:f0d0:1202:10e:47c:99ff:fe5c:60]"}}}';
//                    verifyMessage = {"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: [descending address]."} ;
////                    verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: \\\"[2607:f0d0:1202:10e:47c:99ff:fe5c:e860]\\\"."}' ) ;
                    verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: ClientAddress Value: \\\"[2607:f0d0:1202:10e:47c:99ff:fe5c:e860]-[2607:f0d0:1202:10e:47c:99ff:fe5c:60]\\\"."}' ) ;
//                }
                verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"TestConfigPol":{"ClientAddress":"' + FVT.M1_IPv6_1 + '"}}}' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on Get ClientAddress:"IPv6 descending range"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

        });


        describe('UserID[""](1024)MinOne:', function() {
    //          "About":"A valid user name. Asterisk matches 0 or more character rule for matching is allowed."

            it('should return status 400 on post "ConfigureConfigPol" UserID:"toolong", GroupID":""', function(done) {
                var payload = '{"ConfigurationPolicy":{"ConfigureConfigPol":{"ActionList": "Configure","UserID":"'+ FVT.long1025String +'"}}}';
                verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"ConfigureConfigPol":{"ActionList": "Configure","UserID":"*","GroupID":""}}}' );
//                verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: UserID Value: \\\"'+ FVT.long1025String +'\\\"."}' ) ;
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: ConfigurationPolicy Property: UserID Value: '+ FVT.long1025String +'." }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on get, "ConfigureConfigPol" UserID:"*", GroupID":""', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

        });


        describe('GroupID[""](1024)MinOne:', function() {
    //          "About":"A valid group name. Asterisk matches 0 or more character rule for matching is allowed."

            it('should return status 400 on post "ViewConfigPol" "GroupID":"toolong","CommonNames": ""', function(done) {
                var payload = '{"ConfigurationPolicy":{"ViewConfigPol":{"ActionList": "View","GroupID":"'+ FVT.long1025String +'"}}}';
                verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"ViewConfigPol":{"ActionList": "View","GroupID":"*","CommonNames": ""}}}' );
//                verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: GroupID Value: \\\"'+ FVT.long1025String +'\\\"."}' ) ;
                verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: ConfigurationPolicy Property: GroupID Value: '+ FVT.long1025String +'." }' );
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on get, "ViewConfigPol"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

        });


        describe('CommonNames[""](1024)MinOne:', function() {
    //           "About":"Common names specified in the certificate. Asterisk matches 0 or more character rule for matching is allowed."

        it('should return status 400 on post "MonitorConfigPol" "CommonNames": "toolong", "ClientAddress":""', function(done) {
            var payload = '{"ConfigurationPolicy":{"MonitorConfigPol":{"ActionList": "Monitor","CommonNames": "'+ FVT.long1025String +'"}}}';
            verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"MonitorConfigPol":{"ActionList": "Monitor","CommonNames": "*", "ClientAddress":""}}}' );
//            verifyMessage = JSON.parse( '{"status":400, "Code":"CWLNA0112","Message":"The property value is not valid: Property: CommonNames Value: \\\"'+ FVT.long1025String +'\\\"."}' ) ;
             verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: ConfigurationPolicy Property: CommonNames Value: '+ FVT.long1025String +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "MonitorConfigPol" "CommonNames": "toolong", "ClientAddress":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });


        });

        describe('AllMinOne:', function() {
// 105282 120951
            it('should return status 400 on post "ManageConfigPol" CommonName-s', function(done) {
//                var payload = '{"ConfigurationPolicy":{"ManageConfigPol":{"ActionList": "Manage","ClientAddress": "*","ClientID":"u01*", "GroupID":"Users", "CommonName":"u*"}}}';
                var payload = '{"ConfigurationPolicy":{"ManageConfigPol":{"ActionList": "Manage","ClientAddress": "*",                   "GroupID":"Users", "CommonName":"u*"}}}';
                verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"ManageConfigPol":{"ActionList": "Manage","ClientAddress": "*","UserID":"c01*", "GroupID":"Cars", "CommonNames":"c*"}}}' );
//                verifyMessage = {"status":404, "Code":"CWLNA0138","Message":"The property name is invalid. Object: ConfigurationPolicy Name: ManageConfigPol Property: CommonName"} ;
                verifyMessage = {"status":400, "Code":"CWLNA0138","Message":"The property name is invalid. Object: ConfigurationPolicy Name: ManageConfigPol Property: CommonName"} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on get, "ManageConfigPol" ComonName-s', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });
// 105290
            it('should return status 400 on post "ManageConfigPol" No MinOne', function(done) {
                var payload = '{"ConfigurationPolicy":{"ManageConfigPol":{"ActionList": "Manage","ClientAddress": "","UserID":null, "GroupID":"", "CommonNames":null }}}';
                verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"ManageConfigPol":{"ActionList": "Manage","ClientAddress": "*","UserID":"c01*", "GroupID":"Cars", "CommonNames":"c*"}}}' );
//                verifyMessage = {"status":400, "Code":"CWLNA0138","Message":"The property name is invalid. Object: ConfigurationPolicy Name: ManageConfigPol Property: ClientAddress"} ;
                verifyMessage = {"status":400, "Code":"CWLNA0139","Message":"The object: ConfigurationPolicy must have one of the properties ClientAddress,UserID,GroupID,CommonNames specified"} ;
                FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            });
            it('should return status 200 on get, "ManageConfigPol" No MinOne', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
            });

        });


    });
   
    
    //  ====================  Delete test cases  ====================  //
    describe('Delete Test Cases:', function() {

        it('should return status 200 when deleting Max Length ConfigPolicy', function(done) {
            verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"'+ FVT.long256Name +'":null}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get all after delete, "Max Length ConfigPolicy" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "Max Length ConfigPolicy" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"ConfigurationPolicy":{"'+ FVT.long256Name +'":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConfigurationPolicy Name: '+ FVT.long256Name +'","ConfigurationPolicy":{"'+ FVT.long256Name +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.verify404NotFound, verifyConfig, done );
        });

        it('should return status 200 when deleting "TestConfigPol"', function(done) {
            verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"TestConfigPol":null}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'TestConfigPol', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get all after delete, "TestConfigPol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "TestConfigPol" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"ConfigurationPolicy":{"TestConfigPol":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConfigurationPolicy Name: TestConfigPol","ConfigurationPolicy":{"TestConfigPol":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestConfigPol', FVT.verify404NotFound, verifyConfig, done );
        });
        
        it('should return status 200 when deleting "ConfigureConfigPol"', function(done) {
            verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"ConfigureConfigPol":null}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'ConfigureConfigPol', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get all after delete, "ConfigureConfigPol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "ConfigureConfigPol" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"ConfigurationPolicy":{"ConfigureConfigPol":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConfigurationPolicy Name: ConfigureConfigPol","ConfigurationPolicy":{"ConfigureConfigPol":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'ConfigureConfigPol', FVT.verify404NotFound, verifyConfig, done );
        });
        
        it('should return status 200 when deleting "ViewConfigPol"', function(done) {
            verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"ViewConfigPol":null}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'ViewConfigPol', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get all after delete, "ViewConfigPol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "ViewConfigPol" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"ConfigurationPolicy":{"ViewConfigPol":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConfigurationPolicy Name: ViewConfigPol","ConfigurationPolicy":{"ViewConfigPol":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'ViewConfigPol', FVT.verify404NotFound, verifyConfig, done );
        });
        
        it('should return status 200 when deleting "MonitorConfigPol"', function(done) {
            verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"MonitorConfigPol":null}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'MonitorConfigPol', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get all after delete, "MonitorConfigPol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "MonitorConfigPol" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"ConfigurationPolicy":{"MonitorConfigPol":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConfigurationPolicy Name: MonitorConfigPol","ConfigurationPolicy":{"MonitorConfigPol":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'MonitorConfigPol', FVT.verify404NotFound, verifyConfig, done );
        });
        
        it('should return status 200 when deleting "ManageConfigPol"', function(done) {
            verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"ManageConfigPol":null}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'ManageConfigPol', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get all after delete, "ManageConfigPol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "ManageConfigPol" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"ConfigurationPolicy":{"ManageConfigPol":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConfigurationPolicy Name: ManageConfigPol","ConfigurationPolicy":{"ManageConfigPol":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'ManageConfigPol', FVT.verify404NotFound, verifyConfig, done );
        });
        
        it('should return status 200 when deleting "CoViMoMaConfigPol"', function(done) {
            verifyConfig =  JSON.parse( '{"ConfigurationPolicy":{"CoViMoMaConfigPol":null}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'CoViMoMaConfigPol', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get all after delete, "CoViMoMaConfigPol" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "CoViMoMaConfigPol" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"ConfigurationPolicy":{"CoViMoMaConfigPol":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: ConfigurationPolicy Name: CoViMoMaConfigPol","ConfigurationPolicy":{"CoViMoMaConfigPol":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'CoViMoMaConfigPol', FVT.verify404NotFound, verifyConfig, done );
        });

    });
    
});
