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
var mqtt_clients = Array();
var num_mqtt_clients = 3;
var log_size = 20;
var logfile="";
var timerId;
var num_mqtt_clients;
var verbosity=9;

function setLog(test) {
    // Create the path for the logfile to make
    // sure it gets in the appropriate directory
    var curURL = window.location.pathname;
    prefix = curURL.substr(0,curURL.lastIndexOf('/')+1);
    baseName = test ;
    logfile = prefix + "../../" + baseName;
}


function getTime() {
    var date = new Date();
    var hours = date.getHours();
    var minutes = date.getMinutes();
    var seconds = date.getSeconds();
    var str = "";

    if (hours < 10) {
        str += "0";
    }
    str += hours + ":";

    if (minutes < 10) {
        str += "0";
    }
    str += minutes + ":";

    if (seconds < 10) {
        str += "0";
    }
    str += seconds;

    return str;
}

function MQTTClient(host, port, id, action) {
    this.id = id;
    this.log = Array();
    this.connected = false;
    this.publishing = false;
    this.logSize = 0;
    this.host=host;
    this.port=port;
    this.sentCounter = 0;
    this.recvCounter = 0;
    this.errorCounter = 0;
    this.warnCounter = 0;
    this.action=action;
    this.subAction=0;
    this.pubTopicName=null;
    this.subTopicName=null;
    this.msgTxt=null;
    this.ismclient = new Messaging.Client(host, Number(port), String(this.id));
    
    // this.ismclient.onconnect = (function() {
    // mqtt_clients[id - 1].addLogMsg("Client is connected");
    // $("#" + "client" + id + "status").html("Connected");
    // $("#" + "client" + id + "status").css("color", "green");
    // $("#" + "client" + id + "connect").attr("value", "Disconnect");
    // mqtt_clients[id - 1].connected = true;
    //
    // $("#" + "client" + id + "publish").removeAttr("disabled");
    // $("#" + "client" + id + "startpublish").removeAttr("disabled");
    // $("#" + "client" + id + "subscribe").removeAttr("disabled");
    // $("#" + "client" + id + "unsubscribe").removeAttr("disabled");
    // $("#" + "client" + id + "pubtopic").removeAttr("disabled");
    // $("#" + "client" + id + "pubmsg").removeAttr("disabled");
    // $("#" + "client" + id + "subtopic").removeAttr("disabled");
    // $("#" + "client" + id).css("background-color", "#ffffff");
    // mqtt_clients[id - 1].logSize = log_size;
    // mqtt_clients[id - 1].displayLog();
    // });
    // this.ismclient.onconnectionlost = (function() {
    // mqtt_clients[id - 1].disconnect();
    // });
    // this.ismclient.onmessage = (function(message) {
    // mqtt_clients[id - 1].addLogMsg("<span class='recv'>&gt;&gt;&nbsp;[" +
    // message.topic + "] " + message.payload
    // + "</span>");
    // });
    // this.ismclient.setConnectionCompleteCallback((function() {
    // this.ismclient.setConnectionLostCallback((function() {
    this.ismclient.onConnectionLost = ((function(client, reason) {
        mqtt_clients[id - 1].addLogMsg("calling disconnect from onConnectionLost: reason: " + reason);
        mqtt_clients[id - 1].disconnect();
    }));
    // this.ismclient.setMessageArrivedCallback((function(message) {
    this.ismclient.onMessageArrived = ((function(message) {
        mqtt_clients[id - 1].recvCounter++;
        mqtt_clients[id - 1].addLogMsg("Received message : " +message);
        mqtt_clients[id - 1].addLogMsg("<span class='recv'>&gt;&gt;&nbsp;[" + message.destinationName + "] "
                + message.payloadString + "</span>");
        if(mqtt_clients[id - 1].action.substring(0,10)=="automotive") {
            mqtt_clients[id - 1].addLogMsg ("INFO: disconnect autotomotive user ");
            mqtt_clients[id - 1].ismclient.disconnect();
            mqtt_clients[id - 1].disconnect();
            mqtt_clients[id - 1].addLogMsg ("INFO: connect autotomotive user count: TODO");
            mqtt_clients[id - 1].doConnect();
        }
    }));
}

function doConnectFailure (invocationContext){
    contextId=invocationContext.id;  
    mqtt_clients[contextId - 1].errorCounter++;
    mqtt_clients[contextId - 1].addLogMsg("FAILURE for Client id" + contextId);
}

function doConnectSuccess (invocationContext){
    contextId=invocationContext.id;  
    mqtt_clients[contextId - 1].addLogMsg("Client is connected");
    $("#" + "client" + contextId + "status").html("Connected");
    $("#" + "client" + contextId + "status").css("color", "green");
    $("#" + "client" + contextId + "connect").attr("value", "Disconnect");
    mqtt_clients[contextId - 1].connected = true;

    if(mqtt_clients[contextId - 1].action.substring(0,10)!="automotive"){
        $("#" + "client" + contextId + "publish").removeAttr("disabled");
        $("#" + "client" + contextId + "startpublish").removeAttr("disabled");
        $("#" + "client" + contextId + "subscribe").removeAttr("disabled");
        $("#" + "client" + contextId + "unsubscribe").removeAttr("disabled");
        $("#" + "client" + contextId + "pubtopic").removeAttr("disabled");
        $("#" + "client" + contextId + "pubmsg").removeAttr("disabled");
        $("#" + "client" + contextId + "subtopic").removeAttr("disabled");
    }
    $("#" + "client" + contextId).css("background-color", "#ffffff");
    mqtt_clients[contextId - 1].logSize = log_size;
    mqtt_clients[contextId - 1].displayLog();
    if (mqtt_clients[contextId - 1].action != null){
       	if(mqtt_clients[contextId - 1].action=="subscribe") {
            if (mqtt_clients[contextId - 1].connected == true) {
                var mytopic=$("#client" + contextId + "subtopic").val();
                mqtt_clients[contextId - 1].addLogMsg ("INFO: subscribing to topic: " + mytopic);
                mqtt_clients[contextId - 1].doSubscribe($("#client" + contextId + "subtopic").val(),0);
            } else {
                mqtt_clients[contextId - 1].addLogMsg ("ERROR: unexpected error");
            }
       	}else if(mqtt_clients[contextId - 1].action.substring(0,10)=="automotive" && mqtt_clients[contextId - 1].subAction==0) {
            mqtt_clients[contextId - 1].doAutomotiveAccount();
            mqtt_clients[contextId - 1].subAction=1;
       	}else if(mqtt_clients[contextId - 1].action.substring(0,10)=="automotive" && mqtt_clients[contextId - 1].subAction==1) {
            mqtt_clients[contextId - 1].addLogMsg ("INFO: starting automotive user for Reservations ");
            mqtt_clients[contextId - 1].doAutomotiveReservation();
       	    if(mqtt_clients[contextId - 1].action=="automotive") {
                mqtt_clients[contextId - 1].subAction=0; 
       	    }else if(mqtt_clients[contextId - 1].action=="automotive-unlock") {
                mqtt_clients[contextId - 1].subAction=2; 
            }
       	}else if(mqtt_clients[contextId - 1].action.substring(0,10)=="automotive" && mqtt_clients[contextId - 1].subAction==2) {
            mqtt_clients[contextId - 1].addLogMsg ("INFO: starting automotive user for Car Unlock");
            mqtt_clients[contextId - 1].doAutomotiveUnlock();
            mqtt_clients[contextId - 1].subAction=0;
            mqtt_clients[contextId - 1].addLogMsg ("[INFO]: completed cycle for " + action,0);
       	}else if(mqtt_clients[contextId - 1].action=="DISABLED"){
            console.log ("Client was disabled");   
        }else {
            mqtt_clients[contextId - 1].addLogMsg ("WARNING: Unknown action parameter passed in by URL: " + action);
        }	
    }else {
        mqtt_clients[contextId - 1].addLogMsg ("INFO: no automated action parameter passed in by URL, or it was disabled. ");
    } 
}

function myAddLogMsg (msg){ 
    var time = getTime();
    var str = time +  msg;
    writeLog(logfile, str);
}

MQTTClient.prototype.addLogMsg = function(msg,traceLevel) {
    var time = getTime();
    var str = time + " id:" + this.id + " " + msg;
    this.log.unshift(str);
    mqtt_clients[this.id - 1].displayLog();
    //if ( logfile_initialized==0){
        //setLog("user_scale") ;
        //logfile_initialized=1;
    //}
    if (typeof traceLevel == 'undefined' ){
        traceLevel=9;
    }
    if (verbosity>=traceLevel){
        writeLog(logfile, str);
    }
}

MQTTClient.prototype.displayLog = function() {
    var str = "";
    for ( var i = 0; i < this.logSize; i++) {
        if (this.log[i] != undefined) {
            str += this.log[i];
        }
        str += "<br>";
    }
    $("#" + "client" + this.id + "log").html(str);
}

MQTTClient.prototype.disconnect = function() {
    mqtt_clients[this.id - 1].addLogMsg("Client is disconnected");
    $("#" + "client" + this.id + "status").html("Disconnected");
    $("#" + "client" + this.id + "status").css("color", "red");
    $("#" + "client" + this.id + "connect").attr("value", "Connect");

    $("#" + "client" + this.id + "publish").attr("disabled", "true");
    $("#" + "client" + this.id + "startpublish").attr("disabled", "true");
    $("#" + "client" + this.id + "stoppublish").attr("disabled", "true");
    $("#" + "client" + this.id + "subscribe").attr("disabled", "true");
    $("#" + "client" + this.id + "unsubscribe").attr("disabled", "true");
    $("#" + "client" + this.id + "pubtopic").attr("disabled", "true");
    $("#" + "client" + this.id + "pubmsg").attr("disabled", "true");
    $("#" + "client" + this.id + "subtopic").attr("disabled", "true");
    $("#" + "client" + this.id).css("background-color", "#eeeeff");
    mqtt_clients[this.id - 1].connected = false;
}

MQTTClient.prototype.doPublish = function(topic, message, silent) {
    if (silent != true) {
        mqtt_clients[this.id - 1].addLogMsg("doPublish[" + this.id + "]: topic = " + topic + ", message = " + message);
        mqtt_clients[this.id - 1].addLogMsg("<span class='send'>&lt;&lt;&nbsp;[" + topic + "] " + message + "</span>");
    }
    // this.ismclient.publish(topic, message);
    var pmessage = new Messaging.Message(message);
    // var pubtopic = this.ismclient.getTopic(topic);
    pmessage.destinationName = topic;
    // pubtopic.publish(pmessage);
    this.ismclient.send(pmessage);
    mqtt_clients[this.id - 1].sentCounter++;
    mqtt_clients[this.id - 1].addLogMsg("INFO: message sent");
};

MQTTClient.prototype.startPublish = function(topic, message, interval) {
    mqtt_clients[this.id - 1].addLogMsg("startPublish[" + this.id + "]: topic = " + topic + ", message = " + message + ", interval = "
            + interval + " ms");
    mqtt_clients[this.id - 1].addLogMsg("Start publish: [" + topic + "] " + message);
    this.publishing = true;
    this.publishLoop(topic, message, interval);
    $("#" + "client" + this.id + "startpublish").attr("disabled", "true");
    $("#" + "client" + this.id + "stoppublish").removeAttr("disabled");
    $("#" + "client" + this.id + "publish").attr("disabled", "true");
}

MQTTClient.prototype.stopPublish = function(topic) {
    mqtt_clients[this.id - 1].addLogMsg("stopPublish[" + this.id + "]: topic = " + topic);
    mqtt_clients[this.id - 1].addLogMsg("Stop publish: [" + topic + "]");
    this.publishing = false;
}

MQTTClient.prototype.doAutomotiveAccount = function(){
    mqtt_clients[this.id - 1].addLogMsg("do accounts");
    this.msgText="ACCOUNT_INFO|";
    this.pubTopicName="/APP/1/USER/"+this.id+"/USER/"+this.id;
    this.subTopicName="/USER/"+this.id+"/#"
    this.doSubscribe(this.subTopicName,0);      
}

MQTTClient.prototype.doAutomotiveReservation = function(){
    mqtt_clients[this.id - 1].addLogMsg("do accounts");
    this.msgText="RESERVATION_LIST|";
    this.pubTopicName="/APP/1/USER/"+this.id+"/USER/"+this.id;
    this.subTopicName="/USER/"+this.id+"/#"
    this.doSubscribe(this.subTopicName,1);     
}

MQTTClient.prototype.doAutomotiveUnlock = function(){
    mqtt_clients[this.id - 1].addLogMsg("do accounts");
    this.msgText="UNLOCK|";
    // Note: setup for only one car right now - better would be multiple cars
    this.pubTopicName="/APP/1/CAR/"+this.id+"/USER/"+this.id;
    this.subTopicName="/USER/"+this.id+"/#"
    this.doSubscribe(this.subTopicName,2);
}

MQTTClient.prototype.publishLoop = function(topic, message, interval) {
    if (this.publishing == true) {
        mqtt_clients[this.id - 1].doPublish(topic, message, true);
        setTimeout("mqtt_clients[" + (this.id - 1) + "].publishLoop(\"" + topic + "\", \"" + message + "\", "
                + interval + ")", interval);
    }
}

MQTTClient.prototype.doSubscribe = function(topic, qos ) {
    mqtt_clients[this.id - 1].addLogMsg("doSubscribe[" + this.id + "]: ism: " + this.host + ":" + this.port + " topic = " + topic + " QOS: " + qos);
    mqtt_clients[this.id - 1].addLogMsg("Client: " + this.id + "Subscribed to \"" + topic + "\"");
    var id = this.id;
    var subscribeOpts = new Object();
    subscribeOpts.qos = qos;
    subscribeOpts.onSuccess = function() {
	    if (mqtt_clients[id - 1].action != null){
           	if(mqtt_clients[id - 1].action.substring(0,10)=="automotive") {
                mqtt_clients[id - 1].addLogMsg ("subscribeCompleteCallback invoked for automotive");
                mqtt_clients[id - 1].addLogMsg ("Invoking doPublish on : " + 
                    mqtt_clients[id - 1].pubTopicName + " msg: " + mqtt_clients[id - 1].msgText );
                mqtt_clients[id - 1].doPublish(mqtt_clients[id - 1].pubTopicName, 
                        mqtt_clients[id - 1].msgText, false);
            }
        }
    }
    subscribeOpts.onFailure = function() {
     mqtt_clients[id - 1].addLogMsg("ERROR:subscribeFailureCallback invoked:::::::::::::::::::::::::::::");
     mqtt_clients[id - 1].errorCounter++;
    }   
    this.ismclient.subscribe(topic,subscribeOpts);
};

MQTTClient.prototype.doUnsubscribe = function(topic) {
    mqtt_clients[this.id - 1].addLogMsg("doUnsubscribe[" + this.id + "]: topic = " + topic);
    mqtt_clients[this.id - 1].addLogMsg("Unsubscribing from \"" + topic + "\"");
    this.ismclient.unsubscribe(topic);
};

var myClient = MQTTClient.prototype.ismclient;

MQTTClient.prototype.doConnect = function() {
    if (mqtt_clients[this.id - 1].connected == false) {
        mqtt_clients[this.id - 1].addLogMsg("calling connect");
        mqtt_clients[this.id - 1].ismclient.connect({
            timeout: 1200,
            keepAliveInterval:0,
            invocationContext:this,
            onSuccess:doConnectSuccess,
            onFailure:doConnectFailure 
        });
    } else {
        mqtt_clients[this.id - 1].addLogMsg("calling disconnect from doConnect");
        mqtt_clients[this.id - 1].ismclient.disconnect();
        mqtt_clients[this.id - 1].disconnect();
    }
}

MQTTClient.prototype.displayClient = function() {
    $("#clients").append("<div style='display: none;' id='" + "client" + this.id + "'>");

    $("#" + "client" + this.id + "").append("<h3>Client " + this.id + "</h3><hr/>");

    var str = "<p><input type='button' value='Connect' id='" + "client" + this.id + "connect'/>";
    str += "<span id='" + "client" + this.id + "status'>Disconnected</span></p><hr/>";
    $("#" + "client" + this.id + "").append(str);
    $("#" + "client" + this.id + "status").css("color", "red");
    $("#" + "client" + this.id + "status").css("padding-right", "40px");
    $("#" + "client" + this.id + "status").css("float", "right");

    var str = "<p><input type='button' value='Start Publish' disabled id='" + "client" + this.id
            + "startpublish'/><input type='text' value='topic_name' disabled id='" + "client" + this.id
            + "pubtopic'/></p>";
    str += "<p><input type='button' value='Stop Publish' disabled id='" + "client" + this.id
            + "stoppublish'/><input type='text' value='message' disabled id='" + "client" + this.id + "pubmsg'/></p>";
    str += "<p><input type='button' value='Publish Message' disabled id='" + "client" + this.id
            + "publish'/></p><div class='clear'></div><hr />";
    $("#" + "client" + this.id + "").append(str);
    $("#" + "client" + this.id + "publish").css("float", "left");
    $("#" + "client" + this.id + "pubtopic").css("float", "right");
    $("#" + "client" + this.id + "pubmsg").css("float", "right");

    var str = "<p><input type='button' value='Subscribe' disabled id='" + "client" + this.id
            + "subscribe'/><input type='text' value='topic_name' disabled id='" + "client" + this.id
            + "subtopic'/><input type='button' disabled value='Unsubscribe' id='" + "client" + this.id
            + "unsubscribe'/></p><div class='clear'/><hr/>";

    $("#" + "client" + this.id + "").append(str);
    $("#" + "client" + this.id + "subscribe").css("float", "left");
    $("#" + "client" + this.id + "subtopic").css("float", "right");

    var str = "<div id='" + "client" + this.id + "log'></div>";
    $("#" + "client" + this.id + "").append(str);

    var id = this.id;
    $("#" + "client" + this.id + "connect").click(function() {
        mqtt_clients[id - 1].doConnect();
    });

    $("#" + "client" + this.id + "publish").click(
            function() {
                if (mqtt_clients[id - 1].connected == true) {
                    if (mqtt_clients[id - 1].publishing == false) {
                        mqtt_clients[id - 1].doPublish($("#client" + id + "pubtopic").val(), $(
                                "#client" + id + "pubmsg").val(), false);
                    }
                }
            });
    $("#" + "client" + this.id + "startpublish").click(
            function() {
                if (mqtt_clients[id - 1].connected == true) {
                    if (mqtt_clients[id - 1].publishing == false) {
                        mqtt_clients[id - 1].startPublish($("#client" + id + "pubtopic").val(), $(
                                "#client" + id + "pubmsg").val(), 200);
                        //$("#" + "client" + id + "startpublish").attr("disabled", "true");
                        //$("#" + "client" + id + "stoppublish").removeAttr("disabled");
                        //$("#" + "client" + id + "publish").attr("disabled", "true");
                    }
                }
            });
    $("#" + "client" + this.id + "stoppublish").click(function() {
        if (mqtt_clients[id - 1].connected == true) {
            if (mqtt_clients[id - 1].publishing == true) {
                mqtt_clients[id - 1].stopPublish($("#client" + id + "pubtopic").val());
                $("#" + "client" + id + "stoppublish").attr("disabled", "true");
                $("#" + "client" + id + "startpublish").removeAttr("disabled");
                $("#" + "client" + id + "publish").removeAttr("disabled");
            }
        }
    });

    $("#" + "client" + this.id + "subscribe").click(function() {
        if (mqtt_clients[id - 1].connected == true) {
            mqtt_clients[id - 1].doSubscribe($("#client" + id + "subtopic").val(), 0);
        }
    });

    $("#" + "client" + this.id + "unsubscribe").click(function() {
        if (mqtt_clients[id - 1].connected == true) {
            mqtt_clients[id - 1].doUnsubscribe($("#client" + id + "subtopic").val());
        }
    });

    $("#" + "client" + this.id).css("float", "left");
    $("#" + "client" + this.id).css("border", "1px solid black");
    $("#" + "client" + this.id).css("width", "350px");
    $("#" + "client" + this.id).css("margin", "10px");
    $("#" + "client" + this.id).css("padding", "10px");
    $("#" + "client" + this.id).css("background-color", "#eeeeff");

    $("#" + "client" + this.id).fadeIn(1000);
};

function finishProcessing(timeout_minutes){
    var scount=0;
    var rcount=0;
    var tcount=0;
    var ecount=0;
    var wcount=0;
    var totalSec=timeout_minutes*60;
    myAddLogMsg( "[INFO]: Desired test time has elapsed. " + timeout_minutes + "(minutes). " );
    for ( var i = 0; i < num_mqtt_clients; i++) {
        console.log("disconnecting id " + i);
        mqtt_clients[i].action="DISABLED";
        mqtt_clients[i].disconnect();
    }   
    myAddLogMsg( "[INFO]: print results." );
    for ( var i = 0; i < num_mqtt_clients; i++) {
        rcount+=mqtt_clients[i].recvCounter;
        scount+=mqtt_clients[i].sentCounter;
        ecount+=mqtt_clients[i].errorCounter;
        wcount+=mqtt_clients[i].warnCounter;
    }
    tcount=scount+rcount;
    myAddLogMsg( "JavaScript User Scale Client Summary,n/a,finished_,Ran for," + totalSec +
                 ",sec. Sent," + scount+ ",Recvd," + rcount  + ",Rate:," + tcount/totalSec +
                 " messages/sec ,error count:," + ecount + ",warncount:," +  wcount + ",num clients:," +
                 num_mqtt_clients);

    if (ecount == 0 ){
        if (tcount> num_mqtt_clients*2){
            myAddLogMsg( "[INFO]: SVTUserScale Success" );
        } else {
            myAddLogMsg( "[ERROR]: SVTUserScale Failure - due to message count too low " + tcount + 
                " expected at least a minimum message count of:" + num_mqtt_clients*timeout_minutes  );
        }
    } else {
        myAddLogMsg( "[ERROR]: SVTUserScale Failure - due to error count" );
    }

    myAddLogMsg( "[INFO]: Closing browser window." );
    console.log("Closing browser window to end test");
    window.close();
}

function startProcessing() {

    tmpverbosity = getURLParameter("verbosity");
    if(tmpverbosity!=null) {
        verbosity=tmpverbosity;
    }

    myAddLogMsg( "[INFO]: Starting test"  );
    num_mqtt_clients = getURLParameter("client_count");
    if(num_mqtt_clients==null) {
        document.getElementById('serverData').innerHTML+= "ERROR: client_count URL parameter not specified";
        myAddLogMsg( "[ERROR]: client_count URL parameter not specified");
        return;
    }
    myAddLogMsg( "[INFO]: num_mqtt_clients = " + num_mqtt_clients  );

    tmplog = getURLParameter("logfile");
    if(tmplog==null) {
        setLog("user_scale") ;
    } else {
        myAddLogMsg( "[INFO]: logfile set to " + tmplog);
        setLog(tmplog) ;
        logfile_initialized=1;
    }

    host = getURLParameter("host");
    if(host==null) {
        host=ismserveraddr; 
    }
    myAddLogMsg( "[INFO]: ISM server host set to " + host);

    port = getURLParameter("port");
    if(port==null) {
        port=ismserverport;
    }
    myAddLogMsg( "[INFO]: ISM server port set to " + port);

    test_minutes=getURLParameter("test_minutes");
    if(test_minutes!=null){
        myAddLogMsg( "[INFO]: Setting test timeout for " + test_minutes + "(minutes), in msec: " + test_minutes*1000*60 + " (msec)."  );
        timerId=setTimeout(function(){finishProcessing(test_minutes);},test_minutes*1000*60);
    }

    for ( var i = 0; i < num_mqtt_clients; i++) {
        action = getURLParameter("action");
        mqtt_clients.push(new MQTTClient(host, port, i + 1, action));
        mqtt_clients[i].displayClient();
        mqtt_clients[i].displayLog();
        if(action!=null) {
           mqtt_clients[i].doConnect();    
        }
    }
    console.log("Start clients button disabled");
    //$("#create_clients").attr("disabled", "true");
    $("#serverData").slideUp("slow");
    $("#serverDataTxt").slideUp("slow");

}

function checkForAutoStart() {
    console.log("checking for autostart");
    mystart = getURLParameter("start");
    if(mystart==null) {
        console.log("no autostart specified");
        return;
    }
    console.log("auto start processing");
    startProcessing();
}


function enableAllElements(){
    $("#create_clients").removeAttr("disabled", "false");
}

$(document).ready(function() {
    // do any needed setup here
    console.log("doc is ready");
    enableAllElements;
    $("#create_clients").click(function() {
        console.log("call start processing");
        startProcessing();
    });
    checkForAutoStart();
});
