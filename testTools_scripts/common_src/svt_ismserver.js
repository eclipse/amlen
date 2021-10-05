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
// JavaScript test cases use the following variables
// Required action:
//   Replace A1_IPv4_1 with ISM server IP address
   
var ismserveraddr = "A1_IPv4_1";

// Other common variables:
var ismserverport = "16102";

// Sync driver address
var syncaddress = "M1_IPv4_1";
var syncport = "SYNCPORT";

// http_server_document_root is the value of DocumentRoot set in httpd.conf
var http_server_document_root = "";
var php_write_log_script = "http://M1_HOSTM1_TESTROOT/web/js/writeData.php";
var php_read_log_script = "http://M1_HOSTM1_TESTROOT/web/js/readData.php";
var php_sync_script = "http://M1_HOSTM1_TESTROOT/web/js/sync.php";

function addCSSStyleSheet(jsname,pos) {
    var th = document.getElementsByTagName(pos)[0];
    var s = document.createElement('link');
    s.setAttribute('rel','stylesheet');
    s.setAttribute('href',jsname);
    th.appendChild(s);
} 



function addJavascript(jsname,pos) {
    var th = document.getElementsByTagName(pos)[0];
    var s = document.createElement('script');
    s.setAttribute('type','text/javascript');
    s.setAttribute('src',jsname);
    th.appendChild(s);
} 

addCSSStyleSheet( "http://M1_HOSTM1_IMA_SDK/web/css/ismSamples.css", 'head');
addCSSStyleSheet( "http://M1_HOSTM1_IMA_SDK/web/css/ism_sample_web.css", 'head');

addJavascript("http://M1_HOSTM1_TESTROOT/web/js/jquery.js", 'head');
addJavascript("http://M1_HOSTM1_TESTROOT/web/js/common.js", 'head');
addJavascript("http://M1_HOSTM1_IMA_SDK/web/js/mqttws31.js", 'head');
addJavascript("http://M1_HOSTM1_TESTROOT/web/js/log.js", 'head');
