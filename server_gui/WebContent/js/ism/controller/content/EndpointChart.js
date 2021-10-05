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
		'dojo/date/locale',
		'dojo/sniff',
		'dojo/dom-style',
		'dojo/dom-geometry',        
		'dijit/_base/manager',
		'dijit/form/Button',
		'dojo/i18n!ism/nls/monitoring',
		'ism/widgets/_TemplatedContent',
		'dojo/text!ism/controller/content/templates/EndpointChart.html',
		'ism/config/Resources',
		'ism/IsmUtils',
		'ism/MonitoringUtils',
		'ism/widgets/charting/Chart'
		], function(declare, lang, locale, sniff, domStyle, domGeom, manager, Button, 
				nls, _TemplatedContent, connectchart, Resources, Utils,	MonitoringUtils, Chart) {

	return declare("ism.controller.content.EndpointChart", _TemplatedContent, {

		nls: nls,
		templateString: connectchart,
		chart: null,
		dataPoints: [],
		height: 400,
		margin: {left: 80, top: 25, right: 20, bottom: 110},
		brushMargin: {left: 80, top: 320, right: 20, bottom: 60},
		currentEndpoint: null,
		currentDataset: {
			name: null,
			chartLabel: null
		},
		currentDuration: 3600,
		numPoints: 720,  // data points to display in the chart
		dataCollectionIntervalLabel: nls.monitoring.interval.fiveSeconds,
		optionsChanged: false,
		xAxisLayout: {
			"dur_300":   {numPoints: 60,   ticks: {unit: "second", value: 30}, minorTicks: {unit: "second", value: 5},  dataCollectionIntervalLabel: nls.monitoring.interval.fiveSeconds},
			"dur_600":   {numPoints: 120,  ticks: {unit: "minute", value: 1},  minorTicks: {unit: "second", value: 10}, dataCollectionIntervalLabel: nls.monitoring.interval.fiveSeconds},
			"dur_1800":  {numPoints: 360,  ticks: {unit: "minute", value: 3},  minorTicks: {unit: "second", value: 30}, dataCollectionIntervalLabel: nls.monitoring.interval.fiveSeconds},
			"dur_3600":  {numPoints: 720,  ticks: {unit: "minute", value: 5},  minorTicks: {unit: "minute", value: 1},  dataCollectionIntervalLabel: nls.monitoring.interval.fiveSeconds}, 
			"dur_10800": {numPoints: 180,  ticks: {unit: "minute", value: 15}, minorTicks: {unit: "minute", value: 5},  dataCollectionIntervalLabel: nls.monitoring.interval.oneMinute}, 
			"dur_21600": {numPoints: 360,  ticks: {unit: "minute", value: 30}, minorTicks: {unit: "minute", value: 5},  dataCollectionIntervalLabel: nls.monitoring.interval.oneMinute}, 
			"dur_43200": {numPoints: 720,  ticks: {unit: "hour", value: 1},    minorTicks: {unit: "minute", value: 10}, dataCollectionIntervalLabel: nls.monitoring.interval.oneMinute}, 
			"dur_86400": {numPoints: 1440, ticks: {unit: "hour", value: 2},    minorTicks: {unit: "minute", value: 30}, dataCollectionIntervalLabel: nls.monitoring.interval.oneMinute} 
		},
		xAxisOptions: {
			ticks: {unit: "minute", value: 5}, 
			minorTicks: {unit: "minute", value: 1} 
		},
		yAxisOptions: {
			integers: true,
			legend: ""
		},
		chartNodeId: "endpointChart_node",

		
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
		},
		
		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");
			this.inherited(arguments);
			
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.monitoring.nav.endpointStatistics.topic+":onShow", lang.hitch(this, function(message){
				console.debug(this.declaredClass + " refreshing grid");	
				this.updateChart();					
			}));	
			
			this.changeDuration(this.durationCombo.get('value'));

			this._setupFields(lang.hitch(this, function() {
				this.updateChart();
			}));
		},

		_setupFields: function(onComplete) {
			console.log(this.declaredClass + ".setupFields");
			
			// Populate endpoints combo
			this.endpointCombo.store.newItem({ name: nls.monitoring.grid.allEndpoints, id: nls.monitoring.grid.allEndpoints });		
			this.endpointCombo.store.save();

			MonitoringUtils.getEndpointNames(lang.hitch(this, function(namesValue) {
				if (namesValue.error) {
					console.debug("Error from getEndpointNames: "+namesValue.error);
					MonitoringUtils.showMonitoringPageError(namesValue.error);
					return;
				}

				namesValue.sort();
				
				// add items to combo widget
				dojo.forEach(namesValue, function(item, index) {
					this.endpointCombo.store.newItem({ name: Utils.textDecorator(item), id: item});
				},this);
				this.endpointCombo.store.save();
				
				this.currentEndpoint = nls.monitoring.grid.allEndpoints;
				this.currentDataset.name = MonitoringUtils.Endpoint.Metric.ACTIVE_CONNECTIONS.name;
				this.currentDataset.chartLabel = MonitoringUtils.Endpoint.Metric.ACTIVE_CONNECTIONS.chartLabel;
			    this.yAxisOptions.legend = this.currentDataset.chartLabel;				
				
				// Populate datasets combo
				for (var key in MonitoringUtils.Endpoint.Metric) {
					if (key != "TOTAL_CONNECTIONS") {
						this.datasetCombo.store.newItem({ name: MonitoringUtils.Endpoint.Metric[key].label, key: key });
					}
				}
				this.datasetCombo.store.save();
				if (onComplete) {
					onComplete();
				}
			}));
		},

		createChart: function() {
			console.debug(this.declaredClass + ".createChart()");
		    var box = domGeom.getContentBox(this.chartNodeId, domStyle.get(this.chartNodeId));
			var options = 
				{  margin: 		 this.margin, 
				   width: 		 box.w, 
				   height: 		 this.height,
				   themeColor:	 "blue",
				   zoomBox:		 false,
				   brushMargin:  this.brushMargin,
				   yAxisOptions: this.yAxisOptions,
				   xAxisOptions: this.xAxisOptions };
		
			this.chart = new Chart(this.chartNodeId, options); 
			this.chart.addSeries(this.currentDataset.chartLabel, this.dataPoints);

			this.getDataAndDraw(this.currentDuration, this.currentEndpoint, this.currentDataset.name, this.numPoints);
		},

		getDataAndDraw: function(duration, endpoint, metric, numPoints) {
			console.debug(this.declaredClass + ".getData()");
			
			var requestEndpoint = endpoint !== nls.monitoring.grid.allEndpoints ? endpoint : undefined;
			MonitoringUtils.getEndpointHistory(duration, { endpoint: requestEndpoint, metric: metric, numPoints: numPoints, onComplete: lang.hitch(this, function(data) {
				console.log("for endpoint " + endpoint + ", got data: ", data);
				if (data.error) {
					console.debug("Error from getEndpointHistory: "+data.error);
					MonitoringUtils.showMonitoringPageError(data.error);
					return;
				}
				var type = metric ? metric : data.Type;
				this.dataPoints = [];
		
				var myData = data[type];
				var now = data.LastUpdateTimestamp ? new Date(data.LastUpdateTimestamp) : new Date();
				var num = myData.length;
				var delta = data.Interval ? data.Interval * 1000 : 5000;
				for (var i = 0; i < num; i++) {
					var timeDiff = (num - (i+1)) * delta;  // milliseconds since current time
					this.dataPoints[i] = {date: new Date(now - timeDiff), value: myData[i]};					
				}								

				this._drawAndRender();
				this.lastUpdateTime.innerHTML = "<b>" + Utils.getFormattedDateTime(now) + "</b>";
				Utils.flashElement(this.lastUpdateTime.id);
				this.dataCollectionInterval.innerHTML = "<b>" + this.dataCollectionIntervalLabel + "</b>";
				Utils.flashElement(this.dataCollectionInterval.id);
			})});
			
		},

		_drawAndRender: function() {
			console.debug("drawAndRender()");
			this.chart.updateSeries(this.currentDataset.chartLabel, this.dataPoints);								
			this.chart.render();
		},
		
		changeEndpoint: function(endpoint) {
			console.debug(this.declaredClass + ".changeEndpoint("+endpoint+")");
			this.currentEndpoint = endpoint;
		},
		
		changeDataset: function(sel) {
			console.debug(this.declaredClass + ".changeDataset("+sel+")");
	
			var selKey = null;
			for (var key in MonitoringUtils.Endpoint.Metric) {
				if (sel == MonitoringUtils.Endpoint.Metric[key].label) {
					this.currentDataset.name = MonitoringUtils.Endpoint.Metric[key].name;	
					this.currentDataset.chartLabel = MonitoringUtils.Endpoint.Metric[key].chartLabel;
				    this.yAxisOptions.legend = this.currentDataset.chartLabel;
				    this.yAxisOptions.integers = MonitoringUtils.Endpoint.Metric[key].integers;
				    this.optionsChanged = true;
				    break;
				}
			}
		},

		changeDuration: function(duration) {
			console.debug(this.declaredClass + ".changeDuration("+duration+")");
			this.currentDuration = duration;
			var layout = this.xAxisLayout["dur_"+this.currentDuration];
			this.numPoints = layout.numPoints;
			this.xAxisOptions.ticks = layout.ticks;
			this.xAxisOptions.minorTicks = layout.minorTicks;
			this.dataCollectionIntervalLabel = layout.dataCollectionIntervalLabel;
			this.optionsChanged = true;
		},

		updateChart: function() {
			console.debug(this.declaredClass + ".updateChart()");
			if (this.chart) { 
				// refresh the endpoint names then rebuild the chart
				var t = this;
				MonitoringUtils.getEndpointNames(function(namesValue) {
					if (namesValue.error) {
						console.debug("Error from getEndpointNames: "+namesValue.error);
						MonitoringUtils.showMonitoringPageError(namesValue.error);
						return;
					}

					t.endpointCombo.store.fetch({
						onComplete: function(items, request) {
							console.debug("deleting items");
							var len = items ? items.length : 0;
							for (var i=0; i < len; i++) {
								t.endpointCombo.store.deleteItem(items[i]);
							} 
							t.endpointCombo.store.save({onComplete: function() {
								t.endpointCombo.store.newItem({ name: nls.monitoring.grid.allEndpoints, id: nls.monitoring.grid.allEndpoints });		
								namesValue.sort();							
								dojo.forEach(namesValue, function(item, index) {
									t.endpointCombo.store.newItem({ name: Utils.textDecorator(item), id: item });
								},t);
								t.endpointCombo.store.save({onComplete: function() {
									t.endpointCombo.set('value', t.currentEndpoint);
									if (t.optionsChanged) {
										// rebuild the chart
										console.log("Options changed, rebuilding chart");
										t.optionsChanged = false;
										t.chart.destroy(); 
										t.createChart();
									} else {
										// just refresh the data
										t.getDataAndDraw(t.currentDuration, t.currentEndpoint, t.currentDataset.name, t.numPoints);										
									}
								}});					
							}});
						}});					
				});

			} else {
				this.createChart();
			}
		}
	});

});
