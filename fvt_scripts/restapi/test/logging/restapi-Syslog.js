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

var uriObject = 'Syslog/';
var expectDefault = '{"Syslog":{"Host":"127.0.0.1", "Port":514, "Protocol":"tcp", "Enabled":false }, "Version": "'+ FVT.version +'"}' ;
// old: var expectDefault = '{"Syslog":{"Host":null, "Port":514, "Protocol":"tcp", "Enabled":false }, "Version": "'+ FVT.version +'"}' ;
var expectConfigedTCP = '{"Syslog":{"Host":"'+ FVT.SyslogServer +'", "Port":'+ FVT.SyslogPort_tcp +', "Protocol":"tcp", "Enabled":false }, "Version": "'+ FVT.version +'"}' ;
var expectConfigedUDP = '{"Syslog":{"Host":"'+ FVT.SyslogServer +'", "Port":'+ FVT.SyslogPort_udp +', "Protocol":"udp", "Enabled":false }, "Version": "'+ FVT.version +'"}' ;
var expectEnabledTCP = '{"Syslog":{"Host":"'+ FVT.SyslogServer +'", "Port":'+ FVT.SyslogPort_tcp +', "Protocol":"tcp", "Enabled":true }, "Version": "'+ FVT.version +'"}' ;
var expectEnabledUDP = '{"Syslog":{"Host":"'+ FVT.SyslogServer +'", "Port":'+ FVT.SyslogPort_udp +', "Protocol":"udp", "Enabled":true }, "Version": "'+ FVT.version +'"}' ;

var portMin = 1 ;
var portDefault = 514 ;
var portMax = 65535 ;

var verifyConfig = {};
var verifyMessage = {};

// Test Cases Start Here

// Syslog test cases
describe('Syslog', function() {
this.timeout( FVT.defaultTimeout );
FVT.trace( 0, "SyslogServer: "+ FVT.SyslogServer +", SyslogPort_tcp: "+ FVT.SyslogPort_tcp +" and SyslogPort_udp: "+ FVT.SyslogPort_udp +" are required for this test.");

    // Syslog Get test cases
    describe('Pristine Get:', function() {
	
        it('should return status 200 on get, DEFAULT of "Syslog"', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
    });

    // Host test cases
    describe('Host:[127.0.0.1]255', function() {
// 134680 
		it('should return status 200 POST Syslog', function(done) {
			var payload = expectConfigedTCP ;
			verifyConfig = JSON.parse( expectConfigedTCP ) ;
			FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
		});
		it('should return status 200 on GET after POST Enable without Host', function(done) {
			FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
		});

        it('should return status 200 on RESTART Server and not loose config', function(done) {
            var payload = ( '{"Service":"Server"}' );
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriServiceDomain+"restart/", payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after RESTART Server and not loose config "Syslog"', function(done) {
		    this.timeout( FVT.REBOOT + 3000 );
		    FVT.sleep( FVT.REBOOT );
			verifyConfig = JSON.parse( expectConfigedTCP );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});

    // Port test cases
    describe('Port:[1,514,65535]', function() {
	
        it('should return status 200 POST "Port:1', function(done) {
            var payload = '{"Syslog":{"Port":1 }}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Port:1', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 POST "Port:null (Default)', function(done) {
            var payload = '{"Syslog":{"Port":null }}';
            verifyConfig = {"Syslog":{"Port":514 }} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Port:null (Default)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
        it('should return status 200 POST "Port:65535', function(done) {
            var payload = '{"Syslog":{"Port":65535 }}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Port:65535', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 POST "Port:514', function(done) {
            var payload = '{"Syslog":{"Port":514 }}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Port:514', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
				
	});

    // Protocol test cases
    describe('Protocol:[tcp,udp]', function() {
	
        it('should return status 200 POST "Protocol":"udp"', function(done) {
            var payload = '{"Syslog":{"Protocol":"udp", "Port":'+ FVT.SyslogPort_udp +' }}';
            verifyConfig = JSON.parse( expectConfigedUDP ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 GET, "Protocol":"udp"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 POST "Protocol":null (Default)', function(done) {
            var payload = '{"Syslog":{"Protocol":null}}';
            verifyConfig = {"Syslog":{"Protocol":"tcp" }} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 GET, "Protocol":null (Default)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 POST "Protocol":"udp"', function(done) {
            var payload = '{"Syslog":{"Protocol":"udp" }}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 GET, "Protocol":"udp"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
	
        it('should return status 200 POST "Protocol":"tcp"', function(done) {
            var payload = '{"Syslog":{"Protocol":"tcp" }}';
            verifyConfig = JSON.parse( payload ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 GET, "Protocol":"tcp"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});


    // Enabled test cases
    describe('Enabled:[false,true]', function() {
	
		it('should return status 200 POST Syslog Enabled true', function(done) {
			var payload = '{"Syslog":{"Host":"'+ FVT.SyslogServer +'","Port":'+ FVT.SyslogPort_tcp +',"Protocol":"tcp", "Enabled":true }}' ;
			verifyConfig = JSON.parse( expectEnabledTCP ) ;
			FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
		});
		it('should return status 200 on GET after POST Enabled true', function(done) {
			FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
		});
// it the connection fails, the change is undone  --  at least that has been my observation
		it('should return status 400 POST Syslog Enabled true with BadServer', function(done) {
			var payload = '{"Syslog":{"Host":"'+ FVT.msgServer +'","Port":'+ FVT.SyslogPort_tcp +',"Protocol":"tcp", "Enabled":true }}';
			verifyConfig = JSON.parse( expectEnabledTCP ) ;
			verifyMessage = { "status":400, "Code":"CWLNA0150","Message":"Unable to connect." } ;
			FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
		});
		it('should return status 200 on GET after POST Enabled true with BadServer', function(done) {
			FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
		});
		
        it('should return status 200 on post "Enabled":false', function(done) {
            var payload = '{"Syslog":{"Host":null, "Enabled":false }}';
            verifyConfig = JSON.parse( expectDefault ) ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "Enabled":false', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
		
	});

    // Syslog Delete test cases
    describe('Delete:', function() {
	
        it('should return status 403 DELETE Syslog', function(done) {
            verifyConfig = JSON.parse( expectDefault ) ;
			verifyMessage = {"status":403,"Code":"CWLNA0372","Message":"Delete is not allowed for Syslog object."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 GET after failed DELETE Syslog', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });

    // Syslog Error test cases
    describe('Error', function() {
	    
		// General test cases
		describe('General:', function() {
		
			it('should return status 404 POST EnableSyslog', function(done) {
				var payload = '{"Syslog":{"Host":"'+ FVT.SyslogServer +'", "EnableSyslog":true }}';
				verifyConfig = JSON.parse( payload ) ;
//				verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: Syslog Name: Syslog Property: EnableSyslog" };
				verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: Syslog Name: Syslog Property: EnableSyslog" };
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET after POST EnableSyslog', function(done) {
				verifyConfig = JSON.parse( expectDefault );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
			
		});	
	
	    // Host test cases
		describe('Host and Enabled:', function() {
// 134897
			it('should return status 400 POST Enable without Host', function(done) {
				var payload = '{"Syslog":{"Host":"", "Enabled":true }}';
				verifyConfig = JSON.parse( payload ) ;
//				verifyMessage = {"status":400,"Code":"CWLNA0150","Message":"Unable to connect." };
				verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Host Value: \"\"." };
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET after POST Enable without Host', function(done) {
				verifyConfig = JSON.parse( expectDefault );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
		
			it('should return status 400 POST Port with Host', function(done) {
				var payload = '{"Syslog":{"Host":"'+ FVT.SyslogServer +':'+ FVT.SyslogPort_tcp +'", "Enabled":true }}';
				verifyConfig = JSON.parse( payload ) ;
				verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Host Value: \\\"'+ FVT.SyslogServer +':'+ FVT.SyslogPort_tcp +'\\\"." }' );
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET after POST Enable Port with Host', function(done) {
				verifyConfig = JSON.parse( expectDefault );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
// 136346		
			it('should return status 400 POST with Hostname', function(done) {
				var payload = '{"Syslog":{"Host":"'+ FVT.A1_HOSTNAME_OS +'", "Enabled":false }}';
				verifyConfig = JSON.parse( payload ) ;
				verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Host Value: \\\"'+ FVT.A1_HOSTNAME_OS +'\\\"." }' );
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET after POST with Hostname', function(done) {
				verifyConfig = JSON.parse( expectDefault );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
			
		});

		// Port test cases
		describe('Port:[1,514,65535]', function() {
		
			it('should return status 400 on POST "Port":0', function(done) {
				var payload = '{"Syslog":{"Port":0 }}';
				verifyConfig = JSON.parse( payload ) ;
				verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Port Value: \"0\"." };
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET after POST "Port":0 failed ', function(done) {
				verifyConfig = JSON.parse( expectDefault );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
		
			it('should return status 400 on POST "Port":65536', function(done) {
				var payload = '{"Syslog":{"Port":65536 }}';
				verifyConfig = JSON.parse( payload ) ;
				verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Port Value: \"65536\"." };
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET after POST "Port":65536 failed ', function(done) {
				verifyConfig = JSON.parse( expectDefault );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
			
		});

		// Protocol test cases
		describe('Protocol:[tcp,udp]', function() {
		
			it('should return status 400 on POST "Protocol":65536', function(done) {
				var payload = '{"Syslog":{"Protocol":65536 }}';
				verifyConfig = JSON.parse( payload ) ;
				verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: Syslog Name: Syslog Property: Protocol Type: JSON_INTEGER" };
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET after POST "Protocol":65536 failed ', function(done) {
				verifyConfig = JSON.parse( expectDefault );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
		
			it('should return status 400 on POST "Protocol":"ftp"', function(done) {
				var payload = '{"Syslog":{"Protocol":"ftp" }}';
				verifyConfig = JSON.parse( payload ) ;
				verifyMessage = {"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: Protocol Value: \"ftp\"." };
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET after POST "Protocol":"ftp" failed ', function(done) {
				verifyConfig = JSON.parse( expectDefault );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
			
		});

		// Enabled test cases
		describe('Enabled:[false,true]', function() {
		
			it('should return status 400 on POST "Enabled":1', function(done) {
				var payload = '{"Syslog":{"Enabled":1 }}';
				verifyConfig = JSON.parse( payload ) ;
				verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: Syslog Name: Syslog Property: Enabled Type: JSON_INTEGER" };
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET after POST "Enabled":1 failed ', function(done) {
				verifyConfig = JSON.parse( expectDefault );
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
		
		});		
	});
	

});
