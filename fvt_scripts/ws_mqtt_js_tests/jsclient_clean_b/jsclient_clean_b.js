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

function jsclient_clean_1_b(test) {
    // -On connect, set cleanSession=false.
    // -Subscribe to a topic.
    // -Publish 10 messages to the topic.
    // -Disconnect.
    // -Reconnect the client and publish an additional 10 messages.

    fails=0;
    checks=2;
    numReceivedMsgs=0;
    numExpectedMsgs=0; // This is the total number of expected messages
    messages = new Array();
    expected = new Array();
    message_qos = new Array();
    expected_qos = new Array();

    setLog(test);
    
    var topics=new Array();

    function next() { 

        numReceivedMsgs=0;
        messages["cleanSession/test topic/1"]=0;
        message_qos["cleanSession/test topic/1"+'_0']=0;
        message_qos["cleanSession/test topic/1"+'_1']=0;
        message_qos["cleanSession/test topic/1"+'_2']=0;

        var connection2_options = new Array();
        connection2_options.push(new Option("cleanSession",false));
        connection2_options.push(new Option("invocationContext",testClient));
        connection2_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); 
            checks--;
            publishAll(topics,testClient.client);
            var interval=window.setInterval(function () {checkForEnd(function () {window.clearInterval(interval); checkMsgs(); endTest();})},3000);
        }));
        connection2_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
        var testConnection2 = new TestConnection(connection2_options); 

        connect(testClient,testConnection2);
        
    }

    var expected_qos_opts=[10,0,0];

    var topic_options_1 = new Array();
    topic_options_1.push(new Option("expected",10));
    topic_options_1.push(new Option("expected_qos",expected_qos_opts));

    var topic1 = new TestTopic("cleanSession/test topic/1",topic_options_1);
    topics.push(topic1);

    var client_options = new Array();
    client_options.push(new Option("clientId","mqtt_client_1"));
    client_options.push(new Option("onConnectionLost",function (code,reason) { onConnectionLostSimple("mqtt_client_1",code,reason); }));
    client_options.push(new Option("onMessageArrived",function (message) { onMessageArrived_simple(message, "mqtt_client_1"); }));
    var testClient = new TestClient(client_options);

    var connection_options = new Array();
    connection_options.push(new Option("cleanSession",false));
    connection_options.push(new Option("invocationContext",testClient));
    connection_options.push(new Option("onSuccess",function (context) { onConnectSuccessSimple(context); 
        checks--;
        subscribeAll(topics,testClient.client);
        publishAll(topics,testClient.client);
        var interval=window.setInterval(function () {checkForEnd(function () {window.clearInterval(interval); checkMsgs(); try { testClient.client.disconnect();} catch (error) {fails++; writeLog(logfile,error);} next();});},3000);
    }));
    connection_options.push(new Option("onFailure",function (context,code,reason) { onConnectFailureSimple(context,code,reason); fails++; endTest(); }));
    var testConnection= new TestConnection(connection_options); 

    connect(testClient,testConnection);
}

