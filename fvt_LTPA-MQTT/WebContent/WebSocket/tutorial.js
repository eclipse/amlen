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
var count = 0;
var console = undefined;

// this will be used to divert output to the console
function load() {
	console = new MQTTWebSocketTutorialconsole();

}

function Try() {

	processor = new InputProcessor();
	processor.process();

}
// TODO Change the text area to a div or other tag and add css styles lines
// added

function MQTTWebSocketTutorialconsole() {
	this.consoleID = "ConsoleID";
	this.console = document.getElementById("console");
	this.count = 0;

}

MQTTWebSocketTutorialconsole.prototype.error = function(message) {
	this.append(message);

};

MQTTWebSocketTutorialconsole.prototype.warning = function(message) {
	this.append(message);

};
MQTTWebSocketTutorialconsole.prototype.info = function(message) {
	this.append(message);
};
MQTTWebSocketTutorialconsole.prototype.debug = function(message) {
	this.append(message);
};

MQTTWebSocketTutorialconsole.prototype.log = function(message) {
	this.append(message);

};

MQTTWebSocketTutorialconsole.prototype.append = function(message) {

	if (this.count != 0)
		this.console.value = this.console.value + "\n" + message;
	else
		this.console.value = message;
	this.count++;
};

function InputProcessor() {
	this.input = document.getElementById("newCode").value;
	this.insertionPoint = document.getElementById("page");
	this.scriptTag = "<script type=\"text/javascript\">";
	this.scriptTagEnd = "</script>";
	this.locationOfScriptTag = undefined;
	this.locationOfEndScripttag = undefined;
	this.script = undefined;
	this.headTag = "<head>";
	this.headEndTag = "</head>";
	this.htmlTag = "<html>";
	this.htmlEndTag = "</html>";
	this.bodyTag = "<body>";
	this.bodyEndTag = "</body>";

}

InputProcessor.prototype.process = function() {
	this.removeTags();

	this.locationOfScriptTag = this.input.indexOf(this.scriptTag);
	this.locationOfEndScripttag = this.input.indexOf(this.scriptTagEnd);

	if (this.locationOfScriptTag === -1 || this.locationOfEndScripttag === -1) {
		this.script = this.input;
		this.execute();
	} else {
		this.script = this.input.substring(this.locationOfScriptTag
				+ this.scriptTag.length, this.locationOfEndScripttag);

		this.insertCodeAndProcess();
	}
};

InputProcessor.prototype.insertCodeAndProcess = function() {

	this.insertionPoint.innerHTML = this.input;
	this.execute();

};

// this function removes the tags <html>, <head> and everything inside of head
// leaving script tags and <body>

InputProcessor.prototype.removeTags = function() {
	locationOfhtml = this.input.indexOf(this.htmlTag);
	locationOfhead = this.input.indexOf(this.headTag);

	if (locationOfhtml != -1 && locationOfhead != -1) {
		// strip out the unneed tags in the head
		this.input = this.input.substring(locationOfhead + this.htmlTag.length,
				this.input.length);
	}

	this.locationOfEndScripttag = this.input.indexOf(this.scriptTagEnd);
	locationOfBodyTag = this.input.indexOf(this.bodyTag);

	if (locationOfBodyTag != -1 && this.locationOfEndScripttag != -1) {

		partOne = this.input.substring(0, this.locationOfEndScripttag
				+ this.scriptTagEnd.length);
		partTwo = this.input.substring(locationOfBodyTag + this.bodyTag.length);

		// glue the parts back together
		this.input = partOne + partTwo;
	}
	// remove the closing tags

	locationOfBodyEnd = this.input.indexOf(this.bodyEndTag);
	if (locationOfBodyEnd != -1)
		this.input = this.input.substring(0, locationOfBodyEnd);

};

InputProcessor.prototype.execute = function() {
	try {
	    eval(this.script);
	} catch(error) {
		console.error(error);
	}
};
