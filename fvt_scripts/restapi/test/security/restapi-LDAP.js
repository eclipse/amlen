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

var uriObject = 'LDAP/' ;
var expectDefault = '{"LDAP": { "BaseDN": "", "BindDN": "", "CacheTimeout": 10, "Certificate": "", "EnableCache": true, "Enabled": false, "GroupCacheTimeout": 300, "GroupIdMap": "", "GroupMemberIdMap": "", "GroupSuffix": "", "IgnoreCase": true, "MaxConnections": 100, "NestedGroupSearch": false, "Timeout": 30, "URL": "", "UserIdMap": "", "UserSuffix": "", "Verify": false},"Version": "'+ FVT.version +'"}' ;
var configuredLDAP = '{"LDAP": {"CacheTimeout": 10, "Enabled": true, "URL": "ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "EnableCache": true, "BaseDN": "ou=SVT,o=IBM,c=US", "UserIdMap": "uid", "GroupSuffix": "ou=groups,ou=SVT,o=IBM,c=US", "GroupCacheTimeout": 300, "BindDN": "cn=root,o=IBM,c=US", "Verify": false, "UserSuffix": "ou=users,ou=SVT,o=IBM,c=US", "BindPassword": "XXXXXX", "GroupIdMap": "cn", "Certificate": "", "GroupMemberIdMap": "member", "Timeout": 10, "MaxConnections": 10, "IgnoreCase": true, "NestedGroupSearch": false   }}' ;
var expectFinalDefault = '{"LDAP": { "BaseDN": "", "BindDN": "","BindPassword": "XXXXXX", "CacheTimeout": 10, "Certificate": "", "EnableCache": true, "Enabled": false, "GroupCacheTimeout": 300, "GroupIdMap": "", "GroupMemberIdMap": "", "GroupSuffix": "", "IgnoreCase": true, "MaxConnections": 100, "NestedGroupSearch": false, "Timeout": 30, "URL": "", "UserIdMap": "", "UserSuffix": "", "Verify": false},"Version": "'+ FVT.version +'"}' ;

var verifyConfig = {} ;
var verifyMessage = {};


//  ====================  Test Cases Start Here  ====================  //
//      "ListObjects":"Enabled,URL,Certificate,IgnoreCase,BaseDN,BindDN,UserSuffix,GroupSuffix,UserIdMap,GroupIdMap,GroupMemberIdMap,EnableCache,CacheTimeout,GroupCacheTimeout,Timeout,MaxConnections,NestedGroupSearch",

describe('LDAP:', function() {
this.timeout( FVT.defaultTimeout );
// 104408
    //  ====================   Get test cases  ====================  //
    describe('Get - verify in Pristine State:', function() {

		it('should return status 200 on get, DEFAULT of "LDAP":{}}', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

		it('should return status 200 on get, LDAP/ldapconfig', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'ldapconfig', FVT.getSuccessCallback, verifyConfig, done)
        });
    
    });

    //  ====================   Create - Add - Update test cases  ====================  //
    describe('Prereq keyfiles:', function() {

        it('should return status 200 on PUT "LDAPCert1.pem"', function(done) {
            sourceFile = '../common/imaCA-crt.pem';
            targetFile = 'LDAPCert1.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "LDAPCert2.pem"', function(done) {
            sourceFile = '../common/imaCA-crt.pem';
            targetFile = 'LDAPCert2.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "MaxLenCertKeyName"', function(done) {
            sourceFile = '../common/imaCA-crt.pem';
            targetFile = FVT.maxFileName255;
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "LDAP.pem"', function(done) {
            sourceFile = '../common/imaCA-crt.pem';
            targetFile = 'LDAP.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "BadCertKey.pem"', function(done) {
            sourceFile = 'mar400.wasltpa.keyfile';
            targetFile = 'BadCertKey.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

    });


    describe('Create LDAP:', function() {

        it('should return status 200 on post "LDAP"', function(done) {
            var payload = '{"LDAP":{"Enabled":true, "URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"ima4test", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "EnableCache":true, "CacheTimeout":10, "GroupCacheTimeout":300, "Timeout":30, "MaxConnections":100, "IgnoreCase":false, "NestedGroupSearch":false}}';
            verifyConfig = JSON.parse( '{"LDAP":{"Enabled":true, "URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "EnableCache":true, "CacheTimeout":10, "GroupCacheTimeout":300, "Timeout":30, "MaxConnections":100, "IgnoreCase":false, "NestedGroupSearch":false}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get, "LDAP"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });

  describe('Update LDAP:', function() {
// **********************************************************************************
// * Opting to NOT TEST following KEYS, they require reconfiguration on LDAP to test:
// * These have NO Size Bounds to test anyway and default to NULL String. 
// * REQUIRED:  BaseDN, GroupSuffix, UserIdMap, GroupIdMap, GroupMembershipIdMap
// * OPTIONAL: BindDN, BindPassword, UserSuffix
// **********************************************************************************

    describe('BaseDN[]R:', function() {

         it('should return status 200 on rePOST "BaseDN"', function(done) {
            var payload = '{"LDAP":{"BaseDN":"ou=SVT,o=IBM,c=US" }}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after rePOST "BaseDN', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });	

    });

    describe('BindDN[""]:', function() {
	
         it('should return status 200 on rePOST "BindDN"', function(done) {
            var payload = '{"LDAP":{"BindDN":"cn=root,o=IBM,c=US" }}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after rePOST "BindDn', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });	

    });
		
    describe('BindPassword[""]:', function() {
	
         it('should return status 200 on rePOST "BindPassword"', function(done) {
            var payload = '{"LDAP":{"BindPassword":"ima4test"}}';
            verifyConfig = JSON.parse( '{"LDAP":{"BindPassword":"XXXXXX"}}' );
            verifyMessage = {"status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after rePOST "BindPassword', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });	

    });

    describe('GroupSuffix[]R:', function() {
	
         it('should return status 200 on rePOST "GroupSuffix"', function(done) {
            var payload = '{"LDAP":{"GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US" }}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after rePOST "GroupSuffix', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });	

    });

    describe('GroupIdMap[""]:', function() {
	
         it('should return status 200 on rePOST "GroupIdMap"', function(done) {
            var payload = '{"LDAP":{"GroupIdMap":"cn" }}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after rePOST "GroupIdMap', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });	

    });

    describe('GroupMemberIdMap[""]:', function() {
	
         it('should return status 200 on rePOST "GroupMemberIdMap"', function(done) {
            var payload = '{"LDAP":{"GroupMemberIdMap":"member" }}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after rePOST "GroupMemberIdMap', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });	

    });

    describe('UserSuffix[""]:', function() {
	
         it('should return status 200 on rePOST "UserSuffix"', function(done) {
            var payload = '{"LDAP":{"UserSuffix":"ou=users,ou=SVT,o=IBM,c=US" }}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after rePOST "UserSuffix', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });	

    });

    describe('UserIdMap[""]:', function() {
// 104556
         it('should return status 200 on rePOST "UserIdMap"', function(done) {
            var payload = '{"LDAP":{"UserIdMap":"uid" }}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after rePOST "UserIdMap', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });


    describe('Enabled[T]:', function() {
// 104561
         it('should return status 200 when POST "Enabled:false"', function(done) {
            var payload = '{"LDAP":{"Enabled":false}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST "Enabled:false"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

         it('should return status 200 when POST "Enabled:True"', function(done) {
            var payload = '{"LDAP":{"Enabled":true}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST "Enabled:True"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

         it('should return status 200 when POST "Enabled:False"', function(done) {
            var payload = '{"LDAP":{"Enabled":false}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST "Enabled:False"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 104836
         it('should return status 200 when POST "Enabled:Default(T)"', function(done) {
            var payload = '{"LDAP":{"Enabled":null}}';
            verifyConfig = {"LDAP":{"Enabled":true}};
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after POST "Enabled:Default(T)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });

    describe('Verify[F], Certificate[""] and URL[ldap://, ldaps://]:', function() {
        this.timeout(36000);
// Verify:true will test a LDAP Connection w-w/o Cert  BUT NOT COMMIT the change.....Cert Stays in /tmp/userfiles if specified, URL is not updated
// Verify:false, change is committed, Cert moved from /tmp/userfiles

// 108231
		it('should return status 200 when Verify URL and CERT ', function(done) {
			var payload = '{"LDAP":{"Verify":true, "URL":"ldaps://'+ FVT.LDAP_SERVER_1 + ':' + FVT.LDAP_SERVER_1_SSLPORT +'", "Certificate":"LDAPCert1.pem"}}';
			verifyConfig = JSON.parse( payload );
			FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
		});
		it('should return status 200 on get, after POST to Verify URL and CERT', function(done) {
			verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'","Certificate":"", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
			FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
		});


// Why do I need overwrite here?  REALLY should only be once CERT gets committed? -- check that  104879
// 104879, 134440
        it('should return status 200 on POST "Certificate and URL(SSL)" to commit', function(done) {
            var payload = '{"LDAP":{"Verify":false, "URL":"ldaps://'+ FVT.LDAP_SERVER_1 + ':' + FVT.LDAP_SERVER_1_SSLPORT +'","Certificate":"LDAPCert1.pem","Overwrite":true}}';
            var payload = '{"LDAP":{"Verify":false, "URL":"ldaps://'+ FVT.LDAP_SERVER_1 + ':' + FVT.LDAP_SERVER_1_SSLPORT +'","Certificate":"LDAPCert1.pem"}}';
            verifyConfig = JSON.parse( '{"LDAP":{"Verify":false, "URL":"ldaps://'+ FVT.LDAP_SERVER_1 + ':' + FVT.LDAP_SERVER_1_SSLPORT +'","Certificate":"ldap.pem"}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Certificate and URL(SSL)" to commit', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    });

//133554
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
        it('should return status 200 on get, "LDAPS" persists', function(done) {
            verifyConfig = JSON.parse( '{"LDAP":{"Verify":false, "URL":"ldaps://'+ FVT.LDAP_SERVER_1 + ':' + FVT.LDAP_SERVER_1_SSLPORT +'","Certificate":"ldap.pem"}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });
		
    describe('Verify, Certificate and URL cont:', function() {
		
// 104418
        it('should return status 200 on POST  change "Certificate" MAX Len Name', function(done) {
            var payload = '{"LDAP":{"Certificate":"'+ FVT.maxFileName255 +'", "Overwrite":true}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET change "Certificate" MAX Len Name', function(done) {
			verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldaps://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_SSLPORT +'","Certificate":"ldap.pem", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
	        FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 104409
        it('should return status 200 on POST "remove Certificate and URL(SSL)"', function(done) {
            var payload = '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 + ':' + FVT.LDAP_SERVER_1_PORT +'","Certificate":null}}';
            verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 + ':' + FVT.LDAP_SERVER_1_PORT +'","Certificate":"", "Verify":false }}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "remove Certificate and URL(SSL)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    
        it('should return status 200 on FAILSAFE POST "remove Certificate and URL(SSL)"', function(done) {
            var payload = '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 + ':' + FVT.LDAP_SERVER_1_PORT +'","Certificate":""}}';
            verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 + ':' + FVT.LDAP_SERVER_1_PORT +'","Certificate":"", "Verify":false }}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on FAILSAFE GET "remove Certificate and URL(SSL)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
            
    });

    describe('IgnoreCase[T]:', function() {
// 104882
        it('should return status 200 on POST "IgnoreCase:False"', function(done) {
            var payload = '{"LDAP":{"IgnoreCase":false}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "IgnoreCase:False"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "IgnoreCase:True"', function(done) {
            var payload = '{"LDAP":{"IgnoreCase":true}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "IgnoreCase:True"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "IgnoreCase:False"', function(done) {
            var payload = '{"LDAP":{"IgnoreCase":false}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "IgnoreCase:False"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
//104836
        it('should return status 200 on POST "IgnoreCase:DEFAULT(T)"', function(done) {
            var payload = '{"LDAP":{"IgnoreCase":null}}';
            verifyConfig = {"LDAP":{"IgnoreCase":true}};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "IgnoreCase:DEFAULT(T)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });

    describe('EnableCache[T]:', function() {

         it('should return status 200 when POST "EnableCache:False"', function(done) {
            var payload = '{"LDAP":{"EnableCache":false}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST "EnableCache:False"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

         it('should return status 200 when POST "EnableCache:True"', function(done) {
            var payload = '{"LDAP":{"EnableCache":true}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST "EnableCache:True"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

         it('should return status 200 when POST "EnableCache:False"', function(done) {
            var payload = '{"LDAP":{"EnableCache":false}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST "EnableCache:False"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 104836
         it('should return status 200 when POST "EnableCache:Default(T)"', function(done) {
            var payload = '{"LDAP":{"EnableCache":null}}';
            verifyConfig = {"LDAP":{"EnableCache":true}};
            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after POST "EnableCache:Default(T)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
    });

    describe('NestedGroupSearch[F]:', function() {

        it('should return status 200 on POST "NestedGroupSearch:True"', function(done) {
            var payload = '{"LDAP":{"NestedGroupSearch":true}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "NestedGroupSearch:True"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "NestedGroupSearch:False"', function(done) {
            var payload = '{"LDAP":{"NestedGroupSearch":false}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "NestedGroupSearch:False"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "NestedGroupSearch:True"', function(done) {
            var payload = '{"LDAP":{"NestedGroupSearch":true}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "NestedGroupSearch:True"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 104836
        it('should return status 200 on POST "NestedGroupSearch:DEFAULT(F)"', function(done) {
            var payload = '{"LDAP":{"NestedGroupSearch":null}}';
            verifyConfig = {"LDAP":{"NestedGroupSearch":false}};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "NestedGroupSearch:DEFAULT(F)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });


    describe('MaxConnections[1-10-100]:', function() {

        it('should return status 200 on POST "MaxConnections:MIN"', function(done) {
            var payload = '{"LDAP":{"MaxConnections":1}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "MaxConnections:MIN"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "MaxConnections:69"', function(done) {
            var payload = '{"LDAP":{"MaxConnections":69}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "MaxConnections:69"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "MaxConnections:MAX"', function(done) {
            var payload = '{"LDAP":{"MaxConnections":100}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "MaxConnections:MAX"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 104864
        it('should return status 200 on POST "MaxConnections:DEFAULT(10)"', function(done) {
            var payload = '{"LDAP":{"MaxConnections":null}}';
            verifyConfig = {"LDAP":{"MaxConnections":10}};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "MaxConnections:DEFAULT(10)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });

    describe('Timeout[1-10-60]:', function() {

        it('should return status 200 on POST "Timeout:MIN"', function(done) {
            var payload = '{"LDAP":{"Timeout":1}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Timeout:MIN"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "Timeout:39"', function(done) {
            var payload = '{"LDAP":{"Timeout":39}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Timeout:39"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "Timeout:MAX"', function(done) {
            var payload = '{"LDAP":{"Timeout":60}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Timeout:MAX"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 104864
        it('should return status 200 on POST "Timeout:DEFAULT(10)"', function(done) {
            var payload = '{"LDAP":{"Timeout":null}}';
            verifyConfig = {"LDAP":{"Timeout":10}};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "Timeout:DEFAULT(10)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });
    
    describe('CacheTimeout[1-10-60]:', function() {

        it('should return status 200 on POST "CacheTimeout:MIN"', function(done) {
            var payload = '{"LDAP":{"CacheTimeout":1}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "CacheTimeout:MIN"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "CacheTimeout:39"', function(done) {
            var payload = '{"LDAP":{"CacheTimeout":39}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "CacheTimeout:39"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "CacheTimeout:MAX"', function(done) {
            var payload = '{"LDAP":{"CacheTimeout":60}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "CacheTimeout:MAX"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 104864
        it('should return status 200 on POST "CacheTimeout:DEFAULT(10)"', function(done) {
            var payload = '{"LDAP":{"CacheTimeout":null}}';
            verifyConfig = {"LDAP":{"CacheTimeout":10}};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "CacheTimeout:DEFAULT(10)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });
    
    describe('GroupCacheTimeout[1-300-86400]:', function() {

        it('should return status 200 on POST "GroupCacheTimeout:MIN"', function(done) {
            var payload = '{"LDAP":{"GroupCacheTimeout":1}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "GroupCacheTimeout:MIN"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "GroupCacheTimeout:69"', function(done) {
            var payload = '{"LDAP":{"GroupCacheTimeout":69}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "GroupCacheTimeout:69"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on POST "GroupCacheTimeout:MAX"', function(done) {
            var payload = '{"LDAP":{"GroupCacheTimeout":86400}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "GroupCacheTimeout:MAX"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 104864
        it('should return status 200 on POST "GroupCacheTimeout:DEFAULT(300)"', function(done) {
            var payload = '{"LDAP":{"GroupCacheTimeout":null}}';
            verifyConfig = {"LDAP":{"GroupCacheTimeout":300}};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "GroupCacheTimeout:DEFAULT(300)"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

    });

  });

  
//  ====================  Error test cases  ====================  //
    describe('Error:', function() {
	
//		describe('General:', function() {
// need invalid key test yet beefed up, see note below

		
		describe('Invalid key "Update":', function() {
// 120951
			it('should return status 400 Post "Update":not a Key', function(done) {
				var payload = '{"LDAP":{"Update":true}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":404,"Code":"CWLNA0115","Message":"An argument is not valid: Name: Update."} ;
//				verifyMessage = {"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: LDAP Name: ldapconfig Property: Update"} ;
				verifyMessage = {"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: LDAP Name: ldapconfig Property: Update"} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get, after POST "Update":not a Key', function(done) {
//				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//  above: what should be there, want to verify Invalid Key is NOT there... (regular getSuccessCallback will only look for the above in results and ignore extras...
//				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done)
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			 it('should return status 400 on POST "BaseDn":"BaseDN" - bad case', function(done) {
				var payload = '{"LDAP":{"BaseDn:"BaseDN"}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
				verifyMessage = {"status":400,"Code": "CWLNA6001","Message": "Failed to parse administrative request ':' expected near 'BaseDN': RC=1."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "BaseDn":"BaseDN" - bad case', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	

		});

		describe('Verify[F]:', function() {

			it('should return status 400 Post "Verify":not take string', function(done) {
				var payload = '{"LDAP":{"Verify":"false"}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"Code":"CWLNA0132","Message":"The property value is not valid. Object: LDAP Name: ldapconfig Property: Verify Value: JSON_STRING."} ;
				verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: LDAP Name: ldapconfig Property: Verify Type: JSON_STRING"} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get, after POST "Verify":not take string', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			it('should return status 400 Post "Verify":not take number', function(done) {
				var payload = '{"LDAP":{"Verify":1}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"Code":"CWLNA0132","Message":"The property value is not valid. Object: LDAP Name: ldapconfig Property: Verify Value: JSON_INTEGER."} ;
				verifyMessage = {"Code":"CWLNA0127","Message":"The property type is not valid. Object: LDAP Name: ldapconfig Property: Verify Type: JSON_INTEGER"} ;
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get, after POST "Verify":not take number', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

		});

		describe('BaseDN[]R:', function() {

			 it('should return status 400 on POST "BaseDN:false - bad form', function(done) {
				var payload = '{"LDAP":{"BaseDN:false}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
				verifyMessage = {"status":400,"Code": "CWLNA6001","Message": "Failed to parse administrative request premature end of input near '\"BaseDN:false}}': RC=1."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "BaseDN:false - bad form', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	

			 it('should return status 400 on POST "BaseDN":false', function(done) {
				var payload = '{"LDAP":{"BaseDN":false}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: BaseDN Value: JSON_FALSE."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: BaseDN Type: JSON_FALSE"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "BaseDN":false', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	

			 it('should return status 400 on POST "BaseDN":null', function(done) {
				var payload = '{"LDAP":{"BaseDN":null}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: BaseDN Value: JSON_NULL."};
//				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type  is not valid. Object: LDAP Name: ldapconfig Property: BaseDN Type: JSON_NULL."};
//				verifyMessage = {"status":400,"Code": "CWLNA0112","Message": "The property value is not valid: Property: BaseDN Value: null."};
				verifyMessage = {"status":400,"Code": "CWLNA0112","Message": "The property value is not valid: Property: BaseDN Value: \"NULL\"."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "BaseDN":null', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	
// 104894
			 it('should return status 400 on POST "BaseDN":""', function(done) {
				var payload = '{"LDAP":{"BaseDN":""}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: BaseDN Value: \"\"."};
//				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type  is not valid. Object: LDAP Name: ldapconfig Property: BaseDN Type: \"\"."};
				verifyMessage = {"status":400,"Code": "CWLNA0112","Message": "The property value is not valid: Property: BaseDN Value: \"\"."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "BaseDN":""', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	

		});

		describe('BindDN[""]:', function() {
		
			 it('should return status 400 on POST "BindDN":false', function(done) {
				var payload = '{"LDAP":{"BindDN":false}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: BindDN Value: JSON_FALSE."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: BindDN Type: JSON_FALSE"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "BindDN":false', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	
		
			 it('should return status 400 on POST "BindDN":9', function(done) {
				var payload = '{"LDAP":{"BindDN":9}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: BindDN Value: JSON_INTEGER."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: BindDN Type: JSON_INTEGER"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "BindDN":9', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	

		});
			
		describe('BindPassword[""]:', function() {
// 107386
			 it('should return status 400 on POST "BindPassword"=false', function(done) {
				var payload = '{"LDAP":{"BindPassword":false}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: BindPassword Value: JSON_FALSE."};
//				verifyMessage = {"status":400,"Code": "CWLNA0115","Message": "An argument is not valid: Name: BindPassword."};
//				verifyMessage = {"status":400,"Code": "CWLNA0138","Message": "The property name is invalid. Object: LDAP Name: ldapconfig Property: BindPassword"};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: BindPassword Type: JSON_FALSE"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "BindPassword":false', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	
// 107386
			 it('should return status 400 on POST "BindPassword"=69', function(done) {
				var payload = '{"LDAP":{"BindPassword":69}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: BindPassword Value: JSON_INTEGER."};
//				verifyMessage = {"status":400,"Code": "CWLNA0115","Message": "An argument is not valid: Name: BindPassword."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: BindPassword Type: JSON_INTEGER"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "BindPassword":69', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	

		});

		describe('GroupSuffix[]R:', function() {

			 it('should return status 400 on POST "GroupSuffix":false', function(done) {
				var payload = '{"LDAP":{"GroupSuffix":false}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: GroupSuffix Value: JSON_FALSE."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: GroupSuffix Type: JSON_FALSE"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "GroupSuffix":false', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	

			 it('should return status 400 on POST "GroupSuffix":22', function(done) {
				var payload = '{"LDAP":{"GroupSuffix":22}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: GroupSuffix Value: JSON_INTEGER."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: GroupSuffix Type: JSON_INTEGER"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "GroupSuffix":22', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	

			 it('should return status 400 on POST "GroupSuffix":null', function(done) {
				var payload = '{"LDAP":{"GroupSuffix":null}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: GroupSuffix Value: JSON_NULL."};
//				verifyMessage = {"status":400,"Code": "CWLNA0112","Message": "The property value is not valid: Property: GroupSuffix Value: null."};
				verifyMessage = {"status":400,"Code": "CWLNA0112","Message": "The property value is not valid: Property: GroupSuffix Value: \"NULL\"."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "GroupSuffix":null', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	
// 104894
			 it('should return status 400 on POST "GroupSuffix":""', function(done) {
				var payload = '{"LDAP":{"GroupSuffix":""}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: GroupSuffix Value: JSON_NULL."};
				verifyMessage = {"status":400,"Code": "CWLNA0112","Message": "The property value is not valid: Property: GroupSuffix Value: \"\"."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "GroupSuffix":""', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	

		});

		describe('GroupIdMap[""]:', function() {

			 it('should return status 400 on POST "GroupIdMap":false', function(done) {
				var payload = '{"LDAP":{"GroupIdMap":false}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: GroupIdMap Value: JSON_FALSE."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: GroupIdMap Type: JSON_FALSE"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "GroupIdMap":false', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	

			 it('should return status 400 on POST "GroupIdMap":99', function(done) {
				var payload = '{"LDAP":{"GroupIdMap":99}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: GroupIdMap Value: JSON_INTEGER."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: GroupIdMap Type: JSON_INTEGER"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "GroupIdMap":99', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	

		});

		describe('GroupMemberIdMap[""]:', function() {

			 it('should return status 400 on POST "GroupMemberIdMap":false', function(done) {
				var payload = '{"LDAP":{"GroupMemberIdMap":false}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: GroupMemberIdMap Value: JSON_FALSE."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: GroupMemberIdMap Type: JSON_FALSE"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "GroupMemberIdMap":false', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	

			 it('should return status 400 on POST "GroupMemberIdMap":88', function(done) {
				var payload = '{"LDAP":{"GroupMemberIdMap":88}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: GroupMemberIdMap Value: JSON_INTEGER."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: GroupMemberIdMap Type: JSON_INTEGER"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "GroupMemberIdMap":88', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	

		});

		describe('UserSuffix[""]:', function() {

			 it('should return status 400 on POST "UserSuffix":false', function(done) {
				var payload = '{"LDAP":{"UserSuffix":false}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: UserSuffix Value: JSON_FALSE."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: UserSuffix Type: JSON_FALSE"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "UserSuffix":false', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	

			 it('should return status 400 on POST "UserSuffix":77', function(done) {
				var payload = '{"LDAP":{"UserSuffix":77}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: UserSuffix Value: JSON_INTEGER."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: UserSuffix Type: JSON_INTEGER"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "UserSuffix":77', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});	

		});

		describe('UserIdMap[""]:', function() {

			 it('should return status 400 on POST "UserIdMap":false', function(done) {
				var payload = '{"LDAP":{"UserIdMap":false}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: UserIdMap Value: JSON_FALSE."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: UserIdMap Type: JSON_FALSE"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "UserIdMap":false', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			 it('should return status 400 on POST "UserIdMap":66', function(done) {
				var payload = '{"LDAP":{"UserIdMap":66}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: UserIdMap Value: JSON_INTEGER."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: UserIdMap Type: JSON_INTEGER"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "UserIdMap":66', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

		});


		describe('Enabled[T]:', function() {

			 it('should return status 200 when POST "Enabled":"false" Not A String', function(done) {
				var payload = '{"LDAP":{"Enabled":"false"}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: Enabled Value: JSON_STRING."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: Enabled Type: JSON_STRING"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "Enabled":"false" Not A String', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			 it('should return status 200 when POST "Enabled:1" Not Numeric', function(done) {
				var payload = '{"LDAP":{"Enabled":1}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: Enabled Value: JSON_INTEGER."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: Enabled Type: JSON_INTEGER"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "Enabled:1" Not Numeric', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			 it('should return status 200 when POST "Enabled":""', function(done) {
				var payload = '{"LDAP":{"Enabled":""}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: Enabled Value: JSON_STRING."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: Enabled Type: JSON_STRING"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "Enabled":""', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
			
		});

		describe('Certificate[""] and URL[ldap://, ldaps://]:', function() {
// 104896, 104893   NOT SURE ABOUT THIS MESSAGE
			it('should return status 400 on POST "BAD Certificate and URL(SSL)"', function(done) {
			    this.timeout( 12000 );
				var payload = '{"LDAP":{"URL":"ldaps://'+ FVT.LDAP_SERVER_1 + ':' + FVT.LDAP_SERVER_1_SSLPORT +'","Certificate":"BadCertKey.pem", "Verify":true}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA8280","Message": "The certificate already exists. Set Overwrite to true to replace the existing certificate."};
//				verifyMessage = {"status":400,"Code": "CWLNA8229","Message": "The certificate failed the verification."};
//				verifyMessage = {"status":400,"Code": "CWLNA0447","Message": "Failed to bind with the LDAP server: Timeout. The error code is 447."};
				verifyMessage = {"status":400,"Code": "CWLNA0449","Message": "The certificate for LDAP is not valid or does not exist."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "Bad Certificate and URL(SSL)"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
// Need LDAPS and try to remove CERT 		
			it('should return status 200 on POST change URL to ldaps:// with Certificate', function(done) {
				var payload = '{"LDAP":{"URL":"ldaps://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_SSLPORT +'","Certificate":"LDAP.pem","Overwrite":true}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldaps://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_SSLPORT +'","Certificate":"ldap.pem"}}' );
				FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
			});
			it('should return status 200 on GET change URL to ldaps:// with Certificate', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
//104897
			it('should return status 400 on POST remove "Certificate"', function(done) {
				var payload = '{"LDAP":{"Certificate":""}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldaps://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_SSLPORT +'","Certificate":"ldap.pem", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: Certificate Value: JSON_NULL."};
//				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type  is not valid. Object: LDAP Name: ldapconfig Property: Certificate Type: JSON_NULL."};
				verifyMessage = {"status":400,"Code": "CWLNA0449","Message": "The certificate for LDAP is not valid or does not exist."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET remove "Certificate"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
		
			it('should return status 200 on POST remove URL ldaps:// and Certificate', function(done) {
				var payload = '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 + ':' + FVT.LDAP_SERVER_1_PORT +'","Certificate":""}}';
				verifyConfig = JSON.parse(payload);
				FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
			});
			it('should return status 200 on GET remove URL ldaps:// and Certificate', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
//		
			it('should return status 400 on POST "LDAPS without a Certificate"', function(done) {
				var payload = '{"LDAP":{"URL":"ldaps://'+ FVT.LDAP_SERVER_1 + ':' + FVT.LDAP_SERVER_1_PORT +'","Certificate":null}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'","Certificate":"", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: Certificate Value: JSON_NULL."};
				verifyMessage = {"status":400,"Code": "CWLNA0449","Message": "The certificate for LDAP is not valid or does not exist."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET after POST fails "LDAPS without a Certificate"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
				
		});

		describe('IgnoreCase[T]:', function() {

			it('should return status 400 on POST "IgnoreCase":"False"', function(done) {
				var payload = '{"LDAP":{"IgnoreCase":"false"}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'","Certificate":"", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: IgnoreCase Value: JSON_STRING."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: IgnoreCase Type: JSON_STRING"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "IgnoreCase":"False"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			it('should return status 400 on POST "IgnoreCase":44', function(done) {
				var payload = '{"LDAP":{"IgnoreCase":44}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: IgnoreCase Value: JSON_INTEGER."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: IgnoreCase Type: JSON_INTEGER"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "IgnoreCase:44"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			it('should return status 400 on POST "IgnoreCase":""', function(done) {
				var payload = '{"LDAP":{"IgnoreCase":""}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: IgnoreCase Value: JSON_STRING."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: IgnoreCase Type: JSON_STRING"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "IgnoreCase":""', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

		});

		describe('EnableCache[T]:', function() {

			 it('should return status 400 when POST "EnableCache":"False"', function(done) {
				var payload = '{"LDAP":{"EnableCache":"False"}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: EnableCache Value: JSON_STRING."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: EnableCache Type: JSON_STRING"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "EnableCache":"False"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			 it('should return status 400 when POST "EnableCache":33', function(done) {
				var payload = '{"LDAP":{"EnableCache":33}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: EnableCache Value: JSON_INTEGER."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: EnableCache Type: JSON_INTEGER"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on get after POST "EnableCache":33', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

    		it('should return status 400 when POST "EnableCache":""', function(done) {
				var payload = '{"LDAP":{"EnableCache":""}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: EnableCache Value: JSON_STRING."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: EnableCache Type: JSON_STRING"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET after POST "EnableCache":""', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
		});

		describe('NestedGroupSearch[F]:', function() {

			it('should return status 400 on POST "NestedGroupSearch":"True"', function(done) {
				var payload = '{"LDAP":{"NestedGroupSearch":"true"}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: NestedGroupSearch Value: JSON_STRING."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: NestedGroupSearch Type: JSON_STRING"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "NestedGroupSearch":"True"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			it('should return status 400 on POST "NestedGroupSearch":22', function(done) {
				var payload = '{"LDAP":{"NestedGroupSearch":22}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: NestedGroupSearch Value: JSON_INTEGER."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: NestedGroupSearch Type: JSON_INTEGER"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "NestedGroupSearch":22', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			it('should return status 400 on POST "NestedGroupSearch":""', function(done) {
				var payload = '{"LDAP":{"NestedGroupSearch":""}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: NestedGroupSearch Value: JSON_STRING."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: NestedGroupSearch Type: JSON_STRING"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "NestedGroupSearch":""', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

		});


		describe('MaxConnections[1-10-100]:', function() {

			it('should return status 400 on POST "MaxConnections:less than MIN"', function(done) {
				var payload = '{"LDAP":{"MaxConnections":0}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
				verifyMessage = {"status":400,"Code": "CWLNA0112","Message": "The property value is not valid: Property: MaxConnections Value: \"0\"."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "MaxConnections:less than MIN"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			it('should return status 400 on POST "MaxConnections:more than MAX"', function(done) {
				var payload = '{"LDAP":{"MaxConnections":101}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
				verifyMessage = {"status":400,"Code": "CWLNA0112","Message": "The property value is not valid: Property: MaxConnections Value: \"101\"."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "MaxConnections:more than MAX"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});
// 104895
			it('should return status 400 on POST "MaxConnections:true', function(done) {
				var payload = '{"LDAP":{"MaxConnections":true}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: MaxConnections Value: JSON_TRUE."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: MaxConnections Type: JSON_TRUE"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "MaxConnections":true', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			it('should return status 400 on POST "MaxConnections":""', function(done) {
				var payload = '{"LDAP":{"MaxConnections":""}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: ldapconfig Property: MaxConnections Value: JSON_STRING."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: MaxConnections Type: JSON_STRING"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "MaxConnections":""', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

		});

		describe('Timeout[1-10-60]:', function() {

			it('should return status 400 on POST "Timeout:less than MIN"', function(done) {
				var payload = '{"LDAP":{"Timeout":0}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
				verifyMessage = {"status":400,"Code": "CWLNA0112","Message": "The property value is not valid: Property: Timeout Value: \"0\"."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "Timeout:less than MIN"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			it('should return status 400 on POST "Timeout:more than MAX"', function(done) {
				var payload = '{"LDAP":{"Timeout":61}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
				verifyMessage = {"status":400,"Code": "CWLNA0112","Message": "The property value is not valid: Property: Timeout Value: \"61\"."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "Timeout:more than MAX"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			it('should return status 400 on POST "Timeout:true"', function(done) {
				var payload = '{"LDAP":{"Timeout":true}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: Timeout Property: ldapconfig Value: JSON_TRUE."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: Timeout Type: JSON_TRUE"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "Timeout:true"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			it('should return status 400 on POST "Timeout":""', function(done) {
				var payload = '{"LDAP":{"Timeout":""}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//				verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: Timeout Property: ldapconfig Value: JSON_STRING."};
				verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: Timeout Type: JSON_STRING"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "Timeout":""', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

		});
		
		describe('CacheTimeout[1-10-60]:', function() {

			it('should return status 200 on POST "CacheTimeout:less than MIN"', function(done) {
				var payload = '{"LDAP":{"CacheTimeout":0}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
				verifyMessage = {"status":400,"Code": "CWLNA0112","Message": "The property value is not valid: Property: CacheTimeout Value: \"0\"."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "CacheTimeout:less than MIN"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			it('should return status 200 on POST "CacheTimeout:more than MAX"', function(done) {
				var payload = '{"LDAP":{"CacheTimeout":61}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
				verifyMessage = {"status":400,"Code": "CWLNA0112","Message": "The property value is not valid: Property: CacheTimeout Value: \"61\"."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "CacheTimeout:more than MAX"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			it('should return status 200 on POST "CacheTimeout:false"', function(done) {
				var payload = '{"LDAP":{"CacheTimeout":false}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//                verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: CacheTimeout Property: ldapconfig Value: JSON_FALSE."};
                verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: CacheTimeout Type: JSON_FALSE"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "CacheTimeout:false"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			it('should return status 200 on POST "CacheTimeout":""', function(done) {
				var payload = '{"LDAP":{"CacheTimeout":""}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//                verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: CacheTimeout Property: ldapconfig Value: JSON_STRING."};
                verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: CacheTimeout Type: JSON_STRING"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "CacheTimeout":""', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

		});
		
		describe('GroupCacheTimeout[1-300-86400]:', function() {

			it('should return status 200 on POST "GroupCacheTimeout:less than MIN"', function(done) {
				var payload = '{"LDAP":{"GroupCacheTimeout":0}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
				verifyMessage = {"status":400,"Code": "CWLNA0112","Message": "The property value is not valid: Property: GroupCacheTimeout Value: \"0\"."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "GroupCacheTimeout:less than MIN"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			it('should return status 200 on POST "GroupCacheTimeout:more than MAX"', function(done) {
				var payload = '{"LDAP":{"GroupCacheTimeout":86401}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
				verifyMessage = {"status":400,"Code": "CWLNA0112","Message": "The property value is not valid: Property: GroupCacheTimeout Value: \"86401\"."};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "GroupCacheTimeout:more than MAX"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			it('should return status 200 on POST "GroupCacheTimeout:true"', function(done) {
				var payload = '{"LDAP":{"GroupCacheTimeout":true}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//                verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: GroupCacheTimeout Property: ldapconfig Value: JSON_TRUE."};
                verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: GroupCacheTimeout Type: JSON_TRUE"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "GroupCacheTimeout:true"', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

			it('should return status 200 on POST "GroupCacheTimeout":""', function(done) {
				var payload = '{"LDAP":{"GroupCacheTimeout":""}}';
				verifyConfig = JSON.parse( '{"LDAP":{"URL":"ldap://'+ FVT.LDAP_SERVER_1 +':'+ FVT.LDAP_SERVER_1_PORT +'", "BaseDN":"ou=SVT,o=IBM,c=US", "BindDN":"cn=root,o=IBM,c=US", "BindPassword":"XXXXXX", "UserSuffix":"ou=users,ou=SVT,o=IBM,c=US", "GroupSuffix":"ou=groups,ou=SVT,o=IBM,c=US", "UserIdMap":"uid", "GroupIdMap":"cn", "GroupMemberIdMap":"member", "Verify":false}} ');
//                verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: GroupCacheTimeout Property: ldapconfig Value: JSON_STRING."};
//                verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: LDAP Name: GroupCacheTimeout Property: ldapconfig Value: JSON_NULL."};
                verifyMessage = {"status":400,"Code": "CWLNA0127","Message": "The property type is not valid. Object: LDAP Name: ldapconfig Property: GroupCacheTimeout Type: JSON_STRING"};
				FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
			});
			it('should return status 200 on GET "GroupCacheTimeout":""', function(done) {
				FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
			});

		});


    });
    
    //  ====================  Delete test cases  ====================  //
    describe('Delete:', function() {

    // CAN NOT DELETE LDAP 9/3/15

        it('should return status 400 when POST "LDAP":null to delete(remove config)', function(done) {
            var payload = '{"LDAP":null}';
            verifyConfig = JSON.parse( configuredLDAP );
// 93850            verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: {\"LDAP\":null} is not valid."};
// NOW after validation added I get:
//            verifyMessage = {"status":400,"Code":"CWLNA0371","Message":"Delete is not allowed for LDAP object."};
//			verifyMessage = {"status":400,"Code":"CWLNA0372","Message":"Delete is not allowed for LDAP object."};
//			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The RESTful request: \"LDAP\":null is not valid."};
			verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: \"LDAP\":null is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get after POST "LDAP":null', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
//117907        
        it('should return status 400 when POST "LDAP":"" ', function(done) {
            var payload = '{"LDAP":""}';
            verifyConfig = JSON.parse( configuredLDAP );
// 93850      verifyMessage = {"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
//            verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: {\"LDAP\":null} is not valid."};
// NOW after validation added I get:
//            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: URL Value: null."};
// and now....			
//            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: GroupSuffix Value: null."};
// or
//            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: BaseDN Value: null."};
//            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: GroupSuffix Value: null."};
//            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: URL Value: null."};
//            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: BaseDN Value: null."};
//            verifyMessage = {"Code":"CWLNA0137","Message":"The REST API call: \"LDAP\":null is not valid."};
            verifyMessage = {"Code":"CWLNA0137","Message":"The REST API call: \"LDAP\":\"\" is not valid."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get after POST "LDAP":"" ', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// 93850
        it('should return status 200 when POST "LDAP":{} (was NO_OP now Clears Most Config)', function(done) {
            var payload = '{"LDAP":{}}';
            verifyConfig = JSON.parse( configuredLDAP );
            verifyMessage = {"status":200,"Code":"CWLNA0448","Message":"The LDAP configuration is gone."};
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on get after POST "LDAP":{} (NO_OP now)', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
// DELETE LDAP is no longer allowed 9/2/15
        it('should return status 403 on DELETE "LDAP"', function(done) {
//            verifyConfig = JSON.parse( expectDefault );
            verifyConfig = JSON.parse( configuredLDAP );
//            verifyMessage = {"status":400,"Code": "CWLNA0372","Message":"Delete is not allowed for LDAP object."};
            verifyMessage = {"status":403,"Code": "CWLNA0372","Message":"Delete is not allowed for LDAP object."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after DELETE, "LDAP".', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 403 on DELETE "LDAP/ldapconfig"', function(done) {
//            verifyConfig = JSON.parse( expectDefault );
            verifyConfig = JSON.parse( configuredLDAP );
            verifyMessage = {"status":403,"Code": "CWLNA0372","Message":"Delete is not allowed for LDAP object."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+"ldapconfig", FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after DELETE, "LDAP/ldapconfig".', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
                

        it('should return status 200 on POST "LDAP" cleanup', function(done) {
            payload =  expectDefault ;
            verifyConfig = JSON.parse( expectFinalDefault );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after POST cleanup of "LDAP".', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
        it('should return status 200 on POST restart server after cleanup of "LDAP"', function(done) {
            var payload = '{"Service":"Server"}';
            var verifyPayload = JSON.parse( payload ) ;
            verifyMessage = {"status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully.","text":null} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyPayload, verifyMessage,  done);
        });
        it('should return status 200 on GET ldap config', function(done) {
        	this.timeout(FVT.REBOOT + 5000);
        	FVT.sleep(FVT.REBOOT);
        	verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
                
    });


    
});
