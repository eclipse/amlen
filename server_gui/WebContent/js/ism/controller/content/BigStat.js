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
define([
    'dojo/_base/declare',
    'dojo/_base/lang',
    'dojo/number',
    'ism/widgets/_TemplatedContent',
    'ism/MonitoringUtils',
	'dijit/layout/ContentPane',    
    'dojo/text!ism/controller/content/templates/BigStat.html',
    'dojo/i18n!ism/nls/strings',
    'ism/config/Resources'
], function(declare, lang, number, _TemplatedContent, MonitoringUtils, ContentPane, template, nlsStrings, Resources) {
 
    var BigStat = declare("ism.controller.content.BigStat", _TemplatedContent, {
        
    	templateString: template,
    	flexDashboard: undefined,
		description: "",
		iconClass: "",
		dataItem: "connVolume",
		type: "number",
		lastValue: -1,
		lastMillis: 0,
		monitoringHandle: undefined,
        
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			console.debug(this.declaredClass);
			
			if (this.refreshRateMillis == undefined && this.dataSet) {
				this.refreshRateMillis = Resources.flexDashboard[this.dataSet+'Interval'];
			}
			
			// prime msgVolume stat
			this._primeMsgVolume();
			
			this._subscribeToMonitoring();
			
			dojo.subscribe(Resources.monitoring.stopMonitoringTopic, lang.hitch(this, function(data) {
				this._unsubscribeFromMonitoring();
				this.statTextNode.innerHTML = nlsStrings.global.notAvailable;
				this.statIconNode.style.display = 'none';
			}));
			
			dojo.subscribe(Resources.monitoring.reinitMonitoringTopic, lang.hitch(this, function(data) {
				this._subscribeToMonitoring();
				// prime msgVolume stat
				this._primeMsgVolume();
			}));

		},
		
		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate() for " + this.id);
			this.inherited(arguments);
		},

		startup: function() {
			console.debug(this.declaredClass + ".startup() for " + this.id);
			this.inherited(arguments);	
			this.statTextNode.innerHTML = nlsStrings.global.notAvailable;
			
		},
		
		_subscribeToMonitoring: function() {
			if (this.dataSet && Resources.monitoring[this.dataSet+"Topic"]) {
				this.monitoringHandle = dojo.subscribe(Resources.monitoring[this.dataSet+"Topic"], lang.hitch(this, function(data) {
					this._updateStat(data);
				}));
			}			
		},
		
		_unsubscribeFromMonitoring: function() {
			if (this.monitoringHandle) {
				dojo.unsubscribe(this.monitoringHandle);
			}
		},
		
		_primeMsgVolume: function() {
			if (this.dataItem === "msgVolume") {
				MonitoringUtils.getMsgVolume(lang.hitch(this, function(data) {
					if (data.error) {
						MonitoringUtils.showMonitoringPageError(data.error);
						return;
					}
					this._updateStat(data);
				}));
			}			
		},
		
		_setFlexDashboardAttr: function(flexDashboard) {
			this.flexDashboard = flexDashboard;
			if (this.flexDashboard && this.refreshRateMillis !== undefined) {
				this.flexDashboard.set(this.dataSet+'Interval', this.refreshRateMillis);
			}
		},
		
		_updateStat: function(data) {
			if (data.error) {
				MonitoringUtils.showMonitoringPageError(data.error);
				this.statIconNode.style.display = 'none';
				return;
			}
			
			if (data[this.dataItem] !== undefined) {								
				var value = data[this.dataItem];
				/*if (this.dataItem === "msgVolume") {
					var currentMillis = new Date().getTime();
					var msgDelta = value - this.lastValue;
					if (msgDelta < 0) {
						msgDelta = 0;
					}
					var timeDelta = (currentMillis - this.lastMillis) / 1000;					
					var skip = this.lastMillis === 0;
					this.lastValue = value;
					this.lastMillis = currentMillis;	
					if (skip) {
						this.statIconNode.style.display = 'none';						
						return;
					}
					value = msgDelta === 0 ? 0 : msgDelta / timeDelta;
				}*/

				if (this.type === "number") {
					var places = 0;
					var unit = "";
					if (value >= 1000000) {
						value = value / 1000000;
						unit = " M";
					} else if (value >= 1000) {
						value = value / 1000;						
						unit = " K";
					}
					if (unit !== "" && value < 99.95) {
						places = 1;
					} 
					
					if (unit === "" && value < 1 && value > 0) {
						this.statTextNode.innerHTML = "< 1";
					} else {
						this.statTextNode.innerHTML = number.format(value, {places: places, locale: ism.user.localeInfo.localeString}) + unit;
					}
				} else if (this.type === "time") {
				   	var days = Math.floor(value / 86400);
				   	var hours = Math.floor((value % 86400) / 3600);
				   	var minutes = Math.floor(((value % 86400) % 3600) / 60);
				   	var args = [];
				   	var string = undefined;
					// convert time in seconds to days and hours display				
					if (value < 60) {
						// only up for seconds, show seconds
						args.push(value);
						string = nlsStrings.global.secondsShort;						
					} else if (value < 3600) {
						// only up for minutes, show minutes and seconds
						args.push(minutes);
						args.push((value - minutes*60)); // seconds
						string = nlsStrings.global.minutesSecondsShort;
					} else if (value < 86400) {
						// only up for hours, show hours and minutes
						args.push(hours);
						args.push(minutes);
						string = nlsStrings.global.hoursMinutesShort;						
					} else {
						// up for days, show days and hours
						args.push(days);
						args.push(hours);
						string = nlsStrings.global.daysHoursShort;						
					} 
					this.statTextNode.innerHTML = lang.replace(string, args);
				}
				if (this.iconClass !== "") {
					this.statIconNode.style.display = 'inline-block';
				}

			}
		}
		
		
    });
    
    return BigStat;
 
});
