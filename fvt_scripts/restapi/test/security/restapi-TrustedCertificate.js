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

var FVT = require('../restapi-CommonFunctions.js')

CertCount = 100 ;
MaxCertCount = CertCount + 100 ;
//115421 temporary workaround
CertCount = 70 ;
MaxCertCount = CertCount + 70 ;


var verifyConfig = {} ;
var verifyMessage = {};

var uriObject = 'TrustedCertificate/' ;
var expectDefault = '{"TrustedCertificate":[],"Version": "'+ FVT.version +'"}' ;
var expectPopulated = '{"Version": "'+ FVT.version +'","TrustedCertificate": [ \
    { "SecurityProfileName": "aSecProf'+ (CertCount + CertCount) +'", "TrustedCertificate": "aCACertKey'+ (CertCount + CertCount - 1 ) +'" }, \
    { "SecurityProfileName": "aSecProf'+ (CertCount + CertCount) +'", "TrustedCertificate": "aCACertKey'+ (CertCount + 2 ) +'" }, \
    { "SecurityProfileName": "aSecProf'+ (CertCount + CertCount) +'", "TrustedCertificate": "aCACertKey'+ (CertCount + 1 ) +'" }, \
    { "SecurityProfileName": "aSecProf'+ (CertCount + CertCount) +'", "TrustedCertificate": "aCACertKey'+ CertCount +'" }, \
    { "SecurityProfileName": "aSecProf'+ (CertCount - 1 ) +'", "TrustedCertificate": "aCACertKey'+ (CertCount - 1 ) +'" }, \
    { "SecurityProfileName": "aSecProf2", "TrustedCertificate": "aCACertKey2" }, \
    { "SecurityProfileName": "aSecProf1", "TrustedCertificate": "aCACertKey1" }, \
    { "SecurityProfileName": "aSecProf0", "TrustedCertificate": "aCACertKey0" }, \
    { "SecurityProfileName": "'+ FVT.long32Name +'", "TrustedCertificate": "'+ FVT.maxFileName255 +'" }, \
    { "SecurityProfileName": "TestSecProf", "TrustedCertificate": "TestCACertKey" } ]}' ;

//---------------------------------------------
//  MakeProfiles
//---------------------------------------------
function makeProfiles( i ){

        describe('Prereq Certs, Keys CertProfile and SecurityProfile #'+i, function() {

            it('should return status 200 on PUT "aCACertKey'+i+'"', function(done) {
                sourceFile = '../common/imaCA-crt.pem';
                targetFile = 'aCACertKey' + i;
                FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
            });
        
            it('should return status 200 on PUT "aCert'+ i +'.pem"', function(done) {
                sourceFile = 'certFree.pem';
                targetFile = 'aCert' + i + '.pem';
                FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
            });

            it('should return status 200 on PUT "aKey'+ i +'.pem"', function(done) {
                sourceFile = 'keyFree.pem';
                targetFile = 'aKey'+ i +'.pem';
                FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
            });
        
            it('should return status 200 on post "aCertProf'+ i +'"', function(done) {
                var payload = '{"CertificateProfile":{"aCertProf'+ i +'":{"Certificate": "aCert'+ i +'.pem","Key": "aKey'+ i +'.pem"}}}';
                verifyConfig = JSON.parse(payload);
                FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
            });
            it('should return status 200 on GET, "aCertProf'+ i +'"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/aCertProf'+ i, FVT.getSuccessCallback, verifyConfig, done);
            });

            it('should return status 200 on post "aSecProf'+ i +'"', function(done) {
                var payload = '{"SecurityProfile":{"aSecProf'+ i +'":{"CertificateProfile": "aCertProf'+ i +'"}}}';
                verifyConfig = JSON.parse(payload);
                FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
            });
            it('should return status 200 on GET, "aSecProf'+ i +'"', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+'SecurityProfile/aSecProf'+ i, FVT.getSuccessCallback, verifyConfig, done);
            });

        });
};

//---------------------------------------------
//  MakeTrustedCertificates
//---------------------------------------------
function makeTrustedCertificates( i ){

        describe('TrustedCertificate #'+i, function() {
        
            it('should return status 200 on POST "TrustedCert" #'+i, function(done) {
                var payload = '{"TrustedCertificate":[{"SecurityProfileName":"aSecProf'+ i +'","TrustedCertificate": "aCACertKey'+ i +'"}]}';
                verifyConfig = JSON.parse(payload);
                FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
            });
            it('should return status 200 on GET "TrustedCert" #'+i, function(done) {
// PUT THIS BACK IN!!!
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'aSecProf'+ i +'/aCACertKey'+ i , FVT.getSuccessCallback, verifyConfig, done);
// PUT THIS BACK IN!!!  */
//                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject , FVT.getSuccessCallback, verifyConfig, done);
            });
        
        });
        
};


//---------------------------------------------
//  MakeBigTrustedCertificates - all TrustedCerts in one SecurityProfile
//---------------------------------------------
function makeBigTrustedCertificates( sp, i ){

        describe('TrustedCertificate #'+i, function() {

            it('should return status 200 on PUT "aCACertKey'+i+'"', function(done) {
                sourceFile = '../common/imaCA-crt.pem';
                targetFile = 'aCACertKey' + i;
                FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
            });

            it('should return status 200 on POST "TrustedCert" #'+i, function(done) {
                var payload = '{"TrustedCertificate":[{"SecurityProfileName":"aSecProf'+ sp +'","TrustedCertificate": "aCACertKey'+ i +'"}]}';
                verifyConfig = JSON.parse(payload);
                FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
            });
            it('should return status 200 on GET "TrustedCert" #'+i, function(done) {
// PUT THIS BACK IN!!!
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'aSecProf'+ sp +'/aCACertKey'+ i , FVT.getSuccessCallback, verifyConfig, done);
// PUT THIS BACK IN!!!  */
//                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject , FVT.getSuccessCallback, verifyConfig, done);
            });
        
        });
        
};

//---------------------------------------------
//  DeleteTrustedCertificates
//---------------------------------------------
function deleteTrustedCertificates( i ){

        describe('Delete TrustedCertificate #'+ i, function() {
        
            it('should return status 200 on DELETE TrustedCertificate TrustCert'+i, function(done) {
                verifyConfig = JSON.parse( '{"TrustedCertificate":[{"SecurityProfileName":"aSecProf'+ i +'","TrustedCertificate":"aCACertKey'+i+'"}]}' );
                verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
                FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+ 'aSecProf'+ i +'/aCACertKey'+ i , FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
            });
            it('should return status 200 on GET after DELETE aTrustCert'+ i +' gone', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject , FVT.getVerifyDeleteCallback, verifyConfig, done);
            });
            it('should return status 404 on GET after DELETE aTrustCert'+ i +' not found', function(done) {
//                verifyConfig = JSON.parse( '{"status":404,"TrustedCertificate":[{"SecurityProfileName":"aSecProf'+ i +'","TrustedCertificate":"aCACertKey'+i+'"}]}' );
                verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TrustedCertificate Name: aSecProf'+ i +'/aCACertKey'+ i +'","TrustedCertificate":[{"SecurityProfileName":"aSecProf'+ i +'","TrustedCertificate":"aCACertKey'+i+'"}]}' );
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'aSecProf'+ i +'/aCACertKey'+ i , FVT.verify404NotFound, verifyConfig, done );
            });
        
        });
        

};

//---------------------------------------------
//  DeleteBigTrustedCertificates
//---------------------------------------------
function deleteBigTrustedCertificates( sp, i ) {

        describe('Delete BigTrustedCertificate #'+ i, function() {
        
            it('should return status 200 on DELETE TrustedCertificate TrustCert'+i, function(done) {
                verifyConfig = JSON.parse( '{"TrustedCertificate":[{"SecurityProfileName":"aSecProf'+ sp +'","TrustedCertificate":"aCACertKey'+i+'"}]}' );
                verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
                FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+ 'aSecProf'+ sp +'/aCACertKey'+ i , FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
            });
            it('should return status 200 on GET after DELETE aTrustCert'+ i +' gone', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject , FVT.getVerifyDeleteCallback, verifyConfig, done);
            });
            it('should return status 404 on GET after DELETE aTrustCert'+ i +' not found', function(done) {
//                verifyConfig = JSON.parse( '{"status":404,"TrustedCertificate":[{"SecurityProfileName":"aSecProf'+ sp +'","TrustedCertificate":"aCACertKey'+i+'"}]}' );
                verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TrustedCertificate Name: aSecProf'+ sp +'/aCACertKey'+i+'","TrustedCertificate":[{"SecurityProfileName":"aSecProf'+ sp +'","TrustedCertificate":"aCACertKey'+i+'"}]}' );
                FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'aSecProf'+ sp +'/aCACertKey'+ i , FVT.verify404NotFound, verifyConfig, done );
            });
        
        });
        

};

//---------------------------------------------
//  DeleteProfiles
//---------------------------------------------
function deleteProfiles( i ){

            describe( 'Delete SecurityProfile and CertificateProfile #'+ i, function() {

                it('should return status 200 when deleting "aSecProf' + i + '"', function(done) {
                    verifyConfig = JSON.parse( '{"SecurityProfileName":{"aSecProf' + i + '":null}}' );
                    verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
                    FVT.makeDeleteRequest( FVT.uriConfigDomain+'SecurityProfile/aSecProf'+ i, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
                });
                it('should return status 404 on GET after delete, "SecProf' + i + '" not found', function(done) {
//                    verifyConfig = JSON.parse( '{"status":404,"SecurityProfileName":{"aSecProf' + i + '":null}}' );
                    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: aSecProf'+ i +'","SecurityProfileName":{"aSecProf' + i + '":null}}' );
                    FVT.makeGetRequest( FVT.uriConfigDomain+'SecurityProfile/aSecProf' + i, FVT.verify404NotFound, verifyConfig, done );
                });

                it('should return status 200 when deleting "aCertProf' + i + '"', function(done) {
                    verifyConfig = JSON.parse( '{"CertificateProfile":{"aCertProf' + i + '":null}}' );
                    verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
                    FVT.makeDeleteRequest( FVT.uriConfigDomain+'CertificateProfile/aCertProf' + i, FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
                });
                it('should return status 404 on GET after delete, "aCertProf' + i + '" not found', function(done) {
//                    verifyConfig = JSON.parse( '{"status":404,"CertificateProfile":{"aCertProf' + i + '":null}}' );
                    verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: aCertProf'+i+'","CertificateProfile":{"aCertProf' + i + '":null}}' );
                    FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/aCertProf' + i, FVT.verify404NotFound, verifyConfig, done );
                });
            
            });

};


//  ====================  Test Cases Start Here  ====================  //

describe('TrustedCertificate:', function() {
this.timeout( FVT.defaultTimeout + 30000 );

    // Get test cases
    describe('Pristine Get:', function() {
        it('should return status 200 on GET DEFAULT "TrustedCert":[] ', function(done) {
            verifyConfig = JSON.parse(expectDefault);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
    
    });

//  ====================   Create - Add - Update test cases  ====================  //
    describe('Prereq Cert, Key, TestCertProfile and TestSecProfile:', function() {

        it('should return status 200 on PUT "TestCACertKey"', function(done) {
            sourceFile = '../common/imaCA-crt.pem';
            targetFile = 'TestCACertKey';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "TestCert.pem"', function(done) {
            sourceFile = 'certFree.pem';
            targetFile = 'TestCert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "TestKey.pem"', function(done) {
            sourceFile = 'keyFree.pem';
            targetFile = 'TestKey.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
    
        it('should return status 200 on post "TestCertProf"', function(done) {
            var payload = '{"CertificateProfile":{"TestCertProf":{"Certificate": "TestCert.pem","Key": "TestKey.pem"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "TestCertProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/TestCertProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on post "TestSecProf"', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"CertificateProfile": "TestCertProf", "UsePasswordAuthentication": false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
//  115421
//UNEXPECTED END OF INPUT:  refine to only get ONE profile, not all profiles
        it('should return status 200 on GET, "TestSecProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+"SecurityProfile/TestSecProf", FVT.getSuccessCallback, verifyConfig, done);
        });

    });

//  ====================   Create - Add - Update test cases  ====================  //
    describe('Prereq Cert and Keys for MAX LENGTH Name CertProfile and SecProfile:', function() {

        it('should return status 200 on PUT MAX Length Name "PublicKey"', function(done) {
            sourceFile = '../common/imaCA-crt.pem';
            targetFile = FVT.maxFileName255;
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "MaxCert.pem"', function(done) {
            sourceFile = 'certFree.pem';
            targetFile = 'C' + FVT.long254Name ;
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on PUT "MaxKey.pem"', function(done) {
            sourceFile = 'keyFree.pem';
            targetFile = 'K' + FVT.long254Name;
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
    
        it('should return status 200 on POST MaxLengthName CertProf ', function(done) {
            var payload = '{"CertificateProfile":{"'+ FVT.long256Name +'":{"Certificate": "C' + FVT.long254Name +'","Key":"K' + FVT.long254Name +'"}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MaxLengthName CertProf', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/'+ FVT.long256Name +'', FVT.getSuccessCallback, verifyConfig, done);
        });
// 102445
        it('should return status 200 on POST  MaxLengthName SecProf', function(done) {
            var payload = '{"SecurityProfile":{"'+ FVT.long32Name +'":{"CertificateProfile": "'+ FVT.long256Name +'", "UsePasswordAuthentication": false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET  MaxLengthName SecProf', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+ 'SecurityProfile', FVT.getSuccessCallback, verifyConfig, done);
        });
        
    });

//  ====================   Create - Add - Update test cases  ====================  //

    for ( i = 0 ; i < CertCount ; i++ ) {
        makeProfiles( i );
    };
    
    makeProfiles( MaxCertCount );


    describe('Test TrustedCertificate:', function() {

        it('should return status 200 on POST Test "TrustCertificate"', function(done) {
            var payload = '{"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate": "TestCACertKey"}]}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET TrustedCert "TestCertProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestSecProf/TestCACertKey', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on POST MAX NAME "TrustedCert"', function(done) {
            var payload = '{"TrustedCertificate":[{"SecurityProfileName":"'+ FVT.long32Name +'","TrustedCertificate": "'+ FVT.maxFileName255 +'"}]}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET MAX NAME TrustedCert ', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ FVT.long32Name+'/'+FVT.maxFileName255, FVT.getSuccessCallback, verifyConfig, done);
        });

    });

//  ====================  Overwrite test cases  ====================  //
    describe('Overwrite:', function() {
// TODO :  DELETE and ADD back with SAME NAME, should NOT need OVERWRITE
// 114468
// Update an existing TrustedCert's entry in SecProfile requires OVERWRITE
        it('should return status 200 on PUT "TestCACertKey"', function(done) {
            sourceFile = '../common/imaCA-crt.pem';
            targetFile = 'TestCACertKey';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
        it('should return status 400 on rePOST Test "TrustCertificate" without overwrite', function(done) {
            var payload = '{"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate": "TestCACertKey"}]}';
            verifyConfig = JSON.parse(payload);
            verifyMessage = {"status":400,"Code":"CWLNA6186","Message":"The certificate already exists. Set Overwrite to true to replace the existing certificate." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on rePOST Test "TrustCertificate" with Overwrite:False in wrong place 1', function(done) {
            var payload = {"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate": "TestCACertKey" }],"Overwrite":true};
            verifyConfig = payload ;
            verifyMessage = {"status":404,"Code":"CWLNA0111","Message":"The property name is not valid: Property: Overwrite." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 400 on rePOST Test "TrustCertificate" with Overwrite:False in wrong place 2', function(done) {
            var payload = {"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate": "TestCACertKey"},{"Overwrite":true} ]};
            verifyConfig = payload ;
            verifyMessage = {"status":400,"Code":"CWLNA6186","Message":"The certificate already exists. Set Overwrite to true to replace the existing certificate." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on rePOST Test "TrustCertificate" with Overwrite', function(done) {
            var payload = '{"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate": "TestCACertKey","Overwrite":true}]}';
            verifyConfig = {"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate": "TestCACertKey"}]} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET reposted TrustedCert "TestCertProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestSecProf/TestCACertKey', FVT.getSuccessCallback, verifyConfig, done);
        });
// Delete and RE ADD, Overwrite should not be needed.
        it('should return status 200 on DELETE to reADD TrustedCertificate Test TrustCert', function(done) {
            verifyConfig = JSON.parse( '{"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate":"TestCACertKey"}]}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject + 'TestSecProf/TestCACertKey', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on GET after DELETE to reADD Test TrustCert gone', function(done) {
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestSecProf/TestCACertKey', FVT.getVerifyDeleteCallback, verifyConfig, done);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on GET after DELETE to reADD Test TrustCert not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TrustedCertificate":[{"SecurityProfileName":"TestSecProfile","TrustedCertificate":"TestCACertKey"}]}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TrustedCertificate Name: TestSecProf/TestCACertKey","TrustedCertificate":[{"SecurityProfileName":"TestSecProfile","TrustedCertificate":"TestCACertKey"}]}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestSecProf/TestCACertKey', FVT.verify404NotFound, verifyConfig, done );
        });

        it('should return status 200 on PUT "TestCACertKey"', function(done) {
            sourceFile = '../common/imaCA-crt.pem';
            targetFile = 'TestCACertKey';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on POST Test "TrustCertificate" after deleted', function(done) {
            var payload = '{"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate": "TestCACertKey"}]}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET TrustedCert "TestCertProf" after deleted', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestSecProf/TestCACertKey', FVT.getSuccessCallback, verifyConfig, done);
        });

    });
    
    

    for ( i=0; i < CertCount ; i++ ) {
        makeTrustedCertificates( i );
    };
    
    for ( i=CertCount; i < MaxCertCount ; i++ ) {
        makeBigTrustedCertificates( MaxCertCount, i );
    };


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
        it('should return status 200 on get, "Standby" persists', function(done) {
            verifyConfig = JSON.parse( expectPopulated ) ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done)
        });
        
    });


//  ====================  Error test cases  ====================  //
describe('Error:', function() {
    describe('General:', function() {

        it('should return status 400 when trying to POST Delete All TrustedCertificates (Bad Form)', function(done) {
            var payload = '{"TrustedCertificate":null}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"Code":"CWLNA0108","Message":"A null object is not allowed."} ;
//            verifyMessage = {"Code":"CWLNA0137","Message":"The RESTful request: \"TrustedCertificate\":null is not valid."} ;
//            verifyMessage = {"Code":"CWLNA0137","Message":"The REST API: \"TrustedCertificate\":null is not valid."} ;
            verifyMessage = {"Code":"CWLNA0137","Message":"The REST API call: \"TrustedCertificate\":null is not valid."} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after PORT Delete All TrustedCertificates (Bad Form)', function(done) {
            verifyConfig = JSON.parse( expectPopulated );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//  120951, 121090
        it('should return status 400 on DELETE without complete URI', function(done) {
            verifyConfig = JSON.parse( '{"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate": "TestCACertKey"}]}' );
//            verifyMessage = {"status":400,"Code": "CWLNA8293","Message": "The TrustedCertificate and SecurityProfileName are needed for the operation.\n"};
//            verifyMessage = {"status":400,"Code": "CWLNA8293","Message": "The TrustedCertificate and SecurityProfileName are needed for the operation."};
//            verifyMessage = {"status":400,"Code": "CWLNA6167","Message": "The SecurityProfileName and TrustedCertificate are needed for the operation."};
//            verifyMessage = {"status":400,"Code": "CWLNA6167","Message": "The SecurityProfileName and TrustedCertificate parameters are needed in the payload for the REST call."};
            verifyMessage = {"status":400,"Code": "CWLNA0108","Message": "A null object is not allowed."};
            verifyMessage = {"status":400,"Code": "CWLNA6167","Message": "The SecurityProfileName and TrustedCertificate parameters are needed for the REST call."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on GET after DELETE without complete URI fails', function(done) {
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestSecProf/TestCACertKey', FVT.getSuccessCallback, verifyConfig, done);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    });
    
    describe('Name:', function() {

        it('should return status 404 POST when SecProf Name too long/not found', function(done) {
            var payload = '{"TrustedCertificate":[{"SecurityProfileName":"'+ FVT.long33Name +'","TrustedCertificate": "TestCACertKey"}]}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0136","Message":"The item or object is not found. Type: SecurityProfileName Name: '+ FVT.long33Name +'"}' );
//            verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object is not found. Type: SecurityProfileName Name: '+ FVT.long33Name +'"}' );
            verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfileName Name: '+ FVT.long33Name +'"}' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST failed with SecProf Name too long/not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TrustedCertificate":[{"SecurityProfileName":"'+ FVT.long33Name +'","TrustedCertificate": "TestCertProf"}]}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TrustedCertificate Name: '+ FVT.long33Name +'/TestCertProf","TrustedCertificate":[{"SecurityProfileName":"'+ FVT.long33Name +'","TrustedCertificate": "TestCertProf"}]}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ FVT.long33Name +'/TestCertProf', FVT.verify404NotFound, verifyConfig, done);
        });


        it('should return status 400 POST when CertificateProf Name too long/not found', function(done) {
            var payload = '{"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate": "'+ FVT.long257Name +'"}]}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: Name Name: '+ FVT.long33Name +'."}' );
//            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0112","Message":"The property value is not valid: Property: TrustedCertificate Value: \\\"'+ FVT.long257Name +'\\\"."}' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0144","Message":"The value that is specified for the property on the configuration object is too long. Object: TrustedCertificate Property: TrustedCertificate Value: '+ FVT.long257Name +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after POST failed with CertProf Name too long/not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate": "'+ FVT.long257Name +'"}]}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TrustedCertificate Name: TestSecProf/'+ FVT.long257Name +'","TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate": "'+ FVT.long257Name +'"}]}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestSecProf/' + FVT.long257Name, FVT.verify404NotFound, verifyConfig, done);
        });
        
    });

    describe('TrustedCertificate:', function() {

        it('should return status 400 when NoOP TrustedCertificate set ', function(done) {
            var payload = '{"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate":{}}]}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0108","Message":"A null object is not allowed." };
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: TrustedCertificate Name: null Property: TrustedCertificate Type: JSON_OBJECT" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on GET NO CHANGE after POST NOOP TrustedCertificate', function(done) {
//            verifyConfig = {"status":404,"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate":{}}]} ;
//            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TrustedCertificate Name: null","TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate":{}}]} ;
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestSecProf', FVT.verify404NotFound, verifyConfig, done);
            verifyConfig = {"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate":"TestCACertKey"}]} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestSecProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when No TrustedCertificate set ', function(done) {
            var payload = '{"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate":null}]}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0108","Message":"A null object is not allowed." };
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: TrustedCertificate Name: null Property: TrustedCertificate Type: JSON_NULL" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on GET after POST TrustedCertificate key not set', function(done) {
//            verifyConfig = {"status":404,"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate":null}]};
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TrustedCertificate Name: TestSecProf/null","TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate":null}]} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestSecProf/null', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 when TrustedCertificate Key not provided ', function(done) {
            var payload = '{"TrustedCertificate":[{"SecurityProfileName":"TestSecProf"}]}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0108","Message":"A null object is not allowed." };
            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: TrustedCertificate Value: null." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on GET NO CHANGE after POST when Cert Key not provided', function(done) {
//            verifyConfig = {"status":404,"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate":null}]};
//            verifyConfig = {"status":200,"Code":null,"Message":null,"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate":null}]} ;
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestSecProf/', FVT.verify404NotFound, verifyConfig, done);
            verifyConfig = {"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate":"TestCACertKey"}]} ;
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestSecProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 404 when TrustedCertificate Key is not found', function(done) {
            var payload = '{"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate": "LostInTheEther"}]}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA8278","Message":"Cannot find the TrustedCertificate file specified. Make sure it has been imported.\n" };
//            verifyMessage = {"status":400,"Code":"CWLNA0136","Message":"The item or object is not found. Type: TrustedCertificate Name: LostInTheEther" };
//            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object is not found. Type: TrustedCertificate Name: LostInTheEther" };
            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TrustedCertificate Name: LostInTheEther" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on GET TrustedCertificate Key Not Found', function(done) {
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TrustedCertificate Name: TestSecProf/LostInTheEther","TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate":"LostInTheEther"}]};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestSecProf/LostInTheEther', FVT.verify404NotFound, verifyConfig, done);
        });

    });

    describe('SecurityProfile:', function() {

        it('should return status 400 on POST when NOOP SecProf set ', function(done) {
            var payload = '{"TrustedCertificate":[{"TrustedCertificate":"TestCACertKey","SecurityProfileName":{}}]}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0108","Message":"A null object is not allowed." };
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: TrustedCertificate Name: null Property: SecurityProfileName Type: JSON_OBJECT" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on GET after POST NOOP SecProf', function(done) {
//            verifyConfig = {"status":404,"TrustedCertificate":[{"TrustedCertificate":"TestCACertKey","SecurityProfileName":{}}]} ;
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TrustedCertificate Name: TestCACertKey","TrustedCertificate":[{"SecurityProfileName":{},"TrustedCertificate":"TestCACertKey"}]};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestCACertKey', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST when No SecProf set ', function(done) {
            var payload = '{"TrustedCertificate":[{"TrustedCertificate":"TestCACertKey","SecurityProfileName":null}]}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0108","Message":"A null object is not allowed." };
            verifyMessage = {"status":400,"Code":"CWLNA0127","Message":"The property type is not valid. Object: TrustedCertificate Name: null Property: SecurityProfileName Type: JSON_NULL" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on GET after POST SecProf not set', function(done) {
//            verifyConfig = {"status":404,"TrustedCertificate":[{"TrustedCertificate":"TestCACertKey","SecurityProfileName":null}]};
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TrustedCertificate Name: null/TestCACertKey","TrustedCertificate":[{"SecurityProfileName":null,"TrustedCertificate":"TestCACertKey"}]};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'null/TestCACertKey', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 400 on POST when SecProf not provided ', function(done) {
            var payload = '{"TrustedCertificate":[{"TrustedCertificate":"TestCACertKey"}]}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0108","Message":"A null object is not allowed." };
            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: SecurityProfile Value: null." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on GET after POST when SecProf not provided', function(done) {
//            verifyConfig = {"status":404,"TrustedCertificate":[{"TrustedCertificate":"TestCACertKey","SecurityProfileName":null}]};
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TrustedCertificate Name: TestCACertKey","TrustedCertificate":[{"SecurityProfileName":null,"TrustedCertificate":"TestCACertKey"}]};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestCACertKey/', FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 404 on POST when SecProf is not found', function(done) {
            var payload = '{"TrustedCertificate":[{"TrustedCertificate":"TestCACertKey","SecurityProfileName": "LostInTheEther"}]}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = {"status":400,"Code":"CWLNA0136","Message":"The item or object is not found. Type: SecurityProfileName Name: LostInTheEther" };
//            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object is not found. Type: SecurityProfileName Name: LostInTheEther" };
            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfileName Name: LostInTheEther" };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 404 on GET SecProf Not Found', function(done) {
//            verifyConfig = {"status":404,"TrustedCertificate":[{"TrustedCertificate":"TestCACertKey","SecurityProfileName":"LostInTheEther"}]};
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TrustedCertificate Name: LostInTheEther/TestCACertKey","TrustedCertificate":[{"SecurityProfileName":null,"TrustedCertificate":"TestCACertKey"}]};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'LostInTheEther/TestCACertKey/', FVT.verify404NotFound, verifyConfig, done);
        });

    });


   
});




//  ====================  Delete test cases  ====================  //
    describe('Delete MaxLengthName and Test:', function() {
// 112800 - THIS IS GOING TO BE TRSed, will be code to delete all Trusted Certs before the SecurityPolicy is deleted so no to leave Orphans.
// 112800            it('should return status 400 when deleting MAX Length Name "SecProf" when IN USE by Trusted Certs', function(done) {
// 112800                verifyConfig = JSON.parse( '{"SecurityProfile":{"'+ FVT.long32Name +'":null}}' );
// 112800                verifyMessage = {"status":400,"Code": "CWLNA6011","Message": "The requested configuration change has failed, Profile in Use."};
// 112800                FVT.makeDeleteRequest( FVT.uriConfigDomain+'SecurityProfile/'+ FVT.long32Name , FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
// 112800            });
// 112800            it('should return status 200 on GET after delete, Max Length Name "SecProf" fails,  IS Found -- IN USE by Trusted Certs', function(done) {
// 112800                FVT.makeGetRequest( FVT.uriConfigDomain+'SecurityProfile/'+ FVT.long32Name , FVT.getSuccessCallback, verifyConfig, done );
// 112800            });
        
// 112793 :THE URI Is actually short here, missing the TCert name.  113608 - caused userfiles dir to be deleted...
        it('should return status 400 on DELETE ALL TrustedCertificate in "Max Length Name" SecProfile, when neglect to name TrustedCertificate on URI', function(done) {
            verifyConfig = JSON.parse( '{"TrustedCertificate":[{"SecurityProfileName":"'+ FVT.long32Name +'","TrustedCertificate":"'+ FVT.maxFileName255 +'"}]}' );
//            verifyMessage = {"status":400,"Code": "CWLNA0132","Message": "The property value is not valid. Object: TrustedCertificate Name: null Property: TrustedCertificate Value: \"null\"."};
            verifyMessage = {"status":400,"Code": "CWLNA6167","Message": "The SecurityProfileName and TrustedCertificate parameters are needed for the REST call."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+ FVT.long32Name , FVT.deleteFailureCallback, verifyConfig, verifyMessage, done );
        });
// 115421 (starts here)
        it('should return status 200 on GET after DELETE, "Max Length Name"SecProf when neglected to name TrustedCertificate on URI', function(done) {
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ FVT.long32Name + '/' + FVT.maxFileName255 , FVT.getVerifyDeleteCallback, verifyConfig, done);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject , FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET after DELETE, "Max Length Name"SecProf when neglected to name TrustedCertificate on URI', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ FVT.long32Name + '/' + FVT.maxFileName255, FVT.getSuccessCallback, verifyConfig, done );
        });
// probably what it should have been (TCert name in URL)
        it('should return status 200 when deleting TrustedCertificate "Max Length Name"', function(done) {
            verifyConfig = JSON.parse( '{"TrustedCertificate":[{"SecurityProfileName":"'+ FVT.long32Name +'","TrustedCertificate":"'+ FVT.maxFileName255 +'"}]}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+ FVT.long32Name +'/'+ FVT.maxFileName255 , FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on GET after delete, TrustedCertificate "Max Length Name"  gone', function(done) {
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ FVT.long32Name + '/' + FVT.maxFileName255 , FVT.getVerifyDeleteCallback, verifyConfig, done);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on GET after delete, TrustedCertificate "Max Length Name" not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TrustedCertificate":[{"SecurityProfileName":"'+ FVT.long32Name +'","TrustedCertificate":"'+ FVT.maxFileName255 +'"}]}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TrustedCertificate Name: '+ FVT.long32Name +'/'+ FVT.maxFileName255 +'","TrustedCertificate":[{"SecurityProfileName":"'+ FVT.long32Name +'","TrustedCertificate":"'+ FVT.maxFileName255 +'"}]}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ FVT.long32Name + '/' + FVT.maxFileName255, FVT.verify404NotFound, verifyConfig, done );
        });

        it('should return status 200 on DELETE TrustedCertificate Test TrustCert', function(done) {
            verifyConfig = JSON.parse( '{"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate":"TestCACertKey"}]}' );
            verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject + 'TestSecProf/TestCACertKey', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
        });
        it('should return status 200 on GET after DELETE Test TrustCert gone', function(done) {
//            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestSecProf/TestCACertKey', FVT.getVerifyDeleteCallback, verifyConfig, done);
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on GET after DELETE Test TrustCert not found', function(done) {
//            verifyConfig = JSON.parse( '{"status":404,"TrustedCertificate":[{"SecurityProfileName":"TestSecProfile","TrustedCertificate":"TestCACertKey"}]}' );
            verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: TrustedCertificate Name: TestSecProf/TestCACertKey","TrustedCertificate":[{"SecurityProfileName":"TestSecProfile","TrustedCertificate":"TestCACertKey"}]}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+ 'TestSecProf/TestCACertKey', FVT.verify404NotFound, verifyConfig, done );
        });

    });

    for ( i=0 ; i < CertCount ; i++ ) {
        deleteTrustedCertificates( i );
    };


    for ( i=CertCount ; i < MaxCertCount ; i++ ) {
        deleteBigTrustedCertificates( MaxCertCount, i );
    };

    // verify 113608 - userfiles dir would get deleted and could not upload files...
    describe('Verify still upload and create:', function() {

        it('should return status 200 on rePUT "TestCACertKey"', function(done) {
            sourceFile = '../common/imaCA-crt.pem';
            targetFile = 'TestCACertKey';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on rePUT "TestCert.pem"', function(done) {
            sourceFile = 'certFree.pem';
            targetFile = 'TestCert.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });

        it('should return status 200 on rePUT "TestKey.pem"', function(done) {
            sourceFile = 'keyFree.pem';
            targetFile = 'TestKey.pem';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
    
        it('should return status 200 on rePOST "TestCertProf"', function(done) {
            var payload = '{"CertificateProfile":{"TestCertProf":{"Certificate": "TestCert.pem","Key": "TestKey.pem","Overwrite":true}}}';
            verifyConfig = JSON.parse( '{"CertificateProfile":{"TestCertProf":{"Certificate": "TestCert.pem","Key": "TestKey.pem"}}}' );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "TestCertProf" with overwrite', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/TestCertProf', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on rePOST "TestSecProf"', function(done) {
            var payload = '{"SecurityProfile":{"TestSecProf":{"CertificateProfile": "TestCertProf", "UsePasswordAuthentication": false}}}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET, "TestSecProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+"SecurityProfile/TestSecProf", FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 200 on rePOST Test "TrustCertificate"', function(done) {
            var payload = '{"TrustedCertificate":[{"SecurityProfileName":"TestSecProf","TrustedCertificate": "TestCACertKey"}]}';
            verifyConfig = JSON.parse(payload);
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET TrustedCert "TestCertProf"', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'TestSecProf/TestCACertKey', FVT.getSuccessCallback, verifyConfig, done);
        });

    });    
    
//  ====================  Clean up PreReqs  ====================  //
    describe('Clean UP PreReqs:', function() {
        describe('Test Security and Certificate Profile:', function() {
// NOTE:  This delete now DELETEs the Trusted Certificates associated with this Sec.Profile    
            it('should return status 200 when deleting "TestSecProf"', function(done) {
                verifyConfig = JSON.parse( '{"SecurityProfile":{"TestSecProf":null}}' );
                verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
                FVT.makeDeleteRequest( FVT.uriConfigDomain+'SecurityProfile/TestSecProf', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
            });
//  115421
//UNEXPECTED END OF INPUT:  remove this test and rely on 404 not found
//            it('should return status 200 on GET all after DELETE "TestSecProf" gone', function(done) {
//                FVT.makeGetRequest( FVT.uriConfigDomain+'SecurityProfile/', FVT.getVerifyDeleteCallback, verifyConfig, done);
//            });
            it('should return status 404 on GET after delete, "TestSecProf" not found', function(done) {
//                verifyConfig = JSON.parse( '{"status":404,"SecurityProfile":{"TestSecProf":null}}' );
                verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: TestSecProf","SecurityProfile":{"TestSecProf":null}} ;
                FVT.makeGetRequest( FVT.uriConfigDomain+'SecurityProfile/TestSecProf', FVT.verify404NotFound, verifyConfig, done );
            });
        
            it('should return status 200 when deleting "TestCertProf"', function(done) {
                verifyConfig = JSON.parse( '{"CertificateProfile":{"TestCertProf":null}}' );
                verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
                FVT.makeDeleteRequest( FVT.uriConfigDomain+'CertificateProfile/TestCertProf', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
            });
            it('should return status 200 on GET all after delete, "TestCertProf" gone', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/', FVT.getVerifyDeleteCallback, verifyConfig, done);
            });
            it('should return status 404 on GET after delete, "TestCertProf" not found', function(done) {
//                verifyConfig = JSON.parse( '{"status":404,"CertificateProfile":{"TestCertProf":null}}' );
                verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: TestCertProf","CertificateProfile":{"TestCertProf":null}} ;
                FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/TestCertProf', FVT.verify404NotFound, verifyConfig, done );
            });
        
        });

        describe('MaxLengthName Security and Certificate Profile:', function() {
    
            it('should return status 200 when deleting MAX Length Name "SecProf"', function(done) {
                verifyConfig = JSON.parse( '{"SecurityProfile":{"'+ FVT.long32Name +'":null}}' );
                verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
                FVT.makeDeleteRequest( FVT.uriConfigDomain + 'SecurityProfile/' + FVT.long32Name , FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
            });
//  115421
//UNEXPECTED END OF INPUT:  remove this test and rely on 404 not found
//            it('should return status 200 on GET all after DELETE Max Length Name "SecProf" gone', function(done) {
//                FVT.makeGetRequest( FVT.uriConfigDomain+'SecurityProfile/', FVT.getVerifyDeleteCallback, verifyConfig, done);
//            });
            it('should return status 404 on GET after delete, Max Length Name "SecProf" not found', function(done) {
//                verifyConfig = JSON.parse( '{"status":404,"SecurityProfile":{"'+ FVT.long32Name +'":null}}' );
                verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: SecurityProfile Name: '+ FVT.long32Name +'","SecurityProfile":{"'+ FVT.long32Name +'":null}}' );
                FVT.makeGetRequest( FVT.uriConfigDomain+'SecurityProfile/'+ FVT.long32Name , FVT.verify404NotFound, verifyConfig, done );
            });
        
            it('should return status 200 when deleting MaxLength "CertProf"', function(done) {
                verifyConfig = JSON.parse( '{"CertificateProfile":{"'+  FVT.long256Name +'":null}}' );
                verifyMessage = {"Status":200, "Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
                FVT.makeDeleteRequest( FVT.uriConfigDomain+'CertificateProfile/'+  FVT.long256Name , FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done );
            });
            it('should return status 200 on GET all after delete, MaxLength "CertProf" gone', function(done) {
                FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/', FVT.getVerifyDeleteCallback, verifyConfig, done);
            });
            it('should return status 404 on GET after delete, MaxLength "CertProf" not found', function(done) {
//                verifyConfig = JSON.parse( '{"status":404,"CertificateProfile":{"'+  FVT.long256Name +'":null}}' );
                verifyConfig = JSON.parse( '{"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: CertificateProfile Name: '+  FVT.long256Name +'","CertificateProfile":{"'+  FVT.long256Name +'":null}}' );
                FVT.makeGetRequest( FVT.uriConfigDomain+'CertificateProfile/'+  FVT.long256Name , FVT.verify404NotFound, verifyConfig, done );
            });

        });

        for ( i=0; i < CertCount ; i++ ) {
            deleteProfiles( i );
        };

        deleteProfiles( MaxCertCount );
    
    });

    
});
