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

function jsclient_connect_1_b(test) {
    fails=0;
    checks=0;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","x"));
    client_options.push(new Option("host",imaserveraddr_IPv6));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_connect_2_b(test) {
    fails=0;
    checks=0;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","CLIENT 1"));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_connect_3_b(test) {
    fails=0;
    checks=0;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId"," "));
    client_options.push(new Option("host",imaserveraddr_IPv6));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_connect_4_b(test) {
    fails=0;
    checks=0;
    setLog(test);

    // Client 1
    // Initialize the sync variable that will be used
    requestSync(logfile,'1 client1_connected');

    var client1_options = new Array();
    client1_options.push(new Option("clientId","mqtt_client_1"));
    client1_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("mqtt_client_1",code,reason); fails++; }));
    var testClient1 = new TestClient(client1_options);

    var connection1_options = new Array();
    connection1_options.push(new Option("invocationContext",testClient1));
    connection1_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); requestSync(logfile,'2 client1_connected 1'); }));
    connection1_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
    var testConnection1= new TestConnection(connection1_options); 

    // Client 2
    // Initialize the sync variable that will be used
    requestSync(logfile,'1 client2_connected');

    var client2_options = new Array();
    client2_options.push(new Option("clientId","mqtt_client_2"));
    client2_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("mqtt_client_2",code,reason); fails++; }));
    var testClient2 = new TestClient(client2_options);

    var connection2_options = new Array();
    connection2_options.push(new Option("invocationContext",testClient2));
    connection2_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); requestSync(logfile,'2 client2_connected 1'); }));
    connection2_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
    var testConnection2= new TestConnection(connection2_options); 

    // Client 3
    // Initialize the sync variable that will be used
    requestSync(logfile,'1 client3_connected');

    var client3_options = new Array();
    client3_options.push(new Option("clientId","mqtt_client_3"));
    client3_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("mqtt_client_3",code,reason); fails++; }));
    var testClient3 = new TestClient(client3_options);

    var connection3_options = new Array();
    connection3_options.push(new Option("invocationContext",testClient3));
    connection3_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); requestSync(logfile,'2 client3_connected 1'); }));
    connection3_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
    var testConnection3= new TestConnection(connection3_options); 

    connect(testClient1,testConnection1);
    connect(testClient2,testConnection2);
    connect(testClient3,testConnection3);
    
    requestSync(logfile,'3 client1_connected 1 2000', function (result1) {
        checkWaitResult(result1);
        requestSync(logfile,'3 client2_connected 1 1000', function (result2) { 
            checkWaitResult(result2);
            requestSync(logfile,'3 client3_connected 1 1000', function (result3) {
                checkWaitResult(result3);
                endTest();
            });
        });
    });
    
    
    
}

function jsclient_connect_5_b(test) {
    fails=0;
    checks=3;
    setLog(test);

    // Client 1
    // Initialize the sync variable that will be used
    requestSync(logfile,'1 client1_connected');

    var client1_options = new Array();
    client1_options.push(new Option("clientId","mqtt_client"));
    client1_options.push(new Option("host",imaserveraddr_IPv6));
    client1_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("first(mqtt_client)",code,reason); checks--; }));
    var testClient1 = new TestClient(client1_options);

    var connection1_options = new Array();
    connection1_options.push(new Option("invocationContext",testClient1));
    connection1_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); requestSync(logfile,'2 client1_connected 1'); checks--; }));
    connection1_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
    var testConnection1= new TestConnection(connection1_options); 

    // Client 2
    // Initialize the sync variable that will be used
    requestSync(logfile,'1 client2_connected');

    var client2_options = new Array();
    client2_options.push(new Option("clientId","mqtt_client"));
    client2_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("second(mqtt_client)",code,reason); fails++; }));
    var testClient2 = new TestClient(client2_options);

    var connection2_options = new Array();
    connection2_options.push(new Option("invocationContext",testClient2));
    connection2_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); requestSync(logfile,'2 client2_connected 1'); checks--; }));
    connection2_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
    var testConnection2= new TestConnection(connection2_options); 

    connect(testClient1,testConnection1);
    requestSync(logfile,'3 client1_connected 1 1000', function (result1) {
        checkWaitResult(result1);
        connect(testClient2,testConnection2);
        requestSync(logfile,'3 client2_connected 1 1000', function (result2) {
            checkWaitResult(result2);
            // Give it a second in case the second client is going to be disconnected
            window.setTimeout(function() { endTest(); },1000);
        });       
    });
}

function jsclient_connect_6_b(test) {
    fails=0;
    checks=0;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","client1"));
    client_options.push(new Option("port",4321));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}


function jsclient_connect_e5_b(test) {
    fails=0;
    checks=0;
    setLog(test);
    var client_options = new Array();
    var clientid = generateClientId(24);
    client_options.push(new Option("clientId",clientid));
    client_options.push(new Option("host",imaserveraddr_IPv6));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { writeLog(logfile, "onSuccess called, test failed"); fails++; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { writeLog(logfile, "onFailure called, test passed"); endTest(); }));
    connection_options.push(new Option("catchFunction", function (err) { endTest(); }));
    var testConnection= new TestConnection(connection_options);

    connect(testClient,testConnection);
}




function jsclient_connect_e8_b(test) {
    fails=0;
    checks=0;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId",""));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { writeLog(logfile, "onSuccess called, test failed"); fails++; endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { writeLog(logfile, "onFailure called, test passed"); endTest(); }));
    connection_options.push(new Option("catchFunction", function (err) { writeLog(logfile,err); endTest(); }));
    var testConnection= new TestConnection(connection_options);

    connect(testClient,testConnection);
}

function jsclient_connect_e10_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","client1"));
    client_options.push(new Option("host",imaserveraddr_IPv6));
    client_options.push(new Option("port",4322));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); checks--; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_connect_e11_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","client1"));
    client_options.push(new Option("port",4323));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); checks--; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

function jsclient_connect_e12_b(test) {
    fails=0;
    checks=1;
    setLog(test);
    var client_options = new Array();
    client_options.push(new Option("clientId","client1"));
    client_options.push(new Option("host",imaserveraddr_IPv6));
    client_options.push(new Option("port",4324));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); endTest(); }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); checks--; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

