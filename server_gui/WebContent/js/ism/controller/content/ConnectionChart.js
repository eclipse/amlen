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
define(['dojo/_base/declare',
		'dojo/_base/lang',
		'dojo/_base/Deferred',
		'dojo/dom',
		'dojo/date/locale',
		'dojo/dom-style',
		'dojo/dom-geometry',        
		'dijit/_base/manager',
		'dijit/form/Button',
		'dojo/i18n!ism/nls/monitoring',
		'ism/widgets/_TemplatedContent',
		'dojo/text!ism/controller/content/templates/ConnectionChart.html',
		'ism/config/Resources',
		'ism/IsmUtils',
		'ism/MonitoringUtils',
		'ism/widgets/charting/Chart'
		], function(declare, lang, Deferred, dom, locale, domStyle, domGeom, manager, Button, nls, 
				_TemplatedContent, connectchart, Resources, Utils, MonitoringUtils, Chart) {

	return declare("ism.controller.content.ConnectionChart", _TemplatedContent, {

		nls: nls,
		templateString: connectchart,

		activeConnData: null,
		newConnData: null,
		updateInterval: 5000, //  5 seconds
		updatePoints: 360,  // data points to display in the chart (30 minutes)
		duration: 1800, // 30 minutes
		isPaused: false,
		isStopped: false,		
		lastServerStatus: null,
		lastHaRole: null,
		timeoutHandle: undefined,
		activeChartOptions: {
			margin: {left: 80, top: 25, right: 20, bottom: 40},
 		    height: 200,
 		    width: 600,
 		    themeColor:	 "blue",
			xAxisOptions: {
				ticks: {unit: "minute", value: 2}, 
				minorTicks: {unit: "second", value: 15} 
			},
			yAxisOptions: {
				integers: true,
				legend: nls.monitoring.connectionVolumeAxisTitle
			}
		},
		newChartOptions: {
			margin: {left: 80, top: 25, right: 20, bottom: 40},
 		    height: 200,
 		    width: 600,
 		    themeColor:	 "teal",
 		    bar: true,
 		    mirror: {
 		    	color: "green",
 		    	legend: nls.monitoring.closedConnections
 		    },
			xAxisOptions: {
				ticks: {unit: "minute", value: 2}, 
				minorTicks: {unit: "second", value: 15},
	 		    legend: nls.monitoring.newConnections
			},
			yAxisOptions: {
				integers: true,
				legend: nls.monitoring.connectionRateAxisTitle
			}
		},

		
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			
			// subscribe to server status changes
			// Note: since the server is in maintenance mode when the HA Role is not ACTIVE, don't
			// need to check it, too			
			dojo.subscribe(Resources.serverStatus.adminEndpointStatusChangedTopic, lang.hitch(this, function(data) {
				this._handleServerStatus(data);
			}));

			// when we navigate to a different monitoring tab, stop the chart updates
			dijit.byId("mainContent").watch("selectedChildWidget", lang.hitch(this, function(name, oldValue, newValue) {
				if (newValue.id != "connectionStatistics") {
					console.debug("switching away, stopping connection chart");
					// clear any timeout
					if (this.timeoutHandle) {
						window.clearTimeout(this.timeoutHandle);
					}
					// clear paused state (for consistent behavior when switching to tasks on same/different tab)
					if (this.isPaused) {
						this.isPaused = false;
						this.pauseButton.set('label',nls.monitoring.pauseButton);
					}
				} else { 
					console.debug("switching to, starting connection chart");
					this.updateChart();
				}
			}));
		},
		
		postCreate: function() {		
			console.debug(this.declaredClass + ".startup()");
			this.inherited(arguments);

			this._initChart();
		},

		_handleServerStatus: function(data) {
			var statusRC = data.RC;
			var harole = data.harole;
			console.debug(this.declaredClass + "._handleServerStatus("+statusRC+")");
			
			if (this.lastServerStatus != statusRC || this.lastHaRole != harole) {
				this.lastServerStatus = statusRC;
				this.lastHaRole = harole;
				console.debug("Server status changed");
				if (Utils.isWarnMode(statusRC) || Utils.isErrorMode(statusRC) || Utils.isPrimarySynchronizing(harole)) {
					console.debug("Server down - add specific warning...");
					this.isStopped = true;
					dom.byId("ConnectionChartWarnArea").innerHTML = "<br/>" + 
						"<img src='css/images/msgWarning16.png' width='16' height='16' alt=''/> " + 
						nls.monitoring.monitoringNotAvailable;
				} else {
					console.debug("Server up, clear any warnings...");
					dom.byId("ConnectionChartWarnArea").innerHTML = "";
				}
			}
		},
		
		_initChart: function() {
			
		    var box = domGeom.getContentBox("activeConnectionsChartNode", domStyle.get("activeConnectionsChartNode"));
		    this.activeChartOptions.width = box.w;
		    box = domGeom.getContentBox("newConnectionsChartNode", domStyle.get("newConnectionsChartNode"));
		    this.newChartOptions.width = box.w;
			
			this.activeChart = new Chart("activeConnectionsChartNode", this.activeChartOptions);
			this.newChart = new Chart("newConnectionsChartNode", this.newChartOptions);
									        
			this._initChartData();
	       
	        // Add the series of data
	        this.activeChart.addSeries(nls.monitoring.connectionVolume,this.activeConnData);
	        this.newChart.addSeries(nls.monitoring.connectionRate, this.newConnData);
	        // render charts
            this.activeChart.render();            
            this.newChart.render();
            this.newChart.addLegend("newConnectionsChartLegend", null, {horizontal: true, legend: {"value": nls.monitoring.newConnections, "mirrorValue": nls.monitoring.closedConnections}});
			// kick off first update
			this.updateChart();
		},
		
		_initChartData: function() {
			this.activeConnData = [];
			this.newConnData = [];			
		},
		
		updateChart: function() {

			// schedule next update
			this.timeoutHandle = setTimeout(lang.hitch(this, function(){
				this.updateChart();
			}), this.updateInterval);				

			var activeTime = new Date().getTime();
			var newTime = activeTime;
			var activeData = [];
			var newData = [];
			
			var activeResults = new Deferred();
			MonitoringUtils.getEndpointHistory(this.duration, { metric: "ActiveConnections", numPoints: this.updatePoints, onComplete: lang.hitch(this, function(data) {				
				if (!this._checkResults(data)) {
					activeResults.resolve(false);
					return;
				}				
				activeData = data.ActiveConnections;
				activeTime = data.LastUpdateTimestamp;
				activeResults.resolve(true);				
			})});
			var newResults = new Deferred();
			MonitoringUtils.getEndpointHistory(this.duration, { metric: "Connections", numPoints: this.updatePoints, onComplete: lang.hitch(this, function(data) {				
				if (!this._checkResults(data)) {
					newResults.resolve(false);
					return;
				}	
				// for new connections, we need to calculate the deltas
				newData[0] = 0; // first one will be zero
				for (var i = 1, len=data.Connections.length; i < len; i++) {
					newData[i] = Math.max(0, data.Connections[i] - data.Connections[i-1]); // no negative rates 
				}				
				newTime = data.LastUpdateTimestamp;								
				newResults.resolve(true);
			})});
			
			var dl = new dojo.DeferredList([activeResults, newResults]);						
			dl.then(lang.hitch(this,function(results) {
				if (!results[0][1] || !results[1][1] || this.isPaused) {
					// at least one failed, push a no data label and 0 data items
					// don't push a 'no data' label because the whole thing will just get
					// updated on next good data load
					// Check if we already know of an error condition...
					if (!this.isStopped) {
						this.isStopped = true;
						// error flag was not set - add a warning message
						dom.byId("ConnectionChartWarnArea").innerHTML = "<br/>" +
							"<img src='css/images/msgWarning16.png' width='16' height='16' alt=''/> " +
							nls.monitoring.monitoringError;
						// server could be the cause - fire a server status event
						Utils.updateStatus();
					}
					return;
				} 
				
				// data was obtained from the server...
				if (this.isStopped) {
					// there was a previous error - clear it and fire server status event.
					this.isStopped = false;
					Utils.updateStatus();
				}
				
				// update data
				this.activeConnData = [];
				var num = activeData.length;
				var delta = this.duration / this.updatePoints * 1000;
				var i;
				var timeDiff;
				for (i = 0; i < num; i++) {
					timeDiff = (num - (i+1)) * delta;  // milliseconds since current time
					this.activeConnData[i] = {date: new Date(activeTime - timeDiff), value: activeData[i]};					
				}
				this.newConnData = [];
				num = Math.max(num, newData.length);

				for (i = 0; i < num; i++) {
					timeDiff = (num - (i+1)) * delta;  // milliseconds since current time
					// calculate how many connections closed
					// Closed connections = New connections - Delta active connections during the same time interval
					var closedValue = 0;
					if (i > 0) {						
						closedValue = Math.max(0, newData[i] - (activeData[i] - activeData[i-1])); // No negative rates
						console.debug("i = " + i + "; activeData[i]: " + activeData[i] + 
								"; activeData[i-1]: " + activeData[i-1] +"; newData: " + newData[i] +
								"; closedValue: " + closedValue);
					}
					this.newConnData[i] = {date: new Date(newTime - timeDiff), value: newData[i], mirrorValue: closedValue};					
				}	
								

				// update charts
				this.activeChart.updateSeries(nls.monitoring.connectionVolume,this.activeConnData);
				this.newChart.updateSeries(nls.monitoring.connectionRate, this.newConnData);
				// render the charts
				this.activeChart.render();
				this.newChart.render();
			}));			
		},
		
		_checkResults: function(data) {
			// handle case where the page was switched or we paused during the request
			if (!this.timeoutHandle) {
				return false;
			}

			if (data.error) {
				console.debug("Error from getEndpointHistory: "+data.error);
				return false;
			} 

			return true;
		},
		
		pauseUpdate: function() {
			console.debug(this.declaredClass + ".pauseUpdate()");
			if (this.isPaused) {
				this.isPaused = false;
				// update the chart
				this.updateChart();
				this.pauseButton.set('label',nls.monitoring.pauseButton);		
			} else {
				this.isPaused = true;
				// stop the chart updates
				if (this.timeoutHandle) {
					window.clearTimeout(this.timeoutHandle);
				}				
				this.pauseButton.set('label',nls.monitoring.resumeButton);
			}
		}
	});

});
