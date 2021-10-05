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

function initRemote() {
	$("button").bind("touchend", function(e) {
		e.currentTarget.click();
		e.preventDefault();
	});
	$('#signinModal').modal('show').css(
		{
			'margin-top': function () {
				return -($(this).height() / 2) - 10;
			},
			'margin-left': function () {
				return -($(this).width() / 2);
			}
		}
	);
	remoteApp = new RemoteApp();

	// init DOM events
	$("#signinSubmit").click(function(event) {
		remoteApp.driverName = ($("#signinName").val()).toLowerCase();
		remoteApp.setFields();
		remoteApp.connect();

		$(".connectedTo").html($("#signinName").val());
		$("#controlTopic").html(remoteApp.controlTopic);
	});

	$("#startButton").click(function(event) {
		remoteApp.publish(JSON.stringify({ action: "START", value: true }));
	});
	$("#stopButton").click(function(event) {
		remoteApp.publish(JSON.stringify({ action: "STOP", value: true }));
	});
	$("#unlockButton").click(function(event) {
		remoteApp.publish(JSON.stringify({ action: "UNLOCK", value: true }));
	});
	$("#lockButton").click(function(event) {
		remoteApp.publish(JSON.stringify({ action: "LOCK", value: true }));
	});
	$("#hornButton").click(function(event) {
		remoteApp.publish(JSON.stringify({ action: "HORN", value: true }));
	});
	$("#alarmButton").click(function(event) {
		if ($("#alarmButton").html() == "Alarm OFF") {
			remoteApp.publish(JSON.stringify({ action: "ALARM_ON", value: true }));
		} else {
			remoteApp.publish(JSON.stringify({ action: "ALARM_OFF", value: true }));
		}
	});

	var minTemp = 65;
	var maxTemp = 85;
	$(".tempDown").click(function(event) {
		var curTemp = parseFloat($("#tempValue").val().substr(0, 2));
		curTemp--;
		$(".tempUp")[0].disabled = (curTemp >= maxTemp) ? true : false;
		$(".tempDown")[0].disabled = (curTemp <= minTemp) ? true : false;
		remoteApp.publish(JSON.stringify({ action: "TEMP", value: curTemp }));
	});
	$(".tempUp").click(function(event) {
		var curTemp = parseFloat($("#tempValue").val().substr(0, 2));
		curTemp++;
		$(".tempUp")[0].disabled = (curTemp >= maxTemp) ? true : false;
		$(".tempDown")[0].disabled = (curTemp <= minTemp) ? true : false;
		remoteApp.publish(JSON.stringify({ action: "TEMP", value: curTemp }));
	});
}

/**************
 * RemoteApp *
 **************/
function RemoteApp() {
	this.server = "messagesight.demos.ibm.com";
	this.port = "1883";
	this.driverName = "";
	this.controlTopic = "";
	this.settingsTopic = "";
	this.clientId = "";
	this.username = null;
	this.password = null;
	this.client = null;
}

// set fields from driver name
RemoteApp.prototype.setFields = function() {
	var name = this.driverName.replace(/\s+/g, '');
	name = name.replace(/[^\w]+/g, '');
	this.controlTopic = "AutoRemote/"+name;
	this.settingsTopic = "AutoRemote/"+name+"/settings";

	var randomString = function(length) {
		var str = "";
		var chars = "0123456789";
		for (var i = 0; i < length; i++) {
			str += chars[Math.floor(Math.random() * chars.length)];
		}
		return str;
	}
	this.clientId = "AutoRemote-Key-"+randomString(5);
	this.username = $("#signinUsername").val();
	this.password = $("#signinPassword").val();
	this.server = $("#signinServer").val();
	this.port = $("#signinPort").val();
}

RemoteApp.prototype.updateControls = function(settings) {
	console.log(settings);
	var disable = function(id) { $("#"+id).attr("disabled", "disabled"); }
	var enable = function(id) { $("#"+id).removeAttr("disabled"); }

	if (settings.started) {
		disable("startButton");
		enable("stopButton");
	} else {
		enable("startButton");
		disable("stopButton");
	}

	if (settings.locked) {
		disable("lockButton");
		enable("unlockButton");
	} else {
		enable("lockButton");
		disable("unlockButton");
	}

	if (settings.alarm) {
		$("#alarmButton").html("Alarm ON");
	} else {
		$("#alarmButton").html("Alarm OFF");
	}

	$("#tempValue").val(settings.temp + "\xB0");
}

RemoteApp.prototype.publish = function(message) {
	var msgObj = new Messaging.Message(message);
	msgObj.destinationName = this.controlTopic;
	this.client.send(msgObj);
}

RemoteApp.prototype.bRecvSettings = false;  // set true when an update is received from the car
RemoteApp.prototype.connect = function() {
	try {
		this.client = new Messaging.Client(this.server, parseFloat(this.port), this.clientId);
	} catch (error) {
		alert("Error:"+error);
	}

	this.client.onMessageArrived = (function(self) {
		return function(msg) {
			var topic = msg.destinationName;
			var payload = msg.payloadString;
			console.log(topic, payload);
			if (topic == self.settingsTopic) {
				if (payload == "") {
					$(".clientConnected").hide();
					$(".clientConnectedNoCar").fadeIn();
					remoteApp.bRecvSettings = false;
				} else {
					if (!remoteApp.bRecvSettings) {
						remoteApp.bRecvSettings = true;
						$(".clientConnectedNoCar").hide();
						$(".clientConnected").fadeIn();
					}
					self.updateControls(JSON.parse(payload));
				}
			}
		}
	})(this);
	this.client.onConnectionLost = function() { 
		$(".clientDisconnected").show();
		$(".clientConnected").hide();
		$(".clientConnectedNoCar").hide();
	}

	var connectOptions = new Object();
	connectOptions.useSSL = false;
	connectOptions.cleanSession = true;
	if (this.username != "") {
		connectOptions.userName = username;
	}
	if (this.password != "") {
		connectOptions.password = password;
	}

	connectOptions.keepAliveInterval = 3600;  // if no activity after one hour, disconnect
	connectOptions.onSuccess = (function(self) {
		return function() {
			self.client.subscribe(self.settingsTopic);
			setTimeout(function() {
				$(".clientDisconnected").hide();
				if (!remoteApp.bRecvSettings) {
					$(".clientConnectedNoCar").fadeIn();
				} else {
					$(".clientConnected").fadeIn();
				}
			}, 500);
		}
	})(this);
	connectOptions.onFailure = function() { 
		$(".clientDisconnected").show();
		$(".clientConnected").hide();
		$(".clientConnectedNoCar").hide();
	}

	this.client.connect(connectOptions);
}



