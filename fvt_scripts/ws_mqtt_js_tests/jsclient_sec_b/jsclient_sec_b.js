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

function jsclient_sec_1_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("port",65535));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("userName","user1"));
    connection_options.push(new Option("password","pwuser1"));
    connection_options.push(new Option("useSSL",true));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); checks--; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_sec_2_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("host",imaserveraddr_IPv6));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("userName","user1"));
    connection_options.push(new Option("password","pwuser1"));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); checks--; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_sec_3_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("port",10101));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("userName","user1"));
    connection_options.push(new Option("password","pwuser1"));
    connection_options.push(new Option("useSSL",false));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); checks--; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_sec_4_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("port",10102));
    client_options.push(new Option("host",imaserveraddr_IPv6));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("userName","user1"));
    connection_options.push(new Option("password","pwuser1"));
    connection_options.push(new Option("useSSL",false));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); checks--; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_sec_5_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("port",10103));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("userName","Penny Robbins"));
    connection_options.push(new Option("password","jtvw76nc"));
    connection_options.push(new Option("useSSL",false));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); checks--; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_sec_6_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("port",10104));
    client_options.push(new Option("host",imaserveraddr_IPv6));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("userName","Penny Robbins"));
    connection_options.push(new Option("password","jtvw76nc"));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); checks--; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_sec_e1_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("host",imaserveraddr_IPv6));
    client_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("mqtt_client_1",code,reason); fails++; }));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("password","pwuser1"));
    connection_options.push(new Option("useSSL",false));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); fails++; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); checks--; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_sec_e2_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("mqtt_client_1",code,reason); fails++; }));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("userName","user"+ String.fromCharCode(245))); // 245 should be invalid for UTF-8
    connection_options.push(new Option("password","pwuser1"));
    connection_options.push(new Option("useSSL",false));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); fails++; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); checks--; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_sec_e3_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("host",imaserveraddr_IPv6));
    client_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("mqtt_client_1",code,reason); fails++; }));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("userName","user_dne"));
    connection_options.push(new Option("password","user_dne"));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); fails++; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); checks--; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_sec_e5_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("mqtt_client_1",code,reason); fails++; }));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("userName","user2"));
    connection_options.push(new Option("password","wrongpass"));
    connection_options.push(new Option("useSSL",false));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); fails++; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); checks--; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_sec_e9_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("host",imaserveraddr_IPv6));
    client_options.push(new Option("port",10104));
    client_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("mqtt_client_1",code,reason); fails++; }));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("userName","Fred Robbins"));
    connection_options.push(new Option("password","rig90dsn"));
    connection_options.push(new Option("useSSL",false));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); fails++; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); checks--; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_sec_e10_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("port",65535));
    client_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("mqtt_client_1",code,reason); fails++; }));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("userName","user1"));
    connection_options.push(new Option("password","pwuser1"));
    connection_options.push(new Option("useSSL",false));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); fails++; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); checks--; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

/**
 * This test no longer works, because the JS client has been updated to throw
 * an exception if an attempt to connect without specifying a password is made.
function jsclient_sec_e11_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("host",imaserveraddr_IPv6));
    client_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("mqtt_client_1",code,reason); fails++; }));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("userName","user_empty_pass"));
    connection_options.push(new Option("useSSL",false));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); fails++; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); checks--; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}
*/

function jsclient_sec_e12_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("port",20101));
    client_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("mqtt_client_1",code,reason); fails++; }));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("userName","user1"));
    connection_options.push(new Option("password","pwuser1"));
    connection_options.push(new Option("useSSL",false));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); fails++; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); checks--; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_sec_e13_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("host",imaserveraddr_IPv6));
    client_options.push(new Option("port",20102));
    client_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("mqtt_client_1",code,reason); fails++; }));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("userName","user1"));
    connection_options.push(new Option("password","pwuser1"));
    connection_options.push(new Option("useSSL",false));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); fails++; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); checks--; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_sec_e14_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("mqtt_client_1",code,reason); fails++; }));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("userName",""));
    connection_options.push(new Option("password",""));
    connection_options.push(new Option("useSSL",false));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); fails++; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); checks--; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_sec_e15_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("host",imaserveraddr_IPv6));
    client_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("mqtt_client_1",code,reason); fails++; }));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("userName",null));
    connection_options.push(new Option("password",null));
    connection_options.push(new Option("useSSL",false));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); fails++; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
    connection_options.push(new Option("catchFunction", function (err) { writeLog(logfile,err); checks--; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_sec_e16_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("mqtt_client_1",code,reason); fails++; }));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("userName",""));
    connection_options.push(new Option("password","pwuser1"));
    connection_options.push(new Option("useSSL",false));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); fails++; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); checks--; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

