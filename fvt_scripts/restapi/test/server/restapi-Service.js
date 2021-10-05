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

var FVT = require('../restapi-CommonFunctions.js')

var uriObject = 'status/';
// Numeric Server states can be found in server_admin/admin.h   @[ism_ServerState_t;]


var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here
function getDynamicFieldsCallback(err, res, done ){
    FVT.trace( 0, "@getDynamicFieldsCallback RESPONSE:"+ JSON.stringify( res, null, 2 ) );
    jsonResponse = JSON.parse( res.text );
    expectJSON = JSON.parse( FVT.expectDefaultStatus );
    thisName = jsonResponse.Server.Name ;
    thisVersion = jsonResponse.Server.Version ;
    FVT.trace( 1, 'The Server Name: '+ thisName +' and the Version is: '+ thisVersion );
    expectJSON.Server[ "Name" ] = thisName ;
    expectJSON.Server[ "Version" ] = thisVersion ;
    FVT.expectDefaultStatus = JSON.stringify( expectJSON, null, 0 );

//    console.log( "PARSE: req" );
    res.req.method.should.equal('GET');
//    res.req.url.should.equal( 'http://' + FVT.IMA_AdminEndpoint +'ima/v1/service/Status/');

//    console.log( "PARSE: header" );
    res.header.server.should.equal( FVT.responseHeaderProductName );
//    res.header.access-control-allow-origin.should.equal('*');
//    res.header.access-control-allow-credentials.should.equal('true');

//    res.header.connection.should.equal('keep-alive');
    res.header.connection.should.equal('close');

//    res.header.keep-alive.should.equal('timeout=60');
//    res.header.cache-control.should.equal('no-cache');
//    res.header.content-type.should.equal('application/json');
//    res.header.content-length.should.equal('552');

    
    FVT.trace( 0, "NEW FVT.expectDefaultStatus:"+ JSON.stringify( FVT.expectDefaultStatus, null, 2 ) );
    res.statusCode.should.equal(200);
    FVT.getSuccessCallback( err, res, done )
//    done();
}

// Status test cases
describe('Status:', function() {
this.timeout( FVT.defaultTimeout );

    // Status Get test cases
    describe('Pristine Get Status:', function() {
    
        it('should return status 200 on get, DEFAULT of "Status"', function(done) {
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, getDynamicFieldsCallback, verifyConfig, done)
        });
        
    });

    // Status Update test cases
    describe('Not Settable:', function() {
    
        it('should return status 400 on post "Status"', function(done) {
            var payload = '{"Service":"Server","Name":"mochaTestServer"}';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/service/ is not valid."};
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0137","Message":"The REST API call: '+ FVT.uriServiceDomain +' is not valid."}' );
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "Status": is NOT settable:', function(done) {
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // Status Delete test cases
    describe('Delete', function() {
    
        it('should return status 400 when deleting Status', function(done) {
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/service/Status/ is not valid."};
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0137","Message":"The REST API call: '+ FVT.uriServiceDomain+uriObject +' is not valid."}' );
            FVT.makeDeleteRequest( FVT.uriServiceDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after Failed DELETE', function(done) {
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 when trying to Delete All Service, bad form', function(done) {
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/service/ is not valid."};
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0137","Message":"The REST API call: '+ FVT.uriServiceDomain +' is not valid."}' );
            FVT.makeDeleteRequest( FVT.uriServiceDomain, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, still "Default" after failed POST', function(done) {
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on post "Status":null(Post Status Bad)', function(done) {
            var payload = '{"Status":null}';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0137","Message":"The REST API call: /status is not valid."}' );
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'status', payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get after failed Post Status', function(done) {
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });
    
});

// Restart test cases
describe('RESTART:', function() {
this.timeout( FVT.defaultTimeout ) ; 
    // Restart test cases
    describe('CleanStore:', function() {

        it('should return status 200 on POST "restart with payload"', function(done) {
            var payload = '{"Service":"Server"}';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
            verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart with payload', function(done) {
            this.timeout( FVT.REBOOT + 5000 ) ; 
            FVT.sleep( FVT.REBOOT ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST "restart with CleanStore:F"', function(done) {
            var payload = '{"Service":"Server","CleanStore":false}';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
            verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart with CleanStore:F', function(done) {
            this.timeout( FVT.REBOOT + 5000 ) ; 
            FVT.sleep( FVT.REBOOT ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST "restart with CleanStore:T"', function(done) {
            var payload = '{"Service":"Server","CleanStore":true}';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
            verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart with CleanStore:T', function(done) {
            this.timeout( FVT.CLEANSTORE + 5000 );
            FVT.sleep( FVT.CLEANSTORE ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // Maintenance Mode test cases
    describe('MaintenanceMode:', function() {
//        this.timeout(25000);
    
        it('should return status 200 on POST "restart in Maintenance Mode"', function(done) {
            var payload = '{"Service":"Server","Maintenance":"start"}';
            verifyConfig = JSON.parse( FVT.expectMaintenance ) ;
            verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart in Maintenance Mode', function(done) {
            this.timeout( FVT.MAINTENANCE + 5000 ) ; 
            FVT.sleep( FVT.MAINTENANCE ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on POST "restart end Maintenance Mode"', function(done) {
            var payload = '{"Service":"Server","Maintenance":"stop"}';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
            verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart end Maintenance Mode', function(done) {
            this.timeout( FVT.MAINTENANCE + 5000 ) ; 
            FVT.sleep( FVT.MAINTENANCE ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // Test cases from Defects 
    describe('151664', function() {
    
// Scenario One of the Defect.... should Be able to do a restart with only CleanStore 
//    MM was not part of the original scenario HOWEVER once in this state, I can not PUT into MaintenanceMode either
        it('should return status 400 on POST "restart with Reset:config and CleanStore" Scenario 1', function(done) {
            var payload = '{"Service":"Server","Reset":"config", "CleanStore":true }';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: {\"Service\":\"Server\",\"Reset\":\"config\", \"CleanStore\":true } is not valid.","text":null} ;
// The fix allows this to be successful now CleanStore is redundant if doing a Reset:config -- so "no harm - no foul".
            verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart with Reset:config and CleanStore: Scenario 1', function(done) {
            this.timeout( FVT.MAINTENANCE + 5000 ) ; 
            FVT.sleep( FVT.MAINTENANCE ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// Defect was that this was also flagged as bad
        it('should return status 200 on POST "restart with just CleanStore:true"', function(done) {
            var payload = '{"Service":"Server", "CleanStore":true }';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
            verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart with Reset:config and CleanStore', function(done) {
            this.timeout( FVT.MAINTENANCE + 5000 ) ; 
            FVT.sleep( FVT.MAINTENANCE ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// HAVE to do this before the fix to get out of this odd state so I CAN put into Maintenance Mode
        it('should return status 200 on POST "restart in Reset:config oddly works though"', function(done) {
            var payload = '{"Service":"Server","Reset":"config"}';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
            verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart with Reset:config to correct Error State', function(done) {
            this.timeout( FVT.MAINTENANCE + 5000 ) ; 
            FVT.sleep( FVT.MAINTENANCE ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
// Scenario Two of the Defect.... should not lose MaintenanceMode State (Learned there is a Manual mode and Error Maint. Mode)
        it('should return status 200 on POST "restart in Maintenance Mode"', function(done) {
            var payload = '{"Service":"Server","Maintenance":"start"}';
            verifyConfig = JSON.parse( FVT.expectMaintenance ) ;
            verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart in Maintenance Mode', function(done) {
            this.timeout( FVT.MAINTENANCE + 5000 ) ; 
            FVT.sleep( FVT.MAINTENANCE ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
        it('should return status 400 on POST "restart with Reset:config and CleanStore" Scenario 2', function(done) {
// The fix allows this this payload be successful now CleanStore is redundant if doing a Reset:config -- so "no harm - no foul".
//            var payload = '{"Service":"Server","Reset":"config", "CleanStore":true}';
// changed to
            var payload = '{"Service":"Server","Reset":"config", "Maintenance":"start"}';
            verifyConfig = JSON.parse( FVT.expectMaintenance ) ;
            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: {\"Service\":\"Server\",\"Reset\":\"config\", \"Maintenance\":\"start\"} is not valid.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart with Reset:config and CleanStore:  Scenario 2', function(done) {
            this.timeout( FVT.MAINTENANCE + 5000 ) ; 
            FVT.sleep( FVT.MAINTENANCE ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 on POST "restart and STAY in Maintenance Mode"', function(done) {
            var payload = '{"Service":"Server" }';
            verifyConfig = JSON.parse( FVT.expectMaintenance ) ;
            verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart with Reset:config and CleanStore', function(done) {
            this.timeout( FVT.MAINTENANCE + 5000 ) ; 
            FVT.sleep( FVT.MAINTENANCE ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

// End of Defect 151664    
        it('should return status 200 on POST "restart end Maintenance Mode" end Defect 151664', function(done) {
            var payload = '{"Service":"Server","Maintenance":"stop"}';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
            verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart end Maintenance Mode: end Defect 151664', function(done) {
            this.timeout( FVT.MAINTENANCE + 5000 ) ; 
            FVT.sleep( FVT.MAINTENANCE ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    // Status Error test cases
    describe('Error', function() {
//        this.timeout(15000);

        it('should return status 400 on get "Service" with no action', function(done) {
            var payload = '{}';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: / is not valid."};
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0137","Message":"The REST API call: '+ FVT.uriServiceDomain +' is not valid."}' );
            FVT.makeGetRequest( FVT.uriServiceDomain, FVT.getFailureCallback, verifyMessage, done);
        });
        it('should return status 400 on get, "Status"', function(done) {
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    

        it('should return status 400 GET "/ima/service/status/imaserver", no longer valid', function(done) {
            verifyConfig = JSON.parse( '{"status":400,"Code":"CWLNA0137","Message":"The REST API call: imaserver is not valid."}' );
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/imaserver", FVT.getFailureCallback, verifyConfig, done)
        });
        it('should return status 400 GET "/ima/service/Status", no lowercase', function(done) {
            verifyConfig = JSON.parse( '{"status":400,"Code":"CWLNA0137","Message":"The REST API call: '+ FVT.uriServiceDomain +'Status/ is not valid."}' );
            FVT.makeGetRequest( FVT.uriServiceDomain+"Status/", FVT.getFailureCallback, verifyConfig, done)
        });
    
        it('should return status 400 on post "restart from invalid URI"', function(done) {
//            var payload = null;
            var payload = {} ;
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: /ima/v1/service/restart/imaserver is not valid."} ;
//            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: {} is not valid."} ;
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0137","Message":"The REST API call: {} is not valid."}' );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/service/restart is not valid."}' );
            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/service/restart/imaserver is not valid."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart/imaserver', payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "Status" after restart from invalid URI', function(done) {
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
    
        it('should return status 400 on post "restart from URI"', function(done) {
//            var payload = null;
            var payload = {} ;
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0137","Message":"The REST API call: {} is not valid."}' );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/service/restart is not valid."}' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/service/restart/Server is not valid."}' );
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart/Server', payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "Status" after restart from URI', function(done) {
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 400 on POST "bad keyvalue for Service"', function(done) {
            var payload = '{"Service":"IMA","CleanStore":true}';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0115","Message":"An value is not valid: Name: Service Value: IMA."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: Service Value: IMA."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "Status" after restart with bad keyvalue for Service', function(done) {
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 400 on POST "bad keyword cleanstore"', function(done) {
            var payload = '{"Service":"Server","cleanstore":true}';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
            verifyMessage = {"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: cleanstore."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "Status" after restart with bad keyword cleanstore', function(done) {
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
    
        it('should return status 400 on POST "bad keyword value"', function(done) {
            var payload = '{"Server":"Server","Maintenance":"stop"}';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
            verifyMessage = {"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: Server."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "Status" after restart with Maintenance:T', function(done) {
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 400 on POST "restart with MaintenenceMode:T"', function(done) {
            var payload = '{"Service":"Server","Maintenance":true}';
            verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
//            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: Maintenance:true is not valid."} ;   why not 0134 liek IMA
            verifyMessage = {"status":400,"Code":"CWLNA0115","Message":"An argument is not valid: Name: Maintenance."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET "Status" after restart with Maintenance:T', function(done) {
            FVT.makeGetRequest( FVT.uriServiceDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

});
