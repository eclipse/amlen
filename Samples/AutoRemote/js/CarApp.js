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

(function() {
    var lastTime = 0;
    var vendors = ['ms', 'moz', 'webkit', 'o'];
    for(var x = 0; x < vendors.length && !window.requestAnimationFrame; ++x) {
        window.requestAnimationFrame = window[vendors[x]+'RequestAnimationFrame'];
        window.cancelAnimationFrame = window[vendors[x]+'CancelAnimationFrame'] 
                                   || window[vendors[x]+'CancelRequestAnimationFrame'];
    }
 
    if (!window.requestAnimationFrame)
        window.requestAnimationFrame = function(callback, element) {
            var currTime = new Date().getTime();
            var timeToCall = Math.max(0, 16 - (currTime - lastTime));
            var id = window.setTimeout(function() { callback(currTime + timeToCall); }, 
              timeToCall);
            lastTime = currTime + timeToCall;
            return id;
        };
 
    if (!window.cancelAnimationFrame)
        window.cancelAnimationFrame = function(id) {
            clearTimeout(id);
        };
}());

// add color flash to jQuery element
jQuery.fn.flash = function(color) {
	this.css('color', 'rgb(' + color + ')');
	this.animate({ color: 'rgb(0, 0, 0)' }, 1500)
}

function flashGreen(element) {
	element.flash('0,160,0');
}

function playSound(id) {
	$("#"+id)[0].play();
}

// have 5 identical sounds, so they can overlap if a user does repeated button presses
var hornElem = ["horn1", "horn2", "horn3", "horn4", "horn5"];
var hornIndex = 0;
function playHorn() {
	playSound(hornElem[hornIndex % hornElem.length]);
	hornIndex++;
}

function playAlarm() {
	playSound("alarm");
}

function stopAlarm() {
	$("#alarm")[0].pause();
}

function playStart() {
	playSound("start");
}

function initCar() {
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

	carApp = new CarApp();
	carApp.setCanvas("canvas");

	drawLoop = function() {
		requestAnimationFrame(drawLoop);
		carApp.draw();
	}
	drawLoop();

	$("#alarm")[0].addEventListener("ended", function() { $("#alarm")[0].load(); playAlarm(); });
	$("#horn1")[0].addEventListener("ended", function() { $("#horn1")[0].load() });
	$("#horn2")[0].addEventListener("ended", function() { $("#horn2")[0].load() });
	$("#horn3")[0].addEventListener("ended", function() { $("#horn3")[0].load() });
	$("#horn4")[0].addEventListener("ended", function() { $("#horn4")[0].load() });
	$("#horn5")[0].addEventListener("ended", function() { $("#horn5")[0].load() });
	$("#start")[0].addEventListener("ended", function() { $("#start")[0].load() });

	// init DOM events
	$("#signinSubmit").click(function(event) {
		carApp.driverName = ($("#signinName").val()).toLowerCase();
		carApp.setFields();
		carApp.connect();

		if ($("#car_controlTopic").length > 0) {
			$("#car_controlTopic").html(carApp.controlTopic);
		} else {
			$("#controlTopic").html(carApp.controlTopic);
		}
		$("#connectedTo").html($("#signinName").val());
		$("#carName").html($("#signinName").val() + "'s Car");
	});
}


/**************
 * CarApp *
 **************/
function CarApp() {
	this.server = "messagesight.demos.ibm.com";
	this.port = "1883";
	this.driverName = "";
	this.controlTopic = "";
	this.settingsTopic = "";
	this.clientId = "";
	this.username = null;
	this.password = null;
	this.client = null;
	this.context = $("#canvas")[0].getContext("2d");
	this.context.font = "20pt Calibri";
	this.context.textAlign = "center";
	this.carStarted = false;
	this.carLocked = true;
	this.alerts = [];
	this.time = null;
	this.isLeader = true;
}

CarApp.prototype.setCanvas = function(id) {
	this.canvas = $("#"+id);
}

CarApp.prototype.initSettings = function() {
	console.log("initSettings");
	if (this.isLeader) {
		this.settings = {
			id: this.clientId,
			started: false,
			locked: true,
			alarm: false,
			temp: 75
		};
		this.publishSettings();
	}
}

CarApp.prototype.publishSettings = function() {
	console.log("publishSettings");
	$("#statusStarted").html((this.settings.started) ? "started" : "stopped");
	$("#statusLocked").html((this.settings.locked) ? "locked" : "unlocked");
	$("#statusAlarm").html((this.settings.alarm) ? "alarm on" : "alarm off");
	$("#statusTemp").html(this.settings.temp + "\xB0");
	this.publish(JSON.stringify(this.settings), this.settingsTopic, true);
}

CarApp.prototype.publish = function(message, topic, retained) {
	var msgObj = new Messaging.Message(message);
	msgObj.destinationName = topic;
	if (retained) {
		msgObj.retained = retained;
	}
	this.client.send(msgObj);
}

// set fields from driver name
// this.doFrame();
CarApp.prototype.setFields = function() {
	var name = this.driverName.replace(/\s+/g, '');
	name = name.replace(/[^\w]+/g, '');
	this.controlTopic = "AutoRemote/"+name;
	this.settingsTopic = "AutoRemote/"+name+"/settings";

	var randomString = function(length) {
		var str = "";
		var chars = "abcdefghijklmnopqrstuvwxyz0123456789";
		for (var i = 0; i < length; i++) {
			str += chars[Math.floor(Math.random() * chars.length)];
		}
		return str;
	}
	this.clientId = "AutoRemote-Car-"+randomString(5);
	this.username = $("#signinUsername").val();
	this.password = $("#signinPassword").val();
	this.server = $("#signinServer").val();
	this.port = $("#signinPort").val();
}

CarApp.prototype.addAlert = function(text) {
	var start = { x: this.canvas.width() / 2, y: 200 };
	var alertObj = new CarAppAlert(text, start);
	this.alerts.push(alertObj);
}

CarApp.prototype.flash = function() {
	this.doFlash = true;
}

CarApp.prototype.draw = function() {
	var now = new Date().getTime();
	var delta = now - (this.time || now);
	this.time = now;

	this.animateAlerts(delta);
	this.drawAlerts();
}

CarApp.prototype.animateAlerts = function(delta) {
	for (var i in this.alerts) {
		this.alerts[i].animate(delta);
	}
}

CarApp.prototype.drawAlerts = function() {
	this.context.clearRect(0, 0, this.canvas[0].width, this.canvas[0].height);
	if (this.doFlash) {
		this.context.fillStyle = "rgba(0, 255, 0, 0.3)";
		this.context.fillRect(0, 0, this.canvas[0].width, this.canvas[0].height);
		this.doFlash = false;
	}

	var len = this.alerts.length;
	while (len--) {
		this.alerts[len].draw(this.context);
		if (this.alerts[len].isDone()) {
			this.alerts.splice(len, 1);
		}
	}
}

CarApp.prototype.processCommand = function(msg) {
	var obj = JSON.parse(msg);
	if (!obj.action) { console.log("invalid format"); return; }
	if (obj.action == "START") { this.cmd_Start(); return; }
	if (obj.action == "STOP") { this.cmd_Stop(); return; }
	if (obj.action == "UNLOCK") { this.cmd_Unlock(); return; }
	if (obj.action == "LOCK") { this.cmd_Lock(); return; }
	if (obj.action == "HORN") { this.cmd_Horn(); return; }
	if (obj.action == "ALARM_ON") { this.cmd_AlarmOn(); return; }
	if (obj.action == "ALARM_OFF") { this.cmd_AlarmOff(); return; }
	if (obj.action == "TEMP") { this.cmd_Temp(obj.value); return; }
	console.log("Unknown: " + obj.action);
}

CarApp.prototype.cmd_Start = function() {
	console.log("START");
	if (!this.settings.started) {
		this.settings.started = true;
		flashGreen($("#statusStarted"));
		playStart();
		this.addAlert("Engine started!");
		this.publishSettings();
	} else {
		console.log("    bad command...");
	}
}
CarApp.prototype.cmd_Stop = function() {
	console.log("STOP");
	if (this.settings.started) {
		this.settings.started = false;
		flashGreen($("#statusStarted"));
		this.addAlert("Engine stopped!");
		this.publishSettings();
	} else {
		console.log("    bad command...");
	}
}
CarApp.prototype.cmd_Unlock = function() {
	console.log("UNLOCK");
	if (this.settings.locked) {
		this.settings.locked = false;
		flashGreen($("#statusLocked"));
		this.addAlert("unlock car!");
		this.publishSettings();
	} else {
		console.log("    bad command...");
	}
}
CarApp.prototype.cmd_Lock = function() {
	console.log("LOCK");
	if (!this.settings.locked) {
		this.settings.locked = true;
		flashGreen($("#statusLocked"));
		this.addAlert("lock car!");
		this.publishSettings();
	} else {
		console.log("    bad command...");
	}
}
CarApp.prototype.cmd_Horn = function() {
	this.addAlert("beep beep!");
	playHorn();
	console.log("HORN");
	return true;
}
CarApp.prototype.cmd_AlarmOff = function() {
	console.log("ALARM");
	if (this.settings.alarm) {
		this.settings.alarm = false;
		flashGreen($("#statusAlarm"));
		stopAlarm();
		this.addAlert("ALARM OFF!");
		this.publishSettings();
	} else {
		console.log("    bad command...");
	}
}
CarApp.prototype.cmd_AlarmOn = function() {
	console.log("ALARM ON");
	if (!this.settings.alarm) {
		this.settings.alarm = true;
		flashGreen($("#statusAlarm"));
		playAlarm();
		this.addAlert("ALARM ON!");
		this.publishSettings();
	} else {
		console.log("    bad command...");
	}
}
CarApp.prototype.cmd_Temp = function(val) {
	this.addAlert(val + '\xB0');
	this.settings.temp = val;
	flashGreen($("#statusTemp"));
	this.publishSettings();
	console.log("TEMP - " + val);
	return true;
}

CarApp.prototype.connect = function() {
	try {
		this.client = new Messaging.Client(this.server, parseFloat(this.port), this.clientId);
	} catch (error) {
		alert("Error:"+error);
	}

	this.client.onMessageArrived = (function(self) {
		return function(msg) {
			var topic = msg.destinationName;
			var payload = msg.payloadString;
			//console.log(topic, payload);
			if (topic == self.controlTopic) {
				self.processCommand(payload);
				//self.flash();
			} else if (topic == self.settingsTopic) {
				var settings = JSON.parse(payload);
				// ignore
			}
		}
	})(this);

	this.client.onConnectionLost = function() { 
		$(".clientDisconnected").show();
		$(".clientConnected").hide();
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

	// on disconnect, clear the settings
	connectOptions.willMessage = new Messaging.Message("");
	connectOptions.willMessage.destinationName = this.settingsTopic;
	connectOptions.willMessage.retained = true;

	connectOptions.keepAliveInterval = 3600;  // if no activity after one hour, disconnect
	connectOptions.onSuccess = (function(self) {
		return function() {
			self.client.subscribe(self.controlTopic);
			setTimeout(function() {
				$(".clientDisconnected").hide();
				$(".clientConnected").fadeIn();
			}, 500);
			self.initSettings();
		}
	})(this);

	connectOptions.onFailure = function() { 
		$(".clientDisconnected").show();
		$(".clientConnected").hide();
	}

	this.client.connect(connectOptions);
}


/***************
 * CarAppAlert *
 ***************/
function CarAppAlert(text, start) {
	this.pos = {
		x: start.x,
		y: start.y 
	};

	var angle = Math.random() * Math.PI * 0.6 + Math.PI * 0.2;
	this.vector = {
		x: Math.cos(angle),
		y: -1 * Math.sin(angle)
	};

	this.totalDist = 0;
	this.endDist = 250;

	this.text = text;
}

CarAppAlert.prototype.animate = function(delta) {
	var xPart = this.getSpeed() * this.vector.x;
	var yPart = this.getSpeed() * this.vector.y;
	var dist = Math.sqrt(Math.pow(xPart, 2) + Math.pow(yPart, 2)) * delta / 1000;
	this.pos.x += xPart * delta / 1000;
	this.pos.y += yPart * delta / 1000;
	this.totalDist += dist;
}

CarAppAlert.prototype.getPerc = function() {
	return this.totalDist / this.endDist;
}

CarAppAlert.prototype.draw = function(context) {
	//console.log(this.text + " at " + this.pos.x + ", " + this.pos.y);
	var width = context.measureText(this.text).width;
	context.fillStyle = "rgba(0, 0, 0, " + this.getOpacity() + ")";
	context.fillText(this.text, this.pos.x, this.pos.y);
}

CarAppAlert.prototype.isDone = function() {
	return (this.totalDist > this.endDist);
}

CarAppAlert.prototype.getOpacity = function() {
	var perc = this.getPerc();
	var opacity = 0;
	if (perc < 0.1) {
		opacity = 0.5;
	} else if (perc >= 0.1 && perc <= 0.4) {
		opacity = 0.5 + 0.7 * ((perc - 0.1) / 0.5); 
	} else if (perc > 0.6) {
		opacity = 1.0 - (perc - 0.6) / 0.2;
	} else {
		opacity = 1;
	}
	if (opacity < 0) { opacity = 0; } if (opacity > 1) { opacity = 1; }
	return opacity;
}

CarAppAlert.prototype.getSpeed = function() {
	var perc = this.getPerc();
	var speed = 0;
	if (perc < 0.1) {
		speed = 120;
	} else if (perc >= 0.1 && perc <= 0.4) {
		speed = 120 - 40 * (perc - 0.1) / 0.3;
	} else if (perc > 0.4) {
		speed = 80 - 40 * (perc - 0.4) / 0.6;
	}
	return speed;
}
