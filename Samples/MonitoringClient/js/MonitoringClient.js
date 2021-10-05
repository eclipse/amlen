
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
// requires mqttws31
// requires jquery-ui
// requires jquery
// requires bootstrap
//

// use the namespace "MonitoringClient"
MonitoringClient = (function (global) {

	var Utils = {
		randomString: function(length) {
			var source = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
			var str = "";
			for (var i = 0; i < length; i++) {
				str += source[Math.floor(Math.random() * source.length)];
			}
			return str;
		},

		getBytesString: function(num) {
			var units = "";
			if (num < 1024) {
				units = "";
			} else if (num < (1024*1024)) {
				units = " KB";
				num /= 1024;
			} else if (num < (1024*1024*1024)) {
				units = " MB";
				num /= 1024*1024;
			} else if (num < (1024*1024*1024*1024)) {
				units = " GB";
				num /= 1024*1024*1024;
			} else {
				units = " TB";
				num /= 1024*1024*1024*1024;
			}
			if (units == "") {
				return num;
			} else {
				return num.toFixed(2) + units;
			}
		},

		getMsgString: function(num) {
			var units = "";
			if (num < 100) {
				units = "";
			} else if (num < (1024*1024)) {
				units = "k";
				num /= 1024;
			} else if (num < (1024*1024*1024)) {
				units = "m";
				num /= 1024*1024;
			} else {
				units = "b";
				num /= 1024*1024*1024;
			}
			if (units == "") {
				return num;
			} else {
				return num.toFixed(1) + units;
			}
		},

		addCommas: function(num) {
			num = num.toString();
			var str = "";
			var j = 0;
			for (var i = num.length - 1; i >= 0; i--) {
				if (j != 0 && j % 3 == 0) { str += ","; }
				str += num[i];
				j++;
			}
			return str.split("").reverse().join("");
		},

		getGB: function(num) {
			return (parseFloat(num) / 1024 / 1024 / 1024).toFixed(2);
		}
	}

	var client = null;
	var clientId = "MonitoringClient-" + Utils.randomString(5);

	function getClient() {
		return client;
	}

	function connect(server, port) {
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
		}

		var connectOptions = new Object();
		connectOptions.useSSL = false;
		connectOptions.cleanSession = true;
		connectOptions.keepAliveInterval = 3600;  // if no activity after 
		                                          // one hour, disconnect

		connectOptions.onSuccess = (function(mqttclient) { 
			return function() {
				$("#connectedAlert").html("Connected!");
				$("#connectedAlert").fadeIn();
				$("#errorAlert").fadeOut();
				$(".requiresConnect").attr("disabled",false);
				$(".requiresDisconnect").attr("disabled",true);

				/*****************************************
				 *
				 * STEP 2 - Subscribe to monitoring topic
				 *
				 *****************************************/ 
				mqttclient.subscribe("$SYS/ResourceStatistics/#");

				$("#applianceMonitoringToggle").click();
			}
		})(client);
		connectOptions.onFailure = function() { 
			$("#errorAlertText").html("Failed to connect!");
			$("#connectedAlert").fadeOut();
			$("#errorAlert").fadeIn();
			setTimeout(function() { $("#errorAlert").fadeOut(); }, 2000);
			$(".requiresConnect").attr("disabled",true);
			$(".requiresDisconnect").attr("disabled",false);
		}

		client.connect(connectOptions);
	}

	/*******************************************
	 *
	 * STEP 3 - Handle incoming monitoring data
	 *
	 *******************************************/
	function onMessage(msg) {
		var topic = msg.destinationName;
		var payload = JSON.parse(msg.payloadString);
		var pathBits = topic.split("/");
		if (pathBits.length != 3) { return; }
		var dataType = pathBits[2];

		switch (dataType) {
			case "Store":
				// deep copy
				storePayload = jQuery.extend(true, {}, payload);
				updateStoreData(payload);
				break;

			case "Memory":
				// deep copy
				memoryPayload = jQuery.extend(true, {}, payload);
				updateMemoryData(payload);
				break;

			case "Endpoint":
				processEndpointData(payload);
				break;

			case "Topic":
				processTopicData(payload);
				break;
		}
	}

	function processEndpointData(payload) {
		if (endpointHandle == null) {
			setTimeout(function() {
				if (endpointHandle == null) {
					updateEndpointData();
					endpointHandle = setInterval(updateEndpointData, 2000);
				}
			}, 1000);
		}
		endpointData.push({
			Name: payload.Name,
			ActiveConnections: payload.ActiveConnections,
			BadConnections: payload.BadConnections,
			TotalConnections: payload.TotalConnections,
			MsgRead: payload.MsgRead,
			MsgWrite: payload.MsgWrite,
			BytesRead: payload.BytesRead,
			BytesWrite: payload.BytesWrite,
			TotalMsgs: payload.MsgRead + payload.MsgWrite,
			TotalBytes: payload.BytesRead + payload.BytesWrite
		});
		if (endpointData.length == 1) {
			endpointPayloads = [];
		}
		endpointPayloads.push(payload);
	}

	function processTopicData(payload) {
		if (topicHandle == null) {
			setTimeout(function() {
				if (topicHandle == null) {
					updateTopicData();
					topicHandle = setInterval(updateTopicData, 2000);
				}
			}, 1000);
		}
		topicData.push({
			TopicString: payload.TopicString,
			Subscriptions: payload.Subscriptions,
			PublishedMsgs: payload.PublishedMsgs,
			RejectedMsgs: payload.RejectedMsgs
		});
		if (topicData.length == 1) {
			topicPayloads = [];
		}
		topicPayloads.push(payload);
	}

	var throughputData = [];
	function getCurrentThroughput() {
		if (throughputData.length < 2 || throughputData[0] == 0 || throughputData[throughputData.length - 1] == 0) { return 0; }
		return (throughputData[throughputData.length - 1] - throughputData[0]) / (2 * (throughputData.length - 1));
	}

	function getWidth(val, max) {
		return 100 * val / max;
	}

	var serverData = {
		ActiveConnections: 0,
		BadConnCount: 0,
		TotalConnections: 0,
		BytesRead: 0,
		BytesWrite: 0,
		MsgRead: 0,
		MsgWrite: 0
	};

	var storeData = {
		MemoryUsedPercent: 0,
		DiskUsedPercent: 0,
		DiskFreeBytes: 0
	};
	var storePayload = null;

	var memoryData = {
		MemoryTotalBytes: 0,
		MemoryFreeBytes: 0,
		MemoryFreePercent: 0,
		ServerVirtualMemoryBytes: 0,
		ServerResidentSetBytes: 0,
		MessagePayloads: 0,
		PublishSubscribe: 0,
		Destinations: 0,
		CurrentActivity: 0
	};
	var memoryPayload = null;

	var topicData = [];
	var topicHandle = null;
	var topicPayloads = [];

	var endpointData = [];
	var endpointHandle = null;
	var endpointPayloads = [];

	function getScale(conn, tput) {
		var maxConn = Math.pow(10, conn.toString().length);
		if (maxConn < 10) { maxConn = 10; }
		var maxTput = Math.pow(10, tput.toString().length);
		if (maxTput < 10) { maxTput = 10; }
		return {
			maxConn: maxConn,
			maxTput: maxTput
		}
	}

	function updateGraphLabels(maxConn, maxTput) {

		if (maxConn >= 1000 && maxConn < 1000000) {
			$("#liveConnectionsScale").html((maxConn / 1000) + "k");
		} else if (maxConn >= 1000000) {
			$("#liveConnectionsScale").html((maxConn / 1000000) + "m");
		} else {
			$("#liveConnectionsScale").html(maxConn);
		}

		if (maxConn == 10) { $("#liveConnectionsMidScale").html("5"); }
		if (maxConn == 100) { $("#liveConnectionsMidScale").html("50"); }
		if (maxConn == 1000) { $("#liveConnectionsMidScale").html("500"); }
		if (maxConn == 10000) { $("#liveConnectionsMidScale").html("5k"); }
		if (maxConn == 100000) { $("#liveConnectionsMidScale").html("50k"); }
		if (maxConn == 1000000) { $("#liveConnectionsMidScale").html("500k"); }
		if (maxConn == 10000000) { $("#liveConnectionsMidScale").html("5m"); }

		if (maxTput >= 1000 && maxTput < 1000000) {
			$("#liveThroughputScale").html((maxTput / 1000) + "k");
		} else if (maxTput >= 1000000) {
			$("#liveThroughputScale").html((maxTput / 1000000) + "m");
		} else {
			$("#liveThroughputScale").html(maxTput);
		}

		if (maxTput == 10) { $("#liveThroughputMidScale").html("5"); }
		if (maxTput == 100) { $("#liveThroughputMidScale").html("50"); }
		if (maxTput == 1000) { $("#liveThroughputMidScale").html("500"); }
		if (maxTput == 10000) { $("#liveThroughputMidScale").html("5k"); }
		if (maxTput == 100000) { $("#liveThroughputMidScale").html("50k"); }
		if (maxTput == 1000000) { $("#liveThroughputMidScale").html("500k"); }
		if (maxTput == 10000000) { $("#liveThroughputMidScale").html("5m"); }
	}

	function updateServerData() {
		// reset server stat counters
		newServerData = {
			ActiveConnections: 0,
			BadConnections: 0,
			TotalConnections: 0,
			BytesRead: 0,
			BytesWrite: 0,
			MsgRead: 0,
			MsgWrite: 0
		};

		// add up endpoint stats
		for (var i in endpointData) {
			var r = endpointData[i];
			for (var stat in newServerData) {
				newServerData[stat] += r[stat];
			}
		}

		for (var stat in newServerData) {
			var elem = $("#"+stat);
			if (stat == "BytesRead" || stat == "BytesWrite") {
				elem.html(Utils.getBytesString(newServerData[stat]));
			} else if (stat == "MsgRead" || stat == "MsgWrite") {
				elem.html(Utils.getMsgString(newServerData[stat]));
			} else {
				elem.html(Utils.addCommas(newServerData[stat]));
			}
			serverData[stat] = newServerData[stat];
		}

		var tput = parseFloat(newServerData.MsgRead) + parseFloat(newServerData.MsgWrite);
		throughputData.push(tput);
		if (throughputData.length > 2) { throughputData.splice(0, 1); }
		var scale = getScale(serverData.ActiveConnections, Math.round(getCurrentThroughput()));
		updateGraphLabels(scale.maxConn, scale.maxTput);
		$("#liveConnectionsBar").css("width", getWidth(serverData.ActiveConnections, scale.maxConn) + "%");
		$("#liveThroughputBar").css("width", getWidth(Math.round(getCurrentThroughput()), scale.maxTput) + "%");
		$("#liveConnectionsVal").html("<b>"+Utils.addCommas(serverData.ActiveConnections)+"</b>");
		$("#liveThroughputVal").html("<b>"+Utils.addCommas(Math.round(getCurrentThroughput()))+"</b>");
	}

	function updateStoreData(payload) {
		for (var stat in storeData) {
			if (stat == "DiskFreeBytes") { payload[stat] = Utils.getGB(payload[stat]); }
			var elem = $("#"+stat);
			elem.html(payload[stat]);
			storeData[stat] = payload[stat];
		}
	}

	function updateMemoryData(payload) {
		for (var stat in memoryData) {
			if (stat != "MemoryFreePercent") { payload[stat] = Utils.getGB(payload[stat]); }
			var elem = $("#"+stat);
			elem.html(payload[stat]);
			memoryData[stat] = payload[stat];
		}
	}

	function updateTopicData() {
		if (topicData.length > 0) {
			var str = "<table class='table table-condensed'>";
			str += "<tr><th>Topic String</th><th>Subscriptions</th><th>Published Msgs</th><th>Rejected Msgs</th></tr>";
			for (var i in topicData) {
				var r = topicData[i];
				str += "<tr><td>"+r.TopicString+"</td><td>"+r.Subscriptions+"</td><td>"+Utils.addCommas(r.PublishedMsgs)+"</td><td>"+Utils.addCommas(r.RejectedMsgs)+"</td></tr>";
			}
			str += "</table>";
			$("#topicMonitoringTable").html(str);
		}
		topicData = [];
	}

	function updateEndpointData() {
		updateServerData();
		if (endpointData.length > 0) {
			var str = "<table class='table table-condensed'>";
			str += "<tr><th>Endpoint</th><th>Active Connections</th><th>Total Msgs</th><th>Total Data</th></tr>";
			for (var i in endpointData) {
				var r = endpointData[i];
				str += "<tr><td>"+r.Name+"</td><td>"+r.ActiveConnections+"</td><td>"+Utils.addCommas(r.TotalMsgs)+"</td><td>"+Utils.getBytesString(r.TotalBytes)+"</td></tr>";
			}
			str += "</table>";
			$("#endpointMonitoringTable").html(str);
		}
		endpointData = [];
	}

	function updateEndpointInfo() {
		$("#infoModalTitle").html("Endpoint Monitoring Data");
		var str = "<p>Topic: <code>$SYS/ResourceStatistics/Endpoint</code></p>";
		for (var i in endpointPayloads) {
			str += "<pre>" + JSON.stringify(endpointPayloads[i], undefined, 4) + "</pre>";
		}

		$("#infoModalBody").html(str);
		$("#infoModal").modal('show');
	}

	function updateApplianceInfo() {
		$("#infoModalTitle").html("Appliance Monitoring Data");
		var str = "<p>Memory Topic: <code>$SYS/ResourceStatistics/Memory</code></p>";
		str += memoryPayload == null ? "<pre>n/a</pre>" : "<pre>" + JSON.stringify(memoryPayload, undefined, 4) + "</pre>";
		str += "<p>Store Topic: <code>$SYS/ResourceStatistics/Store</code></p>";
		str += memoryPayload == null ? "<pre>n/a</pre>" : "<pre>" + JSON.stringify(storePayload, undefined, 4) + "</pre>";
		$("#infoModalBody").html(str);
		$("#infoModal").modal('show');
	}

	function updateTopicInfo() {
		$("#infoModalTitle").html("Topic Monitoring Data");
		var str = "<p>Topic: <code>$SYS/ResourceStatistics/Topic</code></p>";
		for (var i in topicPayloads) {
			str += "<pre>" + JSON.stringify(topicPayloads[i], undefined, 4) + "</pre>";
		}

		$("#infoModalBody").html(str);
		$("#infoModal").modal('show');
	}

	// the following methods are "public"
	return {
		connect: connect,
		getClient: getClient,
		updateEndpointInfo: updateEndpointInfo,
		updateApplianceInfo: updateApplianceInfo,
		updateTopicInfo: updateTopicInfo,
		topicPayloads: topicPayloads
	};
})(window);

$(document).ready(function() {
	$(".requiresDisconnect").attr("disabled", false);
	$(".requiresConnect").attr("disabled", true);

	$("#endpointPop").popover();



	/***********************************
	 *
	 * STEP 1 - Connect to MessageSight
	 *
	 ***********************************/
	$("#connectButton").click(function(event) {
		var server = $("#connectServer").val();
		var port = $("#connectPort").val();
		MonitoringClient.connect(server, port);
	});




	$("#disconnectButton").click(function(event) {
		MonitoringClient.getClient().disconnect();
	});

	$("#endpointInfo").click(function(event) {
		MonitoringClient.updateEndpointInfo();
	});

	$("#applianceInfo").click(function(event) {
		MonitoringClient.updateApplianceInfo();
	});

	$("#topicInfo").click(function(event) {
		MonitoringClient.updateTopicInfo();
	});
});

