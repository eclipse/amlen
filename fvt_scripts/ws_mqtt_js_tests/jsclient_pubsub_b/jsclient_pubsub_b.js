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
var numTl1ReceivedMsgs = 0;
var numTl2ReceivedMsgs = 0;
var messages = new Array();
var expected = new Array();
var message_qos = new Array();
var expected_qos = new Array();
var lastNum = 0;
var fails = 0;
var clientOnMessage = "default";
var isSynced = false;
var port=null;
var userName=null;
var password=null;

function myToString(obj) {
    var str="{";
    for (field in obj) {
        str=str + field + '=' + obj[field] + ';';
    }
    str=str+'}';
    return str;
}

Object.prototype.toString = function() {
    return myToString(this);
}

Event.prototype.toString = function() {
    return myToString(this);
}


function Option (property,value) {
    this.property=property;
    this.value=value;
}
// The first argument in the TestTopic constructor is always the topic name.
// Any additional options for the topic are specified as an array of Option
// objects.  
// Supported properties at this time are: 
//     publish: number of messages to publish on this topic
//     payload: the payload to use for the messages (if not specified, or "default" is specified, a default with an incrementing number will be used.
//     expected: number of messages this topic is expected to receive
//     expected_qos: an array where expected[n] is the number of expected messages to receive at qos=n
//     sub_onMessageArrived: the function to specify as the onMessageArrived callback
//         for this subscriber
//     sub_onsub: the function to specify as the callback when this
//         subscription completes
//     sub_qos: quality of service for this subscription
//     pub_qos: quality of service to use for this publisher 
//     publish_only: set to true if this topic only publishes 
//     subscribe_only: set to true if there are no publishes for this TestTopic
//     onunsub: the function to specify as the callback when the unsubscribe completes 
function TestTopic (topic_name,options) {

    this.name=topic_name;

    // Set default values
    this.publish=10;
    this.payload="default";
    this.expected=10;
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
                    break;
                case "expected_qos":
                    this.expected_qos=value;
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

function checkForEnd(endTestFunction,numExpected,numReceived) {
    // If there haven't been any new messages in the last 2 seconds,
    // end the test 
    if (numReceived == lastNum) {
        writeLog(logfile,'INFO: There were no new messages in the last interval. Number of messages received: ' + numReceived + ', expected: ' + numExpected);
        endTestFunction();
    }
    else {
        lastNum = numReceived;
        writeLog(logfile,'INFO: Number of messages received so far: ' + numReceived);
    }
}

// Parse a set of options
function parseOptions(options,topics) { 
    for (var i=0; i<options.length; i++) {
        if (options[i] instanceof TestTopic) {
            // Add this topic to the list
            topics.push(options[i]);
  
            // Add the number of expected messages for this topic option to the total number of expected messages
            numExpectedMsgs+=options[i].expected;

            // Add the number of expected messages for this topic option to the per-topic total number of expected messages 
            if (!expected[options[i].name]) {
                expected[options[i].name]=options[i].expected;
            }
            else {
                expected[options[i].name]+=options[i].expected;
            }
                
            // Keep track of how many messages are expected per qos, per topic       
            if (options[i].expected_qos != null) {
                for (var j=0; j<options[i].expected_qos.length; j++) {
                    if (!expected_qos[options[i].name+'_'+j]) {
                        expected_qos[options[i].name+'_'+j]=options[i].expected_qos[j];
                    }
                    else {
                        expected_qos[options[i].name+'_'+j]+=options[i].expected_qos[j];
                    }
                }
            }
                
            // Set up the array for received messages for this topic
            messages[options[i].name]=0;

            // Set up the array for received messages of a particular qos for this topic
            message_qos[options[i].name+'_0']=0;
            message_qos[options[i].name+'_1']=0;
            message_qos[options[i].name+'_2']=0;
        }

        // If the option is for the whole client, process it
        else if (options[i] instanceof Option) {
            
            var prop=options[i].property;
            var value=options[i].value;
            writeLog(logfile,"Prop: " + prop + ",Value: " + value);
            switch (prop) {
                case "onMessageArrived":
                    clientOnMessage=value;
                    break;
                case "isSynced":
                    isSynced=value;
                    break;
                case "port":
                    port=value;
                    break;
                case "address":
                    address=value;
                    break;
                case "userName":
                    userName=value;
                    break;
                case "password":
                    password=value;
                    break;
            }
        }
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

    // Check expected messages per topic 
    for ( topic in messages ) {
        if (messages[topic] != expected[topic]) {
            fails++;
            writeLog(logfile,'ERROR: Received messages (' + messages[topic] + ') does not match the expected messages (' + expected[topic] + ') for ' + topic + '.');
        }
        else {
            writeLog(logfile,'INFO: Topic ' + topic + ': Expected=' + expected[topic] + ' Received=' + messages[topic]);
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
            if (topics[i].sub_onMessageArrived) {
                writeLog(logfile,'INFO: About to add a message listener ' + topics[i].sub_onMessageArrived + ' for ' + topics[i].name);
                client.addTopicListener(topics[i].name,topics[i].sub_onMessageArrived);
            }
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
                client.send(message);
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

function onMessageArrived_simple(message, client_id, test) {
    numReceivedMsgs++;
    messages[message.destinationName]++;
    var qos=0;
    if (message.qos) {
        qos=message.qos;
    }
    message_qos[message.destinationName+'_'+qos]++;

    var logmsg = 'TRACE: Received message for \"' + client_id + '\":\n';
    for (prop in message) { 
        logmsg = logmsg + prop + ': ' + message[prop] + '\n';
    }
    writeLog(logfile,logmsg);

    if (message.retained) {
        writeLog(logfile,'INFO: Detected retained message (retain=' + message.retained + ') with payload \"' + message.payloadString + '\"');
    }
}

function onMessageArrived_11(message) {
    numReceivedMsgs++;
    messages[message.destinationName]++;
}

function topicListener1(message) {
    numTl1ReceivedMsgs++;
    writeLog(logfile,'TRACE: Received message on topicListener1');
}

function topicListener2(message) {
    numTl2ReceivedMsgs++;
    writeLog(logfile,'TRACE: Received message on topicListener2');
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
    port=null;
    address=null;
    userName=null;
    password=null;
    
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

try {
    // Create the client 
    if (!port) { 
        port=imaserverport; 
    }
    if (!address) { 
        address=imaserveraddr; 
    }
    var client = new Messaging.Client(address, port, clientid);

    // Default client listener
    if (clientOnMessage == "default") {
        clientOnMessage=function (msg) { onMessageArrived_simple(msg,clientid); };
    }

    // Default topic (used if no topics are passed in as options)
    if (topics.length == 0) {
        setupDefaultTopic(topics);
    }

    function endTest() {
        window.clearInterval(interval);
        if (test != "jsclient_pubsub_19_b") {
            try {
                unsubscribeAll(test,topics,client);
            }
            catch (error) {
                fails++;
                writeLog(logfile,error);
            }
        }
        client.disconnect();
        checkMsgs();       

	if (fails == 0) {
            writeLog(logfile,'INFO: Client-side verification PASSED.');
        }
        else {
            writeLog(logfile,'ERROR: Test FAILED. There were ' + fails + ' recorded failures.');
        }
        // Wait a bit before closing the window to let any logging catch up
        window.setTimeout(function() { window.close(); },1500);
    }
    client.randomText = "someTextString";
    var connect_options = new Object();
    connect_options.onSuccess = function () {
        onconnect_simple(clientid);
        subscribeAll(topics,client);
	interval=window.setInterval(function () {checkForEnd(function () {endTest();},numExpectedMsgs,numReceivedMsgs);},1000);
        publishAll(topics,client);
    }    
    if (userName) {
       connect_options.userName=userName;
    }
    if (password) {
       connect_options.password=password;
    }
    client.onConnectionLost = function (client_obj,reason) { onConnectionLost_simple(clientid,client_obj,reason); }
    client.onMessageArrived = clientOnMessage; 
    client.connect(connect_options);
}
catch (e) {
    writeLog(logfile,'ERROR: ' + e);
}
    
}

function jsclient_pubsub_1_b(test) {
    var topic_options = new Array();
    var expected_qos_opts = [10,0,0];
    topic_options.push(new Option("expected_qos",expected_qos_opts));
    var options = [new TestTopic("simple topic",topic_options)];
    jsclient_pubsub(test,options);
}

function jsclient_pubsub_2_b(test) {
    var options = new Array();
    var topic_options = new Array();
    var expected_qos_opts = [10,0,0];
    topic_options.push(new Option("pub_qos",1));
    topic_options.push(new Option("expected_qos",expected_qos_opts));
    options.push(new TestTopic("simple topic",topic_options));
    options.push(new Option("address",imaserveraddr_IPv6));
    jsclient_pubsub(test,options);
}

function jsclient_pubsub_3_b(test) {
    var topic_options = new Array();
    var expected_qos_opts = [10,0,0];
    topic_options.push(new Option("pub_qos",2));
    topic_options.push(new Option("expected_qos",expected_qos_opts));
    var options = [new TestTopic("simple topic",topic_options)];
    jsclient_pubsub(test,options);
}

function jsclient_pubsub_4_b(test) {
    var options = new Array();
    var topic_options = new Array();
    var expected_qos_opts = [10,0,0];
    topic_options.push(new Option("pub_qos",0));
    topic_options.push(new Option("expected_qos",expected_qos_opts));
    options.push(new TestTopic("topic one",topic_options));
    // Generate a topic name that is 32,767 chars long
    // Do not allow the null character, '+', or '#'
    var topic_name="";
    var numSlashes=0;
    for (var i=0; i<32767; i++) {
        var num=Math.round(Math.random()*94)+32;
        if (num == 35 || num == 43) {
            num++;
        }
        // Topic cannot begin with $ or it's a system topic
        if (i == 0 && num == 36) {
            num++;
        }
        // Allow a max of 31 slashes
        if (num == 47) {
            numSlashes++;
            if (numSlashes > 31) {
                num++;
            }
        }
	topic_name=topic_name + String.fromCharCode(num);
    }
    options.push(new TestTopic(topic_name,topic_options));
    options.push(new TestTopic("this/is/a/topic",topic_options));
    options.push(new Option("address",imaserveraddr_IPv6));
    jsclient_pubsub(test,options);
}

function jsclient_pubsub_5_b(test) {
    var options = new Array();
    var topic_options = new Array();
    var expected_qos_opts = [10,0,0];
    topic_options.push(new Option("sub_qos",0));
    topic_options.push(new Option("expected_qos",expected_qos_opts));
    options.push(new TestTopic("TOPIC1",topic_options));
    options.push(new TestTopic("topic1",topic_options));
    jsclient_pubsub(test,options);
}

function jsclient_pubsub_8_b(test) {
    var options = new Array();
    var topic_options = new Array();
    topic_options.push(new Option("sub_onsub",function () { onsub(); }));
    options.push(new TestTopic("topic_1",topic_options));
    options.push(new Option("address",imaserveraddr_IPv6));
    jsclient_pubsub(test,options);
}

function jsclient_pubsub_10_b(test) {
    var client_options = new Array();
    var topic2_options = new Array();
    var topic3_options = new Array();

    topic2_options.push(new Option("sub_onsub",function () { onsub_topic2(); }));
    topic3_options.push(new Option("sub_onsub",function () { onsub_topic3(); }));
    client_options.push(new TestTopic("topic1"));
    client_options.push(new TestTopic("topic2",topic2_options));
    client_options.push(new TestTopic("topic3",topic3_options));
    client_options.push(new Option("address",imaserveraddr_IPv6));
    jsclient_pubsub(test,client_options);
}

function jsclient_pubsub_11_b(test) {
    // This test does not use the jsclient_pubsub function as many
    // of the other happy path tests do
    var client_options = new Array();
    var pubtopic_options = [ new Option("publish_only",true) ];
    var pubtopic_expected = new Array();
    var pub_name="/topic/ready";
    pubtopic_expected[pub_name]=([10,10,10,0,10,0,0,0,0]);
    client_options.push(new TestTopic(pub_name,pubtopic_options));

    pub_name="/topic/set";
    pubtopic_expected[pub_name]=([10,10,10,0,10,0,0,0,0]);
    client_options.push(new TestTopic(pub_name,pubtopic_options));

    pub_name="topic/go";
    pubtopic_expected[pub_name]=([10,0,0,0,0,0,10,0,0]);
    client_options.push(new TestTopic(pub_name,pubtopic_options));

    pub_name="/topic/1/2/3";
    pubtopic_expected[pub_name]=([10,10,0,0,10,0,0,0,0]);
    client_options.push(new TestTopic(pub_name,pubtopic_options));

    pub_name="/topic/1/2";
    pubtopic_expected[pub_name]=([10,10,0,10,10,10,0,0,0]);
    client_options.push(new TestTopic(pub_name,pubtopic_options));

    pub_name="/";
    pubtopic_expected[pub_name]=([10,10,0,0,0,0,10,0,0]);
    client_options.push(new TestTopic(pub_name,pubtopic_options));

    pub_name="/topic/2/2";
    pubtopic_expected[pub_name]=([10,10,0,10,10,10,0,0,0]);
    client_options.push(new TestTopic(pub_name,pubtopic_options));

    pub_name="/topic/3/2";
    pubtopic_expected[pub_name]=([10,10,0,10,10,10,0,0,0]);
    client_options.push(new TestTopic(pub_name,pubtopic_options));

    pub_name="/topic/4/2/";
    pubtopic_expected[pub_name]=([10,10,0,0,10,0,0,0,0]);
    client_options.push(new TestTopic(pub_name,pubtopic_options));

    var subtopic_options = new Array();
    subtopic_options[0]=new Option("subscribe_only",true);
    subtopic_options[1]=new Option("expected",90);
    client_options.push(new TestTopic("#",subtopic_options));
    subtopic_options[1].value=80;
    client_options.push(new TestTopic("/#",subtopic_options));
    subtopic_options[1].value=20;
    client_options.push(new TestTopic("/topic/+",subtopic_options));
    subtopic_options[1].value=30;
    client_options.push(new TestTopic("/+/+/+",subtopic_options));
    subtopic_options[1].value=70;
    client_options.push(new TestTopic("/+/+/#",subtopic_options));
    subtopic_options[1].value=30;
    client_options.push(new TestTopic("/topic/+/2",subtopic_options));
    subtopic_options[1].value=20;
    client_options.push(new TestTopic("+/+",subtopic_options));
    subtopic_options[1].value=0;
    client_options.push(new TestTopic("t/#",subtopic_options));
    subtopic_options[1].value=0;
    client_options.push(new TestTopic("+/g",subtopic_options));

    fails=0;

    setLog(test);

    numExpectedMsgs=0; // This is the total number of expected messages
    clientOnMessage="default";
    var clientid = "client_1";    
    var subscribers = new Array();
    var publishers = new Array();
    // Parse the options
    for (var i=0; i<client_options.length; i++) {
        if (client_options[i] instanceof TestTopic) {
            if (client_options[i].publish_only) {
                publishers.push(client_options[i]);
                messages[client_options[i].name]=0;
            }
            else if (client_options[i].subscribe_only) {
                subscribers.push(client_options[i]);
            }
            else {
                writeLog(logfile,'ERROR: Testcase error. Topics must be either publisher or subscriber for this test (not both).');
                fails++;
            }
        }
    }

    var client = new Messaging.Client(imaserveraddr, imaserverport, clientid);
    // Default client listener
    if (clientOnMessage == "default") {
        clientOnMessage=function (msg) { onMessageArrived_11(msg); };
    }

    function endTest() {
        client.disconnect();
	if (fails == 0) {
            writeLog(logfile,'INFO: Client-side verification PASSED.');
        }
        else {
            writeLog(logfile,'ERROR: Test FAILED. There were ' + fails + ' recorded failures.');
        }
        // Wait a second before closing the window to let any logging catch up
        window.setTimeout(function() { window.close(); },1000);
    }
    
    var subscriber_num = 0;
    var interval;

    function run() {
        numExpectedMsgs = subscribers[subscriber_num].expected;
        numReceivedMsgs = 0;
        messages = new Array();
        for (var i=0; i<publishers.length; i++) {
            messages[publishers[i].name]=0;
            expected[publishers[i].name]=pubtopic_expected[publishers[i].name][subscriber_num];
        }
        writeLog(logfile,'INFO: Running test for topic filter: ' + subscribers[subscriber_num].name);
        client.subscribe(subscribers[subscriber_num].name,subscribers[subscriber_num].sub_options);
	interval=window.setInterval(function () {checkForEnd(function () {nextRun();},numExpectedMsgs,numReceivedMsgs);},1000);
        publishAll(publishers,client);
    }
    function nextRun() {
        window.clearInterval(interval);
        // Unsubscribe from the latest subscription
        client.unsubscribe(subscribers[subscriber_num].name);
        checkMsgs();
        subscriber_num++;
        if (subscriber_num == subscribers.length) {
            endTest();
        }
        else {
            run();
        }
    }
    var connect_options = new Object();
    connect_options.onSuccess = function () { 
        var interval;
        onconnect_simple(clientid);
        run();
    }    
    client.onConnectionLost = function (client_obj,reason) { onConnectionLost_simple(clientid,client_obj,reason); }
    client.onMessageArrived = clientOnMessage; 
    client.connect(connect_options);
    
}

function jsclient_pubsub_12_b(test) {
    var client_options = new Array();
    var publish1_options = new Array();
    var publish2_options = new Array();
    var publish3_options = new Array();
    var subscribe_options = new Array();
    var expected_qos_opts = [10,0,0];
    
    publish1_options.push(new Option("pub_qos",0));
    publish1_options.push(new Option("expected_qos",expected_qos_opts));
    publish1_options.push(new Option("publish_only",true));
    publish2_options.push(new Option("pub_qos",1));
    publish2_options.push(new Option("expected_qos",expected_qos_opts));
    publish2_options.push(new Option("publish_only",true));
    publish3_options.push(new Option("pub_qos",2));
    publish3_options.push(new Option("expected_qos",expected_qos_opts));
    publish3_options.push(new Option("publish_only",true));

    subscribe_options.push(new Option("sub_qos",0));
    subscribe_options.push(new Option("expected",0));
    subscribe_options.push(new Option("subscribe_only",true));

    client_options.push(new TestTopic("topic1",publish1_options));
    client_options.push(new TestTopic("topic1",publish2_options));
    client_options.push(new TestTopic("topic1",publish3_options));
    client_options.push(new TestTopic("topic1",subscribe_options));
    client_options.push(new Option("address",imaserveraddr_IPv6));

    jsclient_pubsub(test,client_options);
}

function jsclient_pubsub_13_b(test) {
    var client_options = new Array();
    var publish1_options = new Array();
    var publish2_options = new Array();
    var publish3_options = new Array();
    var subscribe_options = new Array();
    var expected_qos_publish1 = [10,0,0];
    var expected_qos_publish2 = [0,10,0];
    var expected_qos_publish3 = [0,10,0];
    
    publish1_options.push(new Option("pub_qos",0));
    publish1_options.push(new Option("expected_qos",expected_qos_publish1));
    publish1_options.push(new Option("publish_only",true));
    publish2_options.push(new Option("pub_qos",1));
    publish2_options.push(new Option("expected_qos",expected_qos_publish2));
    publish2_options.push(new Option("publish_only",true));
    publish3_options.push(new Option("pub_qos",2));
    publish3_options.push(new Option("expected_qos",expected_qos_publish3));
    publish3_options.push(new Option("publish_only",true));

    subscribe_options.push(new Option("sub_qos",1));
    subscribe_options.push(new Option("expected",0));
    subscribe_options.push(new Option("subscribe_only",true));

    client_options.push(new TestTopic("topic1",publish1_options));
    client_options.push(new TestTopic("topic1",publish2_options));
    client_options.push(new TestTopic("topic1",publish3_options));
    client_options.push(new TestTopic("topic1",subscribe_options));

    jsclient_pubsub(test,client_options);
}

function jsclient_pubsub_14_b(test) {
    var client_options = new Array();
    var publish1_options = new Array();
    var publish2_options = new Array();
    var publish3_options = new Array();
    var publish4_options = new Array();
    var publish5_options = new Array();
    var publish6_options = new Array();
    var subscribe_options = new Array();
    var expected_qos_publish1 = [10,0,0];
    var expected_qos_publish2 = [0,10,0];
    var expected_qos_publish3 = [0,0,10];
    
    publish1_options.push(new Option("pub_qos",0));
    publish1_options.push(new Option("expected_qos",expected_qos_publish1));
    publish1_options.push(new Option("publish_only",true));
    publish2_options.push(new Option("pub_qos",1));
    publish2_options.push(new Option("expected_qos",expected_qos_publish2));
    publish2_options.push(new Option("publish_only",true));
    publish3_options.push(new Option("pub_qos",2));
    publish3_options.push(new Option("expected_qos",expected_qos_publish3));
    publish3_options.push(new Option("publish_only",true));

    subscribe_options.push(new Option("sub_qos",2));
    subscribe_options.push(new Option("expected",0));
    subscribe_options.push(new Option("subscribe_only",true));

    client_options.push(new TestTopic("topic1",publish1_options));
    client_options.push(new TestTopic("topic1",publish2_options));
    client_options.push(new TestTopic("topic1",publish3_options));
    client_options.push(new TestTopic("topic1",subscribe_options));
    client_options.push(new Option("address",imaserveraddr_IPv6));

    jsclient_pubsub(test,client_options);
}

function jsclient_pubsub_15_b(test) {
    var client_options = new Array();
    var topic1_options = new Array();
    var topic2_options = new Array();
    var topic3_options = new Array();
    var expected_qos_topic1 = [10,0,0];
    var expected_qos_topic2 = [0,10,0];
    var expected_qos_topic3 = [0,0,10];

    topic1_options.push(new Option("pub_qos",0));
    topic1_options.push(new Option("sub_qos",0));
    topic1_options.push(new Option("expected_qos",expected_qos_topic1));
    topic2_options.push(new Option("pub_qos",1));
    topic2_options.push(new Option("sub_qos",1));
    topic2_options.push(new Option("expected_qos",expected_qos_topic2));
    topic3_options.push(new Option("pub_qos",2));
    topic3_options.push(new Option("sub_qos",2));
    topic3_options.push(new Option("expected_qos",expected_qos_topic3));
    
    client_options.push(new TestTopic("topic1",topic1_options));
    client_options.push(new TestTopic("topic2",topic2_options));
    client_options.push(new TestTopic("topic3",topic3_options));
 
    jsclient_pubsub(test,client_options);
}

function jsclient_pubsub_16_b(test) {
    var client_options = new Array();
    var topic1_options = new Array();
    var topic2_options = new Array();
    var topic3_options = new Array();
    var subscriber_options = new Array();
    var expected_qos_topic1 = [10,0,0];
    var expected_qos_topic2 = [10,0,0];
    var expected_qos_topic3 = [10,0,0];

    topic1_options.push(new Option("pub_qos",0));
    topic1_options.push(new Option("expected_qos",expected_qos_topic1));
    topic1_options.push(new Option("publish_only",true));
    topic2_options.push(new Option("pub_qos",1));
    topic2_options.push(new Option("expected_qos",expected_qos_topic2));
    topic2_options.push(new Option("publish_only",true));
    topic3_options.push(new Option("pub_qos",2));
    topic3_options.push(new Option("expected_qos",expected_qos_topic3));
    topic3_options.push(new Option("publish_only",true));
    subscriber_options.push(new Option("sub_qos",0));
    subscriber_options.push(new Option("expected",0));
    subscriber_options.push(new Option("subscribe_only",true));
  
    client_options.push(new TestTopic("topic/0/1",topic1_options));
    client_options.push(new TestTopic("topic/0/2",topic2_options));
    client_options.push(new TestTopic("topic/0/3",topic3_options));
    client_options.push(new TestTopic("topic/0/+",subscriber_options));
    client_options.push(new Option("address",imaserveraddr_IPv6));

    subscriber_options = new Array();
    var topic4_options = new Array();
    var topic5_options = new Array();
    var topic6_options = new Array();
    var expected_qos_topic4 = [10,0,0];
    var expected_qos_topic5 = [0,10,0];
    var expected_qos_topic6 = [0,10,0];
    topic4_options.push(new Option("pub_qos",0));
    topic4_options.push(new Option("expected_qos",expected_qos_topic4));
    topic4_options.push(new Option("publish_only",true));
    topic5_options.push(new Option("pub_qos",1));
    topic5_options.push(new Option("expected_qos",expected_qos_topic5));
    topic5_options.push(new Option("publish_only",true));
    topic6_options.push(new Option("pub_qos",2));
    topic6_options.push(new Option("expected_qos",expected_qos_topic6));
    topic6_options.push(new Option("publish_only",true));
    subscriber_options.push(new Option("sub_qos",1));
    subscriber_options.push(new Option("expected",0));
    subscriber_options.push(new Option("subscribe_only",true));
   
    client_options.push(new TestTopic("topic/1/4",topic4_options));
    client_options.push(new TestTopic("topic/1/5",topic5_options));
    client_options.push(new TestTopic("topic/1/6",topic6_options));
    client_options.push(new TestTopic("topic/1/+",subscriber_options));

   subscriber_options = new Array();
    var topic7_options = new Array();
    var topic8_options = new Array();
    var topic9_options = new Array();
    var expected_qos_topic7 = [10,0,0];
    var expected_qos_topic8 = [0,10,0];
    var expected_qos_topic9 = [0,0,10];
    topic7_options.push(new Option("pub_qos",0));
    topic7_options.push(new Option("expected_qos",expected_qos_topic7));
    topic7_options.push(new Option("publish_only",true));
    topic8_options.push(new Option("pub_qos",1));
    topic8_options.push(new Option("expected_qos",expected_qos_topic8));
    topic8_options.push(new Option("publish_only",true));
    topic9_options.push(new Option("pub_qos",2));
    topic9_options.push(new Option("expected_qos",expected_qos_topic9));
    topic9_options.push(new Option("publish_only",true));
    subscriber_options.push(new Option("sub_qos",2));
    subscriber_options.push(new Option("expected",0));
    subscriber_options.push(new Option("subscribe_only",true));
   
    client_options.push(new TestTopic("topic/2/7",topic7_options));
    client_options.push(new TestTopic("topic/2/8",topic8_options));
    client_options.push(new TestTopic("topic/2/9",topic9_options));
    client_options.push(new TestTopic("topic/2/+",subscriber_options));

    jsclient_pubsub(test,client_options);
}

function jsclient_pubsub_17_b(test) {
    // This test does not use the jsclient_pubsub function as many
    // of the other happy path tests do

    fails=0;
    numExpectedMsgs=0; // This is the total number of expected messages
    clientOnMessage=function (msg) { onMessageArrived_simple(msg, clientid); };
    // The retained message tests should use a topic specific to the test, so that they do not interfere with other tests
    var topicName=test+'_topic';

    setLog(test);

    var clientid = "client_1";    
    var client = new Messaging.Client(imaserveraddr, imaserverport, clientid);
    var interval;

    function endTest() {
        window.clearInterval(interval);
        writeLog(logfile,'INFO: About to unsubscribe from ' + topicName);
        client.unsubscribe(topicName);
        // Check overall expected messages
        if (numReceivedMsgs != numExpectedMsgs) {
            fails++;
            writeLog(logfile, 'ERROR: Total received retained messages (' + numReceivedMsgs + ') does not match the expected messages (' + numExpectedMsgs + ').');
        }
        client.disconnect();
	if (fails == 0) {
            writeLog(logfile,'INFO: Client-side verification PASSED.');
        }
        else {
            writeLog(logfile,'ERROR: Test FAILED. There were ' + fails + ' recorded failures.');
        }
        // Wait a second before closing the window to let any logging catch up
        window.setTimeout(function() { window.close(); },1000);
    }
    

    function run() {
        numExpectedMsgs=10;
        writeLog(logfile,'INFO: About to subscribe to ' + topicName);
        client.subscribe(topicName);
        writeLog(logfile,'INFO: About to publish ' + numExpectedMsgs + ' messages to ' + topicName);
        for (var i=0; i<numExpectedMsgs; i++) {
            var payload;
            var retained=true;
            if (i%2 == 0) {
                if (test == "jsclient_pubsub_18_b") {
                    payload="simple message";
                }
                else {
                    payload="";
                }
            }
            else {
                payload="simple message";
                if (test == "jsclient_pubsub_18_b") { 
                    retained=false;
                }
            }
            var message = new Messaging.Message(payload);
            message.destinationName=topicName;
            message.retained=retained;
            client.send(message);
        }
	interval=window.setInterval(function () {checkForEnd(function () {nextRun();},numExpectedMsgs,numReceivedMsgs);},1000);
    }
    function nextRun() {
        window.clearInterval(interval);
        writeLog(logfile,'INFO: About to unsubscribe from ' + topicName);
        client.unsubscribe(topicName);
        // Check overall expected messages
        if (numReceivedMsgs != numExpectedMsgs) {
            fails++;
            writeLog(logfile, 'ERROR: Total received messages (' + numReceivedMsgs + ') does not match the expected messages (' + numExpectedMsgs + ').');
        }
        numReceivedMsgs=0;
        numExpectedMsgs=1;

        writeLog(logfile,'INFO: About to resubscribe to ' + topicName);
        client.subscribe(topicName);
	interval=window.setInterval(function () {checkForEnd(function () {endTest();},numExpectedMsgs,numReceivedMsgs);},1000);
    }

    var connect_options = new Object();
    connect_options.onSuccess = function () { 
        onconnect_simple(clientid);
        run();
    }    
    client.onConnectionLost = function (client_obj,reason) { onConnectionLost_simple(clientid,client_obj,reason); }
    client.onMessageArrived = clientOnMessage; 
    client.connect(connect_options);
    
}

function jsclient_pubsub_18_b(test) {
    jsclient_pubsub_17_b(test);
}

function jsclient_pubsub_19_b(test) { 
    jsclient_pubsub_1_b(test);
}

function jsclient_pubsub_22_b(test) {
    jsclient_pubsub_1_b(test); 
}

function jsclient_pubsub_24_b(test) {

    fails=0;
    numReceivedMsgs=0;
    numExpectedMsgs=0; // This is the total number of expected messages
    messages = new Array();
    expected = new Array();
    message_qos = new Array();
    expected_qos = new Array();
    isSynced=false;
    
    var client1_options = new Array();
    client1_options.push(new Option("isSynced",true));
    var publisher_options = new Array();
    publisher_options.push(new Option("publish_only",true));
    publisher_options.push(new Option("expected",0));
    client1_options.push(new TestTopic("topic1",publisher_options));
    var client1_id="client1";
    var client1_topics=new Array();
    
    var client2_options = new Array();
    client2_options.push(new Option("isSynced",true));
    var subscriber_options = new Array();
    subscriber_options.push(new Option("subscribe_only",true));
    client2_options.push(new TestTopic("topic1",subscriber_options));
    var client2_id="client2";
    var client2_topics=new Array();
       
    var interval;       

    setLog(test);
    // Initialize the sync variable that will be used
    //requestSync(logfile,'1 client2_subscribe_all');

    // Parse the options
    try {
       parseOptions(client1_options,client1_topics);
       parseOptions(client2_options,client2_topics);
    }
    catch (error) {
       writeLog(logfile,error);
    }

    // Create the clients 
    var client1 = new Messaging.Client(imaserveraddr, imaserverport, client1_id);
    var client2 = new Messaging.Client(imaserveraddr, imaserverport, client2_id);

    function endTest() {
        window.clearInterval(interval);
        try {
            unsubscribeAll(test,client2_topics,client2);
        }
        catch (error) {
            fails++;
            writeLog(logfile,error);
        }
        client1.disconnect();
        client2.disconnect();
        checkMsgs();       

	if (fails == 0) {
            writeLog(logfile,'INFO: Client-side verification PASSED.');
        }
        else {
            writeLog(logfile,'ERROR: Test FAILED. There were ' + fails + ' recorded failures.');
        }
        // Wait a bit before closing the window to let any logging catch up
        window.setTimeout(function() { window.close(); },1500);
    }

    var client1_cxoptions = new Object();
    client1_cxoptions.onSuccess = function () {
        onconnect_simple(client1_id);
        requestSync(logfile,'2 client1_connected 1');
        requestSync(logfile,'3 client2_subscribe_all 1 3000',function (result) {checkWaitResult(result); publishAll(client1_topics,client1);});
    }    
    var client2_cxoptions = new Object();
    client2_cxoptions.onSuccess = function () {
        onconnect_simple(client2_id);
        subscribeAll(client2_topics,client2);
        requestSync(logfile,'3 client1_connected 1 5000', 
          function (result) {
            checkWaitResult(result); 
            requestSync(logfile,'2 client2_subscribe_all 1');
	        interval=window.setInterval(function () {checkForEnd(function () {endTest();},numExpectedMsgs,numReceivedMsgs);},1000);
          });
    }    
    client1.onConnectionLost = function (client_obj,reason) { onConnectionLost_simple(client1_id,client_obj,reason); }
    client2.onConnectionLost = function (client_obj,reason) { onConnectionLost_simple(client2_id,client_obj,reason); }
    client1.onMessageArrived = function (msg) { onMessageArrived_simple(msg,client1_id); }; 
    client2.onMessageArrived = function (msg) { onMessageArrived_simple(msg,client2_id); }; 
    client1.connect(client1_cxoptions);
    client2.connect(client2_cxoptions);
    
    
}

function jsclient_pubsub_25_b(test) {
    var topic_options = new Array();
    var options = new Array();
    var expected_qos_opts = [10,0,0];
    topic_options.push(new Option("expected_qos",expected_qos_opts));
    options.push(new TestTopic("simple topic",topic_options));
    options.push(new Option("port",25200));
    options.push(new Option("address",imaserveraddr_IPv6));
    jsclient_pubsub(test,options);
}

function jsclient_pubsub_26_b(test) {
    var topic_options = new Array();
    var options = new Array();
    var expected_qos_opts = [10,0,0];
    topic_options.push(new Option("expected_qos",expected_qos_opts));
    options.push(new TestTopic("simple topic",topic_options));
    options.push(new Option("port",26200));
    jsclient_pubsub(test,options);
}

function jsclient_pubsub_27_b(test) {
    var topic_options = new Array();
    var options = new Array();
    var expected_qos_opts = [10,0,0];
    topic_options.push(new Option("expected_qos",expected_qos_opts));
    options.push(new TestTopic("jsclient_pubsub_27_b_topic1",topic_options));
    options.push(new Option("port",27200));
    options.push(new Option("userName","user1"));
    options.push(new Option("password","pwuser1"));
    options.push(new Option("address",imaserveraddr_IPv6));
    jsclient_pubsub(test,options);
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
        
        if (client.connected == true) { 
            writeLog(logfile,'TRACE: Disconnecting client');
            client.disconnect();
        }

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
            window.setTimeout(function () { fails++; client.connect(connect_options); },1000);
        }
        else {
            window.setTimeout(function () { endTest(); },1000);
        }
    }
    fails++; // Increment the number of fails, so that an error will be reported in the case that connect fails  
    client.connect(connect_options);

    

}

function jsclient_pubsub_e1_b(test) {

    var topics = new Array();
    var topic_name = "";
    var numSlashes = 0;

    // Create a 65537 character topic
    for (var i=0; i<65537; i++) {
        var num=Math.round(Math.random()*94)+32;
        if (num == 35 || num == 43) {
            num++;
        }
        if (num == 47) {
            numSlashes++;
            if (numSlashes > 31) {
                num++;
            }
        }
        topic_name=topic_name + String.fromCharCode(num);
    }

    //topics.push(new TestTopic(topic_name));
    // JS client basically can't do this anymore - for xtra long
    // topic names, 2 bytes become the topic name, and the
    // rest become part of the payload.
    topics.push(new TestTopic(""));
    topics.push(new TestTopic("a/#"));
    topics.push(new TestTopic("a/+"));

    jsclient_pubsub_error_b(test,topics,"send");
}


function jsclient_pubsub_e2_b(test) {

    var topics = new Array();
    var topic_options0 = [new Option("pub_qos",5)];
    var topic_options1 = [new Option("pub_qos","abcdef")];
    var topic_options2 = [new Option("pub_qos",-1)];

    topics.push(new TestTopic("sample topic",topic_options0));
    topics.push(new TestTopic("sample topic",topic_options1));
    topics.push(new TestTopic("sample topic",topic_options2));

    jsclient_pubsub_error_b(test,topics,"send");
}

function jsclient_pubsub_e3_b(test) {

    var topic = new Array();
    var retain = 5;
    var topic_options = new Array();
    topic_options.push(new Option("pub_qos",1));
    topic_options.push(new Option("pub_retain",retain));

    topic.push(new TestTopic("sample topic",topic_options));

    jsclient_pubsub_error_b(test,topic,"send");
}

function jsclient_pubsub_e4_b(test) {

    var topics = new Array();

    topics.push(new TestTopic("topic/#/invalidwc"))
    topics.push(new TestTopic("topic#/invalidwc"));
    topics.push(new TestTopic("topic+/invalidwc"));

    jsclient_pubsub_error_b(test,topics,"subscribe");
}

function jsclient_pubsub_e6_b(test) {
    var topics = new Array();
    var topic_options0 = [new Option("sub_qos","abcdef")];
    var topic_options1 = [new Option("sub_qos",4)];

    topics.push(new TestTopic("sample topic",topic_options0));
    topics.push(new TestTopic("sample topic",topic_options1));

    jsclient_pubsub_error_b(test,topics,"subscribe");
}

function jsclient_pubsub_e7_b(test) {

    var topics = new Array();
    var notafunction = true;

    var topic_options = new Array();

    topic_options.push(new Option("sub_onsub",notafunction));
    topics.push(new TestTopic("topic1",topic_options));

    jsclient_pubsub_error_b(test,topics,"subscribe");
}

function jsclient_pubsub_e8_b(test) {

    fails=1;
    setLog(test);
    var topics = new Array();
    var client = new Messaging.Client(imaserveraddr, imaserverport, "test_client");

    // Set up the end test callback
    function endTest() {
        if (fails == 0) {
            writeLog(logfile,'INFO: Client-side verification PASSED.');
        } else {
            writeLog(logfile,'ERROR: There were '+fails+' reported failures.');
        }
        window.setTimeout(function() { window.close(); },1000);
    }

    // Set a non-function callback for onMessageArrived
    writeLog(logfile,'INFO: Setting onMessageArrived to \"not a function\"');
    try { 
        client.onMessageArrived="not a function";
    }
    catch (error) {
        fails--;
        writeLog(logfile,'INFO: Setting onMessageArrived resulted in the following error being thrown: ' + error);
    }
    window.setTimeout(function () { endTest(); },2000);
}

function jsclient_pubsub_e11_b(test) {
    fails=4;
    setLog(test);

    function onSuccess() {
        writeLog(logfile,'ERROR: A call was unexpectedly successful.');
    }

    function onFailure() {
        fails--;
        writeLog(logfile,'INFO: A call resulted in an invocation of the onFailure callback.');
    }

    // Create the client
    var client = new Messaging.Client(imaserveraddr, imaserverport, "test_client");
    var options = new Object();
    options.onSuccess = function () { onSuccess(); };
    options.onFailure = function () { onFailure(); };


    // Set up the end test callback
    function endTest() {
        client.disconnect();
        if (fails == 0) {
            writeLog(logfile,'INFO: Client-side verification PASSED.');
        } else {
            writeLog(logfile,'ERROR: There were '+fails+' reported failures.');
        }
        window.setTimeout(function() { window.close(); },1000);
    }

    // Attempt to subscribe, publish and unsubscribe after disconnecting
    var connect_options = new Object();
    connect_options.onSuccess = function() {
        onconnect_simple("test_client");
        try { 
            client.disconnect();
        }
        catch (error) {
            writeLog(logfile,error);
        }
    }

    client.onConnectionLost = function(client_obj,reason) {
        onConnectionLost_simple("test_client",client_obj,reason);
        fails--;
        try {
            writeLog(logfile,'TRACE: About to attempt subscribe after disconnect');
            client.subscribe("sample topic",options);
        } catch (errSub) {
            writeLog(logfile,errSub);
            fails--;
        }
        try {
            var message = new Messaging.Message("sample message");
            message.destinationName="sample topic";
            writeLog(logfile,'TRACE: About to attempt send after disconnect');
            client.send(message);
        } catch (errPub) {
            writeLog(logfile,errPub);
            fails--;
        }
        try {
            writeLog(logfile,'TRACE: About to attempt unsubscribe after disconnect');
            client.unsubscribe("a/b",options);
        } catch (errUnsub) {
            writeLog(logfile,errUnsub);
            fails--;
        }
        window.setTimeout(function () { endTest(); },2000);
    }

    // Connect
    client.connect(connect_options);
}

function jsclient_pubsub_e13_b(test) {

    var topics = new Array();
    var notafunction = true;

    var topic_options = new Array();

    topic_options.push(new Option("onunsub",notafunction));
    topics.push(new TestTopic("topic1",topic_options));

    jsclient_pubsub_error_b(test,topics,"unsubscribe");
}

function jsclient_pubsub_e14_b(test) {
    var topics = new Array();
    var topic_options = new Array();

    topics.push(new TestTopic("topic/#/invalidwc"));
    topics.push(new TestTopic("topic#/invalidwc"));
    topics.push(new TestTopic("topic+/invalidwc"));

    jsclient_pubsub_error_b(test,topics,"unsubscribe");
}

function jsclient_pubsub_e15_b(test) {
    // This test does not use the error path function, but it is an error path test
    var topic_options = new Array();
    topic_options.push(new Option("expected",0));
    var options = new Array();
    options.push(new TestTopic("simple topic",topic_options));
    options.push(new Option("port",28200));
    jsclient_pubsub(test,options);
}

function jsclient_pubsub_e16_b(test) {
    // This test does not use the error path function, but it is an error path test
    var topic_options = new Array();
    topic_options.push(new Option("expected",0));
    var options = new Array();
    options.push(new TestTopic("jsclient_pubsub_e16_b_topic1", topic_options));
    options.push(new Option("port",27200));
    options.push(new Option("userName","user1"));
    options.push(new Option("password","pwuser1"));
    jsclient_pubsub(test,options);
}

function jsclient_pubsub_e17_b(test) {
    setLog(test);
    writeLog(logfile, 'TRACE: e17_is_starting');

    var topics = new Array();
    topics.push(new TestTopic("jsclient_pubsub_e17_b_topic"));

    clientid = 'pubsub_e17_b';

    writeLog(logfile, 'INFO: e17 imaserveraddr: ' + imaserveraddr);
    writeLog(logfile, 'INFO: e17 imaserverport: ' + imaserverport);
    writeLog(logfile, 'INFO: e17 clientid:      ' + clientid);

    var client = new Messaging.Client(imaserveraddr, imaserverport, clientid);
    var connect_options = new Object();
    //connect_options.userName = null;
    //connect_options.password = null;
    connect_options.onSuccess = function() {
        onconnect_simple(clientid);
    }

    client.onConnectionLost = function (client_obj, reason) {
        writeLog(logfile, 'TRACE: e17_forced_disconnect_as_expected');
        window.setTimeout(function() {window.close(); }, 1000);
    }

    writeLog(logfile, 'TRACE: e17_going_to_connect');
    try {
        client.connect(connect_options);
    } catch (e) {
        writeLog(logfile, 'ERROR: ' + e);
    }
    writeLog(logfile, 'TRACE: e17_connected');

    window.setTimeout(function() {
            writeLog(logfile, 'ERROR: e17_unexpectedly_had_no_error');
            window.close();
        }, 10000);

}
