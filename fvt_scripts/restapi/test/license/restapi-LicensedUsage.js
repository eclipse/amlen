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
var mqtt = require('mqtt')

var FVT = require('../restapi-CommonFunctions.js')

var uriObject = 'LicensedUsage/' ;
// By default starts in production with the license accepted
var expectDefault = '{"LicensedUsage": "Production", "Accept":true, "Version": "'+ FVT.version +'"}' ;

var verifyConfig = {};
var verifyMessage = {};

/* *******************************************************************
** NEW - Coming in March/April 2016: 
**  SERVICE URI:  "Accept":[true|false] 
**  can be alone or with LicensedUsage:[OneofFourLicenseTypes]
** Changing LicenseUsage should put Server into Maintenance mode until license is Accepted.
**
** ******************************************************************* */
//  ====================  Test Cases Start Here  ====================  //

describe('LicensedUsage:', function() {
this.timeout( FVT.defaultTimeout );

    // Get test cases
    describe('Pristine Get:', function() {
// 90528
        it('should return status 200 on get, DEFAULT of "LicensedUsage":"Production"', function(done) {
		    verifyConfig = JSON.parse(expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
		});

    });

    
    describe('LicensedUsage No Accept:', function() {

        it('should return status 200 on post "Non-Production"', function(done) {
            var payload = '{"LicensedUsage":"Non-Production"}';
            verifyConfig = {"LicensedUsage":"Non-Production", "Accept":false } ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on get, "Non-Production"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
		});

        it('should return status 200 on GET Status in Maintenance on "Non-Production"', function(done) {
		    verifyConfig = JSON.parse( FVT.expectServerMaintenanceLicense ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});
        it('should return 200 POST to Accept "Non-Production"', function(done) {
            var payload = '{"Accept":true}';
            verifyConfig = {"LicensedUsage":"Non-Production", "Accept":true } ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on GET Status RUNNING in "Non-Production"', function(done) {
		    this.timeout( FVT.REBOOT + 3000 );
			FVT.sleep( FVT.REBOOT );
		    verifyConfig = JSON.parse( FVT.expectDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});

		
        it('should return status 200 on post "Developers"', function(done) {
            var payload = '{"LicensedUsage":"Developers"}';
            verifyConfig = {"LicensedUsage":"Developers", "Accept":false } ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on get, "Developers"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
		});

        it('should return status 200 on GET Status in Maintenance on "Developers"', function(done) {
		    verifyConfig = JSON.parse( FVT.expectServerMaintenanceLicense ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});
        it('should return 200 POST to Accept "Developers"', function(done) {
            var payload = '{"Accept":true}';
            verifyConfig = {"LicensedUsage":"Developers", "Accept":true } ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on GET Status RUNNING in "Developers"', function(done) {
		    this.timeout( FVT.REBOOT + 3000 );
			FVT.sleep( FVT.REBOOT );
		    verifyConfig = JSON.parse( FVT.expectDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});

		
        it('should return status 200 on post "IdleStandby"', function(done) {
            var payload = '{"LicensedUsage":"IdleStandby"}';
            verifyConfig = {"LicensedUsage":"IdleStandby", "Accept":false} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on get, "IdleStandby"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
		});

        it('should return status 200 on GET Status in Maintenance on "IdleStandby"', function(done) {
		    verifyConfig = JSON.parse( FVT.expectServerMaintenanceLicense ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});
        it('should return 200 POST to Accept "IdleStandby"', function(done) {
            var payload = '{"Accept":true}';
            verifyConfig = {"LicensedUsage":"IdleStandby", "Accept":false} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on GET Status RUNNING in "IdleStandby"', function(done) {
		    this.timeout( FVT.REBOOT + 3000 );
			FVT.sleep( FVT.REBOOT );
		    verifyConfig = JSON.parse( FVT.expectDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});

		
        it('should return status 200 on post "Production"', function(done) {
            var payload = '{"LicensedUsage":"Production"}';
            verifyConfig = {"LicensedUsage":"Production", "Accept":false} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on get, "Production"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
		});

        it('should return status 200 on GET Status in Maintenance on "Production"', function(done) {
		    verifyConfig = JSON.parse( FVT.expectServerMaintenanceLicense ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});
        it('should return 200 POST to Accept "Production" ', function(done) {
            var payload = '{"Accept":true}';
            verifyConfig = {"LicensedUsage":"Production", "Accept":true} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on GET Status RUNNING in "Production"', function(done) {
		    this.timeout( FVT.REBOOT + 3000 );
			FVT.sleep( FVT.REBOOT );
		    verifyConfig = JSON.parse( FVT.expectDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
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
        it('should return status 200 on GET "Status" after restart with Production License', function(done) {
            this.timeout( FVT.REBOOT + 5000 ) ; 
            FVT.sleep( FVT.REBOOT ) ;
            var verifyStatus = JSON.parse( FVT.expectDefaultStatus ) ;
//            FVT.verifyServerRestart( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
            FVT.makeGetRequest( FVT.uriServiceDomain+'status', FVT.getSuccessCallback, verifyStatus, done)
        });
        it('should return status 200 on get, "Production" persists', function(done) {
            verifyConfig = {"LicensedUsage":"Production", "Accept":true} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });

    
    describe('LicensedUsage with Accept:', function() {
 
        it('should return 200 on POST to accept "Non-Production"', function(done) {
            var payload = '{"LicensedUsage":"Non-Production", "Accept":true}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 GET accepted "Non-Production"', function(done) {
		    this.timeout( FVT.REBOOT + 3000 );
			FVT.sleep( FVT.REBOOT );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on GET Status RUNNING in "Non-Production"', function(done) {
		    verifyConfig = JSON.parse( FVT.expectDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});

		
        it('should return status 200 on POST to accept "Developers"', function(done) {
            var payload = '{"LicensedUsage":"Developers", "Accept":true}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on get, "Developers"', function(done) {
		    this.timeout( FVT.REBOOT + 3000 );
			FVT.sleep( FVT.REBOOT );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on GET Status RUNNING in "Developers"', function(done) {
		    verifyConfig = JSON.parse( FVT.expectDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});

		
        it('should return status 200 on POST to accept "IdleStandby"', function(done) {
            var payload = '{"LicensedUsage":"IdleStandby", "Accept":true}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on get, "IdleStandby"', function(done) {
		    this.timeout( FVT.REBOOT + 3000 );
			FVT.sleep( FVT.REBOOT );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on GET Status RUNNING in "IdleStandby"', function(done) {
		    verifyConfig = JSON.parse( FVT.expectDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});

		
        it('should return status 200 on POST to accept "Production"', function(done) {
            var payload = '{"LicensedUsage":"Production", "Accept":true}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on get, "Production"', function(done) {
		    this.timeout( FVT.REBOOT + 3000 );
			FVT.sleep( FVT.REBOOT );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on GET Status RUNNING in "Production"', function(done) {
		    verifyConfig = JSON.parse( FVT.expectDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});

       
    });

            
    describe('Reset To Default:', function() {
// 140941
        it('should return status 200 on post null - DEFAULT(Production)"', function(done) {
            var payload = '{"LicensedUsage":null}';
            verifyConfig = JSON.parse( expectDefault );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
		});
        it('should return status 200 on get, "DEFAULT(Production)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
		});
		
        it('should return status 200 on GET Status in Maintenance on "DEFAULT(Developers)"', function(done) {
		    verifyConfig = JSON.parse( FVT.expectServerMaintenanceLicense ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});
        it('should return 200 POST to Accept "DEFAULT(Production)"', function(done) {
            var payload = '{"Accept":true}';
            verifyConfig = JSON.parse( expectDefault );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on GET Status RUNNING in "DEFAULT(Developers)"', function(done) {
		    this.timeout( FVT.REBOOT + 3000 );
			FVT.sleep( FVT.REBOOT );
		    verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});
//   Reset to the same value should not put into maintenance mode
    
        it('should return status 200 on post to DEFAULT(Production)"', function(done) {
            var payload = '{"LicensedUsage":"Production" }';
            verifyConfig = JSON.parse( expectDefault );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
		});
        it('should return status 200 on get, "DEFAULT(Production)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
		});
        it('should return status 200 on GET Status still RUNNING in "DEFAULT(Production)"', function(done) {
		    this.timeout( FVT.REBOOT + 3000 );
			FVT.sleep( FVT.REBOOT );
		    verifyConfig = JSON.parse( FVT.expectDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});
//   Reset to the same value with Accept should not put into maintenance mode  (These were added for valid test and to be sure I got back to Devr License)
    
        it('should return status 200 on REPOST to DEFAULT(Production) and accept"', function(done) {
            var payload = '{"LicensedUsage":"Production", "Accept":true }';
            verifyConfig = JSON.parse( expectDefault );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
		});
        it('should return status 200 on REGET, "DEFAULT(Production)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
		});
        it('should return status 200 on REGET Status still RUNNING in "DEFAULT(Production)"', function(done) {
		    this.timeout( FVT.REBOOT + 3000 );
			FVT.sleep( FVT.REBOOT );
		    verifyConfig = JSON.parse( FVT.expectDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});

		
    });


//  ====================  Delete test cases  ====================  //
    describe('Delete Test Cases:', function() {

        it('should return status 403 when deleting "LicensedUsage"', function(done) {
            verifyConfig = JSON.parse( '{"LicensedUsage":"Developers"}' );
//            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object."};
            verifyMessage = {"status":403,"Code":"CWLNA0337","Message":"Delete is not allowed for a singleton object: LicensedUsage"};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

//  ====================  Error test cases  ====================  //
    describe('Error Test Cases: General:', function() {
// 109491
        it('should return status 400 when trying to POST Invalid Value ""', function(done) {
            var payload = '{"LicensedUsage":""}';
            verifyConfig = JSON.parse( expectAutoDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0118","Message":"The properties are not valid."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: LicensedUsage Value: \"\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);        });
        it('should return status 200 on get, Post Invalid Value ""', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        });
// 138659
        it('should return status 400 when trying to POST Invalid Value (Developer)', function(done) {
            var payload = '{"LicensedUsage":"Developer"}';
            verifyConfig = JSON.parse( expectAutoDefault );
//            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"A property value is not valid: Property: LicensedUsage Value: Developer."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: LicensedUsage Value: \"Developer\"."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);        });
        it('should return status 200 on get, Post Invalid Value (Developer)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        });
// 138662
        it('should return 400 on POST "Accept":false invalid', function(done) {
            var payload = '{"LicensedUsage":"Production", "Accept":false}';
            verifyConfig = JSON.parse(payload);
            verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Accept Value: false."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/service/ is not valid."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);        
		});
        it('should return 200 on GET, "Accept":false fails', function(done) {
		    verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
		});

        it('should return 404 on POST "Accept":false is never valid ', function(done) {
            var payload = '{"LicensedUsage":"Production", "Accept":false}';
            verifyConfig = JSON.parse(payload);
//            verifyMessage = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: Accept."} ;
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: Accept Name: null Property: null Type: JSON_FALSE"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);        
		});
        it('should return status 200 on get, "Accept":false is never valid', function(done) {
		    verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on GET Status RUNNING in "Developers"', function(done) {
		    verifyConfig = JSON.parse( FVT.expectDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});

        it('should return 400 on POST "Accept": wrong Domain', function(done) {
            var payload = '{"LicensedUsage":"Production", "Accept":true}';
            verifyConfig = JSON.parse(payload);
//            verifyMessage = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: Accept."} ;
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0127","Message":"The REST API call: /ima/'+ FVT.version +'/service/ is not valid."}' ) ;
            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/service/ is not valid."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);        
		});
        it('should return status 200 on get, "Accept": wrong Domain', function(done) {
		    verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on GET Status RUNNING in "Developers"', function(done) {
		    verifyConfig = JSON.parse( FVT.expectDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});

        it('should return 404 on POST "Accept": wrong Domain/license', function(done) {
            var payload = '{"LicensedUsage":"Production", "Accept":true}';
            verifyConfig = JSON.parse(payload);
//            verifyMessage = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: Accept."} ;
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0137","Message":"The REST API call: /license is not valid."}' ) ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+"license", payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);        
		});
        it('should return status 200 on get, "Accept": wrong Domain/license', function(done) {
		    verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on GET Status RUNNING in "Production"', function(done) {
		    verifyConfig = JSON.parse( FVT.expectDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});

        it('should return 400 on POST "Accept":"false" string ', function(done) {
            var payload = '{"LicensedUsage":"Production", "Accept":"false"}';
            verifyConfig = JSON.parse(payload);
//            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: Accept Name: null Property: null Type: JSON_STRING"} ;
            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: /ima/v1/service/ is not valid."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);        
		});
        it('should return status 200 on get, "Accept":"false" string', function(done) {
		    verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);        
		});
        it('should return status 200 on GET Status RUNNING in "Production"', function(done) {
		    verifyConfig = JSON.parse( FVT.expectDefault ) ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"status/Server", FVT.getSuccessCallback, verifyConfig, done);        
		});

    });

    
//  ====================  RESET to Test Env Default  ====================  //
    describe('RESET to Test Env Default:', function() {

        it('should return 200 POST "LicensedUsage": what it was at start', function(done) {
            var payload = {};
		    payload = expectDefault ;
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
		});
        
        it('should return 200 GET "LicensedUsage": what it was at start', function(done) {
		    this.timeout( FVT.REBOOT + 3000 );
			FVT.sleep( FVT.REBOOT );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
		});
        
    });
 
});
