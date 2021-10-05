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
var sim = false;
var controller;
var display;
var clickables = Array();

var buffers = Array();  // holds two canvas objects for double buffering
var bufferIndex = 0;
var flipBuffers = function() {
	buffers[1 - bufferIndex][0].style.visibility = 'hidden';
	buffers[bufferIndex][0].style.visibility = 'visible';
	bufferIndex = 1 - bufferIndex;
}
var getCanvas = function() {
	return buffers[bufferIndex];
}

var Globals = {
	FRAMES_PER_MESSAGE: 80,
	FRAMES_PER_SECOND: 40,
	MESSAGES_PER_SECOND_TOPIC: 5,
	MAX_PENDING: 3
}

var MessageType = {
	PUB : {value: 0},
	SUB : {value: 1}
}

var Topics = {
	A : {
		name: "A",
		color: "#815454"
	},
	B : {
		name: "B",
		color: "#497c91"
	},
	C : {
		name: "C",
		color: "#768b4e"
	}
}

var getRandomTopic = function() {
	var rand = Math.random();
	if (rand < 0.33) {
		return Topics.A;
	} else if (rand < 0.66) {
		return Topics.B;
	} else {
		return Topics.C;
	}
}

function Message() {
	this.topic = null;
	this.client = null;
	this.type = null;
	this.path = {
		start: {
			x: null,
			y: null
		},
		end: {
			x: null,
			y: null
		}
	};
	this.pos = {
		x: null,
		y: null
	};
	this.delta = {
		x: null,
		y: null
	};

	var self = this;

	this.draw = function() {
		var context = getCanvas()[0].getContext('2d');
		var radius = 8;
		context.save();
		context.beginPath();
		context.rect(self.pos.x - (radius/2), self.pos.y - (radius/2), radius, radius);
		context.fillStyle = self.topic.color;
		context.fill();
		context.lineWidth = 2;
		context.strokeStyle = "#000";
		context.globalAlpha = self._getOpacity();
		context.stroke();
		context.restore();
	}
	
	this.update = function() {
		var curDelta = self._getDelta();
		self.pos.x += curDelta.x;
		self.pos.y += curDelta.y;
		if ((Math.round(self.pos.x) > Math.round(self.path.end.x) && self.delta.x > 0) ||
			(Math.round(self.pos.x) < Math.round(self.path.end.x) && self.delta.x < 0)) {
			self.isDone();
		}
	}

	this._getDelta = function() {
		var perc = self._getPerc();
		if (perc > 0.5) {
			perc = 1 - perc;
		}
		var scale = 1.0 + (2 * perc);
		var retDelta = {
			x: self.delta.x * scale,
			y: self.delta.y * scale
		};
		return retDelta;
	}

	this._getOpacity = function() {
		var perc = self._getPerc();
		var opacity = Math.abs(0.5 - perc) * 2;
		opacity = Math.pow(opacity, 4);
		opacity = 1 - opacity;
		if (opacity < 0) { 
			opacity = 0;
		} else if (opacity > 1) {
			opacity = 1;
		}
		return opacity;
	}

	this._getPerc = function() {
		return Math.abs(self.path.start.x - self.pos.x) / Math.abs(self.path.end.x - self.path.start.x);
	}
}

function Client() {
	this.image = null;
	this.ismclient = null;
	this.pendingPubMessages = null;
	this.pubMessages = null;
	this.subMessages = null;
	this.connected = false;
	this.connectCallback = null;
	this.errorCallback = null;
	this.name = null;
	this.sendMsgCount = null;
	this.recvMsgCount = null;
	this.pos = {
		x: null,
		y: null
	};
	this.size = {
		w: null,
		h: null
	};

	this.connectionPath = {
		start: {
			x: null,
			y: null
		},
		end: {
			x: null,
			y: null
		},
		perc: 0
	}
	this.pubMessagePath = {
		start: {
			x: null,
			y: null
		},
		end: {
			x: null,
			y: null
		}
	}
	this.subMessagePath = {
		start: {
			x: null,
			y: null
		},
		end: {
			x: null,
			y: null
		}
	}

	this.pubTopics = null;
	this.subTopics = null;

	var self = this;

	this.init = function(name, pos, callback, errback) {
		self.connectCallback = callback;
		self.errorCallback = errback;
		
		if (!sim) {
			if (ismServer == null) {
				ismServer = document.location.hostname;
			}
			if (ismPort == null) {
				ismPort = "16102";
			}
			console.debug("Attempting to connect to " + ismServer + ":" + ismPort);
			self.ismclient = new MQTT.Client(ismServer, ismPort, sessionId + " - " + name);
			self.ismclient.onconnect = function() { 
				self.connect();
				if (self.connectCallback != null) {
					self.connectCallback();
				}
			}
			self.ismclient.onerror = function() { 
				console.log(self.name + " has an error!"); 
			}
			self.ismclient.onconnectionlost = function() {
				console.log(self.name + " onconnectionlost()");
				self.close();
			}
		}
		self.pendingPubMessages = Array();
		self.subMessages = Array();
		self.pubMessages = Array();
		self.pubTopics = Array();
		self.subTopics = Array();

		self.name = name;
		self.pos = pos;
		self.size = { w: 80, h: 60 };
		self.sendMsgCount = 0;
		self.recvMsgCount = 0;

		self.setPaths();
	}

	this.close = function() {
		self.pubMessages = Array();
		self.pendingPubMessages = Array();
		self.subMessages = Array();
		console.log(self.name + " lost connection!"); 
		self.connected = false;
		
		if (self.ismclient && self.ismclient._connected) {
			self.ismclient.disconnect();
		}
		self.ismclient = null;
		if (self.errorCallback) {
			self.errorCallback();
		}

		// start thread to draw connection animation
		var id = setInterval(function() {
			self.connectionPath.perc -= 2; 
			if (self.connectionPath.perc < 0) { 
				self.connectionPath.perc = 0; 
			} 
		}, 1000.0 / 50);
		setTimeout("clearInterval("+id+");", 1500);
	}

	this.setPaths = function(paths) {
		if (paths != null) {
			self.connectionPath.start.x = paths.connectionPath.start.x;
			self.connectionPath.end.x = paths.connectionPath.end.x;
			self.connectionPath.start.y = paths.connectionPath.start.y;
			self.connectionPath.end.y = paths.connectionPath.end.y;

			var slope = (self.connectionPath.end.y - self.connectionPath.start.y) / (self.connectionPath.end.x - self.connectionPath.start.x);
			var perp = -1 / slope;
			var div = Math.sqrt(perp * perp + 1);
			var perpVec = {
				x: 1 / div,
				y: perp / div
			}
			perpVec.x *= 10;
			perpVec.y *= 10;

			self.pubMessagePath.start.x = self.connectionPath.start.x + perpVec.x;
			self.pubMessagePath.end.x = self.connectionPath.end.x + perpVec.x;
			self.pubMessagePath.start.y = self.connectionPath.start.y + perpVec.y;
			self.pubMessagePath.end.y = self.connectionPath.end.y + perpVec.y;

			var slope = (self.connectionPath.start.y - self.connectionPath.end.y) / (self.connectionPath.start.x - self.connectionPath.end.x);
			var perp = -1 / slope;
			var div = Math.sqrt(perp * perp + 1);
			var perpVec = {
				x: 1 / div,
				y: perp / div
			}
			perpVec.x *= 10;
			perpVec.y *= 10;

			self.subMessagePath.start.x = self.connectionPath.end.x - perpVec.x;
			self.subMessagePath.end.x = self.connectionPath.start.x - perpVec.x;
			self.subMessagePath.start.y = self.connectionPath.end.y - perpVec.y;
			self.subMessagePath.end.y = self.connectionPath.start.y - perpVec.y;
		}
	}

	this.setImage = function(src) {
		self.image = new Image();
		self.image.src = src;
		self.size.w = self.image.width;
		self.size.h = self.image.height;
	}

	this.draw = function() {
		if (self.image != null) {
			var context = getCanvas()[0].getContext('2d');
			context.drawImage(self.image, self.pos.x - (self.image.width / 2), self.pos.y - (self.image.height / 2), 
										  self.image.width, self.image.height);
		}
	}

	this.connect = function() {
		controller.server.connections[self.name] = self;
		self.connected = true;

		// start thread to add messages from pending queue
		self.addFromPending();
	
		// start thread to draw connection animation
		var id = setInterval(function() {
			self.connectionPath.perc += 2; 
			if (self.connectionPath.perc > 100) { 
				self.connectionPath.perc = 100; 
			} 
		}, 1000.0 / 50);
		setTimeout("clearInterval("+id+");", 1500);
	}

	this.publishMessage = function(topic) {
		if (bServerConnected && self.pendingPubMessages.length < Globals.MAX_PENDING) {
			var message = new Message();
			message.client = self;
			message.type = MessageType.PUB;
			message.topic = topic;

//			var randomOffsetX = Math.round(Math.random() * 4) - 2;
//			var randomOffsetY = Math.round(Math.random() * 4) - 2;
			var randomOffsetX = 0;
			var randomOffsetY = 0;

			message.path.start.x = self.pubMessagePath.start.x + randomOffsetX;
			message.path.start.y = self.pubMessagePath.start.y + randomOffsetY;
			message.path.end.x = self.pubMessagePath.end.x + randomOffsetX;
			message.path.end.y = self.pubMessagePath.end.y + randomOffsetY;
			message.pos.x = message.path.start.x;
			message.pos.y = message.path.start.y;
			message.delta.x = (1.0 / Globals.FRAMES_PER_MESSAGE) * (message.path.end.x - message.path.start.x);
			message.delta.y = (1.0 / Globals.FRAMES_PER_MESSAGE) * (message.path.end.y - message.path.start.y);
			message.isDone = function() {
				controller.server.receivedPublish(this);
				self.pubMessages.shift();
			}
			self.pendingPubMessages.push(message);
			self.sendMsgCount++;
			controller.server.recvMsgCount++;
		}
	}

	this.receiveMessage = function(message) {
		var subMsg = new Message();
		subMsg.client = self;
		subMsg.type = MessageType.SUB;
		if (sim) {
			subMsg.topic = Topics[message.topic.name];
		} else {
			subMsg.topic = Topics[message.topic[0]];
		}

		var randomOffsetX = Math.round(Math.random() * 4) - 2;
		var randomOffsetY = Math.round(Math.random() * 4) - 2;
//		var randomOffsetX = 0;
//		var randomOffsetY = 0;

		subMsg.path.start.x = self.subMessagePath.start.x + randomOffsetX;
		subMsg.path.start.y = self.subMessagePath.start.y + randomOffsetY;
		subMsg.path.end.x = self.subMessagePath.end.x + randomOffsetX;
		subMsg.path.end.y = self.subMessagePath.end.y + randomOffsetY;
		subMsg.pos.x = subMsg.path.start.x;
		subMsg.pos.y = subMsg.path.start.y;
		subMsg.delta.x = (1.0 / Globals.FRAMES_PER_MESSAGE) * (subMsg.path.end.x - subMsg.path.start.x);
		subMsg.delta.y = (1.0 / Globals.FRAMES_PER_MESSAGE) * (subMsg.path.end.y - subMsg.path.start.y);
		subMsg.isDone = function() {
			self.subMessages.shift();
		}
		self.subMessages.push(subMsg);
		self.recvMsgCount++;
		controller.server.sendMsgCount++;
	}

	this.drawConnection = function() {
		var path = self.connectionPath;
		var xdiff = (path.end.x - path.start.x) * path.perc / 100.0;
		var ydiff = (path.end.y - path.start.y) * path.perc / 100.0;

		var context = getCanvas()[0].getContext('2d');
		context.save();
		context.strokeStyle = "#bbbbbb";
		context.lineWidth = 2;
		context.beginPath();
		context.moveTo(path.start.x, path.start.y);
		context.lineTo(path.start.x + xdiff, path.start.y + ydiff);
		context.closePath();
		context.stroke();
		context.restore();
	}

	this.drawMessages = function() {
		for (var i in self.pubMessages) {
			if (i < 10) {
				self.pubMessages[i].draw();
			}
		}
		for (var i in self.subMessages) {
			if (i < 30) {
				self.subMessages[i].draw();
			}
		}
	}

	this.addFromPending = function() {
		// add messages from the pending queue
		if (self.pendingPubMessages.length > 0) {
			var msg = self.pendingPubMessages.shift();
			self.pubMessages.push(msg);
		}
		setTimeout("controller.getClientByName('"+self.name+"').addFromPending()", 1000 / Globals.MESSAGES_PER_SECOND_TOPIC);
	}

	this.update = function() {
		for (var i in self.pubMessages) {
			self.pubMessages[i].update();
		}
		for (var i in self.subMessages) {
			if (i < 100) {
				self.subMessages[i].update();
			}
		}
	}
}

function Server() {
	this.image = null;
	this.connections = null;
	this.sendMsgCount = null;
	this.recvMsgCount = null;

	this.pos = {
		x: null,
		y: null
	};
	this.size = {
		w: null,
		h: null
	}

	var self = this;

	this.init = function() {
		self.pos = { x: 400, y: 120 };
		self.size = { w: 90, h: 140 };
		self.connections = {};
		self.sendMsgCount = 0;
		self.recvMsgCount = 0;
	}

	this.receivedPublish = function(message) {
		if (sim) {
			for (var clientName in self.connections) {
				var client = self.connections[clientName];
				if (client.subTopics != null) { 
					for (var i in client.subTopics) {
						if (client.subTopics[i].name == message.topic.name) {
							client.receiveMessage(message);
						}
					}
				}
			}
		} else {
			try {
				message.client.ismclient.publish(message.topic.name + "_" + sessionId, "Message from " + message.client.name);
			} catch (e) { console.error(e); }
		}
	}

	this.setImage = function(src) {
		self.image = new Image();
		self.image.src = src;
		self.size.w = self.image.width;
		self.size.h = self.image.height;
		//for (var i in controller.clients) {
		//	controller.clients[i].setPaths();
		//}
	}

	this.draw = function() {
		if (self.image != null) {
			var context = getCanvas()[0].getContext('2d');
			context.drawImage(self.image, self.pos.x - (self.image.width / 2), self.pos.y - (self.image.height / 2), 
										  self.image.width, self.image.height);
		} 	
	}

	this.drawConnections = function() {
		for (var clientName in self.connections) {
			var client = self.connections[clientName];
			client.drawConnection();
		}
	}

	this.drawMessages = function() {
		for (var clientName in self.connections) {
			var client = self.connections[clientName];
			client.drawMessages();
		}
	}
}

function Controller() {
	this.messages = Array();
	this.clients = Array();
	this.server = new Server();
	this.publishing = true;

	var self = this; 

	this.init = function() {
		self.server.init(); 
	}

	// pos = { x: __, y: __ }
	this.addClient = function(name, pos, connectCallback, errorCallback) {
		var client = new Client();
		client.init(name, pos, connectCallback, errorCallback);
		self.clients.push(client);
	}

	this.connectClient = function(name) {
		if (sim) {
			self.getClientByName(name).connect();
			self.getClientByName(name).connectCallback();
		} else {
			var opts = {
				keepAliveInterval: 3600
			};
			if (userName) { opts["userName"] = userName; }	
			if (password) { opts["password"] = password; }	
			if (!self.getClientByName(name).ismclient) {
				var client = self.getClientByName(name);
				self.getClientByName(name).init(client.name, client.pos, client.connectCallback, client.errorCallback);	
			}
			self.getClientByName(name).ismclient.connect(opts);
		}
	}

	this.start = function() {
		self.startPublish(1000);
	}

	this.addPub = function(clientName, topicName) {
		console.log("addPub(" + clientName + ", " + topicName + ")");
		var client = self.getClientByName(clientName);
		var topic = Topics[topicName];
		for (var i in client.pubTopics) {
			if (client.pubTopics[i].name == topic.name) {
				// already has, don't need to add
				return;
			}
		}
		client.pubTopics.push(topic);
	}
	this.removePub = function(clientName, topicName) {
		console.log("removePub(" + clientName + ", " + topicName + ")");
		var client = self.getClientByName(clientName);
		var topic = Topics[topicName];
		for (var i in client.pubTopics) {
			if (client.pubTopics[i].name == topic.name) {
				client.pubTopics.splice(i, 1);
				return;
			}
		}
		// if we reach here, the pub wasn't found
	}

	this.addSub = function(clientName, topicName) {
		console.log("addSub(" + clientName + ", " + topicName + ")");
		var client = self.getClientByName(clientName);
		var topic = Topics[topicName];
		for (var i in client.subTopics) {
			if (client.subTopics[i].name == topic.name) {
				// already has, don't need to add
				return;
			}
		}
		if (sim) {
			client.subTopics.push(topic);
		} else {
			try {
				client.ismclient.subscribe(topic.name + "_" + sessionId, client.receiveMessage, 0, function() { client.subTopics.push(topic); });
			} catch (e) { console.error(e); }
		}
	}

	this.removeSub = function(clientName, topicName) {
		console.log("removeSub(" + clientName + ", " + topicName + ")");
		var client = self.getClientByName(clientName);
		var topic = Topics[topicName];
		for (var i in client.subTopics) {
			if (client.subTopics[i].name == topic.name) {
				client.subTopics.splice(i, 1);
			}
		}
		try {
			client.ismclient.unsubscribe(topic.name + "_" + sessionId);
		} catch (e) { console.error(e); }
	}

	this.doPublish = function(millis) {
		for (var i in self.clients) {
			if (self.clients[i].connected && self.clients[i].pubTopics != null) {
				for (var j in self.clients[i].pubTopics) {
					self.clients[i].publishMessage(self.clients[i].pubTopics[j]);
				}
			}
		}
		if (self.publishing) {
			setTimeout("controller.doPublish("+millis+")", millis);
		}
	}
	
	this.startPublish = function(millis) {
		self.doPublish(millis);
	}

	this.update = function() {
		for (var i in self.clients) {
			if (self.clients[i].connected) {
				self.clients[i].update();
			}
		}
		for (var i in self.messages) {
			self.messages[i].update();
		}
	}

	this.getClientByName = function(name) {
		for (var i in self.clients) {
			if (self.clients[i].name == name) {
				return self.clients[i];
			}
		}
		return null;
	}
}

function Display() {
    this.canvas = null;
    this.canvasHeight = null; 
    this.canvasWidth = null;

    var self = this;
    this.init = function() {
        this.canvasHeight = getCanvas().attr("height");
        this.canvasWidth = getCanvas().attr("width");
    }
 
	this.drawClients = function() {
		for (var i in controller.clients) {
			var client = controller.clients[i];
			client.draw();
		}
	}

	this.drawServer = function() {
		controller.server.draw();
	}

	this.drawConnections = function() {
		controller.server.drawConnections();
	}

	this.drawMessages = function() {
		controller.server.drawMessages();
	}

    this.draw = function() {
        // clear canvas
        var context = getCanvas()[0].getContext('2d');
		context.save();
		context.beginPath();
		context.rect(0, 0, getCanvas()[0].width, getCanvas()[0].height);
		context.fillStyle = "#fff";
		context.fill();
		context.restore();

		self.drawClients();
		self.drawServer();
		self.drawMessages();
		self.drawConnections();
		flipBuffers();
		controller.update();
        setTimeout("display.draw()", 1000 / Globals.FRAMES_PER_SECOND);
    }
}

$(document).ready(function() {
    // do any needed setup here
	buffers.push($("#canvas"));
	buffers.push($("#canvas2"));
	controller = new Controller();
	controller.init();
	controller.start();
	display = new Display();
	display.init();
	display.draw();
});

