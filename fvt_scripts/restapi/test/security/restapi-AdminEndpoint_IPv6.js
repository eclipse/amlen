// Copyright (c) 2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
var assert = require('assert')
var request = require('supertest')
var should = require('should')
var fs = require('fs')
var mqtt = require('mqtt')

var FVT = require('../restapi-CommonFunctions.js')

var verifyConfig = {} ;
var verifyMessage = {};
var uriObject = 'AdminEndpoint/' ;
//var expectDefault = '{"AdminEndpoint":{"ConfigurationPolicies": "AdminDefaultConfigPolicy","Interface": "All","Port":"9089","SecurityProfile": "AdminDefaultSecProfile"},"Version": "'+ FVT.version +'"}' ;
var expectDefault = '{"AdminEndpoint":{"ConfigurationPolicies": "AdminDefaultConfigPolicy","Interface": "All","Port":9089,"SecurityProfile": "AdminDefaultSecProfile"},"Version": "'+ FVT.version +'"}' ;

var policyList = "";
var max256PolicyName = '.11112222222222333333';
//var max256PolicyName = '.1111222222222233333333334444444444555555555566666666667777777777888888888899999999990000000000111111111122222222223333333333444444444455555555556666666666777777777788888888889999999999000000000011111111112222222222333333333344444444445123456';
var maxConfigPolicies = 100;

var url = 'http://' + process.env.IMA_AdminEndpoint;
FVT.trace( 0,   ' === IMA_AdminEndpoint: '+ url );

var index = url.lastIndexOf(':');
var msgServer = url.substring(7, index);
FVT.trace( 0,  'MQTT_Server IP is :  '+ msgServer );

var adminUser = FVT.A1_REST_USER;
var adminPswd = FVT.A1_REST_PW;
// --------------------------------------------------------
//  makeConfigurationPolicy( policyName, userName )
// --------------------------------------------------------
function makeConfigurationPolicy( policyName, userName ){
    describe( 'Make ConfigurationPolicy for: '+userName, function () {
        
        it('should return status 200 on POST "ConfigPolicy for '+ userName +'"', function(done) {
console.log( "user:" + userName );
            var payload = '{"ConfigurationPolicy":{"'+ policyName +'":{"UserID":"'+ userName +'","ActionList":"Configure,View,Monitor,Manage"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on GET "ConfigPolicy for '+ userName +'"', function(done) {
//            verifyConfig = JSON.parse( '{"status":200,"ConfigurationPolicy":{"'+ policyName +'":{}}}' ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"ConfigurationPolicy/"+policyName, FVT.getSuccessCallback, verifyConfig, done);        });
        
    });
};    

// --------------------------------------------------------
//  deleteConfigurationPolicy( policyName, userName )
// --------------------------------------------------------
function deleteConfigurationPolicy( policyName, userName ){
    describe( 'Delete ConfigurationPolicy for: '+userName, function () {
        
        it('should return status 200 on DELETE "ConfigPolicy for '+ userName +'"', function(done) {
console.log( "user:" + userName );
            verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"'+ policyName +'":{}}}' );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+"ConfigurationPolicy/"+policyName, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
            
        it('should return status 404 on GET "ConfigPolicy for '+ userName +'"', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"ConfigurationPolicy":{"'+ policyName +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'ConfigurationPolicy/'+policyName, FVT.verify404NotFound, verifyConfig, done);
        });
            
    });
};    


//  ====================  Test Cases Start Here  ====================  //

describe('AdminEndpoint:', function() {
this.timeout( FVT.defaultTimeout );

    //  ====================   Get test cases  ====================  //
    describe('Get - verify in Pristine State:', function() {
        it('should return status 200 on get, DEFAULT of "AdminEndpoint"', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
    
    });

    //  ====================   Create - Add - Update test cases  ====================  //
    describe('Prereq Keys & Certs:', function() {

        it('should return status 200 on PUT "AdminEndpointCert.pem"', function(done) {
            sourceFile = 'AdminDefaultCert.pem';
            targetFile = 'AdminEndpointCert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "AdminEndpointKey.pem"', function(done) {
            sourceFile = 'AdminDefaultKey.pem';
            targetFile = 'AdminEndpointKey.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "AdminEndpointNewCert.pem"', function(done) {
            sourceFile = 'AdminDefaultCert.pem';
            targetFile = 'AdminEndpointNewCert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "AdminEndpointNewKey.pem"', function(done) {
            sourceFile = 'AdminDefaultKey.pem';
            targetFile = 'AdminEndpointNewKey.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

    });

    describe('Prereq Profiles and Policies:', function() {
// 98608    
        it('should return status 200 on post "TestCertProf"', function(done) {
            var payload = '{"CertificateProfile":{"TestCertProf":{"Certificate": "AdminEndpointCert.pem","Key": "AdminEndpointKey.pem","KeyFilePassword":""}}}';
            verifyConfig = {"CertificateProfile":{"TestCertProf":{"Certificate": "AdminEndpointCert.pem","Key": "AdminEndpointKey.pem"}}} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestCertProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/TestCertProf', FVT.getSuccessCallback, verifyConfig, done);
        });
    
        it('should return status 200 on post "NewTestCertProf"', function(done) {
            var payload = '{"CertificateProfile":{"NewTestCertProf":{"Certificate": "AdminEndpointNewCert.pem","Key": "AdminEndpointNewKey.pem","KeyFilePassword":""}}}';
            verifyConfig = {"CertificateProfile":{"NewTestCertProf":{"Certificate": "AdminEndpointNewCert.pem","Key": "AdminEndpointNewKey.pem"}}} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "NewTestCertProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/NewTestCertProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "TestSecProf"', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"CertificateProfile": "TestCertProf","UsePasswordAuthentication":false, "TLSEnabled":false }}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "TestSecProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+"SecurityProfile/", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "PSWDSecProf"', function(done) {
            var payload = '{"SecurityProfile":{"PSWDSecProf":{"CertificateProfile": "NewTestCertProf","UsePasswordAuthentication":true, "TLSEnabled":false }}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "PSWDSecProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+"SecurityProfile/", FVT.getSuccessCallback, verifyConfig, done);
        });

        for( i = 0 ; i < maxConfigPolicies ; i++ ) {
            var paddedUserName = 'user'+('000' + i).slice(-3);
            paddedPolicyName = 'CfgPol4' + paddedUserName + max256PolicyName ;
            if ( i == 0 ) {
                policyList = paddedPolicyName ;        
            } else {
                policyList = policyList+","+paddedPolicyName ;
            }
            makeConfigurationPolicy( paddedPolicyName, paddedUserName );
        }
        makeConfigurationPolicy( "TestConfigPolicy", "JoeTester" );

        
    });

    describe('NOOP Update:', function() {
//NOOP - should get a new message 93850
        it('should return status 200 on POST "AdminEndpoint" with no change body', function(done) {
            var payload = '{"AdminEndpoint":{}}';
            verifyConfig = JSON.parse( expectDefault );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "AdminEndpoint"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    describe('Port:', function() {

         it('should return status 200 when POST "Port":1 (MIN)', function(done) {
		    FVT.lastPost['stats'][ "PortFrom" ] = 9089;
			FVT.lastPost['stats'][ "PortTo" ] = 1;
            var payload = '{"AdminEndpoint":{"Port":1}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST "Port":1 (MIN)', function(done) {
            FVT.makeGetRequestWithURL( 'http://'+ FVT.msgServer +':1', FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

         it('should return status 200 when POST "Port":65535 (MAX)', function(done) {
			FVT.lastPost['stats'][ "PortTo" ] = 65535;
            var payload = '{"AdminEndpoint":{"Port":65535}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
//			FVT.makePostRequestWithURL( 'http://'+ FVT.msgServer +':1', FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
			FVT.makePostRequestWithURL( 'http://'+ FVT.msgServer +':' + FVT.lastPost['stats'][ "PortFrom" ] , FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST Port":65535 (MAX)', function(done) {
            FVT.makeGetRequestWithURL( 'http://'+ FVT.msgServer +':65535', FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

         it('should return status 200 when POST "Port":9088', function(done) {
			FVT.lastPost['stats'][ "PortTo" ] = 9088;
            var payload = '{"AdminEndpoint":{"Port":9088}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
//            FVT.makePostRequestWithURL( 'http://'+ FVT.msgServer +':65535', FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
			FVT.makePostRequestWithURL( 'http://'+ FVT.msgServer +':' + FVT.lastPost['stats'][ "PortFrom" ] , FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST "Port":9088', function(done) {
            FVT.makeGetRequestWithURL( 'http://'+ FVT.msgServer +':9088', FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 98579 
         it('should return status 200 when POST "Port":null(9089)"', function(done) {
			FVT.lastPost['stats'][ "PortTo" ] = 9089;
            var payload = '{"AdminEndpoint":{"Port":null}}';
            verifyConfig = {"AdminEndpoint":{"Port":9089}};
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
//            FVT.makePostRequestWithURL( 'http://'+ FVT.msgServer +':9088', FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
				FVT.makePostRequestWithURL( 'http://'+ FVT.msgServer +':' + FVT.lastPost['stats'][ "PortFrom" ] , FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after POST "Port":null(9089)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// failsafe when RESET fails... due to 98579 
//         it('should return status 200 when POST "Port":(9089) failsafe"', function(done) {
//            var payload = '{"AdminEndpoint":{"Port":9089}}';
//            verifyConfig = {"AdminEndpoint":{"Port":9089}};
//            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
//            FVT.makePostRequestWithURL( 'http://'+ FVT.msgServer +':9088', FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
//        });
//        it('should return status 200 on GET after POST "Port":(9089) failsafe', function(done) {
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
//        });


    });

    describe('ConfigurationPolicies:', function() {
            
            it('should return status 200 on GET "ConfigPolicy for user000"', function(done) {
                verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"CfgPol4user000'+ max256PolicyName +'":{"UserID":"user000","ActionList":"Configure,View,Monitor,Manage"}}}' );
                FVT.makeGetRequest( FVT.uriConfigDomain+"ConfigurationPolicy/CfgPol4user000"+max256PolicyName, FVT.getSuccessCallback, verifyConfig, done);
            });
            
            it('should return status 404 on GET "ConfigPolicy for user'+maxConfigPolicies+'"', function(done) {
                verifyConfig = JSON.parse( '{"status":404,"ConfigurationPolicy":{"CfgPol4user'+maxConfigPolicies+max256PolicyName +'":{"UserID":"user100","ActionList":"Configure,View,Monitor,Manage"}}}' );
                FVT.makeGetRequest( FVT.uriConfigDomain+'ConfigurationPolicy/CfgPol4user'+maxConfigPolicies+max256PolicyName, FVT.verify404NotFound, verifyConfig, done);            
            });
// 96141 108271
        
        it('should return status 200 on POST  change "ConfigurationPolicy":100 Policies', function(done) {
            var payload = '{"AdminEndpoint":{"ConfigurationPolicies":"'+ policyList +'"}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "100 Config Policies"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
     
        it('should return status 400 on POST "101 Policies"', function(done) {
            var payload = '{"AdminEndpoint":{"ConfigurationPolicies":"'+ policyList +',AdminDefaultConfigPolicy"}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0373","Message":"The number of items exceeded the allowed limit. Item: ConfigurationPolicies Count: 101 Limit: 100"} ;
            verifyMessage = {"status":400,"Code":"CWLNA0374","Message":"The number of items exceeded the allowed limit. Item: ConfigurationPolicies Count: 101 Limit: 100"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET still has just "100 policies"', function(done) {
            verifyConfig= JSON.parse( '{"AdminEndpoint":{"ConfigurationPolicies":"'+ policyList +'"}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });

		
 // end of  96141          
    });

    describe('Interfaces:', function() {

        it('should return status 200,now 400 on POST "Interface":"localhost"', function(done) {
            var payload = '{"AdminEndpoint":{"Interface":"localhost"}}';
            verifyConfig= JSON.parse( '{"AdminEndpoint":{"ConfigurationPolicies":"'+ policyList +'"}}' );
// after defect 96240 fixed: not 200, but 400
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Interface Value: \"localhost\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "Interface":"localhost"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });

        it('should return status 200 on POST "Interface":"'+ msgServer +'"', function(done) {
            var payload = '{"AdminEndpoint":{"Interface":"'+ msgServer +'"}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Interface":"'+ msgServer +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
// 110318 110319
// This sometime (most times) fails saying PORT in Use...
        it('should return status 200 on POST "Interface":"['+ FVT.msgServer_IPv6 +']"', function(done) {
            var payload = '{"AdminEndpoint":{"Interface":"['+ FVT.msgServer_IPv6 +']"}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
// Really this GET should be withURL to IPv6 address if really expected the POST to be successful
        it('should return status 200 on GET "Interface":"['+ FVT.msgServer_IPv6 +']"', function(done) {
            FVT.makeGetRequestWithURL( 'http://'+ FVT.msgServer_IPv6 +':9089', FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
// Change Port when Change to IPv6 --  Asssuming the POST will still fail above
         it('should return status 200 when POST "Port":9088,"Interface":"['+ FVT.msgServer_IPv6 +']', function(done) {
			FVT.lastPost['stats'][ "PortTo" ] = 9088;
            var payload = '{"AdminEndpoint":{"Port":9088,"Interface":"['+ FVT.msgServer_IPv6 +']"}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
			FVT.makePostRequestWithURL( 'http://'+ FVT.msgServer +':' + FVT.lastPost['stats'][ "PortFrom" ] , FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST "Port":9088,"Interface":"['+ FVT.msgServer_IPv6 +']', function(done) {
            FVT.makeGetRequestWithURL( 'http://['+ FVT.msgServer_IPv6 +']:9088', FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

         it('should return status 200 when POST "Port":9089,"Interface":"['+ FVT.msgServer +']"', function(done) {
			FVT.lastPost['stats'][ "PortTo" ] = 9089;
            var payload = '{"AdminEndpoint":{"Port":9089,"Interface":"'+ FVT.msgServer +'"}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
			FVT.makePostRequestWithURL( 'http://['+ FVT.msgServer_IPv6 +']:' + FVT.lastPost['stats'][ "PortFrom" ] , FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after POST "Port":9088,"Interface":"'+ FVT.msgServer +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

// End 110319/101980		
        it('should return status 200 on POST "Interface":"All"', function(done) {
            var payload = '{"AdminEndpoint":{"Interface":"All"}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Interface":"All"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });

    });

    describe('SecurityProfile:', function() {
	
         it('should return status 200 when POST "SecurityProfile":"PSWDSecProf" now PSWD are needed after this', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"PSWDSecProf" }}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST Only "SecurityProfile":"PSWDSecProf" and ConfigPolicies exist', function(done) {
            FVT.makeGetRequestWithURL( 'http://'+ adminUser +':'+ adminPswd +'@'+ FVT.IMA_AdminEndpoint , FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });		

		it('should return status 200 when POST change "SecurityProfile":"TestSecProf" and ConfigPolicies exist', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"TestSecProf" }}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequestWithURL( 'http://'+ adminUser +':'+ adminPswd +'@'+ FVT.IMA_AdminEndpoint , FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST Only "SecurityProfile":"TestSecProf" and ConfigPolicies exist', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });		
// You can remove all security, who knows why, but you can...	99766 comment 2, 98734, 
         it('should return status 200 when POST remove "SecurityProfile" and "ConfigurationPolicies"', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"", "ConfigurationPolicies":"" }}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST remove "SecurityProfile" and "ConfigurationPolicies"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });		
// 99766, 110323
         it('should return status 400 when POST Only "SecurityProfile":"TestSecProf" and NO ConfigPolicies exist', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"TestSecProf" }}';
            verifyConfig = {"AdminEndpoint":{"SecurityProfile":"", "ConfigurationPolicies":"" }};
            verifyMessage = {"status":400,"Code": "CWLNA0373","Message": "If you specify ConfigurationPolicies, you must also specify SecurityProfile. Or if you specify SecurityProfile, you must also specify ConfigurationPolicies."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get after POST Only "SecurityProfile":"TestSecProf" and ConfigPolicies exist', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
// 110323
         it('should return status 400 when POST "SecurityProfile":null and ConfigPolicies are removed', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":null, "ConfigurationPolicies":"" }}';
            verifyConfig = {"AdminEndpoint":{"SecurityProfile":"", "ConfigurationPolicies":"" }};
            verifyMessage = {"status":400,"Code": "CWLNA0373","Message": "If you specify SecurityProfile, you must also specify ConfigurationPolicies. Or if you specify ConfigurationPolicies, you must also specify SecurityProfile."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get after POST "SecurityProfile":null and ConfigPolicies are removed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });			
// 110323
         it('should return status 400 when POST "ConfigPolicies":null and SecurityProfile is removed', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"", "ConfigurationPolicies":null }}';
            verifyConfig = {"AdminEndpoint":{"SecurityProfile":"", "ConfigurationPolicies":"" }};
            verifyMessage = {"status":400,"Code": "CWLNA0373","Message": "If you specify ConfigurationPolicies, you must also specify SecurityProfile. Or if you specify SecurityProfile, you must also specify ConfigurationPolicies."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get after POST "ConfigPolicies":null and SecurityProfile is removed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });			
//99766 - It is suppose to go both ways, if ONE, then THE OTHER is required
        it('should return status 400 when POST Only "ConfigurationPolicies" and NO "SecurityProfile" exists', function(done) {
            var payload = '{"AdminEndpoint":{"ConfigurationPolicies":null}}';
// round and round...            verifyConfig = JSON.parse( '{"AdminEndpoint":{"ConfigurationPolicies":"DefaultAdminConfigPolicy","SecurityProfile":null}}' );
            verifyConfig = {"AdminEndpoint":{"SecurityProfile":"", "ConfigurationPolicies":"" }};
//            verifyMessage = {"status":400,"Code": "CWLNA0372","Message": "Since configuration item ConfigurationPolicies is specified, configuration item SecurityProfile is also required."};
            verifyMessage = {"status":400,"Code": "CWLNA0373","Message": "If you specify ConfigurationPolicies, you must also specify SecurityProfile. Or if you specify SecurityProfile, you must also specify ConfigurationPolicies."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get after POST Only "SecurityProfile":"TestSecProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
// Reset to defaults after 99766 swirl		
        it('should return status 200 when POST restore default "SecurityProfile" and "ConfigurationPolicies"', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":null, "ConfigurationPolicies":null }}';
            verifyConfig = JSON.parse( expectDefault );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST restore default "SecurityProfile" and "ConfigurationPolicies"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });		
		
         it('should return status 200 when POST both "SecurityProfile":"TestSecProf" and ConfigPolicies' , function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"TestSecProf","ConfigurationPolicies": "AdminDefaultConfigPolicy"}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST "SecurityProfile":"TestSecProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
// 99766
        // it('should return status 400 when POST Only "SecurityProfile":"AdminDefaultSecProfile"', function(done) {
            // var payload = '{"AdminEndpoint":{"SecurityProfile":"AdminDefaultSecProfile"}}';
            // verifyConfig = {"AdminEndpoint": {"Port":9089,"Interface": "All","SecurityProfile": "TestSecProf","ConfigurationPolicies": "AdminDefaultConfigPolicy"}} ;
            // verifyMessage = {"status":400,"Code": "CWLNA0372","Message": "Since configuration item SecurityProfile is specified, configuration item ConfigurationPolicies is also required."};
            // FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        // });
        // it('should return status 200 on GET after POST Only "SecurityProfile":"AdminDefaultSecProfile"', function(done) {
            // FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        // });
// 99766
        it('should return status 200 when POST Only "SecurityProfile":"AdminDefaultSecProfile" and ConfigPolicies exist', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"AdminDefaultSecProfile"}}';
//            verifyConfig = {"AdminEndpoint": {"Port":9089,"Interface": "All","SecurityProfile": "TestSecProf","ConfigurationPolicies": "AdminDefaultConfigPolicy"}} ;
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code": "CWLNA0372","Message": "Since configuration item SecurityProfile is specified, configuration item ConfigurationPolicies is also required."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after POST Only "SecurityProfile":"AdminDefaultSecProfile" and ConfigPolicies exist', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });

        it('should return status 200 when POST empty strings on both SecurityProfile and ConfigPolicies (turn off security)', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"","ConfigurationPolicies": ""}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after POST empty strings on both SecurityProfile and ConfigPolicies (turn off security)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST both "SecurityProfile":"AdminDefaultSecProfile" and ConfigPolicies', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"AdminDefaultSecProfile","ConfigurationPolicies": "AdminDefaultConfigPolicy"}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after POST "SecurityProfile":"AdminDefaultSecProfile"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });
    
    //  ====================  Delete test cases  ====================  //
    describe('Delete:', function() {

    // use a POST to UNSET AdminEndpoint, put back to default - shouldn't it work?

        it('should return status 400 when POST "AdminEndpoint":null to delete(can not delete)', function(done) {
            var payload = '{"AdminEndpoint":null}';
            verifyConfig = JSON.parse( expectDefault );
// 93850            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
//             verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: {\"AdminEndpoint\":null} is not valid."};
// back to 137            verifyMessage = {"status":400,"Code":"CWLNA0371","Message":"Delete is not allowed for AdminEndpoint object."};
//             verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: {\"AdminEndpoint\":null} is not valid."};
             verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: \"AdminEndpoint\":null is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get after POST "AdminEndpoint":null', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 99781
        it('should return status 400 when POST "AdminEndpoint":"" (bad form)', function(done) {
            var payload = '{"AdminEndpoint":""}';
            verifyConfig = JSON.parse( expectDefault );
// 93850            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
//            verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: {\"AdminEndpoint\":\"\"} is not valid."};
//            verifyMessage = {"status":400,"Code":"CWLNA0371","Message":"Delete is not allowed for AdminEndpoint object."};
            verifyMessage = {"status":400,"Code":"CWLNA0118","Message":"The properties are not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get after POST "AdminEndpoint":null', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103173
        it('should return status 400 on DELETE "AdminEndpoint"', function(done) {
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: Name." };
//            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: Name Value: null." };
            verifyMessage = {"status":400,"Code":"CWLNA0372","Message":"Delete is not allowed for AdminEndpoint object." };
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after DELETE, "AdminEndpoint".', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 400 on DELETE  any "AdminEndpoint"', function(done) {
            verifyConfig = JSON.parse( expectDefault );
// wtf            verifyMessage = {"status":400,"Code":"CWLNA0371","Message":"Delete is not allowed for * object."};
            verifyMessage = {"status":400,"Code":"CWLNA0372","Message":"Delete is not allowed for * object."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'*', FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after DELETE, "AdminEndpoint/*".', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
        
        it('should return status 400 on DELETE THE "AdminEndpoint"', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            verifyMessage = {"status":400,"Code":"CWLNA0372","Message":"Delete is not allowed for AdminEndpoint object."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+"AdminEndpoint", FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after DELETE THE "AdminEndpoint".', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
// 103173        
        it('should return status 400 on DELETE ALL "AdminEndpoint"', function(done) {
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0371","Message":"Delete is not allowed for AdminEndpoint object."};
// crappy msg, why?			verifyMessage = {"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: Name."};
// back            verifyMessage = {"status":400,"Code":"CWLNA0371","Message":"Delete is not allowed for AdminEndpoint object."};
//            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: Name Value: null."};
            verifyMessage = {"status":400,"Code":"CWLNA0372","Message":"Delete is not allowed for AdminEndpoint object." };
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after DELETE ALL "AdminEndpoint".', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
               
    });

//  ====================  Error test cases  ====================  //
    describe('Error - General:', function() {
// 99781
         it('should return status 400 when POST "Port":""(Invalid)"', function(done) {
            var payload = '{"AdminEndpoint":{"Port":""}}';
            verifyConfig = {"AdminEndpoint":{"Port":9089}};
//            verifyMessage = {"Code": "CWLNA0111","Message": "The property name is not valid: Property:   with payload: {\"AdminEndpoint\":{\"Port\":\"\"}}."};
//            verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property option is invalid. Property: AdminEndpoint Option: Port Value: InvalidType:JSON_STRING."};
			verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: AdminEndpoint Name: AdminEndpoint Property: Port Type: JSON_STRING"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "Port":""(9089)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
//98749 - msg gyration, 100004, 101138  (Like a BATCH partially done?)
        it('should return status 404 when POST both "SecurityProfile" and ConfigPolicies but fat fingered syntax', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"TestSecProf"},"ConfigurationPolicies": "AdminDefaultConfigPolicy"}';
            verifyConfig = {"AdminEndpoint":{"SecurityProfile":"AdminDefaultSecProfile"}} ;
//            verifyMessage = {"status":400,"Code": "CWLNA115","Message": "An argument is not valid: Name: ConfigurationPolicies."};
//            verifyMessage = {"status":400,"Code":"CWLNA0372","Message":"Since configuration item SecurityProfile is specified, configuration item ConfigurationPolicies is also required."};
//            verifyMessage = {"status":404,"Code": "CWLNA0113","Message": "The requested item is not found."};
//              verifyMessage = {"status":400,"Code":"CWLNA0132","Message":"The property value is not valid. Object: AdminEndpoint Name: Port Property: InvalidType: ConfigurationPolicies Value: AdminDefaultConfigPolicy."} ;
//              verifyMessage = {"status":400,"Code":"CWLNA0140","Message":"The object ConfigurationPolicies is not valid."} ;
//              verifyMessage = {"status":404,"Code":"CWLNA0113","Message": "The requested item is not found."};
              verifyMessage = {"status":404,"Code":"CWLNA0111","Message": "The property name is not valid: Property: ConfigurationPolicies."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after fat fingered POST "SecurityProfile":"AdminDefaultSecProfile"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
// added cause part of the command was committed waybackwhen... NOW BATCH processing is implemented and this should be unnecessary
// BACTH does all Singleton's first then there is an order to composite objects based on dependency.
// This is a NOOP with BATCH Implemented, yet should still be successful so left the test in.
        it('should return status 200 when POST to reset "SecurityProfile":"AdminDefaultSecProfile" ', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"AdminDefaultSecProfile"}}';
            verifyConfig = JSON.parse( expectDefault );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after reset POST "SecurityProfile":"AdminDefaultSecProfile"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });

    });
    

    describe('Error - Dependency:', function() {
// 98755
        it('should return status 404 when POST with missing ConfigPolicy', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"TestSecProf","ConfigurationPolicies": "AdminDefaultConfigPolicy,TestConfigPolicy,MissingConfigPolicy"}}';
            verifyConfig = JSON.parse( expectDefault );
            verifyMessage = {"status":404,"Code": "CWLNA0136","Message": "The item or object is not found. Type: ConfigurationPolicy Name: MissingConfigPolicy"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST with missing ConfigurationPolicy', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
// 98755
        it('should return status 404 when POST with missing SecProfile', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"MissingSecProf","ConfigurationPolicies": "AdminDefaultConfigPolicy,TestConfigPolicy"}}';
            verifyConfig = JSON.parse( expectDefault );
            verifyMessage = {"status":404,"Code": "CWLNA0136","Message": "The item or object is not found. Type: SecurityProfile Name: MissingSecProf"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST with missing SecurityProfile', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
// 100006        
        it('should return status 200 when POST both "SecurityProfile" and ConfigPolicies are updated', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"TestSecProf","ConfigurationPolicies": "AdminDefaultConfigPolicy,TestConfigPolicy"}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after POST with two ConfigurationPolicies', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
// 98722
        it('should return status 400 when DELETE an IN USE ConfigPolicy', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"TestSecProf","ConfigurationPolicies": "AdminDefaultConfigPolicy,TestConfigPolicy"}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code": "CWLNA0376","Message": "The item or object is in use. Type: ConfigurationPolicy Name: TestConfigPolicy"};
            verifyMessage = {"status":400,"Code": "CWLNA0376","Message": "The Object: ConfigurationPolicies, Name: TestConfigPolicy is still being used by Object: AdminEndpoint, Name: AdminEndpoint"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'ConfigurationPolicy/TestConfigPolicy', FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET ConfigPolicy after DELETE of In Use ConfigPolicy', function(done) {
		    var verifyConfigDepend = {"ConfigurationPolicy": {"TestConfigPolicy": { "UserID": "JoeTester", "ActionList": "Configure,View,Monitor,Manage", "GroupID": "", "Description": "", "CommonNames": "", "ClientAddress": ""}}}
            FVT.makeGetRequest( FVT.uriConfigDomain+'ConfigurationPolicy/TestConfigPolicy', FVT.getSuccessCallback, verifyConfigDepend, done);        
        });
        it('should return status 200 on GET AdminEndpoint after DELETE of In Use ConfigPolicy', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
// 98728
        it('should return status 400 when DELETE an IN USE SecurityProfile', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"TestSecProf","ConfigurationPolicies": "AdminDefaultConfigPolicy,TestConfigPolicy"}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code": "CWLNA0375","Message": "The Object: SecurityProfile, Name: TestSecProf is still being used by Object: AdminEndpoint, Name: AdminEndpoint"};
            verifyMessage = {"status":400,"Code": "CWLNA0376","Message": "The Object: SecurityProfile, Name: TestSecProf is still being used by Object: AdminEndpoint, Name: AdminEndpoint"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+'SecurityProfile/TestSecProf', FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET SecurityProfile after DELETE of In Use SecurityProfile', function(done) {
		    var verifyConfigDepend = { "SecurityProfile": {"TestSecProf": { "CertificateProfile": "TestCertProf", "UsePasswordAuthentication": false, "TLSEnabled": false, "LTPAProfile": "", "OAuthProfile": "", "MinimumProtocolMethod": "TLSv1", "UseClientCertificate": false, "Ciphers": "Medium", "UseClientCipher": false}} } ;
            FVT.makeGetRequest( FVT.uriConfigDomain+'SecurityProfile/TestSecProf', FVT.getSuccessCallback, verifyConfigDepend, done);        
        });
        it('should return status 200 on GET AdminEndpoint after DELETE of In Use SecurityProfile', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });
    

    describe('Reset to Default:', function() {
// 100006    
        it('should return status 200 when POST to RESET (roundtrip)', function(done) {
            var payload =  expectDefault ;
            verifyConfig = JSON.parse( expectDefault );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after POST to RESET (roundtrip)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
        
    });
    
    describe('Remove Security and Configuration Policies:', function() {
    
        for( i = 0 ; i < maxConfigPolicies ; i++ ) {
            var paddedUserName = 'user'+('000' + i).slice(-3);
            paddedPolicyName = 'CfgPol4' + paddedUserName + max256PolicyName ;
            deleteConfigurationPolicy( paddedPolicyName, paddedUserName );
        }
        deleteConfigurationPolicy( "TestConfigPolicy", "JoeTester" );
        
        it('should return status 200 on DELETE "Security Profile for TestSecProf"', function(done) {
            verifyConfig = JSON.parse( '{"SecurityProfile":{"TestSecProf":{}}}' );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+"SecurityProfile/TestSecProf", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "SecurityProfile for TestSecProf"', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"SecurityProfile":{"TestSecProf":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'SecurityProfile/TestSecProf', FVT.verify404NotFound, verifyConfig, done);
        });
        
        it('should return status 200 on DELETE "Security Profile for PSWDSecProf"', function(done) {
            verifyConfig = JSON.parse( '{"SecurityProfile":{"PSWDSecProf":{}}}' );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+"SecurityProfile/PSWDSecProf", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "SecurityProfile for PSWDSecProf"', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"SecurityProfile":{"PSWDSecProf":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'SecurityProfile/PSWDSecProf', FVT.verify404NotFound, verifyConfig, done);
        });


        it('should return status 200 on DELETE "CertificateProfile":"TestCertProf"', function(done) {
            verifyConfig = JSON.parse( '{"CertificateProfile":{"TestCertProf":{}}}' );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+"CertificateProfile/TestCertProf", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "CertificateProfile":"TestCertProf"', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"CertificateProfile":{"TestCertProf":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/TestCertProf', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 on DELETE "CertificateProfile":"NewTestCertProf"', function(done) {
            verifyConfig = JSON.parse( '{"CertificateProfile":{"NewTestCertProf":{}}}' );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+"CertificateProfile/NewTestCertProf", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "CertificateProfile":"NewTestCertProf"', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"CertificateProfile":{"NewTestCertProf":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/NewTestCertProf', FVT.verify404NotFound, verifyConfig, done);
        });
        
    });

});
