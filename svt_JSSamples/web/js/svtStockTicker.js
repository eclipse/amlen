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
// JavaScript StockTicker test cases use the following variables
// Required action:
//   Dependant on ../common/ismser.js

// Parameters that would COULD come from URL input (See function startSubscriber() for which parameters can be passed with URL)

if ( typeof ismserveraddr == null ) {
    //JCD:  var ismServer="A1_IPv4_1";        // ISM Server IP address/host name  [string] from ../common/ismserver.js
    var ismServer="10.10.3.61";               // ISM Server IP address/host name  [string] from ../common/ismserver.js
} else { 
    var ismServer=ismserveraddr;              // ISM Server IP address/host name  [string] from ../common/ismserver.js 
}

if ( typeof ismserverport == null ) {
    var ismPort=16102;                        // ISM Server Listener Port  [integer] from ../common/ismserver.js
} else { 
    var ismPort=parseInt(ismserverport);      // ISM Server Listener Port  [integer] from ../common/ismserver.js
}
var logfile="svtStockerTickerScale";      // Default logfile name
var winClose=false;                       // Close the Browser Window on completion
var subSecure=false;                      // SSL [true|false]
var pubSecure=false;                      // SSL [true|false]
var mqttVersion=null;
var zeroLenClientID=false;
var maxClientID=false;
var sharedSub=null;


// Subscriber Variables 
var subCount=2;                        // Number of Subscriber sessions to start
var subPace=5000;                      // Pace the start of Subscribers (milliseconds)
//var subUser="admin";                 // Authorization User name  [string]
//var subPswd="admin";                 // Authorization password  [string]
var subUser=null;                      // Authorization User name  [string]
var subPswd=null;                      // Authorization password  [string]
var subClientID_stem="SUB";
var subClient= new Array();
var subClientOpts= new Array();
var subClientConnect= new Array();
var subClientFirstMsg= new Array();
var subClientLastMsg= new Array();
var subClientMsgCount= new Array();
var msgRecv=0;
var subCleanSession= true;             // Save session state on termination or start clean   [true|false]
var subQoS=2;                          // Quality of Service [0|1|2]
var subTopic="/topic/JS/" + subQoS;    // Topic Name  [string]
var subClientVerbose=false;            // Verbose output (mainly do you want to see a line for each msg received)

// Publisher Variables
var pubCount=2;                        // Number of Subscriber sessions to start
var pubPace=5000;                      // Pace the start of Publishers (milliseconds)
var pubThrottle;                       // Timer for Publish Message pacing
//var pubUser="admin";                 // Authorization User name  [string]
//var pubPswd="admin";                 // Authorization password  [string]
var pubUser=null;                      // Authorization User name  [string]
var pubPswd=null;                      // Authorization password  [string]
var pubAckReceived=0;                  // Counter for QoS Acks in Publisher [int]               
var pubClientID_stem="PUB";
var pubClient= new Array();
var pubClientOpts= new Array();
var pubClientConnect= new Array();
var pubComplete= false;                // Overall Publish Complete Status
var pubCompleteStatus= new Array();    // Individual Publisher Complete Status
var pubClientFirstMsg= new Array();
var pubClientLastMsg= new Array();
var pubClientMsgCount= new Array();
var pubCleanSession= true;             // Save session state on termination or start clean  [true|false]
var pubQoS=2;                          // Quality of Service [0|1|2]
var msgCount=400;                      // Number of messages to send/publish [integer]
var msgText="HAPPY HAPPY HAPPY";       // Message Body (Payload)  [string]
var pubTopic="/topic/JS/" + pubQoS;    // Topic Name  [string]
var pubRetain= false;                  // Save messages in message store  [true|false]

// Calcuated Parameters and Tuning Metrics
// Shared variables
var msgTopic="";                       // Topic Name  [string]
var throttleWaitMSec = 0;              // Publish Throttle, msec to wait between msgs, 0 is pedal to the metal
var subFirstMsg=null;                  // TIME of first msg received
var pubFirstMsg=null;                  // TIME of first msg sent
var publisherStarted=false;            // Flag indicates the startPublisher function has been called (only want to call it once, believe me)
var msgQoSCheck= false;
var DEBUG=false;                       // MQTT JavaScript Client Traceing
// If Counts are exposed to URL on the Fly, then these must be moved into *_web.js
var expectedMsgsClient = pubCount * msgCount;
var expectedMsgsTotal = pubCount * msgCount * subCount;

// Global variables NOW come from ../common/ismserver.js  Need to delete below if all works out....

//JCD:  var ismServer="A1_IPv4_1";                // ISM Server IP address/host name  [string]
//JCD:  var ismServer="10.10.3.61";               // ISM Server IP address/host name  [string] from ../common/ismserver.js

// Sync driver address
//JCD var syncAddress = "M1_IPv4_1";
//JCD var syncAddress = "9.3.179.61";

// http_server_document_root is the value of DocumentRoot set in httpd.conf
//JCD var http_server_document_root = "";

//JCD var php_write_log_script = "http://9.3.179.61/niagara/test/ws_mqtt_js_tests/writeData.php";
//JCD var php_read_log_script = "http://9.3.179.61/niagara/test/ws_mqtt_js_tests/readData.php";
//JCD var php_sync_script = "http://9.3.179.61/niagara/test/ws_mqtt_js_tests/sync.php";

//JCD var php_write_log_script = "http://M1_IPv4_1M1_TESTROOT/ws_mqtt_js_tests/writeData.php";
//JCD var php_read_log_script = "http://M1_IPv4_1M1_TESTROOT/ws_mqtt_js_tests/readData.php";
//JCD var php_sync_script = "http://M1_IPv4_1M1_TESTROOT/ws_mqtt_js_tests/sync.php";

