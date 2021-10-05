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

var uriObject = 'CertificateProfile/'
var expectDefault = '{"CertificateProfile":{"AdminDefaultCertProf":{"Certificate": "AdminDefaultCert.pem","Key": "AdminDefaultKey.pem"}},"Version": "v1"}'
// 142046
// $ openssl x509 -dates -in cert.pem -noout
// notBefore=Aug 13 20:51:01 2015 GMT
// notAfter=Aug 10 20:51:01 2025 GMT

// $ openssl x509 -dates -in certFree.pem -noout
// notBefore=Aug 14 15:02:25 2015 GMT
// notAfter=Aug 11 15:02:25 2025 GMT

// $ openssl x509 -dates -in AdminDefaultCert.pem -noout
// notBefore=Sep 26 12:40:02 2012 GMT
// notAfter=Sep 26 12:40:02 2013 GMT
    var adminDefaultCertExpires = '2013-09-26T12:40:02.000Z' ;
    var certExpires = '2025-08-10T20:51:01.000Z' ;
    var certFreeExpires = '2025-08-11T15:02:25.000Z' ;

/* 142046 removed the TimeZone entirely ... it that sticks, 136336 and 142046 were reopened multiple times
if ( FVT.A1_TYPE == "DOCKER" ) {
    FVT.trace(0 , 'IMA Environment is '+ FVT.A1_TYPE );
// 136336
//    var adminDefaultCertExpires = '2013-09-28T04:08:12.000Z' ;
//    var certExpires = '2025-08-12T23:07:12.000Z' ;
//    var certFreeExpires = '2025-08-11T17:31:12.000Z' ;
    var adminDefaultCertExpires = '2013-09-26T12:40:02.000Z' ;
    var certExpires = '2025-08-10T20:51:01.000Z' ;
    var certFreeExpires = '2025-08-11T15:02:25.000Z' ;

} else {  // must be RPM
    var date = new Date();
	var month = date.getMonth();
	var DST_START = 3;  // Daylight Savings Time STARTS
	var DST_END = 10; // Daylight Savings Time ENDS
    if ( month >= DST_START && month <= DST_END ) {
// 136336
		var adminDefaultCertExpires = '2013-09-26T07:40:02.000-05:00' ;
		var certExpires = '2025-08-10T15:51:01.000-05:00' ;
		var certFreeExpires = '2025-08-11T10:02:25.000-05:00' ;
    } else {
// 136336
//	    var adminDefaultCertExpires = '2013-09-28T04:08:12.000-06:00' ;
//		var certExpires = '2025-08-12T23:07:12.000-06:00' ;
//		var certFreeExpires = '2025-08-11T17:31:12.000-06:00' ;
		var adminDefaultCertExpires = '2013-09-26T06:40:02.000-06:00' ;
		var certExpires = '2025-08-10T14:51:01.000-06:00' ;
		var certFreeExpires = '2025-08-11T09:02:25.000-06:00' ;
	}
}
*/
var expectAllCertificateProfiles = '{ "Version": "v1", "CertificateProfile": { \
 "AdminDefaultCertProf": {  "Certificate": "AdminDefaultCert.pem",  "Key": "AdminDefaultKey.pem"  }, \
 "TestCertProf": {  "Certificate": "TestCert.pem",  "Key": "TestKey.pem",  "ExpirationDate": "'+ adminDefaultCertExpires +'"  }, \
 "'+ FVT.long256Name +'": {  "Certificate": "B256Cert.pem",  "Key": "B256Key.pem",  "ExpirationDate": "'+ certExpires +'"  }, \
 "ReuseFreedKeyCertName": {  "Certificate": "A256Cert.pem",  "Key": "A256Key.pem",  "ExpirationDate": "'+ certFreeExpires +'" }}}' ;

//  ====================  Test Cases Start Here  ====================  //
//      "ListObjects":"Name,Certificate,Key,ExpirationDate",
//   also:  CertFilePassword, KeyFilePassword, Overwrite

describe('CertificateProfile:', function() {
this.timeout( FVT.defaultTimeout );

    // Get test cases
    describe('Get - verify in Pristine State:', function() {
        it('should return status 200 on get, DEFAULT of "CertProf":{"AdminDefCP":{...}}', function(done) {
			verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
	
	});

//  ====================   Create - Add - Update test cases  ====================  //
    describe('Put Keys to AdminEndpoint:', function() {

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
    

        it('should return status 200 on PUT "A256Cert.pem"', function(done) {
		    sourceFile = 'certFree.pem';
			targetFile = 'A256Cert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "A256Key.pem"', function(done) {
		    sourceFile = 'keyFree.pem';
			targetFile = 'A256Key.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "B256Cert.pem"', function(done) {
		    sourceFile = 'cert.pem';
			targetFile = 'B256Cert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "B256Key.pem"', function(done) {
		    sourceFile = 'key.pem';
			targetFile = 'B256Key.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

	});


    describe('Create, Update:', function() {

        it('should return status 200 on post "TestCertProf"', function(done) {
            var payload = '{"CertificateProfile":{"TestCertProf":{"Certificate": "TestCert.pem","Key": "TestKey.pem"}}}';
//            verifyConfig = JSON.parse( '{"CertificateProfile":{"TestCertProf":{"Certificate": "TestCert.pem","ExpirationDate": "2013-09-28T04:08+0000","Key": "TestKey.pem"}}}' );
            verifyConfig = JSON.parse( '{"CertificateProfile":{"TestCertProf":{"Certificate": "TestCert.pem","ExpirationDate": "'+ adminDefaultCertExpires +'","Key": "TestKey.pem"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestCertProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"TestCertProf", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "256 Char Name CertProf"', function(done) {
            var payload = '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "A256Cert.pem","Key": "A256Key.pem"}}}';
//            verifyConfig = JSON.parse( '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "A256Cert.pem","Key": "A256Key.pem","ExpirationDate": "2025-08-11T17:31+0000"}}}' );
            verifyConfig = JSON.parse( '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "A256Cert.pem","Key": "A256Key.pem","ExpirationDate": "'+ certFreeExpires +'"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "256 Char Name CertProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 113503
        it('should return status 400 on post "Update Cert and Key" forget password needed', function(done) {
            var payload = '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "B256Cert.pem","Key": "B256Key.pem"}}}';
//            verifyConfig = JSON.parse( '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "A256Cert.pem","Key": "A256Key.pem","ExpirationDate": "2025-08-11T17:31+0000"}}}' );
            verifyConfig = JSON.parse( '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "A256Cert.pem","Key": "A256Key.pem","ExpirationDate": "'+ certFreeExpires +'"}}}' );
//			verifyMessage = {"status":400,"Code":"CWLNA8285","Message":"The key file specified has a password. need input \"KeyFilePassword=value\".\n"} ;
			verifyMessage = {"status":400,"Code":"CWLNA6191","Message":"The key file specified has a password. Need to input a \"KeyFilePassword\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "Update Cert and Key" forgot password', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Update Cert and Key" with password', function(done) {
            var payload = '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "B256Cert.pem","Key": "B256Key.pem", "KeyFilePassword":"mocha"}}}';
//            verifyConfig = JSON.parse( '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "B256Cert.pem","Key": "B256Key.pem","ExpirationDate": "2025-08-12T23:07+0000" }}}' );
            verifyConfig = JSON.parse( '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "B256Cert.pem","Key": "B256Key.pem","ExpirationDate": "'+ certExpires +'" }}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Update Cert and Key" with password', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

		//DEFECT 113503 - Reuse the replaced KEYS, the names should be unattacted to any Profile and Cert and Key names are reuseable

        it('should return status 200 on PUT "A256Cert.pem"', function(done) {
		    sourceFile = 'certFree.pem';
			targetFile = 'A256Cert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "A256Key.pem"', function(done) {
		    sourceFile = 'keyFree.pem';
			targetFile = 'A256Key.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
// 113503
        it('should return status 200 on RE-POST REusing FREEed Key/Cert Names', function(done) {
            var payload = '{"CertificateProfile":{"ReuseFreedKeyCertName":{"Certificate": "A256Cert.pem","Key": "A256Key.pem"}}}';
//            verifyConfig = {"CertificateProfile":{"ReuseFreedKeyCertName":{"Certificate": "A256Cert.pem","Key": "A256Key.pem","ExpirationDate": "2025-08-11T17:31+0000"}}};
            verifyConfig = JSON.parse( '{"CertificateProfile":{"ReuseFreedKeyCertName":{"Certificate": "A256Cert.pem","Key": "A256Key.pem","ExpirationDate": "'+ certFreeExpires +'"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after RE-POST REusing FREEed Key/Cert Names', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });


// RE upload Password Key and Cert Files

        it('should return status 200 on re-PUT "B256Cert.pem"', function(done) {
		    sourceFile = 'cert.pem';
			targetFile = 'B256Cert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on re-PUT "B256Key.pem"', function(done) {
		    sourceFile = 'key.pem';
			targetFile = 'B256Key.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });


        it('should return status 400 on post "Update Cert and Key" same file forget overwrite needed', function(done) {
            var payload = '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "B256Cert.pem","Key": "B256Key.pem", "KeyFilePassword":"mocha"}}}';
//            verifyConfig = JSON.parse( '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "B256Cert.pem","Key": "B256Key.pem","ExpirationDate": "2025-08-12T23:07+0000"}}}' );
            verifyConfig = JSON.parse( '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "B256Cert.pem","Key": "B256Key.pem","ExpirationDate": "'+ certExpires +'"}}}' );
//			verifyMessage = {"status":400,"Code":"CWLNA8280","Message":"The certificate already exists. Set Overwrite=True to replace the existing certificate."} ;
			verifyMessage = {"status":400,"Code":"CWLNA6186","Message":"The certificate already exists. Set Overwrite to true to replace the existing certificate."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "Update Cert and Key" forgot overwrite', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "Update Cert and Key" with password and overwrite', function(done) {
            var payload = '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "B256Cert.pem","Key": "B256Key.pem", "KeyFilePassword":"mocha", "Overwrite":true }}}';
//            verifyConfig = JSON.parse( '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "B256Cert.pem","Key": "B256Key.pem","ExpirationDate": "2025-08-12T23:07+0000"}}}' );
            verifyConfig = JSON.parse( '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "B256Cert.pem","Key": "B256Key.pem","ExpirationDate": "'+ certExpires +'"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Update Cert and Key" with password and overwrite', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
		
	});


    describe('Collisions:', function() {

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
    
        it('should return status 400 on post "Update A256CertProf with InUse CERT"', function(done) {
            var payload = '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "TestCert.pem"}}}';
//            verifyConfig = JSON.parse('{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "B256Cert.pem","Key": "B256Key.pem","ExpirationDate": "2025-08-12T23:07+0000"}},"Version": "v1"}');
            verifyConfig = JSON.parse('{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "B256Cert.pem","Key": "B256Key.pem","ExpirationDate": "'+ certExpires +'"}},"Version": "v1"}');
//			verifyMessage = { "Version":"v1","Code":"CWLNA0112","Message":"A property value is not valid: Property: Key Value: null." };
			verifyMessage = { "Version":"v1","Code":"CWLNA0451","Message":"The specified certificate is in use." };
// back to 451			verifyMessage = { "Version":"v1","Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: Key Value: null." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "A256CertProf still using B256 Key"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on post "Update A256CertProf with InUse KEYs"', function(done) {
            var payload = '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Key": "TestKey.pem"}}}';
//            verifyConfig = JSON.parse('{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "B256Cert.pem","Key": "B256Key.pem","ExpirationDate": "2025-08-12T23:07+0000"}},"Version": "v1"}');
            verifyConfig = JSON.parse('{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "B256Cert.pem","Key": "B256Key.pem","ExpirationDate": "'+ certExpires +'"}},"Version": "v1"}');
//			verifyMessage = { "Version":"v1","Code":"CWLNA0112","Message":"A property value is not valid: Property: Certificate Value: null." };
			verifyMessage = { "Version":"v1","Code":"CWLNA0452","Message":"The specified key file is in use." };
// back to 452			verifyMessage = { "Version":"v1","Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: Key Value: null." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, "A256CertProf still using B256 Keys"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 404 on get, "NotFoundCertProf"', function(done) {
//		    verifyConfig = {"status":404,"CertificateProfile":{"NotFoundCertProf":{}}} ;
		    verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: NotFoundCertProf","CertificateProfile":{"NotFoundCertProf":{}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"NotFoundCertProf", FVT.verify404NotFound, verifyConfig, done);
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
        it('should return status 200 on get, "CertificateProfiles" persists', function(done) {
            verifyConfig = JSON.parse( expectAllCertificateProfiles );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});

//  ====================  Delete test cases  ====================  //
    describe('Delete:', function() {

        it('should return status 200 when deleting "Max Length Name"', function(done) {
            verifyConfig = JSON.parse( '{"CertificateProfile":{"'+ FVT.long256Name +'":{}}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get all after delete, "Max Length Name" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "Max Length Name" not found', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"CertificateProfile":{"'+ FVT.long256Name +'":{}}}' );
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: '+ FVT.long256Name +'","CertificateProfile":{"'+ FVT.long256Name +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.verify404NotFound, verifyConfig, done );
        });
// 113503
        it('should return status 200 when deleting "ReuseFreedKeyCertName"', function(done) {
            verifyConfig = JSON.parse( '{"CertificateProfile":{"ReuseFreedKeyCertName":{}}}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'ReuseFreedKeyCertName', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get all after delete, "ReuseFreedKeyCertName" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "ReuseFreedKeyCertName" not found', function(done) {
//		    verifyConfig = JSON.parse( '{"status":404,"CertificateProfile":{"ReuseFreedKeyCertName":{}}}' );
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: ReuseFreedKeyCertName","CertificateProfile":{"ReuseFreedKeyCertName":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'ReuseFreedKeyCertName', FVT.verify404NotFound, verifyConfig, done );
        });


        it('should return status 200 when deleting "TestCertProf"', function(done) {
            verifyConfig = {"CertificateProfile":{"TestCertProf":{}}} ;
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'TestCertProf', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "TestCertProf" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "TestCertProf" not found', function(done) {
//		    verifyConfig = {"status":404,"CertificateProfile":{"TestCertProf":{}}} ;
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: TestCertProf","CertificateProfile":{"TestCertProf":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestCertProf', FVT.verify404NotFound, verifyConfig, done );
        });
		
    });

//  ====================  Error test cases  ====================  //
    describe('Error: General:', function() {

        it('should return status 400 when trying to Delete All CertifcateProfiles with POST, bad form', function(done) {
            var payload = '{"CertificateProfile":null}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: {\"CertificateProfile\":null} is not valid."} ;
			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: \"CertificateProfile\":null is not valid."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, Delete All CertificateProfiles,  bad form', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
    });
    

describe('Error Test Cases: Missing Required Parameter:', function() {

        var objName = 'CertificateRequired'
        it('should return status 400 when No Certificate set ', function(done) {
            var payload = '{"CertificateProfile":{"'+ objName +'":{"Key": "TestKey.pem"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"Code":"CWLNA0112","Message":"A property value is not valid: Property: Certificate Value: null." };
			verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: Certificate Value: null." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on get, '+ objName +' was not created', function(done) {
//			verifyConfig = JSON.parse( '{"status":404,"CertificateProfile":{"'+ objName +'":{"Key": "TestKey.pem"}}}' );
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: '+ objName +'","CertificateProfile":{"'+ objName +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+objName, FVT.verify404NotFound, verifyConfig, done);
        });

        objName = 'KeyRequired'
        it('should return status 400 when No Key set ', function(done) {
            var payload = '{"CertificateProfile":{"'+ objName +'":{"Certificate": "TestCert.pem"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"Code":"CWLNA0112","Message":"A property value is not valid: Property: Key Value: null." };
			verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: Key Value: null." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on get, '+ objName +' was not created', function(done) {
//			verifyConfig = JSON.parse( '{"status":404,"CertificateProfile":{"'+ objName +'":{"Key": "TestKey.pem"}}}' );
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: '+ objName +'","CertificateProfile":{"'+ objName +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+objName, FVT.verify404NotFound, verifyConfig, done);
        });


        it('should return status 200 on PUT "y.pem"', function(done) {
		    sourceFile = 'keyFree.pem';
			targetFile = 'y.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
        objName = 'CertNotFound'
        it('should return status 400 when CERT is not found', function(done) {
            var payload = '{"CertificateProfile":{"'+ objName +'":{"Certificate": "x.pem","Key":"y.pem"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"status":400,"Code":"CWLNA8278","Message":"Cannot find the certificate file specified. Make sure it has been imported.\n" };
//			verifyMessage = {"status":400,"Code":"CWLNA8278","Message":"Cannot find the certificate file specified. Make sure it has been uploaded." };
			verifyMessage = {"status":400,"Code":"CWLNA6184","Message":"Cannot find the certificate file x.pem. Make sure it has been uploaded." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on get, '+ objName +' was not created', function(done) {
//			verifyConfig = JSON.parse( '{"status":404,"CertificateProfile":{"'+ objName +'":{"Key": "TestKey.pem"}}}' );
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: '+ objName +'","CertificateProfile":{"'+ objName +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+objName, FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 on PUT "x.pem"', function(done) {
		    sourceFile = 'certFree.pem';
			targetFile = 'x.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        objName = 'KeyNotFound'
        it('should return status 400 when KEY is not found ', function(done) {
            var payload = '{"CertificateProfile":{"'+ objName +'":{"Key": "k.pem","Certificate": "x.pem"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"status":400,"Code":"CWLNA8278","Message":"Cannot find the certificate file specified. Make sure it has been imported.\n" };
//			verifyMessage = {"status":400,"Code":"CWLNA8278","Message":"Cannot find the certificate file specified. Make sure it has been uploaded." };
			verifyMessage = {"status":400,"Code":"CWLNA6185","Message":"Cannot find the key file k.pem. Make sure it has been uploaded." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on get, '+ objName +' was not created', function(done) {
//			verifyConfig = JSON.parse( '{"status":404,"CertificateProfile":{"'+ objName +'":{"Key": "TestKey.pem"}}}' );
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: '+ objName +'","CertificateProfile":{"'+ objName +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+objName, FVT.verify404NotFound, verifyConfig, done);
        });

    });
    

describe('Error: Mixup Key Cert:', function() {

        it('should return status 200 on xPUT "TestCert.pem"', function(done) {
		    sourceFile = 'AdminDefaultCert.pem';
			targetFile = 'TestCert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "xcKey.pem"', function(done) {
		    sourceFile = 'AdminDefaultKey.pem';
			targetFile = 'TestKey.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on POST to create Certificate-Key cris-cross ', function(done) {
            var payload = '{"CertificateProfile":{"CertificateCrisCross":{"Certificate": "TestCert.pem","Key": "TestKey.pem"}}}';
		    verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done );
        });
        it('should return status 200 on get, CertificateCrisCross ', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'CertificateCrisCross', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on xPUT "TestCert.pem"', function(done) {
		    sourceFile = 'AdminDefaultCert.pem';
			targetFile = 'TestCert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "xcKey.pem"', function(done) {
		    sourceFile = 'AdminDefaultKey.pem';
			targetFile = 'TestKey.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on xPUT "TestCert.pem"', function(done) {
		    sourceFile = 'AdminDefaultCert.pem';
			targetFile = 'xcCert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on xPUT "TestKey.pem"', function(done) {
		    sourceFile = 'keyFree.pem';
			targetFile = 'xcKey.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "badCert.pem"', function(done) {
		    sourceFile = 'package.json';
			targetFile = 'badCert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "badKey.pem"', function(done) {
		    sourceFile = 'package.json';
			targetFile = 'badKey.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 400 on POST when Certificate-Key cris-cross without overwrite ', function(done) {
            var payload = '{"CertificateProfile":{"CertificateCrisCross":{"Certificate": "TestKey.pem","Key": "TestCert.pem"}}}';
		    verifyConfig = JSON.parse( '{"CertificateProfile":{"CertificateCrisCross":{"Certificate": "TestCert.pem","Key": "TestKey.pem"}}}' );
			verifyMessage = {"status":400,"Code":"CWLNA6180","Message":"The certificate failed the verification." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get, CertificateCrisCross without overwirte - no chg ', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'CertificateCrisCross', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 on POST when Certificate-Key cris-cross with overwrite ', function(done) {
            var payload = '{"CertificateProfile":{"CertificateCrisCross":{"Certificate": "TestKey.pem","Key": "TestCert.pem", "Overwrite":true}}}';
		    verifyConfig = JSON.parse( '{"CertificateProfile":{"CertificateCrisCross":{"Certificate": "TestCert.pem","Key": "TestKey.pem"}}}' );
			verifyMessage = {"status":400,"Code":"CWLNA6180","Message":"The certificate failed the verification." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on get, CertificateCrisCross with overwrite, still no chg ', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'CertificateCrisCross', FVT.getSuccessCallback, verifyConfig, done);
        });

		
        it('should return status 400 on POST when Certificate-Key mismatch ', function(done) {
            var payload = '{"CertificateProfile":{"CertificateKeyMisMatch":{"Certificate": "xcCert.pem","Key": "xcKey.pem"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA6189","Message":"The certificate and key file do not match." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on get, CertificateKeyMisMatch when Certificate-Key mismatch ', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: CertificateKeyMisMatch","CertificateProfile":{"CertificateKeyMisMatch":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'CertificateKeyMisMatch', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST when Bad Certificate ', function(done) {
            var payload = '{"CertificateProfile":{"BadCertificate":{"Certificate": "badCert.pem","Key": "xcKey.pem"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = {"status":400,"Code":"CWLNA6180","Message":"The certificate failed the verification." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on get, BadCertificate when Certificate-Key mismatch ', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: BadCertificate","CertificateProfile":{"BadCertificate":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'BadCertificate', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST when  bad Key ', function(done) {
            var payload = '{"CertificateProfile":{"BadKey":{"Certificate": "xcCert.pem","Key": "badKey.pem"}}}';
		    verifyConfig = JSON.parse( payload );
//			verifyMessage = {"status":400,"Code":"CWLNA8285","Message":"The key failed the verification." };
			verifyMessage = {"status":400,"Code":"CWLNA6191","Message":"The key file specified has a password. Need to input a \"KeyFilePassword\"." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on get, BadKey when Certificate-Key mismatch ', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: BadKey","CertificateProfile":{"BadKey":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'BadKey', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 when deleting "CertificateCrisCross"', function(done) {
            verifyConfig = {"CertificateProfile":{"CertificateCrisCross":{}}} ;
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'CertificateCrisCross', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "CertificateCrisCross" gone', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "CertificateCrisCross" not found', function(done) {
		    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: CertificateCrisCross","CertificateProfile":{"CertificateCrisCross":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'CertificateCrisCross', FVT.verify404NotFound, verifyConfig, done );
        });
// Added per list from R., yet I don't see 144, I see can not find the file
        it('should return status 400 on POST when Certificate Name TooLong ', function(done) {
            var payload = '{"CertificateProfile":{"CertificateNameTooLong":{"Certificate": "'+ FVT.long256Name +'","Key": "TestKey.pem"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: CertificateProfile Property: Certificate Value: '+ FVT.long256Name +'." }' );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA8287","Message":"The value that is specified for the property on the configuration object is too long. Object: CertificateProfile Property: Certificate Value: '+ FVT.long256Name +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on get, CertificateNameTooLong ', function(done) {
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: CertificateNameTooLong","CertificateProfile":{"CertificateNameTooLong":{}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'CertificateNameTooLong', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST when Key Name TooLong ', function(done) {
            var payload = '{"CertificateProfile":{"KeyNameTooLong":{"Key": "'+ FVT.long256Name +'","Certificate": "TestCert.pem"}}}';
		    verifyConfig = JSON.parse( payload );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: CertificateProfile Property: Key Value: '+ FVT.long256Name +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on get, KeyNameTooLong ', function(done) {
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: KeyNameTooLong","CertificateProfile":{"KeyNameTooLong":{}}} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'KeyNameTooLong', FVT.verify404NotFound, verifyConfig, done);
        });

	});
    
    
});
