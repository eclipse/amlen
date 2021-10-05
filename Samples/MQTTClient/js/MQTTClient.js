/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

//
// requires mqttws31.js
//

var client = null;
$("#connectClientID").val("Client" + Math.floor(10000 + Math.random() * 90000));

$("#connectButton").click(function(event) {
	var server = $("#connectServer").val();
	var port = $("#connectPort").val();
	var clientId = $("#connectClientID").val();
	var username = $("#connectUsername").val();
	var password = $("#connectPassword").val();
	var noCleanSession = $("#connectCleanSessionOff").hasClass("active");
	var useSSL = $("#connectSSLOn").hasClass("active");
	connect(server, port, clientId, username, password, noCleanSession, useSSL);
});
$("#disconnectButton").click(function(event) {
	client.disconnect();
});

$("#publishButton").click(function(event) {
	var topic = $("#publishTopic").val();
	var message = $("#publishMessage").val();
	var qos = parseFloat($("#publishQOS").val());
	retained = $("#publishRetainedOn").hasClass("active");
	publish(topic, message, qos, retained);
});

var subsList = {};
$("#subscribeButton").click(function(event) {
	var topic = $("#subscribeTopic").val();
	var qos = parseFloat($("#subscribeQOS").val());
	client.subscribe(topic, {
		qos: qos,
		onSuccess: function() {
			appendLog("Subscribed to [" + topic + "][qos " + qos + "]");
			if (!subsList[topic]) {
				subsList[topic] = true;
				$("#subscribeList").append("<span style='line-height: 20px; margin:5px 5px 5px 0;' id='"+topic+"' class='label label-info'>"+topic+"&nbsp;<button class='close' onclick='unsubscribe(\""+topic+"\");'>&times;</span></span>");
			}
		},
		onFailure: function() {
			appendLog("Subscription failed: [" + topic + "][qos " + qos + "]");
		}
	});
});

$(".requiresConnect").attr("disabled", true);

function unsubscribe(topic) {
	client.unsubscribe(topic, {
		onSuccess: function() {
			subsList[topic] = null;
			var elem = document.getElementById(topic);
			elem.parentNode.removeChild(elem);
			appendLog("Unsubscribed from [" + topic + "]");
		},
		onFailure: function() {
			appendLog("Unsubscribe failed: [" + topic + "]");
		}
	});
}

function publish(topic, message, qos, retained) {
	var msgObj = new Messaging.Message(message);
	msgObj.destinationName = topic;
	if (qos) { msgObj.qos = qos; }
	if (retained) { msgObj.retained = retained; }
	client.send(msgObj);

	var qosStr = ((qos > 0) ? "[qos " + qos + "]" : "");
	var retainedStr = ((retained) ? "[retained]" : "");
	appendLog("<< [" + topic + "]" + qosStr + retainedStr + " " + message);
}

function connect(server, port, clientId, username, password, noCleanSession, useSSL) {
	try {
		client = new Messaging.Client(server, parseFloat(port), clientId);
	} catch (error) {
		alert("Error:"+error);
	}

	client.onMessageArrived = onMessage;
	client.onConnectionLost = function() { 
		$("#connectedAlert").fadeOut();
		$(".requiresConnect").attr("disabled",true);
		$(".requiresDisconnect").attr("disabled",false);
		appendLog("Disconnected from " + server + ":" + port);
	}

	var connectOptions = new Object();
	connectOptions.useSSL = false;
	connectOptions.cleanSession = true;
	if (username) {
		connectOptions.userName = username;
	}
	if (password) {
		connectOptions.password = password;
	}
	if (noCleanSession) {
		connectOptions.cleanSession = false;
	}
	if (useSSL) {
		connectOptions.useSSL = true;
	}

	connectOptions.keepAliveInterval = 3600;  // if no activity after one hour, disconnect
	connectOptions.onSuccess = function() { 
		$("#connectedAlert").html("Connected!");
		$("#connectedAlert").fadeIn();
		$("#errorAlert").fadeOut();
		$(".requiresConnect").attr("disabled",false);
		$(".requiresDisconnect").attr("disabled",true);
		appendLog("Connected to " + server + ":" + port);
	}
	connectOptions.onFailure = function() { 
		$("#errorAlertText").html("Failed to connect!");
		$("#connectedAlert").fadeOut();
		$("#errorAlert").fadeIn();
		setTimeout(function() { $("#errorAlert").fadeOut(); }, 2000);
		$(".requiresConnect").attr("disabled",true);
		$(".requiresDisconnect").attr("disabled",false);
		appendLog("Failed to connect to " + server + ":" + port);
	}

	client.connect(connectOptions);
}

// function called whenever our MQTT connection receives a message
function onMessage(msg) {
	var topic = msg.destinationName;
	var payload = msg.payloadString;
	var qos = msg._getQos();
	var retained = msg._getRetained();

	var qosStr = ((qos > 0) ? "[qos " + qos + "]" : "");
	var retainedStr = ((retained) ? "[retained]" : "");
	appendLog(">> [" + topic + "]" + qosStr + retainedStr + " " + payload);
}

var logEntries = 0;
function appendLog(msg) {
	logEntries++;
	msg = "(" + ((new Date()).toISOString().split("T"))[1].substr(0, 12) + ") " + msg;
	$("#logContents").append(msg + "\n");
	$("#logSize").html(logEntries);
}
