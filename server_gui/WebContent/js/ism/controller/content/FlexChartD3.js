/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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
		'dojo/number',
		'dojo/i18n!ism/nls/home',
		'dojo/i18n!ism/nls/monitoring',		
		'ism/widgets/_TemplatedContent',
		'dojo/text!ism/controller/content/templates/FlexChart.html',
		'ism/config/Resources',
		'ism/IsmUtils',
		'ism/MonitoringUtils',
		'ism/widgets/charting/Chart'
		], function(declare, lang, locale, number, nls, nls_mon, _TemplatedContent, 
				flexchart, Resources, Utils, MonitoringUtils, Chart) {	

	var FlexChartD3 = declare("ism.controller.content.FlexChartD3", _TemplatedContent, {

		nls: nls,
		nls_mon: nls_mon,
		templateString: flexchart,
    	flexDashboard: undefined,

		chart: null,
		chartData: [],
		dataError: false,
		chartNodeId: null,
		warnArea: null,
		inited: false,
		legend: {},
		legendTitle: "",
		duration: 600,
		numPoints: 360,  
		dataItem: "default",
		chartWidth: "200px",
		chartHeight: "200px",
		description: "",
		seriesMap: {},
		lastServerStatus: null,
		lastHaRole: null,
		isStopped: false,
		_legendOptions: undefined,
		horizontalLineAdded: false,
		
		constructor: function(args) {			
			dojo.safeMixin(this, args);
			this.chartNodeId = this.id + "_chartNode";
			this.chartLegendId = this.id + "_chartLegend";
			this.legendTitle = this.legend.title ? this.legend.title : "";
			
			if (this.refreshRateMillis == undefined && this.dataSet) {
				this.refreshRateMillis = Resources.flexDashboard[this.dataSet+'Interval'];
			}
			
			if (this.dataSet && Resources.monitoring[this.dataSet+"Topic"]) {
				dojo.subscribe(Resources.monitoring[this.dataSet+"Topic"], lang.hitch(this, function(data) {
					if (data.error) {
						MonitoringUtils.showMonitoringPageError(data.error);
						this.dataError = true;
					} else if (data.ErrorString && data.RC !== 0) {
						MonitoringUtils.showMonitoringPageError(data);
						this.dataError = true;
					} else if (this.dataError) {
						this.dataError = false;
						Utils.updateStatus();
						if (this.historicalDataFunction) {
							// we want everything to be reloaded
							this._loadHistoricalData();
							return;
						}
					} 
					if (!this.dataError) {
						this._updateChart(data);
					}
				}));
			}
		},

		startup: function() {
			this._initChart();			
		},
		
		_setFlexDashboardAttr: function(flexDashboard) {
			this.flexDashboard = flexDashboard;
			if (!flexDashboard) return;
			if (this.refreshRateMillis !== undefined) {
				this.flexDashboard.set(this.dataSet+'Interval', this.refreshRateMillis);
			} 
		},
		
		_initChart: function() {
			if (this.inited) {
				return;
			}
			this.inited = true;

			var width = this.chartWidth;
			var index = width.indexOf("px");
			width = index > 0 ? width.substring(0, index) : width;
			var height = this.chartHeight;
			index = height.indexOf("px");
			height = index > 0 ? height.substring(0, index) : height;
			var options = {margin: 		 this.margin, 
						   width: 		 width, 
						   height: 		 height,
						   themeColor:	 this.startingColor,
						   yAxisOptions: this.yAxisOptions,
						   xAxisOptions: this.xAxisOptions,
						   stacked:		 this.stackedAreas,
						   pie:			 this.pie,
						   radius:		 this.radius,
						   colors: 		 this.colors,
						   brushMargin:  this.brushMargin};
			
			this.chart = new Chart(this.chartNodeId, options); 
			
			// clear the data
			this.chartData = [];
			
			this["_init"+this.type]();
			
			// Add the series of data and render			
			this.chart.addSeries(this.dataItem,this.chartData, this.seriesMap);			
	        this.chart.render();
			if (this._legendOptions) {
				// show the legend
		        this.legendDiv.style.display = "inline-block";
		        if (this.stackedAreas) {
		        	this._legendOptions.checkboxes = true;
		        	this.chart.addLegend(this.chartLegendId, this.id, this._legendOptions);
	        		dojo.subscribe(this.id, lang.hitch(this, function(message) {
	        			this._toggleSeries(message);
	        		}));		        	
		        } else {
		        	this.chart.addLegend(this.chartLegendId, this.id, this._legendOptions);
		        }
			}
		},	
		
		_updateChart: function(data) {				
			this._updateD3(data);			
			this.chart.updateSeries(this.dataItem,this.chartData);
			if (this.horizontalLine && !this.horizontalLineAdded) {
			    var yValueDataItem = this.horizontalLine.yValue;
			    if (yValueDataItem && data[yValueDataItem] !== undefined) {
	                this.horizontalLineAdded = true;
			        this.horizontalLine.yValue = data[yValueDataItem];
	                console.log("horizontal line yValue: " + this.horizontalLine.yValue);
			        this.chart.addHorizontalLine(this.horizontalLine);
	            }
			}
			if (this.warnMessage) {			    
			    var message = data[this.warnMessage] ? data[this.warnMessage] : "";
			    if (message !== "") {
			        message = "<img src='css/images/msgWarning16.png' width='12' height='12' alt=''/>  " + message;
			    }
			    this.warnArea.innerHTML = message;  
			}
			this.chart.render();						
		},

		
		_toggleSeries: function(message) {
			if (!message.name) {
				console.debug("_toggleSeries message has no name");
				return;
			}
			var name = message.name;
			var seriesInfo = this.seriesMap[name];
			if (!seriesInfo) {
				console.debug("_toggleSeries seriesInfo not found for " + name);
				return;
			}
			
			ism.user.setStatSelected(this.id+"_"+name, message.checked);			
			seriesInfo.included = message.checked;
			
			var scaleYAxis = !message.checked;
			if (!scaleYAxis) {
				// see if any others are not included
				for (var series in this.seriesMap) {
					if (!this.seriesMap[series].included) {
						scaleYAxis = true;
						break;
					}
				}
				// if all are checked, respect max setting
				if (!scaleYAxis) {
					scaleYAxis = this.yAxisOptions.max === undefined;
				}				
			}		
			
			this.chart.setScaleYAxis(scaleYAxis);
			this.chart.updateSeries(this.dataItem,this.chartData);
			this.chart.render();			
		},

		_loadHistoricalData: function() {
			if (!this.historicalDataMetric || !this.historicalDataFunction) {
				return;
			}
			MonitoringUtils[this.historicalDataFunction](this.duration, { metric: this.historicalDataMetric, numPoints: this.numPoints, onComplete: lang.hitch(this, function(data) {
				if (data.error || (data.RC !== undefined && data.RC !== "0")) {
					var error = data.error ? data.error : data;
					MonitoringUtils.showMonitoringPageError(error);
					console.log("An error occurred getting the historical data for " + this.dataItem);
					this.dataError = true;
					return;
				}

				var now = data.LastUpdateTimestamp ? new Date(data.LastUpdateTimestamp) : new Date();
				var delta = this.flexDashboard.get(this.dataSet+'Interval');
				if (!delta || delta < 1) {
					delta = 5000;	// milliseconds between each data point
				}
				var chartData = [];
				var num = 0;
				var i = 0;
				var timeDiff = 0;
				if (this.stackedAreas) {
					num = data[this.dataItem[0]].length;
					for (i = 0; i < num; i++ )	{
						timeDiff = (num - (i+1)) * delta;  // milliseconds since current time
						var value = {};
						for (var n = 0, numItems=this.dataItem.length; n < numItems; n++) {
							var prop = this.dataItem[n];
							value[prop] = data[prop][i];
						}
						chartData[i] = {date: new Date(now - timeDiff), value: value};
					}

				} else {
					var myData = data[this.historicalDataMetric] ? data[this.historicalDataMetric] : data;
					num = data.length ? data.length : data[this.historicalDataMetric].length;
					for (i = 0; i < num; i++) {
						timeDiff = (num - (i+1)) * delta;  // milliseconds since current time
						chartData[i] = {date: new Date(now - timeDiff), value: myData[i]};					
					}	
				}
				if (chartData.length === 0) {
					return;
				}
				this.chartData = chartData;				
				this.chart.updateSeries(this.dataItem,this.chartData);
				
				console.log("_loadHistoricalData rending chart for " + this.dataItem);
				this.chart.render();
			})});						
		},
		
		_initD3: function() {
			
			var chartData = [];
			
			if (this.pie) {			
				if (this.series) {
					for (var i = 0, len=this.series.length; i < len; i++) {
						var slice = this.series[i];
						slice.legendLabel = slice.legend;
						this.chartData[i] = slice;					
					}
				}				
				// show the legend, right now assumes replacement variable of up time
				this._legendOptions = {legend: this.legend};
	        	if (this.legend.note && this.flexDashboard) {
	        		var serverUpTime = this.flexDashboard.get("serverUpTime");
	        		if (serverUpTime) {
		        		var formattedDate = locale.format(serverUpTime, { datePattern: "yyyy-MM-dd", timePattern: "HH:mm (z)" });
		        		this.legendNote.innerHTML = lang.replace(this.legend.note, [formattedDate]);
	        		}
	        	}
	        	return;
			}
			
			var now = new Date();
			var delta = this.flexDashboard.get(this.dataSet+'Interval');
			if (!delta || delta < 1) {
				delta = 5000;	// milliseconds between each data point
			}
			var num = this.numPoints+1;
			
			var value = 0;
			var scaleYAxis = this.yAxisOptions.max === undefined;
			if (this.stackedAreas) {
				value = {};
				this.seriesMap = {};
				for (var n = 0, numItems=this.dataItem.length; n < numItems; n++) {
					var item = this.dataItem[n];
					var legend = this.legend[item] ? this.legend[item] : item;
					var detail = this.legenddetail && this.legenddetail[item] ? this.legenddetail[item] : undefined;
					value[item] = 0;
					var included = ism.user.getStatSelected(this.id+"_"+item, true);
					if (!included) {
						scaleYAxis = true;
					}
					this.seriesMap[item] = {legend: legend, included: included, detail: detail};
				}
				this.chart.setScaleYAxis(scaleYAxis);
		        this._legendOptions = {legend: this.legend};
        		this.legendTd.style.verticalAlign = "top";
        		this.legendTd.style.padding = "30px 0 0 0";
        		this.chartLegendDiv.style.padding = "10px";
			}
			
			for (var j = 0; j < num; j++) {
				var timeDiff = (num - (j+1)) * delta;  // milliseconds since current time
				chartData[j] = {date: new Date(now - timeDiff), value: value};					
			}	
		
			this.chartData = chartData;	
			this._loadHistoricalData();
		},
		
		_updateD3: function(data) {			

			if (this.pie) {
				this._updatePie(data);
				return;
			}

			var now = data.LastUpdateTimestamp ? new Date(data.LastUpdateTimestamp) : new Date();
			var value = undefined;
            var dataItemLen = this.dataItem.length;
			if (this.fullRefresh) {
				// reload of all the data
				console.log("doing a full refresh");
				var num = 0;
				if (this.stackedAreas) {
					for (i = 0; i < dataItemLen; i++) {
						if (!data[this.dataItem[i]]) {
							console.log("cannot refresh, my data item is missing!");
							return;
						}
						num = data[this.dataItem[i]].length > num ? data[this.dataItem[i]].length : num;					
					}					
				} else {
					if (!data[this.dataItem]) {
						console.log("cannot refresh, my data item is missing!");
						return;
					}
					num = data[this.dataItem].length;
				}
				var chartData = [];				
				var delta = this.flexDashboard.get(this.dataSet+'Interval');
				if (!delta || delta < 1) {
					delta = 5000;	// milliseconds between each data point
				}

				var i = 0;
				var timeDiff = 0;
				if (this.stackedAreas) {
					for (i = 0; i < num; i++ )	{
						timeDiff = (num - (i+1)) * delta;  // milliseconds since current time
						value = {};
						for (var n = 0; n < dataItemLen; n++) {
							var prop = this.dataItem[n];
							value[prop] = data[prop][i];
						}
						chartData[i] = {date: new Date(now - timeDiff), value: value};
					}
				} else {
					for (i = 0; i < num; i++) {
						timeDiff = (num - (i+1)) * delta;  // milliseconds since current time
						chartData[i] = {date: new Date(now - timeDiff), value: data[this.dataItem][i]};					
					}	
				}
				
				if (chartData.length === 0) {
					return;
				}
				
				//this.chartData = data[this.dataItem];
				this.chartData = chartData;		
				
			} else {			
				this.chartData.splice(0,1);
				value = 0;
				if (!this.dataError) {
					if (this.stackedAreas) {
						value = {};
						for (var m = 0; m < dataItemLen; m++) {
							var item = this.dataItem[m];
							value[item] = data[item];
						}					
					} else {
						value = data[this.dataItem];
					}
				}
				this.chartData.push({date: new Date(now), value: value});
			}
		},
		
		_updatePie: function(data) {
			console.debug("_updatePie called with " + JSON.stringify(data));
			var slice = undefined;
			var seriesLen = this.series ? this.series.length : 0;
			if (data.error || data.RC) {
				this.chartData = [];
				if (this.series) {
					for (var i=0; i < seriesLen; i++) {
						slice = this.series[i];
						slice.y = 0;
						slice.text = nls_mon.monitoring.noData;
						slice.legend = slice.legendLabel + " (" + slice.text + ")";
						this.chartData[i] = slice;
					}
				}	
        		this.legendNote.innerHTML = nls_mon.monitoring.noData;
				return;
			}

			this.chartData = [];
			if (this.series) {
				// workaround for svg rendering issue - scale values
				var total = 0;
				for (var j=0; j < seriesLen; j++) {
					slice = this.series[j];
					if (slice.dataItem && data[slice.dataItem] !== undefined) {
						total += data[slice.dataItem];
					}
				}
				
				for (var k=0; k < seriesLen; k++) {
					slice = this.series[k];
					if (slice.dataItem && data[slice.dataItem] !== undefined) {
						slice.y = data[slice.dataItem];
						var unit = "";
						var value = slice.y;
						if (slice.y >= 1000000000) { // >= 1 Billion? Show as Millions
							value = slice.y / 1000000;
							unit = " M";
						} else if (slice.y >= 1000000) { // >= 1 Million? Show as Thousands
							value = slice.y / 1000;
							unit = " K";
						}
						slice.text = number.format(value, {places: 0, locale: ism.user.localeInfo.localeString}) + unit; 
						slice.legend = slice.legendLabel + " (" + slice.text + ")";
						// If the total is large, scale the slice.  Goal is to get any non-zero slice to show as a 
						// sliver without causing the rest of the pie to go wonky
						if (total > 100000 && slice.y > 0) {
							slice.y = Math.round((slice.y / total) * 100);
							if (slice.y < 0.1) {
								slice.y = 0.1;
							}
							console.debug("Scaling " + slice.text + " to " + slice.y);
						}
					}
					this.chartData[k] = slice;
				}
			}	

			if (this.legend.note && this.flexDashboard) {
        		var serverUpTime = this.flexDashboard.get("serverUpTime");
        		if (serverUpTime) {
	        		this.legendNote.innerHTML = lang.replace(this.legend.note, [Utils.getFormattedDateTime(serverUpTime)]);
        		}
        	}

		}		
	});

	return FlexChartD3;
});
