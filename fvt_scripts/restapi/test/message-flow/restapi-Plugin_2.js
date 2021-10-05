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

var FVT = require('../restapi-CommonFunctions.js')
var verifyConfig = {};
var verifyMessage = {};  // {"status":#,"Code":"CWLxxx","Message":"blah blah"}

var uriObject = 'Plugin/';
var expectDefault = '{"Plugin":{},"Version": "' + FVT.version + '"}';


//  ====================  Test Cases Start Here  ====================  //
//"ListObjects":"Name"
// also; 
//    File[a jar/zip file that contains file plugin.json with config basics],
//    PropertiesFile[pluginProps.json  local config params for plugin]

describe('Plugin_2', function() {
this.timeout( FVT.defaultTimeout );

    describe('Edit Plugin:', function() {
/*  done by CURL command in framework
        it('should return status 200 on rePUT "jsonplugin.zip"', function(done) {
            sourceFile = FVT.M1_IMA_SDK +'/ImaTools/ImaPlugin/lib/jsonplugin.zip';
            targetFile = 'jsonplugin.zip';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
*/
        it('should return status 400 on rePOST same "Plugin" zip only without Overwrite', function(done) {
            var payload = '{"Plugin":{"json_msg":{"File":"jsonplugin.zip"}}}';
            verifyConfig = JSON.parse( FVT.json_msgPlugin );
//            verifyMessage = { "status":400, "Code":"CWLNA6170", "Message":"The file jsonplugin.zip already exists. Set Overwrite to true to replace the existing file." } ;
            verifyMessage = { "status":400, "Code":"CWLNA6171", "Message":"The File or PropertiesFile already exists. Set Overwrite to true to replace the existing file." } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET rePOST same "Plugin" zip without Overwrite', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on rePOST same "Plugin" zip only with Overwrite', function(done) {
            var payload = '{"Plugin":{"json_msg":{"File":"jsonplugin.zip", "Overwrite":true}}}';
            verifyConfig = JSON.parse( FVT.json_msgPlugin );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET rePOST same "Plugin" zip with Overwrite', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// HOW to Verify ZIP and Properties active or not - prior to restart. ?
// R.:  Once the PLUGIN is started, updates to the PropertiesFile is dynamic (automatically moved from staging to plugins)
//defect 129946
        it('should return status 400 on GET PropertiesFile of json_msg, not there yet', function(done) {
            verifyConfig = FVT.json_msgProperties ;  // guts of file NOT FOUND, not there yet.
//            verifyMessage = { "status":404, "Code":"CWLNA0136", "Message":"The item or object cannot be found. Type: Plugin Name: json_msg" } ;
            verifyMessage = { "status":400, "Code":"CWLNA6198", "Message":"The requested item \"Plugin/json_msg/PropertiesFile\" is not found." } ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"Plugin/json_msg/PropertiesFile", FVT.getFailureCallback, verifyMessage, done);
        });



// Restart #1
        it('should return status 200 on GET "Status" before restart #1 with Plugin', function(done) {
            verifyConfig = JSON.parse( FVT.expectPluginRunning  );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });
        
        it('should return status 400 on POST "restart with plugin mistyped"', function(done) {
            var payload = '{"Service":"plugin"}';
            verifyConfig = JSON.parse( FVT.expectPluginRunning ) ;
            verifyMessage = {"status":400,"Code":"CWLNA0134","Message":"The value specified for the required property is invalid or null. Property: Service Value: plugin."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postFailureCallback, verifyConfig, verifyMessage,  done);
        });

        it('should return status 200 on POST "restart #1 with Plugin"', function(done) {
            var payload = '{"Service":"Plugin"}';
            verifyConfig = JSON.parse( FVT.expectPluginRunning ) ;
            verifyMessage = {"Status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart #1 with Plugin', function(done) {
            this.timeout( FVT.RESETCONFIG  + 3000);
            FVT.sleep( FVT.RESETCONFIG );
            verifyConfig = JSON.parse( FVT.expectPluginRunning  );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on GET "Plugin" after restart #1', function(done) {
            verifyConfig = JSON.parse( FVT.json_msgPlugin );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

//defect 129946
        it('should return status 400 on GET PropertiesFile "Plugin":json_msg', function(done) {
            verifyConfig = FVT.json_msgProperties ;  // guts of file NOT FOUND, not there yet.
//            verifyMessage = { "status":404, "Code":"CWLNA0136", "Message":"The item or object cannot be found. Type: Plugin Name: json_msg" } ;
            verifyMessage = { "status":400, "Code":"CWLNA6198", "Message":"The requested item \"Plugin/json_msg/PropertiesFile\" is not found." } ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"Plugin/json_msg/PropertiesFile", FVT.getFailureCallback, verifyMessage, done);
        });
//defect 136504
        it('should return status 200 on POST add Properties File', function(done) {
            var payload = '{"Plugin":{"json_msg":{"PropertiesFile":"jsonplugin1.json"}}}';
            verifyConfig = JSON.parse( payload );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
// File shows up in config as changed, yet does not show contents until after restart (still in staging)!  R. say Properties are Dynamic Update... does not look like it.
        it('should return status 200 on GET add Properties File', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
//says it can not find, but it has been put by RUN_CLI, will just upload it again
        it('should return status 200 on PUT "jsonplugin.json"', function(done) {
            sourceFile = '../plugin_tests/jsonplugin.json';
            targetFile = 'jsonplugin.json';
            FVT.makePutRequest( FVT.uriFileDomain+targetFile, sourceFile, FVT.putSuccessCallback, done);
        });
// 140253 - Can't have two outstanding PropertiesFile Updates pending in STAGING
        it('should return status 400 on POST overwrite Properties File', function(done) {
            var payload = '{"Plugin":{"json_msg":{"PropertiesFile":"jsonplugin.json", "Overwrite":true }}}';
            verifyConfig = JSON.parse( FVT.json_msgPluginProperty1 );
//            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
//            verifyMessage = { "status":400, "Code":"CWLNA0378", "Message":"The plug-in update failed. The error is: Failed to copy plugin(staging) directory in temporary area.\njava.nio.file.FileAlreadyExistsException: /tmp/plugin/json_msg\n\tat sun.nio.fs.UnixCopyFile.copy(UnixCopyFile.java:563)\n\tat sun.nio.fs.UnixFileSystemProvider.copy(UnixFileSystemProvider.java:265)\n\tat java.nio.file.Files.copy(Files.java:1285)\n\tat com.ibm.ima.plugin.impl.ImaPluginInstaller$Copier.preVisitDirectory(ImaPluginInstaller.java:648)\n\tat com.ibm.ima.plugin.impl.ImaPluginInstaller$Copier.preVisitDirectory(ImaPluginInstaller.java:626)\n\tat java.nio.file.Files.walkFileTree(Files.java:2688)\n\tat com.ibm.ima.plugin.impl.ImaPluginInstaller$Copier.execute(ImaPluginInstaller.java:638)\n\tat com.ibm.ima.plugin.impl.ImaPluginInstaller.updatePluginProperites(ImaPluginInstaller.java:586)\n\tat com.ibm.ima.plugin.impl.ImaPluginInstaller.installPlugin(ImaPluginInstaller.java:467)\n\tat com.ibm.ima.plugin.impl.ImaPluginInstaller.main(ImaPluginInstaller.java:378)\n" } ;
            verifyMessage = { "status":400, "Code":"CWLNA0378", "Message":"The plug-in update failed. The error is: Failed to copy plugin(staging) directory in temporary area.\njava.nio.file.DirectoryNotEmptyException: /tmp/plugin/json_msg" } ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET  overwrite Properties File', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// defect 129946 - fail until restarted
        it('should return status 400 on GET PropertiesFile "Plugin":json_msg before restart #2' , function(done) {
            verifyConfig = FVT.json_msgProperties ;  // guts of file are not available until restart now
//            verifyMessage = { "status":404, "Code":"CWLNA0136", "Message":"The item or object cannot be found. Type: Plugin Name: json_msg" } ;
//            verifyMessage = { "status":404, "Code":"CWLNA0113", "Message":"The requested item is not found." } ;
            verifyMessage = { "status":400, "Code":"CWLNA6198", "Message":"The requested item \"Plugin/json_msg/PropertiesFile\" is not found." } ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"Plugin/json_msg/PropertiesFile", FVT.getFailureCallback, verifyMessage, done);
        });

// HOW to Verify ZIP and Properties active or not - prior to restart.
// Restart #2
        it('should return status 200 on GET "Status" before restart #2 with Plugin', function(done) {
            verifyConfig = JSON.parse( FVT.expectPluginRunning  );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on POST "restart #2 with Plugin"', function(done) {
            var payload = '{"Service":"Plugin"}';
            verifyConfig = JSON.parse( FVT.expectPluginRunning ) ;
        verifyMessage = {"Status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart #2 with Plugin', function(done) {
            this.timeout( FVT.RESETCONFIG  + 3000);
            FVT.sleep( FVT.RESETCONFIG );
            verifyConfig = JSON.parse( FVT.expectPluginRunning  );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on GET Plugin after restart #2', function(done) {
            verifyConfig = JSON.parse( FVT.json_msgPluginProperty1 );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// defect 129946
        it('should return 200 on GET PropertiesFile "Plugin":json_msg after restart #2', function(done) {
            verifyConfig = FVT.json_msgProperties ;  // guts of file expected!
            FVT.makeGetRequest( FVT.uriConfigDomain+"Plugin/json_msg/PropertiesFile", FVT.getSuccessCallback, verifyConfig, done);
        });

        
// 137006 - OVERWRITE Will ALWAYS be required....
        it('should return status 400 on rePOST different "Plugin" zip only - no Overwrite', function(done) {
            var payload = '{"Plugin":{"json_msg":{"File":"jsonplugin2.zip"}}}';
            verifyConfig = JSON.parse( '{"Plugin":{"json_msg":{"File":"jsonplugin2.zip","PropertiesFile":"jsonplugin.json"}}}' );
            verifyMessage = {"status":400,"Code":"CWLNA6171","Message":"The File or PropertiesFile already exists. Set Overwrite to true to replace the existing file."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET rePOST different "Plugin" zip - no Overwrite', function(done) {
            verifyConfig = JSON.parse( FVT.json_msgPluginProperty1 );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'json_msg', FVT.getSuccessCallback, verifyConfig, done);
        });    
        it('should return status 200 on rePOST different "Plugin" zip only - Overwrite', function(done) {
            var payload = '{"Plugin":{"json_msg":{"File":"jsonplugin2.zip", "Overwrite":true}}}';
            verifyConfig = JSON.parse( FVT.json_msgPlugin2Property1 );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET rePOST different "Plugin" zip - Overwrite', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'json_msg', FVT.getSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET PropertiesFile "Plugin":json_msg after restart #2', function(done) {
//            verifyConfig = FVT.json_msgProperties2 ;  // guts of updated file expected!  Updates are Dynamic per R., but I am not seeing that
            verifyConfig = FVT.json_msgProperties ;  // guts of original file expected!  Updates are NOT Dynamic -  not seeing that
            FVT.makeGetRequest( FVT.uriConfigDomain+"Plugin/json_msg/PropertiesFile", FVT.getSuccessCallback, verifyConfig, done);
        });
        
// Restart #3
        it('should return status 200 on GET "Status" before restart #3 with Plugin', function(done) {
            verifyConfig = JSON.parse( FVT.expectPluginRunning  );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on POST "restart #3 with Plugin"', function(done) {
            var payload = '{"Service":"Plugin"}';
            verifyConfig = JSON.parse( FVT.expectPluginRunning ) ;
            verifyMessage = {"Status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart #3 with Plugin', function(done) {
            this.timeout( FVT.RESETCONFIG  + 3000);
            FVT.sleep( FVT.RESETCONFIG );
            verifyConfig = JSON.parse( FVT.expectPluginRunning  );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on GET rePOST differenet "Plugin" zip after restart #3', function(done) {
            verifyConfig = JSON.parse( FVT.json_msgPlugin2Property1 );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'json_msg', FVT.getSuccessCallback, verifyConfig, done);
        });    
        it('should return status 200 on GET PropertiesFile "Plugin":json_msg after restart #3', function(done) {
            verifyConfig = FVT.json_msgProperties ;  // guts of original file expected!  
            FVT.makeGetRequest( FVT.uriConfigDomain+"Plugin/json_msg/PropertiesFile", FVT.getSuccessCallback, verifyConfig, done);
        });

        
        it('should return status 404 on rePOST original name "Plugin" zip not found', function(done) {
            var payload = '{"Plugin":{"json_msg":{"File":"jsonplugin.zip"}}}';
            verifyConfig = JSON.parse( FVT.json_msgPlugin2Property1 );
            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: File Name: jsonplugin.zip"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET rePOST original "Plugin" zip', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });    
        
        it('should return status 400 on rePOST new "Plugin" PropertiesFile - no Overwrite', function(done) {
            var payload = '{"Plugin":{"json_msg":{"File":"jsonplugin2.json"}}}';
            verifyConfig = JSON.parse( FVT.json_msgPlugin2Property1 );
            verifyMessage = {"status":400,"Code":"CWLNA6171","Message":"The File or PropertiesFile already exists. Set Overwrite to true to replace the existing file."};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET rePOST new "Plugin" PropertiesFile - no Overwrite', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });    
//MISMATCH File.vs.PropertiesFile
        it('should return 400 on POST when "Plugin" File is not a ZIP', function(done) {
            var payload = '{"Plugin":{"json_msg":{"File":"jsonplugin2.json", "Overwrite":true}}}';
            verifyConfig = JSON.parse( FVT.json_msgPlugin2Property1 );
//            verifyMessage = {"status":400,"Code":"CWLNA0378","Message":"The plug-in update failed. The error is: Plugin zip file is corrupted\nFailed to install plugin using : jsonplugin2.json\njava.util.zip.ZipException: error in opening zip file\n\tat java.util.zip.ZipFile.open(Native Method)\n\tat java.util.zip.ZipFile.<init>(ZipFile.java:235)\n\tat java.util.zip.ZipFile.<init>(ZipFile.java:165)\n\tat java.util.zip.ZipFile.<init>(ZipFile.java:179)\n\tat com.ibm.ima.plugin.impl.ImaPluginInstaller.validateZipFile(ImaPluginInstaller.java:527)\n\tat com.ibm.ima.plugin.impl.ImaPluginInstaller.installPlugin(ImaPluginInstaller.java:475)\n\tat com.ibm.ima.plugin.impl.ImaPluginInstaller.main(ImaPluginInstaller.java:378)\n"};
            verifyMessage = {"status":400,"Code":"CWLNA0378","Message":"The plug-in update failed. The error is: Plugin zip file is corrupted\nFailed to install plugin using : jsonplugin2.json\njava.util.zip.ZipException: error in opening zip file\n\tat java.util.zip.ZipFile.open(Native Method)"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST when "Plugin" File is not a ZIP', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });    
        it('should return status 200 on GET PropertiesFile after POST when "Plugin" File is not a ZIP', function(done) {
            verifyConfig = FVT.json_msgProperties ;  // guts of original file expected! 
            FVT.makeGetRequest( FVT.uriConfigDomain+"Plugin/json_msg/PropertiesFile", FVT.getSuccessCallback, verifyConfig, done);
        });
//MISMATCH File.vs.PropertiesFile
        it('should return 400 on POST when PropertiesFile is not JSON', function(done) {
            var payload = '{"Plugin":{"json_msg":{"PropertiesFile":"NameTooLong.zip", "Overwrite":true}}}';
            verifyConfig = JSON.parse( FVT.json_msgPlugin2Property1 );
 //           verifyMessage = {"status":400,"Code":"CWLNA0378","Message":"The plug-in update failed. The error is: The parsing of plug-in configuration properties has failed.\njava.lang.Exception: Failed to parse JSON file: pluginproperties.json\n\tat com.ibm.ima.plugin.util.ImaJson.parse(ImaJson.java:446)\n\tat com.ibm.ima.plugin.impl.ImaPluginInstaller.parseConfig(ImaPluginInstaller.java:242)\n\tat com.ibm.ima.plugin.impl.ImaPluginInstaller.validate(ImaPluginInstaller.java:262)\n\tat com.ibm.ima.plugin.impl.ImaPluginInstaller.updatePluginProperites(ImaPluginInstaller.java:606)\n\tat com.ibm.ima.plugin.impl.ImaPluginInstaller.installPlugin(ImaPluginInstaller.java:467)\n\tat com.ibm.ima.plugin.impl.ImaPluginInstaller.main(ImaPluginInstaller.java:378)\nConfiguration properties were not updated for plugin: json_msg\n"};
            verifyMessage = {"status":400,"Code":"CWLNA0378","Message":"The plug-in update failed. The error is: The parsing of plug-in configuration properties has failed.\njava.lang.Exception: Failed to parse JSON file: pluginproperties.json"};
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET after POST when PropertiesFile is not JSON', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });    
        it('should return status 200 on GET PropertiesFile after POST when PropertiesFile is not JSON', function(done) {
            verifyConfig = FVT.json_msgProperties ;  // guts of original file expected!  
            FVT.makeGetRequest( FVT.uriConfigDomain+"Plugin/json_msg/PropertiesFile", FVT.getSuccessCallback, verifyConfig, done);
        });
// Corrected MISMATCH File.vs.PropertiesFile (what I really wanted...)
        it('should return status 200 on rePOST new "Plugin" PropertiesFile - with Overwrite', function(done) {
            var payload = '{"Plugin":{"json_msg":{"PropertiesFile":"jsonplugin2.json", "Overwrite":true}}}';
            verifyConfig = JSON.parse( FVT.json_msgPlugin2Property2 );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET rePOST new "Plugin" PropertiesFile - with Overwrite', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });    
        it('should return status 200 on GET PropertiesFile "Plugin":json_msg after restart #3', function(done) {
//            verifyConfig = FVT.json_msgProperties2 ;  // guts of updated file expected!  Updates are Dynamic per R., but I am not seeing that
            verifyConfig = FVT.json_msgProperties ;  // guts of original file expected!  Updates are NOT Dynamic -  not seeing that
            FVT.makeGetRequest( FVT.uriConfigDomain+"Plugin/json_msg/PropertiesFile", FVT.getSuccessCallback, verifyConfig, done);
        });
        
        it('should return status 404 on rePOST new "Plugin" PropertiesFile - not found', function(done) {
            var payload = '{"Plugin":{"json_msg":{"PropertiesFile":"jsonpluginMissing.json", "Overwrite":true }}}';
            verifyConfig = JSON.parse( FVT.json_msgPlugin2Property2 );
            verifyMessage = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: PropertiesFile Name: jsonpluginMissing.json"} ;
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on GET rePOST new "Plugin" PropertiesFile - not found', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });    
        
// Restart #4
        it('should return status 200 on GET "Status" before restart #4 with Plugin', function(done) {
            verifyConfig = JSON.parse( FVT.expectPluginRunning  );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on POST "restart #4 with Plugin"', function(done) {
            var payload = '{"Service":"Plugin"}';
            verifyConfig = JSON.parse( FVT.expectPluginRunning ) ;
        verifyMessage = {"Status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart #4 with Plugin', function(done) {
            this.timeout( FVT.RESETCONFIG  + 3000);
            FVT.sleep( FVT.RESETCONFIG );
            verifyConfig = JSON.parse( FVT.expectPluginRunning  );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on GET "Plugin" after restart #4', function(done) {
            verifyConfig = JSON.parse( FVT.json_msgPlugin2Property2 );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });    

// 140398 (reopened, prohibit removal)
        it('should return status 400 on remove PropertiesFile', function(done) {
            var payload = '{"Plugin":{"json_msg":{"PropertiesFile":""}}}';
            verifyConfig = JSON.parse( FVT.json_msgPlugin2Property2 );
            verifyMessage = {"status":400,"Code":"CWLNAxxxx","Message":"Can not remove PropertiesFile once established, you can put an empty file"} ;
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET remove PropertiesFile fails', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });    
// defect 129946
        it('should return status 200 on GET PropertiesFile "Plugin":json_msg before restart #5', function(done) {
            verifyConfig = FVT.json_msgProperties2 ;  // guts of file expected until after restart!  
            FVT.makeGetRequest( FVT.uriConfigDomain+"Plugin/json_msg/PropertiesFile", FVT.getSuccessCallback, verifyConfig, done);
        });

// Restart #5
        it('should return status 200 on GET "Status" before restart #5 with Plugin', function(done) {
            verifyConfig = JSON.parse( FVT.expectPluginRunning  );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on POST "restart #5 with Plugin"', function(done) {
            var payload = '{"Service":"Plugin"}';
            verifyConfig = JSON.parse( FVT.expectPluginRunning ) ;
        verifyMessage = {"Status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart #5 with Plugin', function(done) {
            this.timeout( FVT.RESETCONFIG  + 3000);
            FVT.sleep( FVT.RESETCONFIG );
            verifyConfig = JSON.parse( FVT.expectPluginRunning  );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on GET Plugin after restart #5 where DID NOT remove PropertiesFile', function(done) {
            verifyConfig = JSON.parse( FVT.json_msgPlugin2Property2 );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });    
// defect 129946 140398 (redesign, can not remove)
        it('should return status 200 on GET Properties "Plugin", should NOT be removed after restart #5', function(done) {
            verifyConfig = FVT.json_msgProperties2 ;  // NO guts of file, it was removed -- well NOT ANYMORE, can not remove, only replace with empty file...
//            verifyMessage = { "status":400, "Code":"CWLNA8048", "Message":"The requested item \"Plugin/json_msg/PropertiesFile\" is not found." } ;
//            FVT.makeGetRequest( FVT.uriConfigDomain+"Plugin/json_msg/PropertiesFile", FVT.getFailureCallback, verifyMessage, done);
            FVT.makeGetRequest( FVT.uriConfigDomain+"Plugin/json_msg/PropertiesFile", FVT.getSuccessCallback, verifyConfig, done);
        });

        
    });
//  ====================   PropertiesFile  ====================  //
    describe('PropertiesFile[]:', function() {

        it('should return status 200 on POST "REST Plugin" with PropertiesFile', function(done) {
            var payload = '{"Plugin":{"restmsg":{"File":"restplugin.zip","PropertiesFile":"package.json"}}}';
            verifyConfig = JSON.parse( FVT.restmsgPlugin );
            FVT.makePostRequest( FVT.uriConfigDomain, payload, FVT.postSuccessCallback, verifyConfig, done);
        });
        it('should return status 200 on GET "REST Plugin" with PropertiesFile', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// defect 129946
        it('should return status 400 on SERVICE GET Properties "Plugin" restmsg before restart #6', function(done) {
            verifyConfig = JSON.parse( '{"Plugin":{"PropertiesFile":""}}' );  //must go to CONFIG not SERVICE
//            verifyMessage = { "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/v1/service/Plugin is not valid." } ;
            verifyMessage = { "status":400, "Code":"CWLNA0137", "Message":"The REST API call: /ima/v1/service/Plugin/restmsg/PropertiesFile is not valid." } ;
            FVT.makeGetRequest( FVT.uriServiceDomain+"Plugin/restmsg/PropertiesFile", FVT.getFailureCallback, verifyMessage, done);
        });

        it('should return status 400 on CONFIG GET Properties "Plugin" restmsg before restart #6', function(done) {
            verifyConfig = JSON.parse( '{"Plugin":{"PropertiesFile":""}}' );  //? file guts should be in staging still, can see yet.
            verifyMessage = { "status":400, "Code":"CWLNA6198", "Message":"The requested item \"Plugin/restmsg/PropertiesFile\" is not found." } ;
            FVT.makeGetRequest( FVT.uriConfigDomain+"Plugin/restmsg/PropertiesFile", FVT.getFailureCallback, verifyMessage, done);
        });

// How To verify not active until restart?
// Restart #6
        it('should return status 200 on GET "Status" before restart #6 with Plugin', function(done) {
            verifyConfig = JSON.parse( FVT.expectPluginRunning  );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on POST "restart #6 with Plugin"', function(done) {
            var payload = '{"Service":"Plugin"}';
            verifyConfig = JSON.parse( FVT.expectPluginRunning ) ;
            verifyMessage = {"Status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart #6 with Plugin', function(done) {
            this.timeout( FVT.RESETCONFIG  + 3000);
            FVT.sleep( FVT.RESETCONFIG );
            verifyConfig = JSON.parse( FVT.expectPluginRunning  );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on GET "REST Plugin" with PropertiesFile after restart #6', function(done) {
            verifyConfig = JSON.parse( FVT.restmsgPlugin );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
// defect 129946
        it('should return status 200 on GET Properties "Plugin" restmsg after restart #6', function(done) {
            verifyConfig =  FVT.packageProperties ;  // file guts in runtime
            FVT.makeGetRequest( FVT.uriConfigDomain+"Plugin/restmsg/PropertiesFile", FVT.getSuccessCallback, verifyConfig, done);
        });

    });


//  ====================  STOP Service  ====================  //
    describe('STOP:', function() {
// try to STOP Plugin before DELETE
        it('should return status 200 on GET "Status" before STOP with Plugin', function(done) {
            verifyConfig = JSON.parse( FVT.expectPluginRunning   );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on POST STOP with Plugin"', function(done) {
            var payload = '{"Service":"Plugin"}';
            verifyConfig = JSON.parse( FVT.expectPluginDefault ) ;
        verifyMessage = {"Status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'stop', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after STOP with Plugin', function(done) {
            this.timeout( FVT.RESETCONFIG  + 3000);
            FVT.sleep( FVT.RESETCONFIG );
            verifyConfig = JSON.parse( FVT.expectPluginStopped );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on GET after STOP Plugin Service ', function(done) {
            verifyConfig = JSON.parse( FVT.allPlugins );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
// Restart #7  (actaully a START)
        it('should return status 200 on GET "Status" before restart #7 with Plugin', function(done) {
            verifyConfig = JSON.parse( FVT.expectPluginStopped );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on POST "restart #7 with Plugin"', function(done) {
            var payload = '{"Service":"Plugin"}';
            verifyConfig = JSON.parse( FVT.expectPluginRunning ) ;
        verifyMessage = {"Status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'start', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart #7 with Plugin', function(done) {
            this.timeout( FVT.RESETCONFIG  + 3000);
            FVT.sleep( FVT.RESETCONFIG );
            verifyConfig = JSON.parse( FVT.expectPluginRunning   );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on GET after restart #7 ', function(done) {
            verifyConfig = JSON.parse( FVT.allPlugins );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        
    });

//  ====================  Delete test cases  ====================  //
    describe('Delete:', function() {

        it('should return status 404 when deleting "Max Length Name"', function(done) {
            var payload = '{"Plugin":{"'+ FVT.long256Name +'":null}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":404,"Code": "CWLNA0113","Message": "The requested item is not found."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.deleteFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get all after delete, "Max Length Name" was not found', function(done) {
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getVerifyDeleteCallback, verifyConfig, done);
        });
        it('should return status 404 on get after delete, "Max Length Name" not found', function(done) {
            verifyConfig = JSON.parse( '{"status":404,"Code": "CWLNA0136","Message": "The item or object cannot be found. Type: Plugin Name: '+ FVT.long256Name +'","Plugin":{"'+ FVT.long256Name +'":{}}}' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long256Name, FVT.verify404NotFound, verifyConfig, done);
        });

        it('should return status 200 when POST DELETE of "restmsg"', function(done) {
            var payload = {"Plugin":{"restmsg":null}} ;
            verifyConfig = {"Plugin":{"restmsg":{}}};
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'restmsg', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after DELETE, "restmsg" is gone', function(done) {
            verifyMessage = {"status":404,"Code": "CWLNA0136","Message": "The item or object cannot be found. Type: Plugin Name: restmsg" };
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'restmsg', FVT.getFailureCallback, verifyMessage, done);
        });
// Restart #8
        it('should return status 200 on GET "Status" before restart #8 with Plugin', function(done) {
            verifyConfig = JSON.parse( FVT.expectPluginRunning  );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on POST "restart #8 with Plugin"', function(done) {
            var payload = '{"Service":"Plugin"}';
            verifyConfig = JSON.parse( FVT.expectPluginRunning ) ;
        verifyMessage = {"Status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart #8 with Plugin', function(done) {
            this.timeout( FVT.RESETCONFIG  + 3000);
            FVT.sleep( FVT.RESETCONFIG );
            verifyConfig = JSON.parse( FVT.expectPluginRunning  );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 404 on GET after DELETE, "restmsg" is gone after restart #8', function(done) {
            verifyMessage = {"status":404,"Code": "CWLNA0136","Message": "The item or object cannot be found. Type: Plugin Name: restmsg" };
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'restmsg', FVT.getFailureCallback, verifyMessage, done);
        });


        
    });

//  ====================  Error test cases  ====================  //
    describe('Error:', function() {

        it('should return status 400 when trying to Delete All Plugins, just bad form', function(done) {
            var payload = '{"Plugin":null}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = {"status":400,"Code":"CWLNA0137","Message":"The REST API call: \"Plugin\":null is not valid." };
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 200 on get, Delete All Plugins fails', function(done) {
            verifyConfig = JSON.parse( FVT.json_msgPlugin2Property2 );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return status 400 when trying to POST Plugin with Name Too Long', function(done) {
            var payload = '{"Plugin":{"'+ FVT.long257Name +'":{"File":"NameToolong.zip"}}}';
            verifyConfig = JSON.parse( payload );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0133","Message":"The name of the configuration object is too long. Object: Plugin Property: Name Value: '+ FVT.long257Name +'." }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET, Name Too Long was not created', function(done) {
            verifyMessage = JSON.parse( '{ "status":404, "Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Plugin Name: '+  FVT.long257Name +'","Plugin":{"'+ FVT.long257Name +'":{}} }' );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+FVT.long257Name, FVT.verify404NotFound, verifyMessage, done);
        });

        it('should return status 404 when trying to POST Plugin with Invalid parameter Update', function(done) {
            var payload = '{"Plugin":{"BadParamUpdate":{"File":"NameTooLong.zip","Update":true}}}';
            verifyConfig = JSON.parse( payload );
//            verifyMessage = JSON.parse( '{"status":404,"Code":"CWLNA0138","Message":"The property name is invalid. Object: Plugin Name: BadParamUpdate Property: Update" }' );
            verifyMessage = JSON.parse( '{"status":400,"Code":"CWLNA0138","Message":"The property name is invalid. Object: Plugin Name: BadParamUpdate Property: Update" }' );
            FVT.makePostRequestWithVerify( FVT.uriConfigDomain, payload, FVT.postFailureCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on get, Plugin BadParamUpdate was not created', function(done) {
            verifyConfig = {"status":404,"Code":"CWLNA0136","Message":"The item or object cannot be found. Type: Plugin Name: BadParamUpdate","Plugin":{"BadParamUpdate":{}}};
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+"BadParamUpdate", FVT.verify404NotFound, verifyConfig, done);
        });
        
        
    });

//  ====================  Cleanup test cases  ====================  //
    describe('Cleanup:', function() {

        it('should return status 200 when POST DELETE of "json_msg"', function(done) {
            var payload = {"Plugin":{"json_msg":null}} ;
            verifyConfig = {"Plugin":{"json_msg":{}}};
            verifyMessage = {"Status":200,"Code": "CWLNA6011","Message": "The requested configuration change has completed successfully."};
            FVT.makeDeleteRequest( FVT.uriConfigDomain+uriObject+'json_msg', FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
        it('should return status 404 on GET after DELETE, "json_msg" is gone', function(done) {
            verifyMessage = {"status":404,"Code": "CWLNA0136","Message": "The item or object cannot be found. Type: Plugin Name: json_msg" };
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject+'json_msg', FVT.getFailureCallback, verifyMessage, done);
        });
// Restart #9
// Restart not needed? actually with last DELETE Plugin, Plugin service stops automagically
        it('should return status 200 on GET "Status" before restart #9 with Plugin', function(done) {
            FVT.sleep( 3000 );
//            verifyConfig = JSON.parse( FVT.expectPluginStopped   );
            verifyConfig = JSON.parse( FVT.expectPluginUninstalled );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });
        it('should return status 200 on POST "restart #9 with Plugin"', function(done) {
            var payload = '{"Service":"Plugin"}';
            verifyConfig = JSON.parse( FVT.expectPluginDefault ) ;
        verifyMessage = {"Status":200,"Code":"CWLNA6011","Message":"The requested configuration change has completed successfully."} ;
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain+'restart', payload, FVT.postSuccessCallback, verifyConfig, verifyMessage,  done);
        });
        it('should return status 200 on GET "Status" after restart #9 with Plugin', function(done) {
            this.timeout( FVT.RESETCONFIG  + 3000);
            FVT.sleep( FVT.RESETCONFIG );
            verifyConfig = JSON.parse( FVT.expectPluginDefault );
            FVT.makeGetRequest( FVT.uriServiceDomain+'status/Plugin', FVT.getSuccessCallback, verifyConfig, done)
        });

        it('should return status 200 on GET after final Uninstall Restart #9 ', function(done) {
            verifyConfig = JSON.parse( expectDefault );
            FVT.makeGetRequest( FVT.uriConfigDomain+uriObject, FVT.getSuccessCallback, verifyConfig, done);
        });
        

    });

});
