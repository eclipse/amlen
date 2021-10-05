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
    "dojo/_base/declare",
    'dojo/_base/lang',
    'ism/widgets/_TemplatedContent',
    'dojo/dom',
	'dojo/dom-construct',
	'dojo/query',
	'dojo/dom-style',
    'dojo/number',
	'dijit/layout/ContentPane',    
    'dojo/text!ism/controller/content/templates/SystemResourcesPane.html',
	'dojo/i18n!ism/nls/home',
	'dojo/i18n!ism/nls/monitoring',
	'dojo/i18n!ism/nls/strings',
	'dijit/ProgressBar',
	'ism/config/Resources',
    'ism/IsmUtils'
], function(declare, lang, _TemplatedContent, dom, domConstruct, query, domStyle, number, ContentPane, 
		template, nls, nls_mon, nlsStrings, ProgressBar, Resources, Utils) {
 
    return declare("ism.controller.content.SystemResourcesPane", _TemplatedContent, {
        
    	templateString: template,
    	nls: nls,    	
    	resourcesContainerId: undefined,
    	flexDashboard: undefined,
		description: "",
		alertIcon: "<img src=\"css/images/msgError16.png\" title='{0}' alt='" + 
			nls.home.dashboards.flex.applianceQS.alertThresholdAltText + "'/>",
		warningIcon: "<img src=\"css/images/msgWarning16.png\" title='{0}' alt='" + 
			nls.home.dashboards.flex.applianceQS.warningThresholdAltText + "'/>",
        
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			this.resourcesContainerId = this.id + "_resourcesContainerId";
		},
		
		postCreate: function() {
			this.inherited(arguments);
		},
		
		startup: function() {
			this.inherited(arguments);
			this._createBars(this.storeData);
			this._createBars(this.memoryData);
			// add buffered messages row
			this.bufferedMsgsId = this.id+"_bufferedMsgs";
			domConstruct.place("<tr><td nowrap><label id='"+this.bufferedMsgsId+"_label'  >"+nls_mon.monitoring.bufferedMsgs+"</label></td><td nowrap><span id='"+
					this.bufferedMsgsId+"' aria-labelledby='"+this.bufferedMsgsId+"_label'>"+nlsStrings.global.notAvailable+"</span></td></tr>", 
					this.resourcesContainerId);
			// add retained messages row
			this.retainedMsgsId = this.id+"_retainedMsgs";
			domConstruct.place("<tr><td nowrap><label id='"+this.retainedMsgsId+"_label'  >"+nls_mon.monitoring.retainedMsgs+"</label></td><td nowrap><span id='"+
					this.retainedMsgsId+"' aria-labelledby='"+this.retainedMsgsId+"_label'>"+nlsStrings.global.notAvailable+"</span></td></tr>", 
					this.resourcesContainerId);
			// add last updated row
			this.lastUpdatedId = this.id+"_lastUpdated";
			domConstruct.place("<tr><td nowrap><label id='"+this.lastUpdatedId+"_label'  >"+nls_mon.monitoring.lastUpdated+"</label></td><td nowrap><span id='"+
					this.lastUpdatedId+"' aria-labelledby='"+this.lastUpdatedId+"_label'></span></td></tr>", 
					this.resourcesContainerId);
			dojo.subscribe(Resources.monitoring.storeDataTopic, lang.hitch(this, function(data){
				this._updateBars(data, this.storeData);
			}));
			dojo.subscribe(Resources.monitoring.memoryDataTopic, lang.hitch(this, function(data){
				this._updateBars(data, this.memoryData);
			}));
			dojo.subscribe(Resources.monitoring.serverDataTopic, lang.hitch(this, function(data){
				this._updateMsgs(data);
			}));

		},
		
		_setFlexDashboardAttr: function(flexDashboard) {
			this.flexDashboard = flexDashboard;
			if (this.flexDashboard && this.refreshRateMillis !== undefined) {
				this.flexDashboard.set('storeDataInterval', this.refreshRateMillis);
				this.flexDashboard.set('memoryDataInterval', this.refreshRateMillis);
			}
		},
		
		_createBars: function(bars) {			
			if (!bars) {
				return;
			}
			for (var name in bars) {
				var bar = bars[name];
				// create a cell for the bar
				var cellId = this.id + "_" + name + "_cell";
				var barId = this.id + "_" + name + "_ProgressBar";
				var iconId = this.id + "_" + name + "_icon";
				domConstruct.place("<tr><td nowrap><label id='"+barId+"_description'>"+bar.title+"</label></td><td id='"+cellId+"'></td><td><div id='"+iconId+"'></div></td></tr>", this.resourcesContainerId);
				var progressBar = new ProgressBar({style: 'width: 150px', id: barId}).placeAt(cellId);
				bar.progressBar = progressBar;
				bar.color = "";
				bar.iconId = iconId;
				// place it in the cell and start it up
				progressBar.startup();
				
				// every update to the progress bar will set and overwrite aria-labelledby so we
				// will set describedby here
				dijit.setWaiState(progressBar, "describedby", barId + "_description");
			}
		},
		
		_updateBars: function(data, bars) {
			if (!data || !bars) {
				console.log("_updateBars called with missing parameters");
				return;
			}

			for (var name in bars) {
				
				var bar = bars[name];
				var progressBar = bar.progressBar;				
				var value = data[name]; 
				if (value === undefined) {
					value = progressBar.get('value'); 
				} else if (bar.invert) {
					value = 100 - value;
				}
				progressBar.set('value', value);
				
				// determine if we need to set the bar style based on thresholds
				var color = "#008abf"; 
				var icon = "";
				if (bar.thresholdPercents.alert && value > bar.thresholdPercents.alert) {
					// change the color to alert color
					color = "#d9182d";
					icon = lang.replace(this.alertIcon, [bar.alertThresholdText]);
				} else if (bar.thresholdPercents.warn && value > bar.thresholdPercents.warn) {
					// change the color to the warn color
					color = "#fdb813";
					icon = lang.replace(this.warningIcon, [bar.warningThresholdText]);
				} 
				if (color !== bar.color) {
					var nodes = dojo.query(".dijitProgressBarTile", bar.progressBar.domNode);
					if (nodes.length > 0) {
						domStyle.set(nodes[0], "backgroundColor", color);						
					}
					nodes = dojo.query(".dijitProgressBarFull", bar.progressBar.domNode);
					if (nodes.length > 0) {
						domStyle.set(nodes[0], "borderTop", "1px solid "+color);
						domStyle.set(nodes[0], "borderBottom", "1px solid "+color);
					}
					var iconNode = dom.byId(bar.iconId);
					if (iconNode) {
						iconNode.innerHTML = icon;
					}
					bar.color = color;
				}
			} // end for each bar	
			
			dom.byId(this.lastUpdatedId).innerHTML = Utils.getFormattedDateTime(new Date());			
		},
		
		_updateMsgs: function(data) {
			var updated = false;
			if (data.BufferedMessages !== undefined) {
				dom.byId(this.bufferedMsgsId).innerHTML = number.format(data.BufferedMessages, {places: 0, locale: ism.user.localeInfo.localeString});
				updated = true;
			}
			if (data.RetainedMessages !== undefined) {
				dom.byId(this.retainedMsgsId).innerHTML = number.format(data.RetainedMessages, {places: 0, locale: ism.user.localeInfo.localeString});
				updated = true;
			}	
			if (updated) {
				dom.byId(this.lastUpdatedId).innerHTML = Utils.getFormattedDateTime(new Date());
			}
		}
		

    });
 
});
