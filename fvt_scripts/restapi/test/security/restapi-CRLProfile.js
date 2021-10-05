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
var crlUrlFile = '/intCA1-client-crt-crl.pem'
var crlUrlRoot = '/restapi/test/CRL-Certs';

var expectDefault = '{"CRLProfile": {},"Version": "'+ FVT.version +'"}' ;
var expectTestCRLProfile = '{"CRLProfile": {"TestCRLProf": { "CRLSource": "crl-list.pem", "UpdateInterval": 5, "RevalidateConnection": true }},"Version": "'+ FVT.version +'"}' ;

//====================  Test Cases Start Here  ====================  //
describe('CRLProfile:', function() {

    this.timeout( FVT.defaultTimeout );

    //  ====================   GET test cases  ====================  //
    describe('GET - verify in Pristine State:', function() {

        it('should return status 200 on GET, DEFAULT of "CRLProfile":{}}', function(done) {
            verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile', FVT.getSuccessCallback, verifyConfig, done);
        });
    });
	
    //  ====================   Create - Add - Update test cases  ====================  //
    describe('Prereq files:', function() {

        it('should return status 200 on put "Server Certificate"', function(done) {
            sourceFile = './test/CRL-Certs/imaserver-crt.pem';
            targetFile = 'CRLServerCert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on put "Server Key"', function(done) {
            sourceFile = './test/CRL-Certs/imaserver-key.pem';
            targetFile = 'CRLServerKey.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on put "Trusted Intermediate Certificate"', function(done) {
            sourceFile = './test/CRL-Certs/intCA1-client-crt.pem';
            targetFile = 'TrustedIntermediateCert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
    
        it('should return status 200 on put "Trusted Root Certificate"', function(done) {
            sourceFile = './test/CRL-Certs/rootCA-crt.pem';
            targetFile = 'TrustedRootCert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
    
        it('should return status 200 on put "CRL File"', function(done) {
            sourceFile = './test/CRL-Certs/intCA1-client-crt-crl.pem';
            targetFile = 'crl-list.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

    });

    describe('Prereq Objects: Create Server Certificate, Security Profile, Trusted Certificates', function() {

        it('should return status 200 on post create "CRLCertProf"', function(done) {
            var payload = '{"CertificateProfile":{"CRLCertProf":{"Certificate": "CRLServerCert.pem","Key": "CRLServerKey.pem"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
	
        it('should return status 200 on get, "CRLCertProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/CRLCertProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post create "CRLSecProf"', function(done) {
            var payload = '{"SecurityProfile":{"CRLSecProf":{"CertificateProfile": "CRLCertProf","UsePasswordAuthentication":false, "UseClientCertificate":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
	
        it('should return status 200 on get, "CRLSecProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'SecurityProfile/CRLSecProf', FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 200 on post create "Trusted Root Certificate"', function(done) {
            var payload = '{"TrustedCertificate":[{"SecurityProfileName":"CRLSecProf","TrustedCertificate": "TrustedRootCert.pem"}]}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 200 on get "Trusted Root Certificate"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'TrustedCertificate/CRLSecProf/TrustedRootCert.pem', FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 200 on post "Trusted Intermediate Certificate"', function(done) {
            var payload = '{"TrustedCertificate":[{"SecurityProfileName":"CRLSecProf","TrustedCertificate": "TrustedIntermediateCert.pem"}]}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 200 on get "Trusted Intermediate Certificate"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'TrustedCertificate/CRLSecProf/TrustedIntermediateCert.pem', FVT.getSuccessCallback, verifyConfig, done);
        });
        
    });
	
    describe('Enable Endpoint with Security Profile', function() {
	
        it('should return status 200 on post "DemoEndpoint"', function(done) {
            var payload = '{"Endpoint":{"DemoEndpoint":{"Enabled": true, "SecurityProfile": "CRLSecProf"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 on get, "DemoEndpoint"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'Endpoint/DemoEndpoint', FVT.getSuccessCallback, verifyConfig, done);
        });
    });
	
    describe('Create CRLProfile, Add to Security Profile, Update CRL Profile Properties', function() {

        it('should return status 200 on post create "TestCRLProf"', function(done) {
            var payload = '{"CRLProfile":{"TestCRLProf":{"CRLSource": "crl-list.pem"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on get, "TestCRLProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post add CRL to "CRLSecProf"', function(done) {
            var payload = '{"SecurityProfile":{"CRLSecProf":{"CRLProfile": "TestCRLProf"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 on get, "CRLSecProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'SecurityProfile/CRLSecProf', FVT.getSuccessCallback, verifyConfig, done);
        });
	
    });
	

    describe('RevalidateConnection[F|T]', function() {

        it('should return status 200 POST "RevalidateConnection": true', function(done) {
            var payload = '{"CRLProfile":{"TestCRLProf":{"RevalidateConnection": true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 GET "RevalidateConnection": true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 POST "RevalidateConnection":null (Default)', function(done) {
            var payload = '{"CRLProfile":{"TestCRLProf":{"RevalidateConnection":null}}}';
            verifyConfig = {"CRLProfile":{"TestCRLProf":{"RevalidateConnection": false}}};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 GET "RevalidateConnection":null', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 POST "RevalidateConnection": true', function(done) {
            var payload = '{"CRLProfile":{"TestCRLProf":{"RevalidateConnection": true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 GET "RevalidateConnection": true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 POST "RevalidateConnection": false', function(done) {
            var payload = '{"CRLProfile":{"TestCRLProf":{"RevalidateConnection": false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 GET "RevalidateConnection": true', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 POST "RevalidateConnection": true for testing', function(done) {
            var payload = '{"CRLProfile":{"TestCRLProf":{"RevalidateConnection": true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 GET "RevalidateConnection": true for testing', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
        });        

    });
	

    describe('UpdateInterval[5-60-43200]', function() {

        it('should return status 200 POST "UpdateInterval": 5', function(done) {
            var payload = '{"CRLProfile":{"TestCRLProf":{"UpdateInterval": 5}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 GET "UpdateInterval": 5', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 POST "UpdateInterval":null (Default 60)', function(done) {
            var payload = '{"CRLProfile":{"TestCRLProf":{"UpdateInterval": null }}}';
            verifyConfig = {"CRLProfile":{"TestCRLProf":{"UpdateInterval": 60 }}};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 GET "UpdateInterval": null (Default 60)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 POST "UpdateInterval": 6', function(done) {
            var payload = '{"CRLProfile":{"TestCRLProf":{"UpdateInterval": 6}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 GET "UpdateInterval": 6', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 POST "UpdateInterval": 43200', function(done) {
            var payload = '{"CRLProfile":{"TestCRLProf":{"UpdateInterval": 43200}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 GET "UpdateInterval": 43200', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 POST "UpdateInterval": 43199', function(done) {
            var payload = '{"CRLProfile":{"TestCRLProf":{"UpdateInterval": 43199}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 GET "UpdateInterval": 43199', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 POST "UpdateInterval": 5 for testing', function(done) {
            var payload = '{"CRLProfile":{"TestCRLProf":{"UpdateInterval": 5}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 GET "UpdateInterval": 5 for testing', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
        });

    });
	

    describe('CRLSource[File or URL]R', function() {
		        
        it('should return status 200 POST update source to url for "TestCRLProf"', function(done) {
            var payload = '{"CRLProfile":{"TestCRLProf":{"CRLSource": "'+ FVT.M1_URL + crlUrlRoot + crlUrlFile +'", "Overwrite":true}}}';
            //verifyConfig = JSON.parse( '{"CRLProfile":{"TestCRLProf":{"CRLSource": "'+ FVT.M1_URL + crlUrlRoot + crlUrlFile +'" }}}' );
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 200 GET, update source to url for "TestCRLProf"', function(done) {
        	delete verifyConfig.CRLProfile.TestCRLProf.Overwrite;
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
        });   
    });
    
    describe('Switch back between url and uploaded file while disabling and enabling Endpoint', function() {
    	
		it('should return status 200 on post disable Endpoint: "DemoEndpoint"', function(done) {
            var payload = '{"Endpoint":{"DemoEndpoint":{"Enabled": false, "SecurityProfile": ""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
		it('should return status 200 on get Endpoint: "DemoEndpoint"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'Endpoint/DemoEndpoint', FVT.getSuccessCallback, verifyConfig, done);
        });
		
		it('should return status 200 on post enable Endpoint: "DemoEndpoint"', function(done) {
            var payload = '{"Endpoint":{"DemoEndpoint":{"Enabled": true, "SecurityProfile": "CRLSecProf"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
		it('should return status 200 on get Endpoint: "DemoEndpoint"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'Endpoint/DemoEndpoint', FVT.getSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 on put back of initial uploaded file "CRL File"', function(done) {
            sourceFile = './test/CRL-Certs/intCA1-client-crt-crl.pem';
            targetFile = 'crl-list.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
// 146713
        it('should return status 200 on post update from url to uploaded file for "TestCRLProf"', function(done) {
            var payload = '{"CRLProfile":{"TestCRLProf":{"CRLSource": "crl-list.pem", "Overwrite":true}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on get, "TestCRLProf"', function(done) {
        	delete verifyConfig.CRLProfile.TestCRLProf.Overwrite;
            FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
        });   
    });

    //  ====================   Error Cases With Existing CRL Profile ====================  //
	describe('Errors: ', function() {
		
		describe('Try to overwrite an existing CRL file with overwrite property set to false', function() {
		
			it('should return status 200 on put "CRL File"', function(done) {
				sourceFile = 'test/CRL-Certs/intCA1-client-crt-crl.pem';
				targetFile = 'crl-overwrite-list.pem';
				FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
			});

			it('should return status 400 POST attempt overwrite:false "TestCRLProf"', function(done) {
				var payload = '{"CRLProfile":{"TestCRLProf":{"CRLSource": "crl-overwrite-list.pem","Overwrite":false}}}';
				verifyConfig = JSON.parse(expectTestCRLProfile);
				verifyMessage = {"status":400,"Code":"CWLNA6186","Message":"The certificate already exists. Set Overwrite to true to replace the existing certificate."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 attempt overwrite:false "TestCRLProf"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
			});   
		});

		describe('Try to delete an in use CRLProfile', function() {

			var objCRLName = 'TestCRLProf';
			var objSecName = 'CRLSecProf';
			
			it('should return status 400 on delete "'+ objCRLName +'"', function(done) {
//				verifyConfig = JSON.parse( '{"CRLProfile":{"TestCRLProf":null}}' );
				verifyConfig = JSON.parse(expectTestCRLProfile);
				verifyMessage = {"status":400,"Code":"CWLNA0376","Message":"The Object: CRLProfile, Name: "+ objCRLName + " is still being used by Object: SecurityProfile, Name: " + objSecName };
				FVT.makeDeleteRequest( FVT.uriConfigDomain + 'CRLProfile/' + objCRLName, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 attempt overwrite:false "TestCRLProf"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
			});   
		});
		
		describe('Try to create some invalid CRL Profiles', function() {

			var objCRLName = 'TestCRLProf';
		
			it('should return status 400 POST empty CRLSource value for "TestCRLProf"', function(done) {
				var payload = '{"CRLProfile":{"TestCRLProf":{"CRLSource": ""}}}';
				verifyConfig = JSON.parse(expectTestCRLProfile);
				verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: CRLSource Value: null."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 GET empty CRLSource value for "TestCRLProf"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
			});   
			
			it('should return status 400 POST unknown host name value for "TestCRLProf"', function(done) {
			    this.timeout( FVT.crlTimeout );   // This command can take a while to return
				var payload = '{"CRLProfile":{"TestCRLProf":{"CRLSource": "http://kdjfjdfi.gov", "Overwrite":true}}}';
				verifyConfig = JSON.parse(expectTestCRLProfile);
				verifyMessage = {"status":400,"Code":"CWLNA0386","Message":"Error received from the CRL server. The cURL return code is 6"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 GET unknown host name value for "TestCRLProf"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
			});   

			it('should return status 400 POST bad UpdateInterval value for "TestCRLProf"', function(done) {
				var payload = '{"CRLProfile":{"TestCRLProf":{"CRLSource": "crl-list.pem", "UpdateInterval": "60"}}}';
				verifyConfig = JSON.parse(expectTestCRLProfile);
				verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: CRLProfile Name: TestCRLProf Property: UpdateInterval Type: JSON_STRING"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 GET bad UpdateInterval value for "TestCRLProf"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
			});   
			
			it('should return status 400 POST bad Revalidation value for "TestCRLProf"', function(done) {
				var payload = '{"CRLProfile":{"TestCRLProf":{"CRLSource": "crl-list.pem", "RevalidateConnection": 60}}}';
				verifyConfig = JSON.parse(expectTestCRLProfile);
				verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: CRLProfile Name: " + objCRLName + " Property: RevalidateConnection Type: JSON_INTEGER"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 GET bad Revalidation value for "TestCRLProf"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
			});   

			it('should return status 200 POST CRLSource Not Found', function(done) {
				var payload = '{"CRLProfile":{"TestCRLProf":{"CRLSource": "fileNotFound", "Overwrite":true}}}';
				verifyConfig = JSON.parse( expectTestCRLProfile );
				verifyMessage = {"status":400,"Code":"CWLNA6184","Message":"Cannot find the certificate file fileNotFound. Make sure it has been uploaded."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 GET CRLSource Not Found', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
			});   

		});

		describe('RevalidateConnection[F|T]', function() {

			it('should return status 200 POST "RevalidateConnection": "FALSE" not a boolean', function(done) {
				var payload = '{"CRLProfile":{"TestCRLProf":{"RevalidateConnection": "FALSE"}}}';
				verifyConfig = JSON.parse(expectTestCRLProfile);
				verifyMessage={ "status":400, "Code":"CWLNA0127", "Message":"The property type is not valid. Object: CRLProfile Name: TestCRLProf Property: RevalidateConnection Type: JSON_STRING" } ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 GET "RevalidateConnection":true, "FALSE" not a boolean', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return status 200 POST "RevalidateConnection":"" not an empty string', function(done) {
				var payload = '{"CRLProfile":{"TestCRLProf":{"RevalidateConnection":""}}}';
				verifyConfig = JSON.parse(expectTestCRLProfile);
				verifyMessage={ "status":400, "Code":"CWLNA0127", "Message":"The property type is not valid. Object: CRLProfile Name: TestCRLProf Property: RevalidateConnection Type: JSON_STRING" } ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 GET "RevalidateConnection":"" not an empty string', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return status 200 POST "RevalidateConnection": 0 not Integer', function(done) {
				var payload = '{"CRLProfile":{"TestCRLProf":{"RevalidateConnection": 0 }}}';
				verifyConfig = JSON.parse(expectTestCRLProfile);
				verifyMessage={ "status":400, "Code":"CWLNA0127", "Message":"The property type is not valid. Object: CRLProfile Name: TestCRLProf Property: RevalidateConnection Type: JSON_INTEGER" } ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 GET "RevalidateConnection":  0 not Integer', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
			});

		});
		

		describe('UpdateInterval[5-60-43200]', function() {

			it('should return status 200 POST "UpdateInterval": 4, too low', function(done) {
				var payload = '{"CRLProfile":{"TestCRLProf":{"UpdateInterval": 4}}}';
				verifyConfig = JSON.parse(expectTestCRLProfile);
				verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: UpdateInterval Value: \\\"4\\\"." }' ) ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 GET "UpdateInterval": 4, too low', function(done) {
				verifyConfig = JSON.parse(expectTestCRLProfile);
				FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return status 200 POST "UpdateInterval": 43201, too high', function(done) {
				var payload = '{"CRLProfile":{"TestCRLProf":{"UpdateInterval": 43201 }}}';
				verifyConfig = JSON.parse(expectTestCRLProfile);
				verifyMessage = JSON.parse( '{ "status":400, "Code":"CWLNA0112", "Message":"The property value is not valid: Property: UpdateInterval Value: \\\"43201\\\"." }' );
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 GET "UpdateInterval": 43201, too high', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return status 200 POST "UpdateInterval": true, not boolean', function(done) {
				var payload = '{"CRLProfile":{"TestCRLProf":{"UpdateInterval": true }}}';
				verifyConfig = JSON.parse(expectTestCRLProfile);
				verifyMessage={ "status":400, "Code":"CWLNA0127", "Message":"The property type is not valid. Object: CRLProfile Name: TestCRLProf Property: UpdateInterval Type: JSON_TRUE" } ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 GET "UpdateInterval": true, not boolean', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return status 200 POST "UpdateInterval": "43200", not string', function(done) {
				var payload = '{"CRLProfile":{"TestCRLProf":{"UpdateInterval": "43200"}}}';
				verifyConfig = JSON.parse(expectTestCRLProfile);
				verifyMessage={ "status":400, "Code":"CWLNA0127", "Message":"The property type is not valid. Object: CRLProfile Name: TestCRLProf Property: UpdateInterval Type: JSON_STRING" } ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 GET "UpdateInterval": "43200", not string', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+'CRLProfile/TestCRLProf', FVT.getSuccessCallback, verifyConfig, done);
			});

		});
		

    });
		
    //  ====================   Cleanup Test Cases for CRL Profile ====================  //
    describe('Clear CRL Profile from Security Profile, Clear other profiles, delete CRLProfile', function() {
    	
    	var objSecName = 'CRLSecProf';
    	var objCRLName = 'TestCRLProf';
    	var objRootName = 'TrustedRootCert.pem';
    	var objIntName = 'TrustedIntermediateCert.pem';
    	var objCertName = 'CRLCertProf';

		it('should return status 200 on post Security Profile: "'+objSecName+'"', function(done) {
            var payload = '{"SecurityProfile":{"'+objSecName+'":{"CRLProfile": ""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 on get SecurityProfile: "'+objSecName+'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'SecurityProfile/'+ objSecName, FVT.getSuccessCallback, verifyConfig, done);
        });
        
		it('should return status 200 on post Endpoint: "DemoEndpoint"', function(done) {
            var payload = '{"Endpoint":{"DemoEndpoint":{"Enabled": false, "SecurityProfile": ""}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
		
		it('should return status 200 on get Endpoint: "DemoEndpoint"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'Endpoint/DemoEndpoint', FVT.getSuccessCallback, verifyConfig, done);
        });
		
        it('should return status 200 on delete Trusted Root Certificate', function(done) {
            verifyConfig = JSON.parse( '{"TrustedCertificate":[{"SecurityProfileName":"'+objSecName+'","TrustedCertificate":"'+objRootName+'"}]}' );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain + 'TrustedCertificate/' + objSecName + '/' + objRootName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        
        it('should return status 404 on get after delete, Trusted Root Certificate gone', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TrustedCertificate Name: '+ objSecName + '/'+ objRootName + '","TrustedCertificate":{"'+ objRootName +'":null}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain + 'TrustedCertificate/' + objSecName + '/' + objRootName, FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 on delete Trusted Intermediate Certificate', function(done) {
            verifyConfig = JSON.parse( '{"TrustedCertificate":[{"SecurityProfileName":"'+objSecName+'","TrustedCertificate":"'+objIntName+'"}]}' );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain + 'TrustedCertificate/' + objSecName + '/' + objIntName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        
        it('should return status 404 on get after delete, Trusted Intermediate Certificate gone', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TrustedCertificate Name: '+ objSecName + '/'+ objIntName + '","TrustedCertificate":{"'+ objIntName +'":null}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain + 'TrustedCertificate/' + objSecName + '/' + objIntName, FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 on delete Security Profile: "'+ objSecName +'"', function(done) {
            verifyConfig = JSON.parse( '{"SecurityProfile":{"'+ objSecName +'":null}}' );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'SecurityProfile/'+objSecName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });

        it('should return status 404 on get after delete, "'+ objSecName +'" gone', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: '+ objSecName +'","SecurityProfile":{"'+ objSecName +'":null}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'SecurityProfile/'+objSecName, FVT.verify404NotFound, verifyConfig, done );
        });

        it('should return status 200 on delete CertificateProfile: "'+ objCertName +'"', function(done) {
            verifyConfig = JSON.parse( '{"CertificateProfile":{"'+ objCertName +'":null}}' );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'CertificateProfile/'+objCertName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });

        it('should return status 404 on get after delete, "'+ objCertName +'" gone', function(done) {
           verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: '+ objCertName +'","CertificateProfile":{"'+ objCertName +'":null}}' );
           FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/'+objCertName, FVT.verify404NotFound, verifyConfig, done );
        });
        
        it('should return status 200 on delete CRLProfile: "'+ objCRLName +'"', function(done) {
            verifyConfig = JSON.parse( '{"CRLProfile":{"TestCRLProf":null}}' );
            verifyMessage = {"Status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain + 'CRLProfile/' + objCRLName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });

        it('should return status 404 on get after delete, "'+ objCRLName +'" gone', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CRLProfile Name: '+ objCRLName +'","CRLProfile":{"'+ objCRLName +'":null}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain + 'CRLProfile/' + objCRLName, FVT.verify404NotFound, verifyConfig, done);
        });

    });
});