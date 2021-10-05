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

var verifyConfig = {} ;
var verifyMessage = {};

var uriObject = 'SecurityProfile/' ;
var expectDefault = '{"SecurityProfile":{"AdminDefaultSecProfile":{"CertificateProfile":"AdminDefaultCertProf","Ciphers": "Fast","MinimumProtocolMethod": "TLSv1.2","TLSEnabled": false,"UseClientCertificate": false,"UseClientCipher": false,"UsePasswordAuthentication": false}},"Version": "'+ FVT.version +'"}' ;

var minimalSecurityProfile = {"SecurityProfile":{"CertificateNotRequired": {"CertificateProfile": "","Ciphers": "Fast", "LTPAProfile": "", "MinimumProtocolMethod": "TLSv1.2", "OAuthProfile": "", "TLSEnabled": false, "UseClientCertificate": false, "UseClientCipher": false, "UsePasswordAuthentication": true }}}

var expectAllSecurityProfiles = '{"Version": "'+ FVT.version +'", "SecurityProfile": { \
 "AdminDefaultSecProfile": {  "MinimumProtocolMethod": "TLSv1.2",  "UseClientCertificate": false,  "Ciphers": "Fast",  "CertificateProfile": "AdminDefaultCertProf",  "UseClientCipher": false,  "UsePasswordAuthentication": false,  "TLSEnabled": false  }, \
 "TestSecProf": {  "CertificateProfile": "TestCertProf",  "OAuthProfile": "",  "UsePasswordAuthentication": true,  "TLSEnabled": true,  "UseClientCertificate": false,  "MinimumProtocolMethod": "TLSv1.2",  "Ciphers": "Fast",  "UseClientCipher": false,  "LTPAProfile": "",  "CRLProfile": ""  }, \
 "'+ FVT.long32Name +'": {  "CertificateProfile": "aCertProf",  "OAuthProfile": "",  "UsePasswordAuthentication": true,  "TLSEnabled": true,  "UseClientCertificate": false,  "MinimumProtocolMethod": "TLSv1.2",  "Ciphers": "Fast",  "UseClientCipher": false,  "LTPAProfile": "",  "CRLProfile": ""  }, \
 "CertificateNotRequired": {  "TLSEnabled": false,  "CertificateProfile": "",  "OAuthProfile": "",  "UsePasswordAuthentication": true,  "UseClientCertificate": false,  "MinimumProtocolMethod": "TLSv1.2",  "Ciphers": "Fast",  "UseClientCipher": false,  "LTPAProfile": "",  "CRLProfile": "" } }}' ;


//  ====================  Test Cases Start Here  ====================  //
//      "ListObjects":"Name,TLSEnabled,MinimumProtocolMethod,UseClientCertificate,UsePasswordAuthentication,Ciphers,CertificateProfile,UseClientCipher,LTPAProfile,OAuthProfile",
// new:  CRLProfile
describe('SecurityProfile:', function() {
this.timeout( FVT.defaultTimeout );

    // Get test cases
    describe('Get - verify in Pristine State:', function() {
        it('should return status 200 on get, DEFAULT of "SecProf":{"DefaultSP":{...}}', function(done) {
            verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on get, specifically {"SecProf":{"DefaultSP":{...}}}', function(done) {
            verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"AdminDefaultSecProfile", FVT.getSuccessCallback, verifyConfig, done );
        });
    
    });

//  ====================   Create - Add - Update test cases  ====================  //
    describe('Prereq Certs, Keys and CertProfile:', function() {

        it('should return status 200 on PUT "TestCert.pem"', function(done) {
            sourceFile = 'AdminDefaultCert.pem';
            targetFile = 'TestCert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "TestKey.pem"', function(done) {
            sourceFile = 'AdminDefaultKey.pem';
            targetFile = 'TestKey.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
    
        it('should return status 200 on post "TestCertProf"', function(done) {
            var payload = '{"CertificateProfile":{"TestCertProf":{"Certificate": "TestCert.pem","Key": "TestKey.pem"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestCertProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/TestCertProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on PUT "aCert.pem"', function(done) {
            sourceFile = 'AdminDefaultCert.pem';
            targetFile = 'aCert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "aKey.pem"', function(done) {
            sourceFile = 'AdminDefaultKey.pem';
            targetFile = 'aKey.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
    
        it('should return status 200 on post "aCertProf"', function(done) {
            var payload = '{"CertificateProfile":{"aCertProf":{"Certificate": "aCert.pem","Key": "aKey.pem"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "aCertProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/aCertProf', FVT.getSuccessCallback, verifyConfig, done);
        });
    

        it('should return status 200 on PUT "CRList.crl"', function(done) {
            sourceFile = '../common/intCA1-client-crt-crl.pem';
            targetFile = 'CRList.crl';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
    
        it('should return status 200 on post "TestCRLProf"', function(done) {
            var payload = '{"CRLProfile":{"TestCRLProf":{"CRLSource": "CRList.crl"}}}';
            verifyConfig = JSON.parse( '{"CRLProfile":{"TestCRLProf":{"CRLSource": "CRList.crl", "UpdateInterval":60, "RevalidateConnection":false}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestCRLProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
        });

    });


    describe('Prereqs LTPAProfile:', function() {


        it('should return status 200 on PUT "LTPAKeyfile"', function(done) {
            sourceFile = 'mar400.wasltpa.keyfile';
            targetFile = 'LTPAKeyfile';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on post "TestLTPAProf"', function(done) {
            var payload = '{"LTPAProfile":{"TestLTPAProf":{"KeyFileName":"LTPAKeyfile","Password":"imasvtest"}}}';
            verifyConfig = JSON.parse( '{"LTPAProfile":{"TestLTPAProf":{"KeyFileName":"LTPAKeyfile","Password":"XXXXXX"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestLTPAProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'LTPAProfile', FVT.getSuccessCallback, verifyConfig, done);
        });

    });


    describe('Prereqs OAuthProfile:', function() {

        it('should return status 200 on post "TestOAuthProf"', function(done) {
            var payload = '{"OAuthProfile":{"TestOAuthProf":{"ResourceURL":"http://oauth.server.com"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestOAuthProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'OAuthProfile', FVT.getSuccessCallback, verifyConfig, done);
        });

    });


    describe('CertificateProfile:', function() {

        it('should return status 200 on post "TestSecProf"', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"CertificateProfile": "TestCertProf"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestSecProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "32 Char Name SecurityProf"', function(done) {
            var payload = '{"SecurityProfile":{"'+ FVT.long32Name +'":{"CertificateProfile": "TestCertProf"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "32 Char Name SecurityProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Update CertProfile"', function(done) {
            var payload = '{"SecurityProfile":{"'+ FVT.long32Name +'":{"CertificateProfile": "aCertProf"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Update Cert and Key"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 POST CertificateNotRequired Profile"', function(done) {
            var payload = '{"SecurityProfile":{"CertificateNotRequired": {"CertificateProfile": "", "TLSEnabled": false }}}' ;
            verifyConfig =  minimalSecurityProfile ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET CertificateNotRequired', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        

    });


    describe('CRLProfile:', function() {

        it('should return 400 POST TestSecProf with "TestCRLProf" without UseClientCert:true', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"CRLProfile": "TestCRLProf"}}}';
            verifyConfig = {"SecurityProfile":{"TestSecProf":{"CRLProfile": ""}}};
			verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: UseClientCertificate Value: false." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 GET, TestSecProf with "TestCRLProf" without UseClientCert:true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 400 POST TestSecProf with "TestCRLProf" with UseClientCert:false', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"CRLProfile": "TestCRLProf", "UseClientCertificate":false}}}';
            verifyConfig = {"SecurityProfile":{"TestSecProf":{"CRLProfile": ""}}};
			verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: UseClientCertificate Value: false." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 GET, TestSecProf with "TestCRLProf", UseClientCert:false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 POST TestSecProf with "TestCRLProf", UseClientCert', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"CRLProfile": "TestCRLProf", "UseClientCertificate":true}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 GET, TestSecProf with "TestCRLProf", UseClientCert', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 140640 - once CRL there, not allowing changes
        it('should return 200 POST TestSecProf with "TestCRLProf", TLSEnabled:false', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{ "TLSEnabled":false }}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 GET, TestSecProf with "TestCRLProf", TLSEnabled:false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 140640 - once CRL there, not allowing changes
        it('should return 200 POST TestSecProf with "TestCRLProf", TLSEnabled:true', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"CRLProfile": "TestCRLProf", "TLSEnabled":true}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 GET, TestSecProf with "TestCRLProf", TLSEnabled:true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 400 POST TestSecProf with "TestCRLProf" remove UseClientCert', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"CRLProfile": "TestCRLProf", "UseClientCertificate":false}}}';
            verifyConfig = {"SecurityProfile":{"TestSecProf":{"CRLProfile": "TestCRLProf", "UseClientCertificate":true}}};
//			verifyMessage = { "status":400, "Code":"CWLNA0376", "Message":"The Object: CRLProfile, Name: TestCRLProf is still being used by Object: SecurityProfile, Name: TestSecProf" };
			verifyMessage = { "status":400, "Code":"CWLNA0134", "Message":"The value specified for the required property is invalid or null. Property: UseClientCertificate Value: false." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return 200 GET, TestSecProf with "TestCRLProf", remove UseClientCert', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 POST TestSecProf remove "TestCRLProf", UseClientCert', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"CRLProfile": null, "UseClientCertificate":false}}}';
            verifyConfig = JSON.parse( '{"SecurityProfile":{"TestSecProf":{"CRLProfile": "", "UseClientCertificate":false}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return 200 GET, TestSecProf remove "TestCRLProf", UseClientCert', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
		
    });


    describe('TLSEnabled(T):', function() {

        it('should return status 200 on post "TLSEnabled":false', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"TLSEnabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TLSEnabled":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "TLSEnabled":true', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"TLSEnabled":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TLSEnabled":true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "TLSEnabled":false', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"TLSEnabled":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TLSEnabled":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//99400 105424
        it('should return status 200 on post "TLSEnabled:null DEFAULT(T)"', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"TLSEnabled":null}}}';
            verifyConfig = JSON.parse( '{"SecurityProfile":{"TestSecProf":{"TLSEnabled":true}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TLSEnabled:null DEFAULT(T)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });


    describe('UseClientCertificate(F):', function() {

        it('should return status 200 on post "UseClientCertificate":true', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UseClientCertificate":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "UseClientCertificate":true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "UseClientCertificate":false', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UseClientCertificate":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "UseClientCertificate":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "UseClientCertificate":true', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UseClientCertificate":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "UseClientCertificate":true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//99400 105424
        it('should return status 200 on post "UseClientCertificate:null DEFAULT(F)"', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UseClientCertificate":null}}}';
            verifyConfig = JSON.parse( '{"SecurityProfile":{"TestSecProf":{"UseClientCertificate":false}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "UseClientCertificate:null DEFAULT(F)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });


    describe('UsePasswordAuth(T):', function() {

        it('should return status 200 on post "UsePasswordAuthentication":false', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UsePasswordAuthentication":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "UsePasswordAuthentication":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "UsePasswordAuthentication":true', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UsePasswordAuthentication":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "UsePasswordAuthentication":true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "UsePasswordAuthentication":false', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UsePasswordAuthentication":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "UsePasswordAuthentication":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//99400 105424
        it('should return status 200 on post "UsePasswordAuthentication:null DEFAULT(T)"', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UsePasswordAuthentication":null}}}';
            verifyConfig = JSON.parse( '{"SecurityProfile":{"TestSecProf":{"UsePasswordAuthentication":true}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "UsePasswordAuthentication:null DEFAULT(T)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });


    describe('UseClientCipher(F):', function() {

        it('should return status 200 on post "UseClientCipher":true', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UseClientCipher":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "UseClientCipher":true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "UseClientCipher":false', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UseClientCipher":false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "UseClientCipher":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "UseClientCipher":true', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UseClientCipher":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "UseClientCipher":true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//99400 105424
        it('should return status 200 on post "UseClientCipher":null DEFAULT(F)', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UseClientCipher":null}}}';
            verifyConfig = JSON.parse( '{"SecurityProfile":{"TestSecProf":{"UseClientCipher":false}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "UseClientCipher":null DEFAULT(F)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    describe('MinimumProtocolMethod(TLSv1,TLSv1.1,[TLSv1.2]):', function() {

        it('should return status 400 on POST "MinimumProtocolMethod":"SSLv3", no longer supported', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"MinimumProtocolMethod":"SSLv3"}}}';
            verifyConfig = {"SecurityProfile":{"TestSecProf":{"MinimumProtocolMethod":"TLSv1.2"}}} ;
//			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MinimumProtocolMethod Value: SSLv3." } ;
			verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MinimumProtocolMethod Value: \"SSLv3\"." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "MinimumProtocolMethod":"SSLv3", no longer supported', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MinimumProtocolMethod":"TLSv1.1"', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"MinimumProtocolMethod":"TLSv1.1"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MinimumProtocolMethod":"TLSv1.1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MinimumProtocolMethod":"TLSv1.2"', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"MinimumProtocolMethod":"TLSv1.2"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MinimumProtocolMethod":"TLSv1.2"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "MinimumProtocolMethod":"TLSv1"', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"MinimumProtocolMethod":"TLSv1"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MinimumProtocolMethod":"TLSv1"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//99400, 105428
        it('should return status 200 on post "MinimumProtocolMethod:null DEFAULT(TLSv1.2)"', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"MinimumProtocolMethod":null}}}';
            verifyConfig = JSON.parse( '{"SecurityProfile":{"TestSecProf":{"MinimumProtocolMethod":"TLSv1.2"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "MinimumProtocolMethod":null DEFAULT(TLSv1)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    });

    describe('Ciphers(Best,Medium,[Fast]):', function() {

        it('should return status 200 on post "Ciphers":"Best"', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"Ciphers":"Best"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Ciphers":"Best"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Ciphers":"Fast"', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"Ciphers":"Fast"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Ciphers":"Fast"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Ciphers":"Medium"', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"Ciphers":"Medium"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Ciphers":"Medium"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//99400 105428
        it('should return status 200 on post "Ciphers:null DEFAULT(Fast)"', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"Ciphers":null}}}';
            verifyConfig = JSON.parse( '{"SecurityProfile":{"TestSecProf":{"Ciphers":"Fast"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Ciphers:null DEFAULT(Fast)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });


//  ====================  Error test cases  ====================  //
describe('Error:', function() {
    describe('General:', function() {
// 105422
        it('should return status 400 when trying to Delete All SecurityProfiles with POST', function(done) {
            var payload = '{"SecurityProfile":null}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: {\"SecurityProfile\":null} is not valid."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: \"SecurityProfile\":null is not valid."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: \"SecurityProfile\":null is not valid."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, Delete All SecurityProfiles,  should be in use', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
    });
    
    describe('Name:', function() {

        it('should return status 400 when trying with Name too long', function(done) {
            var payload = '{"SecurityProfile":{"'+ FVT.long33Name +'":{"CertificateProfile": "TestCertProf"}}}';
            verifyConfig = JSON.parse( payload );
// came back!            verifyMessage = JSON.parse( '{"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: Name Name: '+ FVT.long33Name +'."}' );
//            verifyMessage = {"status":400,"Code":"CWLNA8291","Message":"The property \"SecurityProfile\" has a value which has exceeded the maximum length of 32 characters."} ;
//            verifyMessage = JSON.parse( '{"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: SecurityProfile Name: '+ FVT.long33Name +'."}' );
            verifyMessage = JSON.parse( '{"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: SecurityProfile Property: Name Value: '+ FVT.long33Name +'."}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST failed with Name too long', function(done) {
//            verifyConfig = {"status":404,"SecurityProfile":{"'+ FVT.long33Name +'":{}}};
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: '+ FVT.long33Name +'","SecurityProfile":{"'+ FVT.long33Name +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long33Name, FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 when trying with Name with LeadingSpaces', function(done) {
            var payload = '{"SecurityProfile":{"%20LeadingSpaceNamedSecProfile":{"CertificateProfile": "TestCertProf"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0115","Message":"An argument is not valid: Name:  LeadingSpaceNamedSecProfile."} ;
// came back           verifyMessage = {"Code":"CWLNA0122","Message":"The Unicode value is not valid."} ;
 //           verifyMessage = {"Code":"CWLNA8462","Message":"The property \"SecurityProfile\" must only contain alphanumeric characters."} ;
            verifyMessage = {"Code":"CWLNA0122","Message":"The Unicode value is not valid."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST failed with Name with LeadingSpaces', function(done) {
//            verifyConfig = {"status":404,"SecurityProfile":{" LeadingSpaceNamedSecProfile":{}}};
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: %20LeadingSpaceNamedSecProfile","SecurityProfile":{" LeadingSpaceNamedSecProfile":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"%20LeadingSpaceNamedSecProfile", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 when trying with Name with TrailingSpaces', function(done) {
            var payload = '{"SecurityProfile":{"TrailingSpaceNamedSecProfile ":{"CertificateProfile": "TestCertProf"}}}';
            verifyConfig = JSON.parse( payload );
// came back           verifyMessage = {"Code":"CWLNA0115","Message":"An argument is not valid: Name: TrailingSpaceNamedSecProfile ."};
//            verifyMessage = {"Code":"CWLNA0122","Message":"The Unicode value is not valid."} ;
//            verifyMessage = {"Code":"CWLNA8462","Message":"The property \"SecurityProfile\" must only contain alphanumeric characters."} ;
            verifyMessage = {"Code":"CWLNA0115","Message":"An argument is not valid: Name: TrailingSpaceNamedSecProfile ."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST failed with Name with TrailingSpaces', function(done) {
//            verifyConfig = {"status":404,"SecurityProfile":{"TrailingSpaceNamedSecProfile ":{}}};
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: TrailingSpaceNamedSecProfile%20","SecurityProfile":{"TrailingSpaceNamedSecProfile ":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"TrailingSpaceNamedSecProfile%20", FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 when trying with Name with Comma', function(done) {
            var payload = '{"SecurityProfile":{"Comma,SecProfile":{"CertificateProfile": "TestCertProf"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0115","Message":"An argument is not valid: Name: Comma,SecProfile."} ;
// came back           verifyMessage = {"Code":"CWLNA0122","Message":"The Unicode value is not valid."} ;
//            verifyMessage = {"Code":"CWLNA8462","Message":"The property \"SecurityProfile\" must only contain alphanumeric characters."} ;
            verifyMessage = {"Code":"CWLNA0122","Message":"The Unicode value is not valid."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST failed with Name with Comma', function(done) {
//            verifyConfig = {"status":404,"SecurityProfile":{"Comma,SecProfile":{}}};
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: Comma,SecProfile","SecurityProfile":{"Comma,SecProfile":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"Comma,SecProfile", FVT.verify404NotFound, verifyConfig, done);
        });
		
    });

// 121108 - Not getting 144 like R. said to expect, I get 136 and that seems corrrect.
    describe('CertificateProfile:', function() {
        it('should return status 404 CertificateProfile Name TooLong', function(done) {
            var payload = '{"SecurityProfile":{"CertProfileNameTooLong":{"CertificateProfile":"'+ FVT.long257Name +'"}}}';
            verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: SecurityProfile Property: CertificateProfile Value: '+ FVT.long257Name +'." }' );
			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: '+ FVT.long257Name +'" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, Delete All SecurityProfiles,  should be in use', function(done) {
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: CertProfileNameTooLong","SecurityProfile":{"CertProfileNameTooLong":{}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'CertProfileNameTooLong', FVT.verify404NotFound, verifyConfig, done);
        });
    });
	
    describe('CRLProfile:', function() {
	
        it('should return status 400 CRLProfile Name TooLong, no CertificateProfile', function(done) {
            var payload = '{"SecurityProfile":{"CRLProfileNameTooLong":{"CRLProfile":"'+ FVT.long257Name +'"}}}';
            verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":404, "Code":"CWLNA0136", "Message":"The item or object cannot be found. Type: CRLProfile Name: '+ FVT.long257Name +'" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return GET 404 CRLProfile Name TooLong, no CertificateProfile', function(done) {
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: CRLProfileNameTooLong","SecurityProfile":{"CRLProfileNameTooLong":{}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'CRLProfileNameTooLong', FVT.verify404NotFound, verifyConfig, done);
        });
// 113153
        it('should return status 400 CRLProfile Name TooLong, with CertificateProfile', function(done) {
            var payload = '{"SecurityProfile":{"CRLProfileNameTooLong":{"CRLProfile":"'+ FVT.long257Name +'", "CertificateProfile": "TestCertProf"}}}';
            verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: SecurityProfile Property: CRLProfile Value: '+ FVT.long257Name +'." }' );
			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CRLProfile Name: '+ FVT.long257Name +'" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET, CRLProfile Name TooLong, with CertificateProfile', function(done) {
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: CRLProfileNameTooLong","SecurityProfile":{"CRLProfileNameTooLong":{}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'CRLProfileNameTooLong', FVT.verify404NotFound, verifyConfig, done);
        });
// 113153	
        it('should return status 404 CRLProfile Name TooLong, with TLSEnabled', function(done) {
            var payload = '{"SecurityProfile":{"CRLProfileNameTooLong":{"CRLProfile":"'+ FVT.long257Name +'", "TLSEnabled": true }}}';
            verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: SecurityProfile Property: CRLProfile Value: '+ FVT.long257Name +'." }' );
			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CRLProfile Name: '+ FVT.long257Name +'" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET CRLProfile Name TooLong, with TLSEnabled', function(done) {
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: CRLProfileNameTooLong","SecurityProfile":{"CRLProfileNameTooLong":{}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'CRLProfileNameTooLong', FVT.verify404NotFound, verifyConfig, done);
        });
		
    });
	
    describe('LTPAProfile:', function() {
        it('should return status 404 LTPAProfile Name TooLong', function(done) {
            var payload = '{"SecurityProfile":{"LTPAProfileNameTooLong":{"LTPAProfile":"'+ FVT.long257Name +'"}}}';
            verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: SecurityProfile Property: LTPAProfile Value: '+ FVT.long257Name +'." }' );
			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: LTPAProfile Name: '+ FVT.long257Name +'" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, Delete All SecurityProfiles,  should be in use', function(done) {
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: LTPAProfileNameTooLong","SecurityProfile":{"LTPAProfileNameTooLong":{}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'LTPAProfileNameTooLong', FVT.verify404NotFound, verifyConfig, done);
        });
    });
    describe('OAuthProfile:', function() {
        it('should return status 404 OAuthProfile Name TooLong', function(done) {
            var payload = '{"SecurityProfile":{"OAuthProfileNameTooLong":{"OAuthProfile":"'+ FVT.long257Name +'"}}}';
            verifyConfig = JSON.parse( payload );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: SecurityProfile Property: OAuthProfile Value: '+ FVT.long257Name +'." }' );
			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: OAuthProfile Name: '+ FVT.long257Name +'" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, Delete All SecurityProfiles,  should be in use', function(done) {
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: OAuthProfileNameTooLong","SecurityProfile":{"OAuthProfileNameTooLong":{}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'OAuthProfileNameTooLong', FVT.verify404NotFound, verifyConfig, done);
        });
    });

    describe('Missing Required Parameter:', function() {
// 103724 105438 105441
        it('should return status 400 On POST: CertificateRequired ', function(done) {
            var payload = '{"SecurityProfile":{"CertificateRequired":{}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0186","Message":"The certificate profile must be set if security is enabled." };
            verifyMessage = {"status":400,"Code":"CWLNA0186","Message":"The certificate profile must be set if TLSEnabled is true." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on GET "CertificateRequired" was created', function(done) {
//            verifyConfig = {"status":404,"SecurityProfile":{"CertificateRequired":{}}};
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: CertificateRequired","SecurityProfile":{"CertificateRequired":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'CertificateRequired', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 404 when CertificateNotFound', function(done) {
            var payload = '{"SecurityProfile":{"CertificateNotFound":{"CertificateProfile": "LostInTheEther"}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA8278","Message":"Cannot find the certificate file specified. Make sure it has been imported.\n" };
//            verifyMessage = {"status":400,"Code":"CWLNA0136","Message":"The item or object is not found. Type: CertificateProfile Name: LostInTheEther" };
            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: LostInTheEther" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on GET CertificateNotFound was not created', function(done) {
//            verifyConfig = {"status":404,"SecurityProfile":{"CertificateNotFound":{}}};
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: CertificateNotFound","SecurityProfile":{"CertificateNotFound":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'CertificateNotFound', FVT.verify404NotFound, verifyConfig, done);
        });

    });

    describe('TLSEnabled:', function() {
//99398
        it('should return status 400 on post "TLSEnabled":""', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"TLSEnabled":""}}}';
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: TLSEnabled Property: InvalidType Value: JSON_STRING."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: TestSecProf Property: TLSEnabled Value: JSON_STRING."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: SecurityProfile Name: TestSecProf Property: TLSEnabled Type: JSON_STRING"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "TLSEnabled":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on post "TLSEnabled":"false" STRING', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"TLSEnabled":"false"}}}';
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: TLSEnabled Property: InvalidType Value: JSON_STRING."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: TestSecProf Property: TLSEnabled Value: JSON_STRING."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: SecurityProfile Name: TestSecProf Property: TLSEnabled Type: JSON_STRING"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "TLSEnabled":"false" STRING', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });


    describe('UseClientCertificate:', function() {
//99398
        it('should return status 400 on post "UseClientCertificate":""', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UseClientCertificate":""}}}';
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: UseClientCertificate Property: InvalidType Value: JSON_STRING."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: UseClientCertificate Property: InvalidType Value: JSON_STRING."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: TestSecProf Property: UseClientCertificate Value: JSON_STRING."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: SecurityProfile Name: TestSecProf Property: UseClientCertificate Type: JSON_STRING"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "UseClientCertificate":"" ', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on post "UseClientCertificate":"true" STRING', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UseClientCertificate":"true"}}}';
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: UseClientCertificate Property: InvalidType Value: JSON_STRING."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: TestSecProf Property: UseClientCertificate Value: JSON_STRING."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: SecurityProfile Name: TestSecProf Property: UseClientCertificate Type: JSON_STRING"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "UseClientCertificate":"true" STRING', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });


    describe('UsePasswordAuth:', function() {
//99398
        it('should return status 400 on post "UsePasswordAuthentication":""', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UsePasswordAuthentication":""}}}';
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: UsePasswordAuthentication Property: InvalidType Value: JSON_STRING."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: TestSecProf Property: UsePasswordAuthentication Value: JSON_STRING."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: SecurityProfile Name: TestSecProf Property: UsePasswordAuthentication Type: JSON_STRING"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "UsePasswordAuthentication":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 99398		
        it('should return status 400 on post "UsePasswordAuthentication":1', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UsePasswordAuthentication":1}}}';
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: UsePasswordAuthentication Property: InvalidType Value: JSON_INTEGER."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: TestSecProf Property: UsePasswordAuthentication Value: JSON_INTEGER."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: SecurityProfile Name: TestSecProf Property: UsePasswordAuthentication Type: JSON_INTEGER"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "UsePasswordAuthentication":1', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });


    describe('UseClientCipher:', function() {
//99398
        it('should return status 400 on post "UseClientCipher":""', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UseClientCipher":""}}}';
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: UseClientCipher."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: UseClientCipher Property: InvalidType Value: JSON_STRING."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: TestSecProf Property: UseClientCipher Value: JSON_STRING."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: SecurityProfile Name: TestSecProf Property: UseClientCipher Type: JSON_STRING"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "UseClientCipher":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on post "UseClientCipher":0', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UseClientCipher":0}}}';
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: UseClientCipher."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: UseClientCipher Property: InvalidType Value: JSON_INTEGER."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: TestSecProf Property: UseClientCipher Value: JSON_INTEGER."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: SecurityProfile Name: TestSecProf Property: UseClientCipher Type: JSON_INTEGER"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "UseClientCipher":0', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 120951
        it('should return status 400 on post "UseClientCipherS":false', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"UseClientCiphers":false}}}';
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: UseClientCiphers."} ;
//            verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SecurityProfile Name: TestSecProf Property: UseClientCiphers"} ;
            verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: SecurityProfile Name: TestSecProf Property: UseClientCiphers"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "UseClientCiphers":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    describe('MinimumProtocolMethod[(TLSv1),TLSv1.1,TLSv1.2]:', function() {
//99406 105436
        it('should return status 400 on post "MinimumProtocolMethod":""', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"MinimumProtocolMethod":""}}}';
            verifyConfig = JSON.parse( expectDefault );
//           verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: MinimumProtocolMethod Property: InvalidType Value: JSON_STRING."} ;
//           verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: MinimumProtocolMethod Property: InvalidType Value: JSON_STRING."} ;
// 105436    verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: TestSecProf Property: MinimumProtocolMethod Value: null."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: TestSecProf Property: MinimumProtocolMethod Value: /"/"."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MinimumProtocolMethod Value: \"\"."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MinimumProtocolMethod Value: \"\"\"\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "MinimumProtocolMethod":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 400 on post "MinimumProtocolMethod":"SSL"', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"MinimumProtocolMethod":"SSL"}}}';
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MinimumProtocolMethod Value: SSL."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: MinimumProtocolMethod Value: \"SSL\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST fails "MinimumProtocolMethod":"SSL"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
       
    });

    describe('Ciphers[Best,(Medium),Fast]:', function() {
//99409
        it('should return status 400 on post "Ciphers":""', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"Ciphers":""}}}';
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: Ciphers Property: InvalidType Value: JSON_STRING."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: SecurityProfile Name: TestSecProf Property: Ciphers Value: null."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Ciphers Value: \"\"."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Ciphers Value: \"\"\"\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "Ciphers":""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on post "Ciphers":"BEST"', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"Ciphers":"BEST"}}}';
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Ciphers Value: BEST."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Ciphers Value: \"BEST\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "Ciphers":"BEST"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

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
        it('should return status 200 on get, "SecurityProfiles" persists', function(done) {
            verifyConfig = JSON.parse( expectAllSecurityProfiles ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});


//  ====================  Delete test cases  ====================  //
    describe('Delete:', function() {

        it('should return status 200 when deleting SecurityProfile "Max Length Name"', function(done) {
            verifyConfig = JSON.parse( '{"SecurityProfile":{"'+ FVT.long32Name +'":null}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+FVT.long32Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get all after delete, "Max Length Name" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "Max Length Name" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"SecurityProfile":{"'+ FVT.long32Name +'":null}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: '+ FVT.long32Name +'","SecurityProfile":{"'+ FVT.long32Name +'":null}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long32Name, FVT.verify404NotFound, verifyConfig, done );
        });

        it('should return status 200 when deleting "TestSecProf"', function(done) {
            verifyConfig = JSON.parse( '{"SecurityProfile":{"TestSecProf":null}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'TestSecProf', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "TestSecProf" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "TestSecProf" not found', function(done) {
 //           verifyConfig = JSON.parse( '{"status":404,"SecurityProfile":{"TestSecProf":null}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: TestSecProf","SecurityProfile":{"TestSecProf":null}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestSecProf', FVT.verify404NotFound, verifyConfig, done );
        });

        it('should return status 200 when deleting "CertificateNotRequired"', function(done) {
            verifyConfig = JSON.parse( '{"SecurityProfile":{"CertificateNotRequired":null}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'CertificateNotRequired', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "CertificateNotRequired" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "CertificateNotRequired" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"SecurityProfile":{"CertificateNotRequired":null}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: CertificateNotRequired","SecurityProfile":{"CertificateNotRequired":null}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'CertificateNotRequired', FVT.verify404NotFound, verifyConfig, done );
        });

    });

	
	
//  ====================  Clean up PreReqs  ====================  //
    describe('Clean UP PreReqs:', function() {
      describe('CertificateProfile:', function() {
    
        var objName = 'TestCertProf';
        it('should return status 200 when deleting "'+ objName +'"', function(done) {
            verifyConfig = JSON.parse( '{"CertificateProfile":{"'+ objName +'":null}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'CertificateProfile/'+objName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get all after delete, "'+ objName +'" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/', FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "'+ objName +'" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"CertificateProfile":{"'+ objName +'":null}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: '+ objName +'","CertificateProfile":{"'+ objName +'":null}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/'+objName, FVT.verify404NotFound, verifyConfig, done );
        });

      });

      describe('CertificateProfile:', function() {
    
        var objName = 'aCertProf';
        it('should return status 200 when deleting "'+ objName +'"', function(done) {
            verifyConfig = JSON.parse( '{"CertificateProfile":{"'+ objName +'":null}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'CertificateProfile/'+objName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get all after delete, "'+ objName +'" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/', FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "'+ objName +'" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"CertificateProfile":{"'+ objName +'":null}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: '+ objName +'","CertificateProfile":{"'+ objName +'":null}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/'+objName, FVT.verify404NotFound, verifyConfig, done );
        });

      });

      describe('LTPAProfile:', function() {
    
        var objName = 'TestLTPAProf';
        it('should return status 200 when deleting "'+ objName +'"', function(done) {
            verifyConfig = JSON.parse( '{"LTPAProfile":{"'+ objName +'":null}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'LTPAProfile/'+objName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get all after delete "'+ objName +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'LTPAProfile/', FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete "'+ objName +'"', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"LTPAProfile":{"'+ objName +'":null}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: LTPAProfile Name: '+ objName +'","LTPAProfile":{"'+ objName +'":null}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'LTPAProfile/'+objName, FVT.verify404NotFound, verifyConfig, done );
        });

      });

      describe('OAuthProfile:', function() {
    
        var objName = 'TestOAuthProf';
        it('should return status 200 when deleting "'+ objName +'"', function(done) {
            verifyConfig = JSON.parse( '{"OAuthProfile":{"'+ objName +'":{}}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'OAuthProfile/'+objName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get all after delete "'+ objName +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'OAuthProfile/', FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete "'+ objName +'"', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"OAuthProfile":{"'+ objName +'":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: OAuthProfile Name: '+ objName +'","OAuthProfile":{"'+ objName +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'OAuthProfile/'+objName, FVT.verify404NotFound, verifyConfig, done );
        });
        
      });
      

      describe('CRLProfile:', function() {
    
        var objName = 'TestCRLProf';
        it('should return status 200 when deleting "'+ objName +'"', function(done) {
            verifyConfig = JSON.parse( '{"CRLProfile":{"'+ objName +'":{}}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'CRLProfile/'+objName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get all after delete "'+ objName +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/', FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete "'+ objName +'"', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"OAuthProfile":{"'+ objName +'":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CRLProfile Name: '+ objName +'","CRLProfile":{"'+ objName +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/'+objName, FVT.verify404NotFound, verifyConfig, done );
        });
        
      });
	  
    });

});
