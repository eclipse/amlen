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
var logfile = "";
var numReceivedMsgs = 0;
var numExpectedMsgs = 0;
var numRetainedMsgs = 0;
var numExpectedRetMsgs = 0;
var numTl1ReceivedMsgs = 0;
var numTl2ReceivedMsgs = 0;
var messages = new Array();
var messages_ret = new Array();
var expected = new Array();
var expected_ret = new Array();
var message_qos = new Array();
var expected_qos = new Array();
var lastNum = 0;
var fails = 0;
var checks = 0;
var clientOnMessage = "default";
var isSynced = false;

function myToString(obj) {
    var str="{";
    for (field in obj) {
        str=str + field + '=' + obj[field] + ';';
    }
    str=str+'}';
    return str;
}
/*
Object.prototype.toString = function() {
    return myToString(this);
}

Event.prototype.toString = function() {
    return myToString(this);
}
*/

function initMsgCounts() {
    numReceivedMsgs=0;
    numExpectedMsgs=0; 
    numRetainedMsgs=0;
    numExpectedRetMsgs = 0;
    messages = new Array();
    messages_ret = new Array();
    expected = new Array();
    expected_ret = new Array();
    message_qos = new Array();
    expected_qos=new Array();
}

function generateClientId(length) {

    var topic_name="";
    for (var i=0; i<length; i++) {
        var num=Math.round(Math.random()*94)+32;
	topic_name=topic_name + String.fromCharCode(num);
    }
    return topic_name;
}


function Option (property,value) {
    this.property=property;
    this.value=value;
}


function TestClient (options) {

    this.client=null;

    // Set default values
    this.clientId="default_client";
    this.host=imaserveraddr;
    this.port=imaserverport;

    this.onMessageArrived=null;
    this.onMessageDelivered=null;
    this.onConnectionLost=null;

    // Read in options
    if (options) {
        for (var i=0; i<options.length; i++) {
            var property=options[i].property;
            var value=null;
            if (options[i].value != "null") {
                value=options[i].value;
            }
            switch (property) {
                case "clientId":
                    this.clientId=value;
                    break;
                case "host":
                    this.host=value;
                    break;
                case "port":
                    this.port=value;
                    break;
                case "onMessageArrived":
                    this.onMessageArrived=value;
                    break;
                case "onMessageDelivered":
                    this.onMessageDelivered=value;
                    break;
                case "onConnectionLost":
                    this.onConnectionLost=value;
                    break;
            }
        }
    }
    this.toString = function() { return myToString(this);};
}

function TestMessage (options) {

    this.payload=null;
    this.destinationName=null;
    this.qos=null;
    this.retained=null;

    this.catchFunction=null;
    this.message=null;
    this.toString = function() { return myToString(this);};

    if (options) {
        for (var i=0; i<options.length; i++) { 
            var property=options[i].property;
            var value=options[i].value;
            switch (property) {
                case "payload":
                    this.payload=value;
                    break;
                case "destinationName":
                    this.destinationName=value;
                    break;
                case "qos":
                    this.qos=value;
                    break;
                case "retained":
                    this.retained=value;
                    break;
                case "catchFunction":
                    this.catchFunction=value;
                    break;
            }
        }
    }
    
    // Create the message
    try {
        var message = new Messaging.Message(this.payload);
        if (this.destinationName) {
             message.destinationName=this.destinationName;
        }
        if (this.qos) {
             message.qos=this.qos;
        }
        if (this.retained) {
             message.retained=this.retained;
        }
        this.message=message;

    }
    catch (error) {
         if (this.catchFunction) {
             this.catchFunction(error);
         }
         else {
             fails++;
             writeLog(logfile,error);
         }
    }

}
         

function TestConnection (options) {

    this.connectOptions=new Object();
    this.catchFunction=null;
    
    // Read in options
    if (options) {
        for (var i=0; i<options.length; i++) {
            var property=options[i].property;
            value=options[i].value;
            switch (property) {
                case "timeout":
                    this.connectOptions.timeout=value;
                    break;
                case "cleanSession":
                    this.connectOptions.cleanSession=value;
                    break;
                case "userName":
                    this.connectOptions.userName=value;
                    break;
                case "password":
                    this.connectOptions.password=value;
                    break;
                case "keepAliveInterval":
                    this.connectOptions.keepAliveInterval=value;
                    break;
                case "willMessage":
                    this.connectOptions.willMessage=value;
                    break;        
                case "useSSL":
                    this.connectOptions.useSSL=value;
                    break;        
                case "invocationContext":
                    this.connectOptions.invocationContext=value;
                    break;        
                case "onSuccess":
                    this.connectOptions.onSuccess=value;
                    break;        
                case "onFailure":
                    this.connectOptions.onFailure=value;
                    break;        
                case "catchFunction":
                    this.catchFunction=value;
                    break;        
            }
        }
    }
}
    


// The first argument in the TestTopic constructor is always the topic name.
// Any additional options for the topic are specified as an array of Option
// objects.  
function TestTopic (topic_name,options) {

    this.name=topic_name;
    // Set up the array for received messages for this topic
    messages[topic_name]=0;
    messages_ret[topic_name]=0;
    
    // The default for expected retained messages should be 0 for every topic
    if (!expected_ret[topic_name]) {
        expected_ret[topic_name]=0;
    }

    // Set up the array for received messages of a particular qos for this topic
    message_qos[topic_name+'_0']=0;
    message_qos[topic_name+'_1']=0;
    message_qos[topic_name+'_2']=0;

    // Set default values
    this.publish=10;
    this.payload="default";
    this.expected=10;
    this.expected_ret=0;
    this.expected_qos=null;
    this.sub_onMessageArrived=null;
    this.sub_options=null;
    this.pub_qos=null;
    this.pub_retain=null;
    this.publish_only=false;
    this.subscribe_only=false;
    this.unsub_options=null;
    if (options)
    {
        for (var i=0; i<options.length; i++) {
            var property=options[i].property;
            var value=options[i].value;
            switch (property) {
                case "publish":
                    this.publish=value;
                    break;
                case "payload":
                    this.payload=value;
                    break;
                case "expected":
                    this.expected=value;
                    // Add the number of expected messages for this topic to the total number of expected messages
                    numExpectedMsgs+=value;
                    // Add the number of expected messages for this topic to the per-topic total number of expected messages 
                    if (!expected[topic_name]) {
                        expected[topic_name]=value;
                    }
                    else {
                        expected[topic_name]+=value;
                    }                
                    break;
                case "expected_ret":
                    this.expected_ret=value;
                    // Add the number of expected retained messages for this topic to the total number of expected retained messages
                    numExpectedRetMsgs+=value;
                    // Add the number of expected retained messages for this topic to the per-topic total
                    expected_ret[topic_name]+=value;
                    break;
                case "expected_qos":
                    this.expected_qos=value;
                    // Keep track of how many messages are expected per qos, per topic       
                    for (var j=0; j<value.length; j++) {
                        if (!expected_qos[topic_name+'_'+j]) {
                            expected_qos[topic_name+'_'+j]=value[j];
                        }
                        else {
                            expected_qos[topic_name+'_'+j]+=value[j];
                        }
                    }
                
                    break;
                case "sub_onMessageArrived":
                    this.sub_onMessageArrived=value;
                    break;
                case "sub_onsub":
                    if (!this.sub_options) {
                        this.sub_options=new Object();
                    }
                    this.sub_options.onSuccess=value;
                    break;
                case "sub_onSuccess":
                    if (!this.sub_options) {
                        this.sub_options=new Object();
                    }
                    this.sub_options.onSuccess=value;
                    break;
                case "sub_onFailure":
                    if (!this.sub_options) {
                        this.sub_options=new Object();
                    }
                    this.sub_options.onFailure=value;
                    break;
                case "sub_context":
                    if (!this.sub_options) {
                        this.sub_options=new Object();
                    }
                    this.sub_options.invocationContext=value;
                    break;
                case "sub_qos":
                    if (!this.sub_options) {
                        this.sub_options=new Object();
                    }
                    this.sub_options.qos=value;
                    break;
                case "pub_qos":
                    this.pub_qos=value;
                    break;
                case "pub_retain":
                    this.pub_retain=value;
                    break;
                case "publish_only":
                    this.publish_only=value;
                    break;
                case "subscribe_only":
                    this.subscribe_only=value;
                    break;
                case "onunsub":
                    if (!this.unsub_options) {
                        this.unsub_options=new Object();
                    }
                    this.unsub_options.onSuccess=value;
                    break;
            }

        } 
    }
}

function onConnectionLostSimple(clientId,code,message) {
    writeLog(logfile,'INFO: onConnectionLost was called for ' + clientId + '. Reason Code: ' + code + ', Reason Message: ' + message );
}

function onConnectSuccessSimple(context) {
    if (context) {
        writeLog(logfile,'INFO: The client has connected successfully. Context: ' + context.invocationContext.toString());     
    }
    else { 
        writeLog(logfile,'INFO: The client has connected successfully. No context was provided to the callback.');
    }
    
}

function onConnectFailureSimple(context,code,reason) {
    if (context) {
        writeLog(logfile,'INFO: The client has failed to connect. Code: ' + code + ', Reason: ' + reason + ', Context: ' + context.invocationContext.toString());     
    }
    else { 
        writeLog(logfile,'INFO: The client has failed to connect. Code: ' + code + ', Reason: ' + reason + '. No context was provided to the callback.');
    }
}

function endTest() {
    if (fails == 0 && checks == 0) {
        writeLog(logfile,'INFO: Client-side verification PASSED.');
    }
    else {
        writeLog(logfile,'ERROR: Test FAILED. There were ' + (fails + checks) + ' recorded failures.');
    }
    // Wait a bit before closing the window to let any logging catch up
    window.setTimeout(function() { window.close(); },1000);
}

function connect(client,connection) { 
    if (connection && connection instanceof TestConnection) {
        if (client && client instanceof TestClient) {
            try {
                var newclient = new Messaging.Client(client.host,client.port,client.clientId); 
                newclient.connect(connection.connectOptions);
                if (client.onMessageArrived) {
                    newclient.onMessageArrived=client.onMessageArrived;
                }
                if (client.onMessageDelivered) {
                    newclient.onMessageDelivered=client.onMessageDelivered;
                }
                if (client.onConnectionLost) {
                    newclient.onConnectionLost=client.onConnectionLost;
                }
                client.client=newclient;
            }
            catch (error) {
                if (connection.catchFunction) { 
                    connection.catchFunction(error);
                }
                else {
                    fails++;
                    writeLog(logfile,error);
                }
            }
        }    
        else {
            writeLog(logfile,'ERROR: Testcase error: A TestClient object must be provided when requesting connect');
        }
    }
    else {
        writeLog(logfile,'ERROR: Testcase error: A connection object must be provided when requesting connect');
    }
}

function disconnect(testClient) {
    if (testClient && testClient instanceof TestClient) {
        try {
            testClient.client.disconnect();
        }
        catch (error) {
            fails++;
            writeLog(logfile,error);
        }
    }
    else {
        writeLog(logfile,'ERROR: Testcase error: A TestClient object must be provided when requesting connect');
    }
}

function checkForEnd(endTestFunction) {
    // If there haven't been any new messages in the last interval,
    // end the test 
    var date = new Date();
    var date_string = date.getHours() + ':' + date.getMinutes() + ':' + date.getSeconds() + ':';
    if (numReceivedMsgs == lastNum) {
        writeLog(logfile,'INFO: ' + date_string + ' There were no new messages in the last interval. Number of messages received: ' + numReceivedMsgs + ', expected: ' + numExpectedMsgs);
        endTestFunction();
    }
    else {
        lastNum = numReceivedMsgs;
        writeLog(logfile,'INFO: ' + date_string + ' Number of messages received so far: ' + numReceivedMsgs);
    }
}

function setupDefaultTopic(topics) {
    topics.push(new TestTopic("simple topic"));
    numExpectedMsgs+=topics[0].expected;
    expected[topics[0].name]=topics[0].expected;
    expected_qos[topics[0].name+'_0']=topics[0].expected_qos[0];
    expected_qos[topics[0].name+'_1']=topics[0].expected_qos[1];
    expected_qos[topics[0].name+'_2']=topics[0].expected_qos[2];
    messages[topics[0].name]=0;
    message_qos[topics[0].name+'_0']=0;
    message_qos[topics[0].name+'_1']=0;
    message_qos[topics[0].name+'_2']=0;
}

function unsubscribeAll(test,topics,client) {
    for (var i=0; i<topics.length; i++) {
        writeLog(logfile,'INFO: About to unsubscribe from topic ' + topics[i].name);
        // Do not specify a callback if this is a publish only topic
        if (topics[i].publish_only == false) {
            client.unsubscribe(topics[i].name,topics[i].unsub_options);
        }
        else {
            client.unsubscribe(topics[i].name);
        }
        if (topics[i].sub_onMessageArrived) {
            writeLog(logfile,'INFO: About to remove listener ' + topics[i].sub_onMessageArrived + ' for ' + topics[i].name);
            var result=client.removeTopicListener(topics[i].name,topics[i].sub_onMessageArrived);
            if (result == true) {
                fails++;
                writeLog(logfile,'ERROR: Listeners remain for ' + topics[i].name);
            }
            else {
                writeLog(logfile,'INFO: No listeners remain for ' + topics[i].name);
            }
        }
    }
    // If this is test 22, repeat the unsubscribe
    if (test == "jsclient_pubsub_22_b") { 
        for (var i=0; i<topics.length; i++) {
            writeLog(logfile,'INFO: About to unsubscribe again from topic ' + topics[i].name);
            if (topics[i].publish_only == false){ 
                client.unsubscribe(topics[i].name,topics[i].unsub_options);
            }
            else {
                client.unsubscribe(topics[i].name);
            }
        }
    }
}

function checkMsgs() {

    // Check overall expected messages
    if (numReceivedMsgs != numExpectedMsgs) {
        fails++;
        writeLog(logfile, 'ERROR: Received messages (' + numReceivedMsgs + ') does not match the expected messages (' + numExpectedMsgs + ').'); 
    }
    
    // Check overall retained messages
    if (numRetainedMsgs != numExpectedRetMsgs) {
        fails++;
        writeLog(logfile, 'ERROR: Received retained messages (' + numRetainedMsgs + ') does not match the expected retained messages (' + numExpectedRetMsgs + ').'); 
    }

    // Check expected messages per topic 
    for ( topic in messages ) {
        if (messages[topic] != expected[topic]) {
            fails++;
            writeLog(logfile,'ERROR: Received messages (' + messages[topic] + ') does not match the expected messages (' + expected[topic] + ') for ' + topic + '.');
        }
        else {
            writeLog(logfile,'INFO: Topic ' + topic + ': Expected=' + expected[topic] + ' Received=' + messages[topic]);
        }
        if (messages_ret[topic] != expected_ret[topic]) {
            fails++;
            writeLog(logfile,'ERROR: Received retained messages (' + messages_ret[topic] + ') does not match the expected retained messages (' + expected_ret[topic] + ') for ' + topic + '.');
        }
        else {
            writeLog(logfile,'INFO: Topic ' + topic + ': Expected retained=' + expected_ret[topic] + ' Received retained=' + messages_ret[topic]);
        }
        for (var i=0; i<3; i++) {
            if (expected_qos[topic+'_' +i] != null){
                if (message_qos[topic+'_'+i] != expected_qos[topic+'_'+i]) {
                    fails++;
                    writeLog(logfile,'ERROR: Number of received messages at QoS=' + i + ' (' + message_qos[topic+'_'+i] + ') does not match the expected number (' + expected_qos[topic+'_'+i] +') for topic ' + topic);
                }
            }
        }
    }
}

function subscribeAll(topics,client) {
    for (var i=0; i<topics.length; i++) {
        if (!topics[i].publish_only) {
            var logmsg = 'INFO: About to subscribe: ';
            for ( prop in topics[i] ) {
                logmsg += prop + ': ' + topics[i][prop] + ' ';
            }
            writeLog(logfile,logmsg);
            client.subscribe(topics[i].name,topics[i].sub_options);
        }
    }
}

function publishAll(topics,client) {
    for (var i=0; i<topics.length; i++) {
        if (!topics[i].subscribe_only) {
            writeLog(logfile,'INFO: About to publish ' + topics[i].publish + ' messages to: ' + topics[i].name);
            for (var j=0; j<topics[i].publish; j++) {
                var payload;
                if (topics[i].payload == "default") {
                    payload="simple message" + (j+1);
                }
                else {
                    payload=topics[i].payload;
                }
                var message = new Messaging.Message(payload);
                message.destinationName=topics[i].name;
                if (topics[i].pub_qos) {
                    message.qos=topics[i].pub_qos;
                }
                try {
                    client.send(message);
                }
                catch (error) { 
                    fails++;
                    writeLog(logfile,error);
                }
            }
        }
    }
}

function setLog(test) {
    // Create the path for the logfile to make
    // sure it gets in the appropriate directory
    var curURL = window.location.pathname;
    prefix = curURL.substr(0,curURL.lastIndexOf('/')+1);
    baseName = test + ".log";
    logfile = prefix + "../" + baseName;
}

function onconnect_simple(client_id) {
    writeLog(logfile, 'INFO: Successfully connected \"' + client_id + '\" to ISM Server.');
}

function onMessageArrived_simple(message, client_id) {
    numReceivedMsgs++;
    messages[message.destinationName]++;
    var qos=0;
    if (message.qos) {
        qos=message.qos;
    }
    message_qos[message.destinationName+'_'+qos]++;

    var logmsg = 'TRACE: Received message for \"' + client_id + '\":\n';
    //for (prop in message) { 
        //logmsg = logmsg + prop + ': ' + message[prop] + '\n';
    //}
    logmsg = logmsg + 'destinationName: ' + message.destinationName + '\n qos: ' + message.qos + '\n retained: ' + message.retained + '\n duplicate: ' + message.duplicate + '\n payloadString: ' + message.payloadString + '\n payloadBytes: ' + message.payloadBytes;
    writeLog(logfile,logmsg);

    if (message.retained) {
        numRetainedMsgs++;
        messages_ret[message.destinationName]++;
        writeLog(logfile,'INFO: Detected retained message (retain=' + message.retained + ') with payload \"' + message.payloadString + '\"');
    }
}

function ontopicmessage(message) {
    numReceivedMsgs++;
    messages[message.destinationName]++;

    var qos=0;
    if (message.qos) {
        qos=message.qos;
    }
    message_qos[message.destinationName+'_'+qos]++;

    var logmsg = 'TRACE: Received message on subscription callback:\n';
    for (prop in message) { 
        logmsg = logmsg + prop + ': ' + message[prop] + '\n';
    }
    writeLog(logfile,logmsg);
}

function unsubscribeComplete() { 
    writeLog(logfile,'INFO: Unsubscribe request completed');
}

function onsub() {
    writeLog(logfile,'INFO: Subscribe request completed');
}

function onsub_topic2() {
    writeLog(logfile,'INFO: Subscribe request completed for topic2');
}

function onsub_topic3() {
    writeLog(logfile,'INFO: Subscribe request completed for topic3');
}

function onConnectionLost_simple(client_id,client_obj,reason) {
    writeLog(logfile,'WARN: \"' + client_id + '\" has been disconnected.');
    if (reason) {
        writeLog(logfile,'INFO: ' + reason.toString());
    }
}

function checkWaitResult(waitResult) {
    var worked = /Waited:\s1/g;
    var timed_out = /Waited:\s0/g;
    
    if (worked.exec(waitResult)) {
        writeLog(logfile,'INFO: Synchronization Succeeded');
    }
    else if (timed_out.exec(waitResult)) {
        fails++;
        writeLog(logfile,'ERROR: Synchronization Failed. Wait timed out. Result: ' + waitResult);
    }
    else {
        fails++;
        writeLog(logfile,'ERROR: Synchronization Failed. Result: ' + waitResult);
    }
}

function jsclient_pubsub(test, options) {

    fails=0;
    numReceivedMsgs=0;
    numExpectedMsgs=0; // This is the total number of expected messages
    messages = new Array();
    expected = new Array();
    message_qos = new Array();
    expected_qos = new Array();
    clientOnMessage="default";
    isSynced=false;
    
    var clientid="client_1";    
    var interval;
    var topics=new Array();
       

    setLog(test);

    // Parse the options
    if (options) {
        try {
           parseOptions(options,topics);
        }
        catch (error) {
           writeLog(logfile,error);
        }
    }

    // Create the client 
    var client = new Messaging.Client(imaserveraddr, imaserverport, clientid);

    // Default client listener
    if (clientOnMessage == "default") {
        clientOnMessage=function (msg) { onMessageArrived_simple(msg,clientid); };
    }

    // Default topic (used if no topics are passed in as options)
    if (topics.length == 0) {
        setupDefaultTopic(topics);
    }

    client.randomText = "someTextString";
    var connect_options = new Object();
    connect_options.onSuccess = function () {
        onconnect_simple(clientid);
        subscribeAll(topics,client);
	interval=window.setInterval(function () {checkForEnd(function () {endTest();},numExpectedMsgs,numReceivedMsgs);},1000);
        publishAll(topics,client);
    }    
    client.onConnectionLost = function (client_obj,reason) { onConnectionLost_simple(clientid,client_obj,reason); }
    client.onMessageArrived = clientOnMessage; 
    client.connect(connect_options);
    
}

function jsclient_pubsub_error_b(test, topics, action) {
    
    setLog(test);
    var fails = topics.length;
    var topicIndex = 0;

    function onSuccess() {
        topicIndex++;
        writeLog(logfile,'ERROR: A call to ' + action + ' was unexpectedly successful.');
        if (topicIndex < topics.length) {
            execute();
        }
        else {
            window.setTimeout(function () { endTest(); },1000);
        }
    }

    function onFailure() {
        topicIndex++;
        fails--;
        writeLog(logfile,'INFO: A call to ' + action + ' resulted in an invocation of the onFailure callback.');
        if (topicIndex < topics.length) {
            execute();
        }
        else {
            window.setTimeout(function () { endTest(); },1000);
        }
    }

    function catchError(error) {
        topicIndex++;
        fails--;
        writeLog(logfile,'INFO: A call to ' + action + ' resulted in the following error being thrown: ' + error);
        if (topicIndex < topics.length) {
            execute();
        }
        else {
            window.setTimeout(function () { endTest(); },1000);
        }
    }

    function setOptions(options) {
        if (!options){
            options = new Object();
            options.onSuccess = function () {onSuccess();};
            options.onFailure = function () {onFailure();};
        }
        else {
            if (!options.onSuccess) {
                options.onSuccess = function () {onSuccess();};
            }
            if (!options.onFailure) {
                options.onFailure = function () {onFailure();};
            }
        }
        return options;
    }

    function endTest() {
        writeLog(logfile,'TRACE: In endTest');

        // For a normal disconnect, use a different callback
        client.onConnectionLost = function(client_obj,reason) {
            onConnectionLost_simple("test_client",client_obj,reason);
        }
        writeLog(logfile,'TRACE: Disconnecting client');
        client.disconnect();

        if (fails == 0) {
            writeLog(logfile,'INFO: Client-side verification PASSED.');
        }
        else {
            writeLog(logfile,'ERROR: Test FAILED. There were '+fails+' recorded failures.');
        }
        // Wait a second before closing the window to let any logging catch up
        window.setTimeout(function() { window.close(); },1000);
    }

    function execute() { 
        writeLog(logfile,'TRACE: In execute');
        if (action == "unsubscribe") { 
            topics[topicIndex].unsub_options=setOptions(topics[topicIndex].unsub_options);
            writeLog(logfile,'INFO: About to call unsubscribe(' + topics[topicIndex].name + ',' + topics[topicIndex].unsub_options + ')');
            try {
                client.unsubscribe(topics[topicIndex].name,topics[topicIndex].unsub_options);
            }
            catch (error) {
                catchError(error);
            }
        }
        else if (action == "subscribe") { 
            topics[topicIndex].sub_options=setOptions(topics[topicIndex].sub_options);
            writeLog(logfile,'INFO: About to call subscribe(' + topics[topicIndex].name + ',' + topics[topicIndex].sub_options + ')');
            try {
                client.subscribe(topics[topicIndex].name,topics[topicIndex].sub_options);
            }
            catch (error) {
                catchError(error);
            }
        }
        else if (action == "send") { 
            var message = new Messaging.Message("sample message");
            try {
                message.destinationName = topics[topicIndex].name;
                if (topics[topicIndex].pub_qos) { 
                    message.qos=topics[topicIndex].pub_qos;
                }
                if (topics[topicIndex].pub_retain) { 
                    message.retained=topics[topicIndex].pub_retain;
                }
            }
            catch (error) {
                catchError('OnMessageConfig ' + error);
            }
            writeLog(logfile,'INFO: About to call send(' + message + ')');
            try {
                client.send(message);
            }
            catch (error) {
                catchError('OnSend ' + error);
            }
        }
    }
    
    var client = new Messaging.Client(imaserveraddr, imaserverport, "test_client");
   
    var connect_options=new Object(); 
    connect_options.onSuccess = function () {
        fails--;
        onconnect_simple("test_client");
        execute();
    }

    client.onConnectionLost = function(client_obj,reason) {
        writeLog(logfile,'INFO: A call to ' + action + ' resulted in a disconnection from the ISM Server.');
        onConnectionLost_simple("test_client",client_obj,reason);
        topicIndex++;
        fails--;
        if (topicIndex < topics.length) {
            writeLog(logfile,'INFO: Reconnecting');
            window.setTimeout(function () { fails++; client.connect(); },1000);
        }
        else {
            window.setTimeout(function () { endTest(); },1000);
        }
    }
    fails++; // Increment the number of fails, so that an error will be reported in the case that connect fails  
    client.connect(connect_options);
}

