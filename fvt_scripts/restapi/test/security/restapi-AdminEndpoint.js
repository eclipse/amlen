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

var portMin = 2 ;  // Port:1 causes browser problems. defect 116513
var portChangeTimeout = 500;

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
            verifyConfig = JSON.parse( '{"status":404,"Code\":\"CWLNA0136\",\"Message\":\"The item or object cannot be found. Type: ConfigurationPolicy Name: '+ policyName +'","ConfigurationPolicy":{"'+ policyName +'":{}}}' );
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

         it('should return status 200 when POST "Port":'+ portMin +' (MIN)', function(done) {
		    FVT.lastPost['stats'][ "PortFrom" ] = 9089;
			FVT.lastPost['stats'][ "PortTo" ] = portMin;
            var payload = '{"AdminEndpoint":{"Port":'+ portMin +'}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST "Port":'+ portMin +' (MIN)', function(done) {
			FVT.sleep( portChangeTimeout );
            FVT.makeGetRequestWithURL( 'http://'+ FVT.msgServer +':'+ portMin +'', FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
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
			FVT.sleep( portChangeTimeout );
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
			FVT.sleep( portChangeTimeout );
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
			FVT.sleep( portChangeTimeout );
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
//  			FVT.sleep( portChangeTimeout );
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
//        });


    });

    describe('ConfigurationPolicies:', function() {
            
            it('should return status 200 on GET "ConfigPolicy for user000"', function(done) {
                verifyConfig = JSON.parse( '{"ConfigurationPolicy":{"CfgPol4user000'+ max256PolicyName +'":{"UserID":"user000","ActionList":"Configure,View,Monitor,Manage"}}}' );
                FVT.makeGetRequest( FVT.uriConfigDomain+"ConfigurationPolicy/CfgPol4user000"+max256PolicyName, FVT.getSuccessCallback, verifyConfig, done);
            });
            
            it('should return status 404 on GET "ConfigPolicy for user'+maxConfigPolicies+'"', function(done) {
//                verifyConfig = JSON.parse( '{"status":404,"ConfigurationPolicy":{"CfgPol4user'+maxConfigPolicies+max256PolicyName +'":{"UserID":"user100","ActionList":"Configure,View,Monitor,Manage"}}}' );
                verifyConfig = JSON.parse( '{"status":404,"Code\":\"CWLNA0136\",\"Message\":\"The item or object cannot be found. Type: ConfigurationPolicy Name: CfgPol4user'+maxConfigPolicies+max256PolicyName +'","ConfigurationPolicy":{"CfgPol4user'+maxConfigPolicies+max256PolicyName +'":{}}}' );
                FVT.makeGetRequest( FVT.uriConfigDomain+'ConfigurationPolicy/CfgPol4user'+maxConfigPolicies+max256PolicyName, FVT.verify404NotFound, verifyConfig, done);            
            });
// 96141 108271
        
        it('should return status 200 on POST  change "ConfigurationPolicy":100 Policies', function(done) {
            var payload = '{"AdminEndpoint":{"ConfigurationPolicies":"'+ policyList +'"}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "100 Config Policies"', function(done) {
        	var auth = 'user000:'
            FVT.makeGetRequestWithAuth( null, FVT.uriConfigDomain+uriObject, auth, FVT.getSuccessCallback, verifyConfig, done);        
        });
		
        it('should return status 400 on POST "101 Policies"', function(done) {
            var payload = '{"AdminEndpoint":{"ConfigurationPolicies":"'+ policyList +',AdminDefaultConfigPolicy"}}';
            var auth = 'user000:'
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0373","Message":"The number of items exceeded the allowed limit. Item: ConfigurationPolicies Count: 101 Limit: 100"} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0374","Message":"The number of items exceeded the allowed limit. Item: ConfigurationPolicies Count: 101 Limit: 100"} ;
            verifyMessage = {"status":400,"Code":"CWLNA0374","Message":"The number of items exceeded the limit. Item: ConfigurationPolicies Count: 101 Limit: 100"} ;
            //FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
            FVT.makePostRequestWithAuthVerify( null, FVT.uriConfigDomain, auth, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET still has just "100 policies"', function(done) {
        	var auth = 'user000:'
            verifyConfig= JSON.parse( '{"AdminEndpoint":{"ConfigurationPolicies":"'+ policyList +'"}}' );
            //FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done); 
            FVT.makeGetRequestWithAuth( null, FVT.uriConfigDomain+uriObject, auth, FVT.getSuccessCallback, verifyConfig, done)
        });
        
// Config persists Check
        it('should return status 200 POST restart Server', function(done) {
            var payload = '{"Service":"Server"}';
            var auth = 'user000:'
            var verifyPayload = JSON.parse( payload ) ;
            FVT.makePostRequestWithAuth( null, FVT.uriServiceDomain+'restart', auth, payload, FVT.postSuccessCallback, verifyPayload, done);
        });
        
        it('should return status 200 on GET "Status" after restart with payload', function(done) {
    		this.timeout( FVT.REBOOT + 5000 ) ; 
    		FVT.sleep( FVT.REBOOT ) ;
			var verifyStatus = JSON.parse( FVT.expectDefaultStatus ) ;
			var auth = 'user000:'
//            FVT.verifyServerRestart( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
            FVT.makeGetRequestWithAuth( null, FVT.uriServiceDomain+'status', auth, FVT.getSuccessCallback, verifyStatus, done)
        });
        
        it('should return status 200 on get, "100 Config Policies" persists', function(done) {
            verifyConfig = JSON.parse( '{"AdminEndpoint":{"ConfigurationPolicies":"'+ policyList +'"}}' ) ;
            var auth = 'user000:'
            FVT.makeGetRequestWithAuth( null, FVT.uriConfigDomain+uriObject, auth, FVT.getSuccessCallback, verifyConfig, done)
        });

     
        it ('should return 200 to set back to default config policy', function(done) {
            var payload = '{"AdminEndpoint":{"ConfigurationPolicies":"AdminDefaultConfigPolicy"}}';
            var auth = 'user000:'
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithAuth( null, FVT.uriConfigDomain, auth, payload, FVT.postSuccessCallback, verifyConfig, done);
            //FVT.makePostRequest( FVT.uriConfigDomain, auth, payload, FVT.postSuccessCallback, verifyConfig, done);

        });

        /* This test not needed due to configuration policy changes taking immediate effect */
        //it('should return status 200 POST restart Server to take effect of default config policy', function(done) {
          //  var payload = '{"Service":"Server"}';
          //  var verifyPayload = JSON.parse( payload ) ;
          //  var auth = 'user000:'
		//	verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
        //    FVT.makePostRequestWithAuth( null, FVT.uriServiceDomain+'restart', auth, payload, FVT.postSuccessCallback, verifyPayload, done);
        //});
        
        it('should return status 200 on GET "Status" after restart with payload', function(done) {
    		//this.timeout( FVT.REBOOT + 5000 ) ; 
    		//FVT.sleep( FVT.REBOOT ) ;
			var verifyStatus = JSON.parse( FVT.expectDefaultStatus ) ;
//            FVT.verifyServerRestart( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
            FVT.makeGetRequest( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
        });
		
 // end of  96141          
    });

    describe('Interfaces:', function() {

        it('should return status 200,now 400 on POST "Interface":"localhost"', function(done) {
            var payload = '{"AdminEndpoint":{"Interface":"localhost"}}';
            verifyConfig= JSON.parse( '{"AdminEndpoint":{"ConfigurationPolicies":"AdminDefaultConfigPolicy"}}' );
// after defect 96240 fixed: not 200, but 400
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Interface Value: \"localhost\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "Interface":"localhost"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });

        it('should return status 200 on POST "Interface":"'+ FVT.A1_IPv4_INTERNAL_1 +'"', function(done) {
            var payload = '{"AdminEndpoint":{"Interface":"'+ FVT.A1_IPv4_INTERNAL_1 +'"}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Interface":"'+ FVT.A1_IPv4_INTERNAL_1 +'"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
// Remove IPv6
// 110318 110319
// This sometime (most times) fails saying PORT in Use...
//        it('should return status 200 on POST "Interface":"['+ FVT.msgServer_IPv6 +']"', function(done) {
//            var payload = '{"AdminEndpoint":{"Interface":"['+ FVT.msgServer_IPv6 +']"}}';
//            verifyConfig = JSON.parse(payload);
//            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
//        });
// Really this GET should be withURL to IPv6 address if really expected the POST to be successful
//        it('should return status 200 on GET "Interface":"['+ FVT.msgServer_IPv6 +']"', function(done) {
//            FVT.makeGetRequestWithURL( 'http://'+ FVT.msgServer_IPv6 +':9089', FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
//        });
// Change Port when Change to IPv6 --  Asssuming the POST will still fail above
//         it('should return status 200 when POST "Port":9088,"Interface":"['+ FVT.msgServer_IPv6 +']', function(done) {
//			FVT.lastPost['stats'][ "PortTo" ] = 9088;
//            var payload = '{"AdminEndpoint":{"Port":9088,"Interface":"['+ FVT.msgServer_IPv6 +']"}}';
//            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
//			FVT.makePostRequestWithURL( 'http://'+ FVT.msgServer +':' + FVT.lastPost['stats'][ "PortFrom" ] , FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
//        });
//        it('should return status 200 on get after POST "Port":9088,"Interface":"['+ FVT.msgServer_IPv6 +']', function(done) {
//            FVT.makeGetRequestWithURL( 'http://['+ FVT.msgServer_IPv6 +']:9088', FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
//        });
//
//         it('should return status 200 when POST "Port":9089,"Interface":"['+ FVT.msgServer +']"', function(done) {
//			FVT.lastPost['stats'][ "PortTo" ] = 9089;
//            var payload = '{"AdminEndpoint":{"Port":9089,"Interface":"'+ FVT.msgServer +'"}}';
//            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
//			FVT.makePostRequestWithURL( 'http://['+ FVT.msgServer_IPv6 +']:' + FVT.lastPost['stats'][ "PortFrom" ] , FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
//        });
        it('should return status 200 on GET after POST "Port":9089,"Interface":"'+ FVT.msgServer +'"', function(done) {
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
//*  TEMP REMOVE for 114116 resolution	
         it('should return status 200 when POST "SecurityProfile":"PSWDSecProf" now PSWD are needed after this', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"PSWDSecProf" }}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST Only "SecurityProfile":"PSWDSecProf" and ConfigPolicies exist', function(done) {
            FVT.makeGetRequestWithURL( 'http://'+ adminUser +':'+ adminPswd +'@'+ FVT.IMA_AdminEndpoint , FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });		
//    TEMP REMOVE for 114116 resolution */
		it('should return status 200 when POST change "SecurityProfile":"TestSecProf" and ConfigPolicies exist', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"TestSecProf" }}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequestWithURL( 'http://'+ adminUser +':'+ adminPswd +'@'+ FVT.IMA_AdminEndpoint , FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST Only "SecurityProfile":"TestSecProf" and ConfigPolicies exist', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });		
        
// Update: Config Policy is replaced with default config policy if string is empty: 170146
        it('should return status 200 when POST remove "SecurityProfile" and "ConfigurationPolicies"', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"", "ConfigurationPolicies":"" }}';
            verifyConfig = JSON.parse('{"AdminEndpoint":{"SecurityProfile":"", "ConfigurationPolicies":"AdminDefaultConfigPolicy" }}');
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST remove "SecurityProfile" and "ConfigurationPolicies"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });		


        it('should return status 200 when POST "SecurityProfile":null and ConfigPolicies are removed', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":null, "ConfigurationPolicies":"" }}';
            verifyConfig = {"AdminEndpoint":{"SecurityProfile":"AdminDefaultSecProfile", "ConfigurationPolicies":"AdminDefaultConfigPolicy" }};
//            verifyMessage = {"status":400,"Code": "CWLNA0373","Message": "If you specify SecurityProfile, you must also specify ConfigurationPolicies. If you specify ConfigurationPolicies, you must also specify SecurityProfile."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST "SecurityProfile":null and ConfigPolicies are removed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });			

         it('should return status 200 when POST "ConfigPolicies":null and SecurityProfile is removed', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"", "ConfigurationPolicies":null }}';
            verifyConfig = {"AdminEndpoint":{"SecurityProfile":"", "ConfigurationPolicies":"AdminDefaultConfigPolicy" }};
//            verifyMessage = {"status":400,"Code": "CWLNA0373","Message": "If you specify ConfigurationPolicies, you must also specify SecurityProfile. If you specify SecurityProfile, you must also specify ConfigurationPolicies."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST "ConfigPolicies":null and SecurityProfile is removed', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });			

        it('should return status 200 when POST Only "ConfigurationPolicies" and NO "SecurityProfile" exists', function(done) {
            var payload = '{"AdminEndpoint":{"ConfigurationPolicies":null}}';
            verifyConfig = {"AdminEndpoint":{"SecurityProfile":"", "ConfigurationPolicies":"AdminDefaultConfigPolicy" }};
//            verifyMessage = {"status":400,"Code": "CWLNA0373","Message": "If you specify ConfigurationPolicies, you must also specify SecurityProfile. If you specify SecurityProfile, you must also specify ConfigurationPolicies."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 when POST restore default "SecurityProfile" and "ConfigurationPolicies"', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":null, "ConfigurationPolicies":null }}';
            verifyConfig = JSON.parse( expectDefault );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST restore default "SecurityProfile" and "ConfigurationPolicies"', function(done) {
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
//             verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: \"AdminEndpoint\":null is not valid."};
             verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: \"AdminEndpoint\":null is not valid."};
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
//            verifyMessage = {"status":400,"Code":"CWLNA0118","Message":"The properties are not valid."};
            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: \"AdminEndpoint\":\"\" is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get after POST "AdminEndpoint":null', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// 103173 116642
        it('should return status 403 on DELETE "AdminEndpoint"', function(done) {
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: Name." };
//            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: Name Value: null." };
            verifyMessage = {"status":403,"Code":"CWLNA0372","Message":"Delete is not allowed for AdminEndpoint object." };
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after DELETE, "AdminEndpoint".', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 403 on DELETE  any "AdminEndpoint"', function(done) {
            verifyConfig = JSON.parse( expectDefault );
// wtf            verifyMessage = {"status":400,"Code":"CWLNA0371","Message":"Delete is not allowed for * object."};
            verifyMessage = {"status":403,"Code":"CWLNA0372","Message":"Delete is not allowed for AdminEndpoint object."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'*', FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after DELETE, "AdminEndpoint/*".', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
        
        it('should return status 403 on DELETE THE "AdminEndpoint"', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            verifyMessage = {"status":403,"Code":"CWLNA0372","Message":"Delete is not allowed for AdminEndpoint object."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+"AdminEndpoint", FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after DELETE THE "AdminEndpoint".', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
               
    });

//  ====================  Error test cases  ====================  //
    describe('Error - General:', function() {

         it('should return status 400 when POST "EnableAbout":false', function(done) {
            var payload = '{"AdminEndpoint":{"EnableAbout":false}}';
            verifyConfig = JSON.parse( expectDefault ) ;
			verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: AdminEndpoint Name: AdminEndpoint Property: EnableAbout"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "EnableAbout":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
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
    });
    

    describe('Error - Description:', function() {
//133396 
    	it('should return status 400 when POST "Description":TooLONG', function(done) {
            var payload = '{"AdminEndpoint":{"Description":"'+ FVT.long1025String +'"}}';
            verifyConfig = JSON.parse( expectDefault );
//			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: AdminEndpoint Property: Description Value: '+ FVT.long1025String +'." }' );
//			verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA8675","Message":"Updates to the Description configuration item are not allowed." }' );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA6208","Message":"Updates to the Description configuration item are not allowed." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "Description":TooLONG', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });

    });
    

    describe('Error - SecurityProfile:', function() {

    	it('should return status 400 when POST "SecurityProfile":TooLONG', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"'+ FVT.long33Name +'"}}';
            verifyConfig = JSON.parse( expectDefault );
			verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: AdminEndpoint Property: SecurityProfile Value: '+ FVT.long33Name +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST "SecurityProfile":TooLONG', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });

    });

    describe('Error - Dependency:', function() {
// 98755
        it('should return status 404 when POST with missing ConfigPolicy', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"TestSecProf","ConfigurationPolicies": "AdminDefaultConfigPolicy,TestConfigPolicy,MissingConfigPolicy"}}';
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":404,"Code": "CWLNA0136","Message": "The item or object is not found. Type: ConfigurationPolicy Name: MissingConfigPolicy"};
            verifyMessage = {"status":404,"Code": "CWLNA0136","Message": "The item or object cannot be found. Type: ConfigurationPolicy Name: MissingConfigPolicy"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST with missing ConfigurationPolicy', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
        });
// 98755
        it('should return status 404 when POST with missing SecProfile', function(done) {
            var payload = '{"AdminEndpoint":{"SecurityProfile":"MissingSecProf","ConfigurationPolicies": "AdminDefaultConfigPolicy,TestConfigPolicy"}}';
            verifyConfig = JSON.parse( expectDefault );
//            verifyMessage = {"status":404,"Code": "CWLNA0136","Message": "The item or object is not found. Type: SecurityProfile Name: MissingSecProf"};
            verifyMessage = {"status":404,"Code": "CWLNA0136","Message": "The item or object cannot be found. Type: SecurityProfile Name: MissingSecProf"};
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
		    var verifyConfigDepend = { "SecurityProfile": {"TestSecProf": { "CertificateProfile": "TestCertProf", "UsePasswordAuthentication": false, "TLSEnabled": false, "LTPAProfile": "", "OAuthProfile": "", "MinimumProtocolMethod": "TLSv1.2", "UseClientCertificate": false, "Ciphers": "Fast", "UseClientCipher": false}} } ;
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
//            verifyConfig = JSON.parse( '{"status":404,"SecurityProfile":{"TestSecProf":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: TestSecProf","SecurityProfile":{"TestSecProf":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'SecurityProfile/TestSecProf', FVT.verify404NotFound, verifyConfig, done);
        });
        
        it('should return status 200 on DELETE "Security Profile for PSWDSecProf"', function(done) {
            verifyConfig = JSON.parse( '{"SecurityProfile":{"PSWDSecProf":{}}}' );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+"SecurityProfile/PSWDSecProf", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "SecurityProfile for PSWDSecProf"', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"SecurityProfile":{"PSWDSecProf":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: PSWDSecProf","SecurityProfile":{"PSWDSecProf":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'SecurityProfile/PSWDSecProf', FVT.verify404NotFound, verifyConfig, done);
        });


        it('should return status 200 on DELETE "CertificateProfile":"TestCertProf"', function(done) {
            verifyConfig = JSON.parse( '{"CertificateProfile":{"TestCertProf":{}}}' );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+"CertificateProfile/TestCertProf", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "CertificateProfile":"TestCertProf"', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"CertificateProfile":{"TestCertProf":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: TestCertProf","CertificateProfile":{"TestCertProf":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/TestCertProf', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 on DELETE "CertificateProfile":"NewTestCertProf"', function(done) {
            verifyConfig = JSON.parse( '{"CertificateProfile":{"NewTestCertProf":{}}}' );
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+"CertificateProfile/NewTestCertProf", FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET "CertificateProfile":"NewTestCertProf"', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"CertificateProfile":{"NewTestCertProf":{}}}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: NewTestCertProf","CertificateProfile":{"NewTestCertProf":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/NewTestCertProf', FVT.verify404NotFound, verifyConfig, done);
        });
        
    });

});
