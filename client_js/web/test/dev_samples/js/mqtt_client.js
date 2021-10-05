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
var mqtt_clients = Array();
var num_mqtt_clients = 3;
var log_size = 20;

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

function MQTTClient(host, port, id) {
    this.id = id;
    this.log = Array();
    this.connected = false;
    this.publishing = false;
    this.logSize = 0;

    this.ismclient = new IsmMqtt.Client(host, port, this.id);

    this.ismclient.onconnect = (function() {
        mqtt_clients[id - 1].addLogMsg("Client is connected");
        $("#" + "client" + id + "status").html("Connected");
        $("#" + "client" + id + "status").css("color", "green");
        $("#" + "client" + id + "connect").attr("value", "Disconnect");
        mqtt_clients[id - 1].connected = true;

        $("#" + "client" + id + "publish").removeAttr("disabled");
        $("#" + "client" + id + "startpublish").removeAttr("disabled");
        $("#" + "client" + id + "subscribe").removeAttr("disabled");
        $("#" + "client" + id + "unsubscribe").removeAttr("disabled");
        $("#" + "client" + id + "pubtopic").removeAttr("disabled");
        $("#" + "client" + id + "pubmsg").removeAttr("disabled");
        $("#" + "client" + id + "subtopic").removeAttr("disabled");
        $("#" + "client" + id).css("background-color", "#ffffff");
        mqtt_clients[id - 1].logSize = log_size;
        mqtt_clients[id - 1].displayLog();
    });
    this.ismclient.onconnectionlost = (function() {
        mqtt_clients[id - 1].disconnect();
    });
    this.ismclient.onmessage = (function(message) {
        mqtt_clients[id - 1].addLogMsg("<span class='recv'>&gt;&gt;&nbsp;[" + message.topic + "] " + message.payload
                + "</span>");
    });
}

MQTTClient.prototype.addLogMsg = function(msg) {
    var time = getTime();
    var str = time + "&nbsp;&nbsp;&nbsp;" + msg;
    this.log.unshift(str);
    mqtt_clients[this.id - 1].displayLog();
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
        console.log("doPublish[" + this.id + "]: topic = " + topic + ", message = " + message);
        mqtt_clients[this.id - 1].addLogMsg("<span class='send'>&lt;&lt;&nbsp;[" + topic + "] " + message + "</span>");
    }
    this.ismclient.publish(topic, message);
};

MQTTClient.prototype.startPublish = function(topic, message, interval) {
    console.log("startPublish[" + this.id + "]: topic = " + topic + ", message = " + message + ", interval = "
            + interval + " ms");
    mqtt_clients[this.id - 1].addLogMsg("Start publish: [" + topic + "] " + message);
    this.publishing = true;
    this.publishLoop(topic, message, interval);
}

MQTTClient.prototype.stopPublish = function(topic) {
    console.log("stopPublish[" + this.id + "]: topic = " + topic);
    mqtt_clients[this.id - 1].addLogMsg("Stop publish: [" + topic + "]");
    this.publishing = false;
}

MQTTClient.prototype.publishLoop = function(topic, message, interval) {
    if (this.publishing == true) {
        mqtt_clients[this.id - 1].doPublish(topic, message, true);
        setTimeout("mqtt_clients[" + (this.id - 1) + "].publishLoop(\"" + topic + "\", \"" + message + "\", "
                + interval + ")", interval);
    }
}

MQTTClient.prototype.doSubscribe = function(topic, func) {
    console.log("doSubscribe[" + this.id + "]: topic = " + topic);
    mqtt_clients[this.id - 1].addLogMsg("Subscribed to \"" + topic + "\"");
    var id = this.id;
    this.ismclient.subscribe(topic);
};

MQTTClient.prototype.doUnsubscribe = function(topic) {
    console.log("doUnsubscribe[" + this.id + "]: topic = " + topic);
    mqtt_clients[this.id - 1].addLogMsg("Unsubscribing from \"" + topic + "\"");
    this.ismclient.unsubscribe(topic);
};

var myClient = MQTTClient.prototype.ismclient;
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
        if (mqtt_clients[id - 1].connected == false) {
            mqtt_clients[id - 1].ismclient.connect();
        } else {
            mqtt_clients[id - 1].ismclient.disconnect();
            mqtt_clients[id - 1].disconnect();
        }
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
                        $("#" + "client" + id + "startpublish").attr("disabled", "true");
                        $("#" + "client" + id + "stoppublish").removeAttr("disabled");
                        $("#" + "client" + id + "publish").attr("disabled", "true");
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
            mqtt_clients[id - 1].doSubscribe($("#client" + id + "subtopic").val());
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

$(document).ready(function() {
    // do any needed setup here

    $("#create_clients").click(function() {
        num_mqtt_clients = $("#client_count").val();
        for ( var i = 0; i < num_mqtt_clients; i++) {
            mqtt_clients.push(new MQTTClient($("#host").val(), $("#port").val(), i + 1));
            mqtt_clients[i].displayClient();
            mqtt_clients[i].displayLog();
        }
        $("#create_clients").attr("disabled", "true");
        $("#client_count").attr("disabled", "true");
        $("#serverData").slideUp("slow");
    });
});
