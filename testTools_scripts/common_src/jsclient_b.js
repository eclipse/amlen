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
var imaserveraddr = "A1_IPv4_1";
var imaserveraddr_2 = "A1_IPv4_2";
var imaserveraddr_IPv6 = "[A1_IPv6_1]";

// Other common variables:
var imaserverport = 16102;

// Sync driver address
var syncaddress = "M1_IPv4_1";
var syncport = "SYNCPORT";

// http_server_document_root is the value of DocumentRoot set in httpd.conf
var http_server_document_root = "";
var php_write_log_script = "http://M1_IPv4_1M1_TESTROOT/ws_mqtt_js_tests/writeData.php";
var php_read_log_script = "http://M1_IPv4_1M1_TESTROOT/ws_mqtt_js_tests/readData.php";
var php_sync_script = "http://M1_IPv4_1M1_TESTROOT/ws_mqtt_js_tests/sync.php";
